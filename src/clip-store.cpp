#include "clip-store.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include <optional>
#include <utility>

static constexpr int FILE_VERSION = 1;

static QJsonObject clipToJson(const Clip &clip)
{
	return {{"path", clip.path},
		{"type", clip.type == ClipType::Replay ? "replay" : "recording"},
		{"recordedAt", clip.recordedAt.toString(Qt::ISODateWithMs)}};
}

static std::optional<Clip> clipFromJson(const QJsonObject &object)
{
	QString path = object["path"].toString();
	if (path.isEmpty())
		return std::nullopt;

	ClipType type = object["type"].toString() == "replay" ? ClipType::Replay : ClipType::Recording;
	QDateTime recordedAt = QDateTime::fromString(object["recordedAt"].toString(), Qt::ISODateWithMs);
	return Clip{path, type, recordedAt};
}

ClipStore::ClipStore(const QString &filePath, QObject *parent) : QObject(parent), filePath(filePath) {}

void ClipStore::load()
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
		return;

	QJsonParseError parseError;
	QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
	if (!document.isObject()) {
		emit loadFailed(parseError.errorString());
		return;
	}

	// QJsonArray hands out temporary values, so the loops copy each one instead of binding a reference.
	QJsonObject root = document.object();
	const QJsonArray clipsJson = root["clips"].toArray();
	for (const QJsonValue value : clipsJson) {
		if (auto clip = clipFromJson(value.toObject()))
			items.append(*clip);
	}

	const QJsonArray pendingJson = root["pending"].toArray();
	for (const QJsonValue value : pendingJson) {
		QJsonObject object = value.toObject();
		if (auto clip = clipFromJson(object))
			pending.append({*clip, object["remuxPath"].toString()});
	}
}

void ClipStore::add(const Clip &clip)
{
	for (const Clip &existing : items) {
		if (samePath(existing.path, clip.path))
			return;
	}

	items.append(clip);
	save();
	emit clipAdded(clip);
}

void ClipStore::remove(const QString &path)
{
	for (int i = 0; i < items.size(); i++) {
		if (samePath(items[i].path, path)) {
			QString removedPath = items.takeAt(i).path;
			save();
			emit clipRemoved(removedPath);
			return;
		}
	}
}

void ClipStore::savePending(const QList<PendingClip> &unfinished)
{
	pending = unfinished;
	save();
}

QList<PendingClip> ClipStore::takeSavedPending()
{
	return std::exchange(pending, {});
}

void ClipStore::save()
{
	QJsonArray clipsJson;
	for (const Clip &clip : items)
		clipsJson.append(clipToJson(clip));

	QJsonArray pendingJson;
	for (const PendingClip &entry : pending) {
		QJsonObject object = clipToJson(entry.clip);
		if (!entry.remuxPath.isEmpty())
			object["remuxPath"] = entry.remuxPath;
		pendingJson.append(object);
	}

	QJsonObject root{{"version", FILE_VERSION}, {"clips", clipsJson}, {"pending", pendingJson}};

	// QSaveFile writes a temporary copy and swaps it in, so a crash mid-save never leaves a half-written list.
	QDir().mkpath(QFileInfo(filePath).absolutePath());
	QSaveFile file(filePath);
	if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(root).toJson()) < 0 || !file.commit())
		emit saveFailed(file.errorString());
}
