#include "inbox-dock.hpp"
#include "clip-naming.hpp"
#include "clip-row.hpp"
#include "clip-store.hpp"
#include "file-actions.hpp"
#include "settings-dialog.hpp"

#include <obs-module.h>
#include <plugin-support.h>

#include <QDockWidget>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

InboxDock::InboxDock(ClipStore *store, FileActions *fileActions, const QString &settingsPath, QWidget *parent)
	: QWidget(parent),
	  store(store),
	  fileActions(fileActions),
	  settingsPath(settingsPath),
	  settings(loadSettings(settingsPath))
{
	// The theme supplies the gear icon through the "icon-gear" class. The text only shows if a theme has no icon.
	auto *settingsButton = new QToolButton(this);
	settingsButton->setText(obs_module_text("Settings"));
	settingsButton->setToolTip(obs_module_text("SettingsTitle"));
	settingsButton->setProperty("class", "icon-gear");
	connect(settingsButton, &QToolButton::clicked, this, &InboxDock::openSettings);

	auto *toolbar = new QHBoxLayout();
	toolbar->addStretch();
	toolbar->addWidget(settingsButton);

	// Shown whenever the inbox has nothing waiting.
	emptyLabel = new QLabel(obs_module_text("EmptyInbox"), this);
	emptyLabel->setAlignment(Qt::AlignCenter);
	emptyLabel->setWordWrap(true);

	// Rows stack from the top, newest first, with any spare space pushed below them.
	auto *rowsWidget = new QWidget();
	rowsLayout = new QVBoxLayout(rowsWidget);
	rowsLayout->setContentsMargins(0, 0, 0, 0);
	rowsLayout->addStretch();

	scrollArea = new QScrollArea(this);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea->setWidget(rowsWidget);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(toolbar);
	layout->addWidget(emptyLabel, 1);
	layout->addWidget(scrollArea, 1);

	for (const Clip &clip : store->clips())
		addRow(clip);

	connect(store, &ClipStore::clipAdded, this, &InboxDock::addRow);
	connect(store, &ClipStore::clipRemoved, this, &InboxDock::removeRow);

	connect(fileActions, &FileActions::renameWaiting, this,
		[this](const QString &path) { showStatus(path, obs_module_text("WaitingForFile")); });
	connect(fileActions, &FileActions::renameFailed, this, [this](const QString &path, const QString &error) {
		showStatus(path, QString::fromUtf8(obs_module_text("RenameFailed")).arg(error));
	});
	connect(fileActions, &FileActions::recycleBusy, this,
		[this](const QString &path) { showStatus(path, obs_module_text("DeleteBusy")); });
	connect(fileActions, &FileActions::recycleRefused, this,
		[this](const QString &path) { showStatus(path, obs_module_text("DeleteRefused")); });
	connect(fileActions, &FileActions::recycleFailed, this, [this](const QString &path, const QString &error) {
		showStatus(path, QString::fromUtf8(obs_module_text("DeleteFailed")).arg(error));
	});
	connect(fileActions, &FileActions::detailsLoaded, this,
		[this](const QString &path, const ClipDetails &details) {
			if (ClipRow *row = rows.value(path))
				row->setDetails(details);
		});

	updateCount();
}

// Shows "Clip Inbox (4)" in the dock's title bar. OBS owns the dock frame around this widget, so the title lives there.
void InboxDock::updateCount()
{
	int count = store->clips().size();
	emptyLabel->setVisible(count == 0);
	scrollArea->setVisible(count > 0);

	QString title = obs_module_text("ClipInbox");
	if (count > 0)
		title += QString(" (%1)").arg(count);
	if (auto *dock = qobject_cast<QDockWidget *>(parentWidget()))
		dock->setWindowTitle(title);
}

void InboxDock::addRow(const Clip &clip)
{
	auto *row = new ClipRow(clip);
	row->setQuickTags(settings.quickTags);
	connect(row, &ClipRow::nameEntered, this, [this, clip](const QString &text) { requestRename(clip, text); });
	connect(row, &ClipRow::skipClicked, this, [this, clip]() { skip(clip); });
	connect(row, &ClipRow::deleteClicked, this, [this, clip]() { confirmDelete(clip); });

	rows.insert(clip.path, row);
	rowsLayout->insertWidget(0, row);
	updateCount();

	QMetaObject::invokeMethod(fileActions,
				  [actions = fileActions, path = clip.path]() { actions->loadDetails(path); });
}

void InboxDock::removeRow(const QString &path)
{
	if (ClipRow *row = rows.take(path))
		row->deleteLater();
	updateCount();
}

// The rename itself runs on the file thread. The row leaves once the store hears it worked.
void InboxDock::requestRename(const Clip &clip, const QString &typed)
{
	QString name = cleanName(typed);
	if (name.isEmpty()) {
		showStatus(clip.path, obs_module_text("EmptyName"));
		return;
	}

	showStatus(clip.path, {});
	QString stem = buildStem(name, clip.recordedAt.date(), settings.dateFormat);
	QMetaObject::invokeMethod(fileActions,
				  [actions = fileActions, path = clip.path, stem]() { actions->rename(path, stem); });
}

// The file keeps its original name and simply leaves the inbox.
void InboxDock::skip(const Clip &clip)
{
	obs_log(LOG_INFO, "skipped: %s", clip.path.toUtf8().constData());
	QMetaObject::invokeMethod(fileActions, [actions = fileActions, path = clip.path]() { actions->cancel(path); });
	store->remove(clip.path);
}

void InboxDock::confirmDelete(const Clip &clip)
{
	QString question = QString::fromUtf8(obs_module_text("DeleteQuestion")).arg(QFileInfo(clip.path).fileName());
	QMessageBox::StandardButton answer = QMessageBox::question(this, obs_module_text("DeleteTitle"), question,
								   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	if (answer != QMessageBox::Yes)
		return;

	showStatus(clip.path, {});
	QMetaObject::invokeMethod(fileActions, [actions = fileActions, path = clip.path]() { actions->recycle(path); });
}

void InboxDock::openSettings()
{
	SettingsDialog dialog(settings, this);
	if (dialog.exec() != QDialog::Accepted)
		return;

	settings = dialog.settings();
	if (!saveSettings(settingsPath, settings))
		obs_log(LOG_WARNING, "could not save settings to %s", settingsPath.toUtf8().constData());

	for (ClipRow *row : std::as_const(rows))
		row->setQuickTags(settings.quickTags);
}

void InboxDock::showStatus(const QString &path, const QString &message)
{
	if (ClipRow *row = rows.value(path))
		row->showStatus(message);
}
