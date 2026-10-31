#pragma once

#include <QDate>
#include <QString>

constexpr int MAX_NAME_LENGTH = 120;
constexpr const char *DEFAULT_DATE_FORMAT = "yyyy-MM-dd";

// Turns what the user typed into text that's safe in a Windows file name. Empty means nothing usable was typed.
QString cleanName(const QString &typed);

// The new file name without its extension: "<name> - <date>", or just the name when the date format is empty.
QString buildStem(const QString &name, const QDate &date, const QString &dateFormat = DEFAULT_DATE_FORMAT);

// The file name to try on a given attempt: the stem as is first, then "stem (2)", "stem (3)", and so on.
QString numberedFileName(const QString &stem, const QString &extension, int attempt);
