#include "clip-row.hpp"
#include "clip-naming.hpp"

#include <obs-module.h>

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

static const QSize THUMBNAIL_SIZE(128, 72);

ClipRow::ClipRow(const Clip &clip, QWidget *parent)
	: QFrame(parent),
	  typeText(obs_module_text(clip.type == ClipType::Replay ? "Replay" : "Recording"))
{
	// Until Windows hands back a thumbnail, and for clips it can't preview, the box shows a short note instead.
	thumbnailLabel = new QLabel(obs_module_text("NoThumbnail"), this);
	thumbnailLabel->setFixedSize(THUMBNAIL_SIZE);
	thumbnailLabel->setAlignment(Qt::AlignCenter);
	thumbnailLabel->setFrameShape(QFrame::StyledPanel);

	typeLabel = new QLabel(typeText, this);
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

	auto *keepButton = new QPushButton(obs_module_text("Keep"), this);
	keepButton->setToolTip(obs_module_text("KeepTooltip"));
	connect(keepButton, &QPushButton::clicked, this, &ClipRow::keepClicked);

	auto *deleteButton = new QPushButton(obs_module_text("Delete"), this);
	deleteButton->setToolTip(obs_module_text("DeleteTooltip"));
	connect(deleteButton, &QPushButton::clicked, this, &ClipRow::deleteClicked);

	statusLabel = new QLabel(this);
	statusLabel->setWordWrap(true);
	statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	statusLabel->hide();

	auto *info = new QVBoxLayout();
	info->addWidget(typeLabel);
	info->addWidget(fileLabel);
	info->addStretch();

	auto *top = new QHBoxLayout();
	top->addWidget(thumbnailLabel);
	top->addLayout(info, 1);

	auto *actions = new QHBoxLayout();
	actions->addWidget(nameEdit, 1);
	actions->addWidget(keepButton);
	actions->addWidget(deleteButton);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(top);
	layout->addLayout(actions);
	layout->addWidget(statusLabel);
}

void ClipRow::setDetails(const ClipDetails &details)
{
	if (!details.thumbnail.isNull()) {
		QPixmap pixmap = QPixmap::fromImage(details.thumbnail)
					 .scaled(THUMBNAIL_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		thumbnailLabel->setPixmap(pixmap);
		thumbnailLabel->setFrameShape(QFrame::NoFrame);
	}

	QString length = formatDuration(details.durationMs);
	typeLabel->setText(length.isEmpty() ? typeText : typeText + "  ·  " + length);
}

// An empty message hides the status line.
void ClipRow::showStatus(const QString &message)
{
	statusLabel->setText(message);
	statusLabel->setVisible(!message.isEmpty());
}
