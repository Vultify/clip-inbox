/*
Clip Inbox
Copyright (C) 2026 Caleb Castro

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <plugin-support.h>

#include <QMainWindow>
#include <QStringList>
#include <QThread>

#include "clip-events.hpp"
#include "clip-store.hpp"
#include "file-actions.hpp"
#include "file-ready.hpp"
#include "inbox-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static constexpr const char *DOCK_ID = "clip-inbox";

// Everything that runs while OBS is open. The file thread does all the disk work so OBS never waits on a slow drive.
struct Inbox {
	ClipStore *store = nullptr;
	ClipEvents *events = nullptr;
	FileReadyWatcher *watcher = nullptr;
	FileActions *fileActions = nullptr;
	QThread *fileThread = nullptr;
};

static Inbox inbox;

static QString configFilePath(const char *fileName)
{
	char *path = obs_module_config_path(fileName);
	QString result = QString::fromUtf8(path);
	bfree(path);
	return result;
}

static void logPath(int level, const char *message, const QString &path)
{
	obs_log(level, "%s: %s", message, path.toUtf8().constData());
}

static void stopFileThread()
{
	if (!inbox.fileThread)
		return;

	inbox.fileThread->quit();
	inbox.fileThread->wait();
	delete inbox.watcher;
	delete inbox.fileActions;
	delete inbox.fileThread;
	inbox.watcher = nullptr;
	inbox.fileActions = nullptr;
	inbox.fileThread = nullptr;
}

// Saves anything that wasn't ready yet, then stops the file thread before OBS unloads the plugin.
static void shutDownInbox()
{
	if (!inbox.fileThread)
		return;

	inbox.events->finish();

	QList<PendingClip> unfinished;
	QMetaObject::invokeMethod(
		inbox.watcher,
		[&unfinished]() {
			unfinished = inbox.watcher->takePending();
			inbox.fileActions->stop();
		},
		Qt::BlockingQueuedConnection);
	inbox.store->savePending(unfinished);

	stopFileThread();
}

static void onFrontendEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_EXIT)
		shutDownInbox();
}

// Finished files flow from OBS's events, through the watcher until they're safe, into the store the dock shows.
static void connectInbox()
{
	QObject::connect(inbox.events, &ClipEvents::clipFinished, inbox.watcher, &FileReadyWatcher::watch);
	QObject::connect(inbox.watcher, &FileReadyWatcher::clipReady, inbox.store, &ClipStore::add);
	QObject::connect(inbox.watcher, &FileReadyWatcher::clipGone, inbox.store,
			 [](const Clip &clip) { logPath(LOG_WARNING, "gave up waiting for", clip.path); });

	QObject::connect(inbox.fileActions, &FileActions::renamed, inbox.store,
			 [](const QString &oldPath, const QString &newPath) {
				 logPath(LOG_INFO, "renamed", oldPath + " -> " + newPath);
				 inbox.store->remove(oldPath);
			 });
	QObject::connect(inbox.fileActions, &FileActions::recycled, inbox.store, [](const QString &path) {
		logPath(LOG_INFO, "sent to the Recycle Bin", path);
		inbox.store->remove(path);
	});
	QObject::connect(inbox.fileActions, &FileActions::recycleRefused, inbox.store, [](const QString &path) {
		logPath(LOG_WARNING, "left alone because Windows would have deleted it permanently", path);
	});
	QObject::connect(inbox.fileActions, &FileActions::recycleFailed, inbox.store,
			 [](const QString &path, const QString &error) {
				 logPath(LOG_WARNING, "delete failed", path + " (" + error + ")");
			 });
	QObject::connect(inbox.fileActions, &FileActions::missing, inbox.store, [](const QString &path) {
		logPath(LOG_INFO, "file no longer exists", path);
		inbox.store->remove(path);
	});
	QObject::connect(inbox.fileActions, &FileActions::renameFailed, inbox.store,
			 [](const QString &path, const QString &error) {
				 logPath(LOG_WARNING, "rename failed", path + " (" + error + ")");
			 });

	QObject::connect(inbox.store, &ClipStore::clipAdded, inbox.store,
			 [](const Clip &clip) { logPath(LOG_INFO, "clip added", clip.path); });
}

// Picks up where the last session left off: re-checks clips that were still finishing and drops deleted files.
static void resumeLastSession()
{
	for (const PendingClip &entry : inbox.store->takeSavedPending()) {
		QMetaObject::invokeMethod(inbox.watcher,
					  [entry]() { inbox.watcher->watch(entry.clip, entry.remuxPath); });
	}

	QStringList paths;
	for (const Clip &clip : inbox.store->clips())
		paths.append(clip.path);
	QMetaObject::invokeMethod(inbox.fileActions, [paths]() { inbox.fileActions->dropMissing(paths); });
}

// OBS wraps the dock widget and owns it from here on, so it's freed with the main window.
bool obs_module_load(void)
{
	qRegisterMetaType<Clip>();
	qRegisterMetaType<ClipDetails>();

	auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());

	inbox.store = new ClipStore(configFilePath("inbox.json"), mainWindow);
	QObject::connect(inbox.store, &ClipStore::loadFailed, inbox.store,
			 [](const QString &error) { logPath(LOG_WARNING, "could not read the saved inbox", error); });
	QObject::connect(inbox.store, &ClipStore::saveFailed, inbox.store,
			 [](const QString &error) { logPath(LOG_WARNING, "could not save the inbox", error); });
	inbox.store->load();

	// The file thread starts before the dock exists, so the thumbnails the dock asks for never load on OBS's UI thread.
	inbox.watcher = new FileReadyWatcher();
	inbox.fileActions = new FileActions();
	inbox.fileThread = new QThread();
	inbox.watcher->moveToThread(inbox.fileThread);
	inbox.fileActions->moveToThread(inbox.fileThread);
	inbox.fileThread->start();

	auto *dock = new InboxDock(inbox.store, inbox.fileActions, mainWindow);
	if (!obs_frontend_add_dock_by_id(DOCK_ID, obs_module_text("ClipInbox"), dock)) {
		stopFileThread();
		delete dock;
		delete inbox.store;
		inbox = {};
		obs_log(LOG_ERROR, "could not add the Clip Inbox dock");
		return false;
	}

	inbox.events = new ClipEvents(mainWindow);
	connectInbox();
	resumeLastSession();

	obs_frontend_add_event_callback(onFrontendEvent, nullptr);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
	stopFileThread();
	obs_log(LOG_INFO, "plugin unloaded");
}
