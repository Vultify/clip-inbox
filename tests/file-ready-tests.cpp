#include "file-ready.hpp"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QHash>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>

static int failures = 0;

static void check(bool condition, const char *description)
{
	std::printf("%s  %s\n", condition ? "PASS" : "FAIL", description);
	if (!condition)
		failures++;
}

static void testRemuxTargets()
{
	RemuxSettings on;
	on.autoRemux = true;

	check(remuxTarget("D:/Rec/a.mkv", on) == "D:/Rec/a.mp4", "MKV remuxes to MP4");
	check(remuxTarget("D:/Rec/a.mp4", on).isEmpty(), "Hybrid MP4 is already final");
	check(remuxTarget("D:/Rec/a.avi", on).isEmpty(), "AVI is never remuxed");
	check(remuxTarget("D:/Rec/a.mkv", RemuxSettings{}).isEmpty(), "No remux when auto-remux is off");

	RemuxSettings ffmpeg = on;
	ffmpeg.ffmpegOutput = true;
	check(remuxTarget("D:/Rec/a.mkv", ffmpeg).isEmpty(), "Custom FFmpeg output is never remuxed");

	RemuxSettings fragmented = on;
	fragmented.fragmented = true;
	check(remuxTarget("D:/Rec/a.mp4", fragmented) == "D:/Rec/a.remuxed.mp4", "Fragmented MP4 keeps its container");

	RemuxSettings prores = on;
	prores.prores = true;
	check(remuxTarget("D:/Rec/a.mkv", prores) == "D:/Rec/a.mov", "ProRes remuxes to MOV");
}

// A fake disk the tests can change between checks.
struct FakeDisk {
	QHash<QString, FileSnapshot> files;

	void put(const QString &path, qint64 size, bool inUse = false) { files[path] = {true, size, inUse}; }

	SnapshotFunction snapshot() const
	{
		return [this](const QString &path) {
			return files.value(path);
		};
	}
};

static ReadyState checkTimes(PendingClip &pending, const FakeDisk &disk, int times, const ReadyTiming &timing = {})
{
	ReadyState state = ReadyState::Waiting;
	for (int i = 0; i < times && state == ReadyState::Waiting; i++)
		state = checkPending(pending, disk.snapshot(), timing);
	return state;
}

static void testFinishedFile()
{
	FakeDisk disk;
	disk.put("a.mp4", 100);
	PendingClip pending{Clip{"a.mp4"}, {}};

	check(checkTimes(pending, disk, 2) == ReadyState::Waiting, "Not ready until the size has held still");
	check(checkTimes(pending, disk, 1) == ReadyState::Ready, "Ready once the size held still twice");
}

static void testGrowingFile()
{
	FakeDisk disk;
	PendingClip pending{Clip{"a.mp4"}, {}};

	disk.put("a.mp4", 100);
	checkPending(pending, disk.snapshot());
	disk.put("a.mp4", 200);
	checkPending(pending, disk.snapshot());
	disk.put("a.mp4", 300);
	check(checkPending(pending, disk.snapshot()) == ReadyState::Waiting, "Still waiting while the file grows");
	check(checkTimes(pending, disk, 2) == ReadyState::Ready, "Ready after the growth stops");
}

static void testFileInUse()
{
	FakeDisk disk;
	disk.put("a.mp4", 100, true);
	PendingClip pending{Clip{"a.mp4"}, {}};

	check(checkTimes(pending, disk, 10) == ReadyState::Waiting, "Never ready while something is writing to it");
	disk.put("a.mp4", 100, false);
	check(checkTimes(pending, disk, 2) == ReadyState::Ready, "Ready once the writer lets go");
}

static void testRemuxAppears()
{
	FakeDisk disk;
	disk.put("a.mkv", 500);
	PendingClip pending{Clip{"a.mkv"}, "a.mp4"};

	check(checkTimes(pending, disk, 3) == ReadyState::Waiting, "Waits for the remux to start");
	disk.put("a.mp4", 100, true);
	check(checkTimes(pending, disk, 3) == ReadyState::Waiting, "Waits while the remux is writing");
	disk.put("a.mp4", 500, false);
	check(checkTimes(pending, disk, 3) == ReadyState::Ready, "Ready when the remux finishes");
	check(pending.clip.path == "a.mp4", "The clip points at the remuxed file");
}

static void testRemuxNeverAppears()
{
	FakeDisk disk;
	disk.put("a.mkv", 500);
	PendingClip pending{Clip{"a.mkv"}, "a.mp4"};
	ReadyTiming timing;
	timing.remuxGraceChecks = 3;

	check(checkTimes(pending, disk, 3, timing) == ReadyState::Waiting, "Gives the remux a grace period");
	check(checkTimes(pending, disk, 3, timing) == ReadyState::Ready, "Falls back to the original file");
	check(pending.clip.path == "a.mkv", "The clip keeps the original file");
}

static void testMissingFile()
{
	FakeDisk disk;
	PendingClip pending{Clip{"gone.mp4"}, {}};
	ReadyTiming timing;
	timing.missingChecks = 5;

	check(checkTimes(pending, disk, 5, timing) == ReadyState::Waiting, "Waits a while for a missing file");
	check(checkTimes(pending, disk, 1, timing) == ReadyState::Gone, "Gives up on a file that never shows up");
}

#ifdef _WIN32
static void testRealFileInUse(const QString &dir)
{
	QString path = dir + "/open.mp4";
	QFile file(path);
	file.open(QIODevice::WriteOnly);
	file.write("data");
	file.flush();

	check(snapshotFile(path).inUse, "A file open for writing counts as in use");
	file.close();
	check(!snapshotFile(path).inUse, "A closed file is free");
}
#endif

// Runs the real watcher against a real file and waits for it to report the clip.
static void testWatcherReportsClip(const QString &dir)
{
	QString path = dir + "/replay.mp4";
	QFile file(path);
	file.open(QIODevice::WriteOnly);
	file.write("finished clip");
	file.close();

	FileReadyWatcher watcher(20);
	QEventLoop loop;
	QString reported;
	QObject::connect(&watcher, &FileReadyWatcher::clipReady, &loop, [&](const Clip &clip) {
		reported = clip.path;
		loop.quit();
	});
	QTimer::singleShot(3000, &loop, &QEventLoop::quit);

	watcher.watch(Clip{path, ClipType::Replay}, {});
	loop.exec();

	check(reported == path, "The watcher reports a finished file");
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QTemporaryDir dir;

	testRemuxTargets();
	testFinishedFile();
	testGrowingFile();
	testFileInUse();
	testRemuxAppears();
	testRemuxNeverAppears();
	testMissingFile();
#ifdef _WIN32
	testRealFileInUse(dir.path());
#endif
	testWatcherReportsClip(dir.path());

	std::printf("\n%d failed\n", failures);
	return failures == 0 ? 0 : 1;
}
