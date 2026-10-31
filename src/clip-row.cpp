#include "clip-row.hpp"
#include "clip-naming.hpp"

#include <obs-module.h>

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

ClipRow::ClipRow(const Clip &clip, QWidget *parent) : QFrame(parent)
{
	auto *typeLabel = new QLabel(obs_module_text(clip.type == ClipType::Replay ? "Replay" : "Recording"), this);
	QFont typeFont = typeLabel->font();
	typeFont.setBold(true);
	typeLabel->setFont(typeFont);

	// An ignored width lets long names wrap instead of stretching the whole dock.
	auto *fileLabel = new QLabel(QFileInfo(clip.path).fileName(), this);
	fileLabel->setWordWrap(true);
	fileLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	fileLabel->setToolTip(QDir::toNativeSeparators(clip.path));

	auto *nameEdit = new QLineEdit(this);
	nameEdit->setPlaceholderText(obs_module_text("NamePlaceholder"));
	nameEdit->setMaxLength(MAX_NAME_LENGTH);
	connect(nameEdit, &QLineEdit::returnPressed, this, [this, nameEdit]() { emit nameEntered(nameEdit->text()); });

	statusLabel = new QLabel(this);
	statusLabel->setWordWrap(true);
	statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	statusLabel->hide();

	auto *header = new QHBoxLayout();
	header->addWidget(typeLabel);
	header->addWidget(fileLabel, 1);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(header);
	layout->addWidget(nameEdit);
	layout->addWidget(statusLabel);
}

// An empty message hides the status line.
void ClipRow::showStatus(const QString &message)
{
	statusLabel->setText(message);
	statusLabel->setVisible(!message.isEmpty());
}
