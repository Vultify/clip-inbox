#pragma once

#include "clip-settings.hpp"

#include <QDialog>

class QComboBox;
class QLabel;
class QPlainTextEdit;

// The Clip Inbox settings window: how dates look in new names, and the quick tag buttons on each clip.
class SettingsDialog : public QDialog {
	Q_OBJECT

public:
	explicit SettingsDialog(const ClipSettings &settings, QWidget *parent = nullptr);

	ClipSettings settings() const;

private:
	void updateExample();

	QComboBox *dateFormatBox;
	QLabel *exampleLabel;
	QPlainTextEdit *tagsEdit;
};
