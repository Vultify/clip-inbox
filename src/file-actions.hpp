#pragma once

#include "file-info.hpp"

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class QTimer;

enum class RenameOutcome { Renamed, Busy, Missing, Failed };

struct RenameResult {
	RenameOutcome outcome = RenameOutcome::Failed;
	QString newPath;
	QString error;
};

// Renames a file inside its own folder. Never overwrites: a taken name gets " (2)", " (3)", and so on.
RenameResult renameClipFile(const QString &path, const QString &stem);

enum class RecycleOutcome { Recycled, Busy, Missing, NotRecyclable, Failed };

struct RecycleResult {
	RecycleOutcome outcome = RecycleOutcome::Failed;
	QString error;
};

// Sends a file to the Recycle Bin. When Windows would delete it for good instead, the file is left alone.
RecycleResult recycleClipFile(const QString &path);

// Does the disk work for the inbox on the file thread, and keeps retrying renames that another app is blocking.
class FileActions : public QObject {
	Q_OBJECT

public:
	explicit FileActions(int retryIntervalMs = 2000, QObject *parent = nullptr);

	void rename(const QString &path, const QString &stem);
	void recycle(const QString &path);
	void cancel(const QString &path);
	void loadDetails(const QString &path);
	void dropMissing(const QStringList &paths);
	void stop();

signals:
	void renamed(const QString &oldPath, const QString &newPath);
	void renameWaiting(const QString &path);
	void renameFailed(const QString &path, const QString &error);
	void recycled(const QString &path);
	void recycleBusy(const QString &path);
	void recycleRefused(const QString &path);
	void recycleFailed(const QString &path, const QString &error);
	void missing(const QString &path);
	void detailsLoaded(const QString &path, const ClipDetails &details);

private:
	bool tryRename(const QString &path, const QString &stem);
	void retryWaiting();

	QTimer *retryTimer;
	QHash<QString, QString> waiting;
};
