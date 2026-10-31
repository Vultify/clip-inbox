#include "settings-dialog.hpp"

#include <obs-module.h>

#include <QComboBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const ClipSettings &settings, QWidget *parent) : QDialog(parent)
{
	setWindowTitle(obs_module_text("SettingsTitle"));

	// Each choice is shown as today's date in that format, which is easier to pick from than a format code.
	dateFormatBox = new QComboBox(this);
	QStringList formats = dateFormatChoices();
	if (!formats.contains(settings.dateFormat))
		formats.prepend(settings.dateFormat);
	for (const QString &format : formats) {
		QString label = format.isEmpty() ? QString::fromUtf8(obs_module_text("NoDate"))
						 : QDate::currentDate().toString(format);
		dateFormatBox->addItem(label, format);
	}
	dateFormatBox->setCurrentIndex(formats.indexOf(settings.dateFormat));
	connect(dateFormatBox, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateExample);

	exampleLabel = new QLabel(this);

	tagsEdit = new QPlainTextEdit(settings.quickTags.join('\n'), this);
	tagsEdit->setPlaceholderText(obs_module_text("QuickTagsPlaceholder"));

	auto *tagsHelp = new QLabel(obs_module_text("QuickTagsHelp"), this);
	tagsHelp->setWordWrap(true);

	auto *form = new QFormLayout();
	form->addRow(obs_module_text("DateFormat"), dateFormatBox);
	form->addRow(QString(), exampleLabel);
	form->addRow(obs_module_text("QuickTags"), tagsEdit);
	form->addRow(QString(), tagsHelp);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(buttons);

	updateExample();
}

ClipSettings SettingsDialog::settings() const
{
	ClipSettings result;
	result.dateFormat = dateFormatBox->currentData().toString();
	result.quickTags = parseQuickTags(tagsEdit->toPlainText());
	return result;
}

void SettingsDialog::updateExample()
{
	QString stem = buildStem("clutch 1v3 ridge", QDate::currentDate(), dateFormatBox->currentData().toString());
	exampleLabel->setText(QString::fromUtf8(obs_module_text("DateExample")).arg(stem + ".mp4"));
}
