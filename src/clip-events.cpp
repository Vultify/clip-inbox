#include "clip-events.hpp"
#include "file-ready.hpp"

#include <util/config-file.h>

#include <QFileInfo>

#include <cstring>

// OBS hands out strings it allocated, so copy them and free the original.
static QString takeObsString(char *text)
{
	QString result = QString::fromUtf8(text);
	bfree(text);
	return result;
}

// Reads the same profile settings OBS checks before it auto-remuxes a recording.
static RemuxSettings currentRemuxSettings()
{
	RemuxSettings settings;
	config_t *config = obs_frontend_get_profile_config();
	if (!config)
		return settings;

	settings.autoRemux = config_get_bool(config, "Video", "AutoRemux");

	const char *mode = config_get_string(config, "Output", "Mode");
	bool simpleMode = !mode || std::strcmp(mode, "Simple") == 0;
	if (!simpleMode) {
		const char *recType = config_get_string(config, "AdvOut", "RecType");
		settings.ffmpegOutput = recType &&
					QString::fromUtf8(recType).compare("FFmpeg", Qt::CaseInsensitive) == 0;
	}

	const char *format = config_get_string(config, simpleMode ? "SimpleOutput" : "AdvOut", "RecFormat2");
	settings.fragmented = format && std::strncmp(format, "fragmented", 10) == 0;

	OBSOutputAutoRelease output = obs_frontend_get_recording_output();
	obs_encoder_t *encoder = output ? obs_output_get_video_encoder(output) : nullptr;
	const char *codec = encoder ? obs_encoder_get_codec(encoder) : nullptr;
	settings.prores = codec && std::strcmp(codec, "prores") == 0;

	return settings;
}

ClipEvents::ClipEvents(QObject *parent) : QObject(parent)
{
	obs_frontend_add_event_callback(onFrontendEvent, this);
}

ClipEvents::~ClipEvents()
{
	obs_frontend_remove_event_callback(onFrontendEvent, this);
}

void ClipEvents::onFrontendEvent(enum obs_frontend_event event, void *data)
{
	auto *self = static_cast<ClipEvents *>(data);

	switch (event) {
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		self->currentRecordingPath = takeObsString(obs_frontend_get_last_recording());
		self->startWatchingSplits();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		self->stopWatchingSplits();
		self->report(takeObsString(obs_frontend_get_last_recording()), ClipType::Recording);
		self->currentRecordingPath.clear();
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED:
		self->report(takeObsString(obs_frontend_get_last_replay()), ClipType::Replay);
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		self->stopWatchingSplits();
		break;
	default:
		break;
	}
}

// File splitting isn't a frontend event. It's a signal on the recording output itself, so hold on to that output
// while recording to keep the connection safe.
void ClipEvents::startWatchingSplits()
{
	stopWatchingSplits();

	recordingOutput = obs_frontend_get_recording_output();
	if (!recordingOutput)
		return;

	fileChangedSignal.Connect(obs_output_get_signal_handler(recordingOutput), "file_changed",
				  onRecordingFileChanged, this);
}

void ClipEvents::stopWatchingSplits()
{
	fileChangedSignal.Disconnect();
	recordingOutput = nullptr;
}

// Runs on OBS's recording thread, so hop over to the UI thread before touching anything.
void ClipEvents::onRecordingFileChanged(void *data, calldata_t *params)
{
	auto *self = static_cast<ClipEvents *>(data);
	QString nextPath = QString::fromUtf8(calldata_string(params, "next_file"));

	QMetaObject::invokeMethod(self, [self, nextPath]() { self->splitFinished(nextPath); }, Qt::QueuedConnection);
}

void ClipEvents::splitFinished(const QString &nextPath)
{
	report(currentRecordingPath, ClipType::Recording);
	currentRecordingPath = nextPath;
}

void ClipEvents::report(const QString &path, ClipType type)
{
	if (path.isEmpty())
		return;

	// The file's creation time is when OBS started writing it, which is the moment the clip was recorded.
	QFileInfo info(path);
	QDateTime recordedAt = info.birthTime();
	if (!recordedAt.isValid())
		recordedAt = info.lastModified();
	if (!recordedAt.isValid())
		recordedAt = QDateTime::currentDateTime();

	emit clipFinished(Clip{path, type, recordedAt}, remuxTarget(path, currentRemuxSettings()));
}
