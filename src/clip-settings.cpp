#include "clip-settings.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

static constexpr int FILE_VERSION = 1;

QStringList dateFormatChoices()
{
	return {DEFAULT_DATE_FORMAT, "MM-dd-yyyy", "dd-MM-yyyy", "MMM d yyyy", ""};
}

ClipSettings loadSettings(const QString &filePath)
{
	ClipSettings settings;
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
		return settings;

	QJsonDocument document = QJsonDocument::fromJson(file.readAll());
	if (!document.isObject())
		return settings;

	QJsonObject root = document.object();
	if (root.contains("dateFormat"))
		settings.dateFormat = root["dateFormat"].toString();

	QStringList tags;
	const QJsonArray tagsJson = root["quickTags"].toArray();
	for (const QJsonValue value : tagsJson)
		tags.append(value.toString());
	settings.quickTags = parseQuickTags(tags.join('\n'));

	return settings;
}

bool saveSettings(const QString &filePath, const ClipSettings &settings)
{
	QJsonObject root{{"version", FILE_VERSION},
			 {"dateFormat", settings.dateFormat},
			 {"quickTags", QJsonArray::fromStringList(settings.quickTags)}};

	QDir().mkpath(QFileInfo(filePath).absolutePath());
	QSaveFile file(filePath);
	return file.open(QIODevice::WriteOnly) && file.write(QJsonDocument(root).toJson()) >= 0 && file.commit();
}

QStringList parseQuickTags(const QString &text)
{
	QStringList tags;
	for (const QString &line : text.split('\n')) {
		QString tag = cleanName(line);
		if (!tag.isEmpty() && !tags.contains(tag, Qt::CaseInsensitive))
			tags.append(tag);
		if (tags.size() == MAX_QUICK_TAGS)
			break;
	}
	return tags;
}
