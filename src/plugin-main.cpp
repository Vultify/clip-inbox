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
#include <QThread>

#include "clip-events.hpp"
#include "clip-store.hpp"
#include "file-ready.hpp"
#include "inbox-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

static constexpr const char *DOCK_ID = "clip-inbox";

static QThread *watcherThread = nullptr;

// The watcher's thread has to finish before OBS unloads the plugin, so stop it as soon as OBS starts exiting.
static void stopWatcherThread()
{
	if (!watcherThread)
		return;

	watcherThread->quit();
	watcherThread->wait();
	delete watcherThread;
	watcherThread = nullptr;
}

static void onFrontendEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_EXIT)
		stopWatcherThread();
}

// Finished files flow from OBS's events, through the watcher until they're safe, into the store the dock shows.
static void startClipPipeline(QMainWindow *mainWindow, ClipStore *store)
{
	auto *events = new ClipEvents(mainWindow);
	auto *watcher = new FileReadyWatcher();

	watcherThread = new QThread();
	watcher->moveToThread(watcherThread);
	QObject::connect(watcherThread, &QThread::finished, watcher, &QObject::deleteLater);

	QObject::connect(events, &ClipEvents::clipFinished, watcher, &FileReadyWatcher::watch);
	QObject::connect(watcher, &FileReadyWatcher::clipReady, store, &ClipStore::add);
	QObject::connect(watcher, &FileReadyWatcher::clipGone, store, [](const Clip &clip) {
		obs_log(LOG_WARNING, "gave up waiting for %s", clip.path.toUtf8().constData());
	});
	QObject::connect(store, &ClipStore::clipAdded, store,
			 [](const Clip &clip) { obs_log(LOG_INFO, "clip added: %s", clip.path.toUtf8().constData()); });

	watcherThread->start();
	obs_frontend_add_event_callback(onFrontendEvent, nullptr);
}

// OBS wraps the dock widget and owns it from here on, so it's freed with the main window.
bool obs_module_load(void)
{
	qRegisterMetaType<Clip>();

	auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	auto *store = new ClipStore(mainWindow);
	auto *dock = new InboxDock(store, mainWindow);

	if (!obs_frontend_add_dock_by_id(DOCK_ID, obs_module_text("ClipInbox"), dock)) {
		delete dock;
		delete store;
		obs_log(LOG_ERROR, "could not add the Clip Inbox dock");
		return false;
	}

	startClipPipeline(mainWindow, store);

	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(onFrontendEvent, nullptr);
	stopWatcherThread();
	obs_log(LOG_INFO, "plugin unloaded");
}
