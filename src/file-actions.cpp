#include "file-actions.hpp"
#include "clip-naming.hpp"
#include "clip.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

static constexpr int MAX_NUMBERED_ATTEMPTS = 999;

enum class MoveResult { Moved, TargetExists, SourceBusy, SourceMissing, Error };

#ifdef _WIN32
// Without MOVEFILE_REPLACE_EXISTING, Windows itself refuses to touch a file that already has the new name.
static MoveResult moveWithoutReplacing(const QString &from, const QString &to, QString &error)
{
	std::wstring source = QDir::toNativeSeparators(from).toStdWString();
	std::wstring target = QDir::toNativeSeparators(to).toStdWString();
	if (MoveFileExW(source.c_str(), target.c_str(), 0))
		return MoveResult::Moved;

	DWORD code = GetLastError();
	switch (code) {
	case ERROR_ALREADY_EXISTS:
	case ERROR_FILE_EXISTS:
		return MoveResult::TargetExists;
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
		return MoveResult::SourceBusy;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return MoveResult::SourceMissing;
	default:
		error = qt_error_string(static_cast<int>(code));
		return MoveResult::Error;
	}
}
#else
static MoveResult moveWithoutReplacing(const QString &from, const QString &to, QString &error)
{
	if (QFileInfo::exists(to))
		return MoveResult::TargetExists;
	if (!QFileInfo::exists(from))
		return MoveResult::SourceMissing;

	QFile file(from);
	if (file.rename(to))
		return MoveResult::Moved;

	error = file.errorString();
	return MoveResult::Error;
}
#endif

RenameResult renameClipFile(const QString &path, const QString &stem)
{
	QFileInfo info(path);
	if (!info.isFile())
		return {RenameOutcome::Missing};

	QDir folder = info.dir();
	QString extension = info.suffix();

	for (int attempt = 1; attempt <= MAX_NUMBERED_ATTEMPTS; attempt++) {
		QString target = folder.filePath(numberedFileName(stem, extension, attempt));
		if (samePath(target, path))
			return {RenameOutcome::Renamed, path};

		QString error;
		switch (moveWithoutReplacing(path, target, error)) {
		case MoveResult::Moved:
			return {RenameOutcome::Renamed, target};
		case MoveResult::TargetExists:
			continue;
		case MoveResult::SourceBusy:
			return {RenameOutcome::Busy};
		case MoveResult::SourceMissing:
			return {RenameOutcome::Missing};
		case MoveResult::Error:
			return {RenameOutcome::Failed, {}, error};
		}
	}

	return {RenameOutcome::Failed, {}, QStringLiteral("every numbered version of this name is taken")};
}

FileActions::FileActions(int retryIntervalMs, QObject *parent) : QObject(parent), retryTimer(new QTimer(this))
{
	retryTimer->setInterval(retryIntervalMs);
	connect(retryTimer, &QTimer::timeout, this, &FileActions::retryWaiting);
}

void FileActions::rename(const QString &path, const QString &stem)
{
	// A newer name replaces one that was still waiting for the file.
	waiting.remove(path);
	if (tryRename(path, stem))
		return;

	waiting.insert(path, stem);
	emit renameWaiting(path);
	if (!retryTimer->isActive())
		retryTimer->start();
}

void FileActions::dropMissing(const QStringList &paths)
{
	for (const QString &path : paths) {
		if (!QFileInfo(path).isFile())
			emit missing(path);
	}
}

void FileActions::stop()
{
	retryTimer->stop();
	waiting.clear();
}

// Returns false only when another app is holding the file, so the caller knows to try again later.
bool FileActions::tryRename(const QString &path, const QString &stem)
{
	RenameResult result = renameClipFile(path, stem);
	switch (result.outcome) {
	case RenameOutcome::Renamed:
		emit renamed(path, result.newPath);
		return true;
	case RenameOutcome::Missing:
		emit missing(path);
		return true;
	case RenameOutcome::Failed:
		emit renameFailed(path, result.error);
		return true;
	case RenameOutcome::Busy:
		return false;
	}
	return true;
}

void FileActions::retryWaiting()
{
	const auto snapshot = waiting;
	for (auto it = snapshot.cbegin(); it != snapshot.cend(); ++it) {
		if (tryRename(it.key(), it.value()))
			waiting.remove(it.key());
	}

	if (waiting.isEmpty())
		retryTimer->stop();
}
