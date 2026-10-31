#include "file-actions.hpp"
#include "test-support.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

static QString makeFile(const QString &folder, const QString &name, const QByteArray &contents)
{
	QString path = folder + "/" + name;
	QFile file(path);
	file.open(QIODevice::WriteOnly);
	file.write(contents);
	return path;
}

static QByteArray readFile(const QString &path)
{
	QFile file(path);
	file.open(QIODevice::ReadOnly);
	return file.readAll();
}

static void testRename(const QString &folder)
{
	QString path = makeFile(folder, "2026-10-30 19-27-27.mp4", "clip");
	RenameResult result = renameClipFile(path, "clutch - 2026-10-30");

	check(result.outcome == RenameOutcome::Renamed, "A free file is renamed");
	check(result.newPath == folder + "/clutch - 2026-10-30.mp4", "The new name keeps the folder and extension");
	check(!QFileInfo::exists(path) && readFile(result.newPath) == "clip", "The file moved with its contents");
}

static void testNeverOverwrites(const QString &folder)
{
	QString taken = makeFile(folder, "taken - 2026-10-30.mp4", "old");
	QString second = makeFile(folder, "second.mp4", "new");
	QString third = makeFile(folder, "third.mp4", "newer");

	RenameResult secondResult = renameClipFile(second, "taken - 2026-10-30");
	RenameResult thirdResult = renameClipFile(third, "taken - 2026-10-30");

	check(secondResult.newPath == folder + "/taken - 2026-10-30 (2).mp4", "A taken name gets (2)");
	check(thirdResult.newPath == folder + "/taken - 2026-10-30 (3).mp4", "The next one gets (3)");
	check(readFile(taken) == "old", "The file that already had the name is untouched");
	check(readFile(secondResult.newPath) == "new" && readFile(thirdResult.newPath) == "newer",
	      "Each renamed file keeps its own contents");
}

static void testSameName(const QString &folder)
{
	QString path = makeFile(folder, "already - 2026-10-30.mp4", "clip");
	RenameResult result = renameClipFile(path, "already - 2026-10-30");
	check(result.outcome == RenameOutcome::Renamed && result.newPath == path,
	      "Renaming to its own name is a no-op");
}

static void testMissing(const QString &folder)
{
	check(renameClipFile(folder + "/nope.mp4", "anything").outcome == RenameOutcome::Missing,
	      "A file that's gone is reported missing");
}

#ifdef _WIN32
// Holds the file open the way a media player does, without letting anyone rename it.
static void testBusy(const QString &folder)
{
	QString path = makeFile(folder, "playing.mp4", "clip");
	std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
	HANDLE handle = CreateFileW(nativePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
				    FILE_ATTRIBUTE_NORMAL, nullptr);

	RenameResult whileOpen = renameClipFile(path, "playing - 2026-10-30");
	check(whileOpen.outcome == RenameOutcome::Busy, "A file another app holds is reported busy");
	check(QFileInfo::exists(path), "A busy file stays where it was");

	CloseHandle(handle);
	check(renameClipFile(path, "playing - 2026-10-30").outcome == RenameOutcome::Renamed,
	      "The rename works once the app lets go");
}

// Only checks the cases that stop before the Recycle Bin, so test runs never fill it with junk files.
static void testRecycleSafety(const QString &folder)
{
	check(recycleClipFile(folder + "/nope.mp4").outcome == RecycleOutcome::Missing,
	      "Deleting a file that's gone is reported missing");

	QString path = makeFile(folder, "open-in-player.mp4", "clip");
	std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
	HANDLE handle = CreateFileW(nativePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
				    FILE_ATTRIBUTE_NORMAL, nullptr);

	check(recycleClipFile(path).outcome == RecycleOutcome::Busy, "A file another app holds isn't deleted");
	check(QFileInfo::exists(path), "The held file is still there");
	CloseHandle(handle);
}
#endif

void runFileActionsTests(const QString &dir)
{
	QString folder = dir + "/actions";
	QDir().mkpath(folder);

	testRename(folder);
	testNeverOverwrites(folder);
	testSameName(folder);
	testMissing(folder);
#ifdef _WIN32
	testBusy(folder);
	testRecycleSafety(folder);
#endif
}
