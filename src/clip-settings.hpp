#pragma once

#include "clip-naming.hpp"

#include <QString>
#include <QStringList>

constexpr int MAX_QUICK_TAGS = 20;

// What the user can change in the settings window.
struct ClipSettings {
	QString dateFormat = DEFAULT_DATE_FORMAT;
	QStringList quickTags;
};

// The date formats offered in the settings window. An empty format means new names get no date.
QStringList dateFormatChoices();

// A missing or damaged settings file gives the defaults.
ClipSettings loadSettings(const QString &filePath);
bool saveSettings(const QString &filePath, const ClipSettings &settings);

// Turns the settings box's text, one tag per line, into clean tags without blanks or repeats.
QStringList parseQuickTags(const QString &text);
