#pragma once

#include "clip.hpp"

#include <obs.hpp>
#include <obs-frontend-api.h>

#include <QObject>
#include <QString>

// Hears OBS finish a recording, a replay, or one part of a split recording, and reports each finished file.
class ClipEvents : public QObject {
	Q_OBJECT

public:
	explicit ClipEvents(QObject *parent = nullptr);
	~ClipEvents() override;

	void finish();

signals:
	void clipFinished(const Clip &clip, const QString &remuxPath);

private:
	static void onFrontendEvent(enum obs_frontend_event event, void *data);
	static void onRecordingFileChanged(void *data, calldata_t *params);

	void startWatchingSplits();
	void stopWatchingSplits();
	void splitFinished(const QString &nextPath);
	void report(const QString &path, ClipType type);

	// Declared in this order so the signal disconnects before the output it belongs to is released.
	OBSOutputAutoRelease recordingOutput;
	OBSSignal fileChangedSignal;
	QString currentRecordingPath;
};
