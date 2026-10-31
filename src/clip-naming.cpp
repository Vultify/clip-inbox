#include "clip-naming.hpp"

#include <QStringList>

QString cleanName(const QString &typed)
{
	// Characters Windows forbids become spaces, so "ridge/church" reads as "ridge church" instead of "ridgechurch".
	static const QString forbidden = QStringLiteral("\\/:*?\"<>|");

	QString cleaned = typed;
	for (QChar &c : cleaned) {
		if (c.unicode() < 32 || forbidden.contains(c))
			c = ' ';
	}

	cleaned = cleaned.simplified();
	cleaned.truncate(MAX_NAME_LENGTH);

	// Windows silently drops trailing dots and spaces, so remove them up front.
	while (cleaned.endsWith('.') || cleaned.endsWith(' '))
		cleaned.chop(1);

	return cleaned;
}

// Names Windows reserves for devices, which can't be used as a file name even with an extension.
static bool isReservedName(const QString &stem)
{
	static const QStringList reserved = {"CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4",
					     "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3",
					     "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
	return reserved.contains(stem, Qt::CaseInsensitive);
}

QString buildStem(const QString &name, const QDate &date, const QString &dateFormat)
{
	QString dateText;
	if (!dateFormat.isEmpty() && date.isValid())
		dateText = cleanName(date.toString(dateFormat));

	QString stem = dateText.isEmpty() ? name : name + " - " + dateText;
	if (isReservedName(stem))
		stem += '_';

	return stem;
}

QString numberedFileName(const QString &stem, const QString &extension, int attempt)
{
	QString name = attempt <= 1 ? stem : QString("%1 (%2)").arg(stem, QString::number(attempt));
	return extension.isEmpty() ? name : name + '.' + extension;
}
