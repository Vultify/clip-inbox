#include "file-ready.hpp"

#include <QFileInfo>
#include <QTimer>

#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

QString remuxTarget(const QString &path, const RemuxSettings &settings)
{
	if (!settings.autoRemux || settings.ffmpegOutput)
		return {};

	QString suffix = QFileInfo(path).suffix();
	if (suffix.isEmpty() || suffix.compare("avi", Qt::CaseInsensitive) == 0)
		return {};

	QString base = path.left(path.size() - suffix.size());
	QString target;
	if (settings.fragmented)
		target = base + "remuxed." + suffix;
	else if (settings.prores)
		target = base + "mov";
	else
		target = base + "mp4";

	// OBS skips the remux when the file is already in its final container (Hybrid MP4, for example).
	return target == path ? QString() : target;
}

#ifdef _WIN32
// Opening without sharing write access fails while any other program still has the file open for writing.
static bool isFileInUse(const QString &path)
{
	std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
	HANDLE handle = CreateFileW(nativePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
				    FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		return true;

	CloseHandle(handle);
	return false;
}
#else
static bool isFileInUse(const QString &)
{
	return false;
}
#endif

FileSnapshot snapshotFile(const QString &path)
{
	FileSnapshot snapshot;
	QFileInfo info(path);
	snapshot.exists = info.isFile();
	if (!snapshot.exists)
		return snapshot;

	snapshot.size = info.size();
	snapshot.inUse = isFileInUse(path);
	return snapshot;
}

ReadyState checkPending(PendingClip &pending, const SnapshotFunction &snapshot, const ReadyTiming &timing)
{
	pending.checks++;

	// When OBS is going to remux, follow the remuxed file as soon as it appears. If it never does, keep the original.
	if (!pending.remuxPath.isEmpty()) {
		if (snapshot(pending.remuxPath).exists) {
			pending.clip.path = pending.remuxPath;
			pending.remuxPath.clear();
			pending.lastSize = -1;
			pending.stableChecks = 0;
		} else if (pending.checks <= timing.remuxGraceChecks) {
			return ReadyState::Waiting;
		} else {
			pending.remuxPath.clear();
		}
	}

	FileSnapshot file = snapshot(pending.clip.path);
	if (!file.exists || file.size == 0)
		return pending.checks > timing.missingChecks ? ReadyState::Gone : ReadyState::Waiting;

	bool stable = file.size == pending.lastSize && !file.inUse;
	pending.lastSize = file.size;
	pending.stableChecks = stable ? pending.stableChecks + 1 : 0;

	return pending.stableChecks >= timing.stableChecksNeeded ? ReadyState::Ready : ReadyState::Waiting;
}

FileReadyWatcher::FileReadyWatcher(int intervalMs, ReadyTiming timing, QObject *parent)
	: QObject(parent),
	  timer(new QTimer(this)),
	  timing(timing)
{
	timer->setInterval(intervalMs);
	connect(timer, &QTimer::timeout, this, &FileReadyWatcher::checkAll);
}

void FileReadyWatcher::watch(const Clip &clip, const QString &remuxPath)
{
	// A split right before stopping can report the same file twice.
	for (const PendingClip &existing : pending) {
		if (samePath(existing.clip.path, clip.path))
			return;
	}

	pending.append({clip, remuxPath});
	if (!timer->isActive())
		timer->start();
}

// Stops watching and hands back whatever wasn't ready yet, so it can be saved when OBS closes.
QList<PendingClip> FileReadyWatcher::takePending()
{
	timer->stop();
	return std::exchange(pending, {});
}

void FileReadyWatcher::checkAll()
{
	for (auto it = pending.begin(); it != pending.end();) {
		switch (checkPending(*it, snapshotFile, timing)) {
		case ReadyState::Ready:
			emit clipReady(it->clip);
			it = pending.erase(it);
			break;
		case ReadyState::Gone:
			emit clipGone(it->clip);
			it = pending.erase(it);
			break;
		case ReadyState::Waiting:
			++it;
			break;
		}
	}

	if (pending.isEmpty())
		timer->stop();
}
