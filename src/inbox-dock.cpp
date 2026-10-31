#include "inbox-dock.hpp"

#include <obs-module.h>

#include <QLabel>
#include <QVBoxLayout>

InboxDock::InboxDock(QWidget *parent) : QWidget(parent)
{
	// Shown whenever the inbox has nothing waiting.
	emptyLabel = new QLabel(obs_module_text("EmptyInbox"), this);
	emptyLabel->setAlignment(Qt::AlignCenter);
	emptyLabel->setWordWrap(true);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(emptyLabel);
}
