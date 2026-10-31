#pragma once

#include "clip.hpp"

#include <QList>
#include <QObject>
#include <QString>

#include <functional>

class QTimer;

// The OBS settings that decide whether a finished file gets remuxed, and into what.
struct RemuxSettings {
	bool autoRemux = false;
	bool ffmpegOutput = false;
	bool fragmented = false;
	bool prores = false;
};

// Where OBS's auto-remux will write the final file, or an empty string when there won't be a remux.
// Mirrors OBSBasic::AutoRemux in OBS 31.
QString remuxTarget(const QString &path, const RemuxSettings &settings);

// One look at a file on disk.
struct FileSnapshot {
	bool exists = false;
	qint64 size = 0;
	bool inUse = true;
};

FileSnapshot snapshotFile(const QString &path);

// How patient to be, counted in checks (one check per watcher interval).
struct ReadyTiming {
	int stableChecksNeeded = 2;
	int remuxGraceChecks = 15;
	int missingChecks = 30;
};

// A clip OBS reported as finished that isn't safe to show yet.
struct PendingClip {
	Clip clip;
	QString remuxPath;
	qint64 lastSize = -1;
	int stableChecks = 0;
	int checks = 0;
};

enum class ReadyState { Waiting, Ready, Gone };

using SnapshotFunction = std::function<FileSnapshot(const QString &)>;

// Advances one pending clip by one check. A file is ready once its size holds still and nothing is writing to it.
ReadyState checkPending(PendingClip &pending, const SnapshotFunction &snapshot, const ReadyTiming &timing = {});

// Polls pending clips on its own thread so slow drives never stall OBS.
class FileReadyWatcher : public QObject {
	Q_OBJECT

public:
	explicit FileReadyWatcher(int intervalMs = 1000, ReadyTiming timing = {}, QObject *parent = nullptr);

	void watch(const Clip &clip, const QString &remuxPath);
	QList<PendingClip> takePending();

signals:
	void clipReady(const Clip &clip);
	void clipGone(const Clip &clip);

private:
	void checkAll();

	QTimer *timer;
	ReadyTiming timing;
	QList<PendingClip> pending;
};
