#include "clip-row.hpp"

#include <obs-module.h>

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>

ClipRow::ClipRow(const Clip &clip, QWidget *parent) : QFrame(parent)
{
	auto *typeLabel = new QLabel(obs_module_text(clip.type == ClipType::Replay ? "Replay" : "Recording"), this);
	QFont typeFont = typeLabel->font();
	typeFont.setBold(true);
	typeLabel->setFont(typeFont);

	// An ignored width lets long names wrap instead of stretching the whole dock.
	auto *nameLabel = new QLabel(QFileInfo(clip.path).fileName(), this);
	nameLabel->setWordWrap(true);
	nameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	nameLabel->setToolTip(QDir::toNativeSeparators(clip.path));

	auto *layout = new QHBoxLayout(this);
	layout->addWidget(typeLabel);
	layout->addWidget(nameLabel, 1);
}
