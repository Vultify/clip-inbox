#include "inbox-dock.hpp"
#include "clip-row.hpp"
#include "clip-store.hpp"

#include <obs-module.h>

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

InboxDock::InboxDock(ClipStore *store, QWidget *parent) : QWidget(parent), store(store)
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
	updateEmptyState();
}

void InboxDock::addRow(const Clip &clip)
{
	rowsLayout->insertWidget(0, new ClipRow(clip));
	updateEmptyState();
}

void InboxDock::updateEmptyState()
{
	bool empty = store->clips().isEmpty();
	emptyLabel->setVisible(empty);
	scrollArea->setVisible(!empty);
}
