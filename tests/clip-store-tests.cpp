#include "clip-store.hpp"
#include "test-support.hpp"

#include <QFile>

static const QDateTime RECORDED_AT(QDate(2026, 10, 30), QTime(19, 27, 27));

static void testSaveAndLoad(const QString &dir)
{
	QString file = dir + "/store/inbox.json";
	{
		ClipStore store(file);
		store.add(Clip{"D:/Rec/replay.mp4", ClipType::Replay, RECORDED_AT});
		store.add(Clip{"D:/Rec/recording.mp4", ClipType::Recording, RECORDED_AT});
		store.add(Clip{"d:/rec/REPLAY.mp4", ClipType::Replay, RECORDED_AT});
		store.savePending({{Clip{"D:/Rec/long.mkv", ClipType::Recording, RECORDED_AT}, "D:/Rec/long.mp4"}});
	}

	ClipStore loaded(file);
	loaded.load();
	const QList<Clip> &clips = loaded.clips();
	check(clips.size() == 2, "Clips survive a restart, and the same file isn't added twice");
	check(clips.size() == 2 && clips[0].path == "D:/Rec/replay.mp4" && clips[0].type == ClipType::Replay &&
		      clips[0].recordedAt == RECORDED_AT,
	      "Each clip keeps its path, type, and recording time");

	QList<PendingClip> pending = loaded.takeSavedPending();
	check(pending.size() == 1 && pending[0].remuxPath == "D:/Rec/long.mp4",
	      "Clips that were still finishing are remembered with their remux");
	check(loaded.takeSavedPending().isEmpty(), "Remembered clips are handed back only once");
}

static void testRemove(const QString &dir)
{
	QString file = dir + "/store/remove.json";
	{
		ClipStore store(file);
		store.add(Clip{"D:/Rec/keep.mp4"});
		store.add(Clip{"D:/Rec/renamed.mp4"});
		store.remove("D:/Rec/renamed.mp4");
	}

	ClipStore loaded(file);
	loaded.load();
	check(loaded.clips().size() == 1 && loaded.clips()[0].path == "D:/Rec/keep.mp4", "Removed clips stay removed");
}

static void testCorruptFile(const QString &dir)
{
	QString file = dir + "/store/corrupt.json";
	QFile broken(file);
	broken.open(QIODevice::WriteOnly);
	broken.write("{ not json");
	broken.close();

	ClipStore store(file);
	bool reported = false;
	QObject::connect(&store, &ClipStore::loadFailed, [&reported]() { reported = true; });
	store.load();
	check(reported && store.clips().isEmpty(), "A damaged save file is reported and the inbox starts empty");
}

void runClipStoreTests(const QString &dir)
{
	testSaveAndLoad(dir);
	testRemove(dir);
	testCorruptFile(dir);
}
