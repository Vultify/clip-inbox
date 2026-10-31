#include "clip-row.hpp"
#include "clip-naming.hpp"
#include "flow-layout.hpp"

#include <obs-module.h>

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPixmap>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

static const QSize THUMBNAIL_SIZE(128, 72);
static const QString DETAIL_SEPARATOR = QStringLiteral("  ·  ");

// Today's clips just show the time. Older ones add the day so a backlog is easy to sort out.
static QString recordedTimeText(const QDateTime &recordedAt)
{
	if (!recordedAt.isValid())
		return {};

	QLocale locale;
	QString time = locale.toString(recordedAt.time(), QLocale::ShortFormat);
	if (recordedAt.date() == QDate::currentDate())
		return time;
	return locale.toString(recordedAt.date(), "MMM d") + ", " + time;
}

ClipRow::ClipRow(const Clip &clip, QWidget *parent)
	: QFrame(parent),
	  typeText(obs_module_text(clip.type == ClipType::Replay ? "Replay" : "Recording")),
	  recordedText(recordedTimeText(clip.recordedAt))
{
	// Until Windows hands back a thumbnail, and for clips it can't preview, the box shows a short note instead.
	thumbnailLabel = new QLabel(obs_module_text("NoThumbnail"), this);
	thumbnailLabel->setFixedSize(THUMBNAIL_SIZE);
	thumbnailLabel->setAlignment(Qt::AlignCenter);
	thumbnailLabel->setFrameShape(QFrame::StyledPanel);

	detailsLabel = new QLabel(this);
	detailsLabel->setWordWrap(true);
	detailsLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	QFont detailsFont = detailsLabel->font();
	detailsFont.setBold(true);
	detailsLabel->setFont(detailsFont);
	setDetails({});

	// An ignored width lets long names wrap instead of stretching the whole dock.
	auto *fileLabel = new QLabel(QFileInfo(clip.path).fileName(), this);
	fileLabel->setWordWrap(true);
	fileLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	fileLabel->setToolTip(QDir::toNativeSeparators(clip.path));

	// Enter and the Rename button do the same thing, so it's obvious how to save a name.
	nameEdit = new QLineEdit(this);
	nameEdit->setPlaceholderText(obs_module_text("NamePlaceholder"));
	nameEdit->setMaxLength(MAX_NAME_LENGTH);
	connect(nameEdit, &QLineEdit::returnPressed, this, [this]() { emit nameEntered(nameEdit->text()); });

	auto *renameButton = new QPushButton(obs_module_text("Rename"), this);
	renameButton->setToolTip(obs_module_text("RenameTooltip"));
	connect(renameButton, &QPushButton::clicked, this, [this]() { emit nameEntered(nameEdit->text()); });

	// Skip and Delete sit up by the clip's details, away from the name box, so neither is mistaken for saving.
	auto *skipButton = new QPushButton(obs_module_text("Skip"), this);
	skipButton->setToolTip(obs_module_text("SkipTooltip"));
	connect(skipButton, &QPushButton::clicked, this, &ClipRow::skipClicked);

	auto *deleteButton = new QPushButton(obs_module_text("Delete"), this);
	deleteButton->setToolTip(obs_module_text("DeleteTooltip"));
	connect(deleteButton, &QPushButton::clicked, this, &ClipRow::deleteClicked);

	tagsWidget = new QWidget(this);
	tagsLayout = new FlowLayout(tagsWidget);
	tagsWidget->hide();

	statusLabel = new QLabel(this);
	statusLabel->setWordWrap(true);
	statusLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	statusLabel->hide();

	auto *clipButtons = new QHBoxLayout();
	clipButtons->addWidget(skipButton);
	clipButtons->addWidget(deleteButton);
	clipButtons->addStretch();

	auto *info = new QVBoxLayout();
	info->addWidget(detailsLabel);
	info->addWidget(fileLabel);
	info->addStretch();
	info->addLayout(clipButtons);

	auto *top = new QHBoxLayout();
	top->addWidget(thumbnailLabel, 0, Qt::AlignTop);
	top->addLayout(info, 1);

	auto *actions = new QHBoxLayout();
	actions->addWidget(nameEdit, 1);
	actions->addWidget(renameButton);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(top);
	layout->addWidget(tagsWidget);
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

	// Reads like "Replay · 0:28 · 12.4 MB · 7:53 PM", leaving out anything Windows couldn't tell.
	QStringList parts{typeText};
	if (details.durationMs >= 0)
		parts.append(formatDuration(details.durationMs));
	if (details.sizeBytes >= 0)
		parts.append(QLocale().formattedDataSize(details.sizeBytes, 1, QLocale::DataSizeTraditionalFormat));
	if (!recordedText.isEmpty())
		parts.append(recordedText);
	detailsLabel->setText(parts.join(DETAIL_SEPARATOR));
}

void ClipRow::setQuickTags(const QStringList &tags)
{
	while (QLayoutItem *item = tagsLayout->takeAt(0)) {
		delete item->widget();
		delete item;
	}

	for (const QString &tag : tags) {
		auto *button = new QToolButton(tagsWidget);
		button->setText(tag);
		connect(button, &QToolButton::clicked, this, [this, tag]() { addTagToName(tag); });
		tagsLayout->addWidget(button);
	}

	tagsWidget->setVisible(!tags.isEmpty());
}

// Adds the tag after whatever's typed so far and leaves the cursor ready for more typing or Enter.
void ClipRow::addTagToName(const QString &tag)
{
	QString text = nameEdit->text();
	if (!text.isEmpty() && !text.endsWith(' '))
		text += ' ';
	nameEdit->setText(text + tag);
	nameEdit->setFocus();
	nameEdit->end(false);
}

// An empty message hides the status line.
void ClipRow::showStatus(const QString &message)
{
	statusLabel->setText(message);
	statusLabel->setVisible(!message.isEmpty());
}
