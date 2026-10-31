#include "inbox-dock.hpp"
#include "clip-naming.hpp"
#include "clip-row.hpp"
#include "clip-store.hpp"
#include "file-actions.hpp"

#include <obs-module.h>

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

InboxDock::InboxDock(ClipStore *store, FileActions *fileActions, QWidget *parent)
	: QWidget(parent),
	  store(store),
	  fileActions(fileActions)
{
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
	layout->addWidget(emptyLabel);
	layout->addWidget(scrollArea);

	for (const Clip &clip : store->clips())
		addRow(clip);

	connect(store, &ClipStore::clipAdded, this, &InboxDock::addRow);
	connect(store, &ClipStore::clipRemoved, this, &InboxDock::removeRow);

	connect(fileActions, &FileActions::renameWaiting, this,
		[this](const QString &path) { showStatus(path, obs_module_text("WaitingForFile")); });
	connect(fileActions, &FileActions::renameFailed, this, [this](const QString &path, const QString &error) {
		showStatus(path, QString::fromUtf8(obs_module_text("RenameFailed")).arg(error));
	});

	updateEmptyState();
}

void InboxDock::addRow(const Clip &clip)
{
	auto *row = new ClipRow(clip);
	connect(row, &ClipRow::nameEntered, this, [this, clip](const QString &text) { requestRename(clip, text); });

	rows.insert(clip.path, row);
	rowsLayout->insertWidget(0, row);
	updateEmptyState();
}

void InboxDock::removeRow(const QString &path)
{
	if (ClipRow *row = rows.take(path))
		row->deleteLater();
	updateEmptyState();
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
	QString stem = buildStem(name, clip.recordedAt.date());
	QMetaObject::invokeMethod(fileActions,
				  [actions = fileActions, path = clip.path, stem]() { actions->rename(path, stem); });
}

void InboxDock::showStatus(const QString &path, const QString &message)
{
	if (ClipRow *row = rows.value(path))
		row->showStatus(message);
}

void InboxDock::updateEmptyState()
{
	bool empty = store->clips().isEmpty();
	emptyLabel->setVisible(empty);
	scrollArea->setVisible(!empty);
}
