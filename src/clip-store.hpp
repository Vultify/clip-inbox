#pragma once

#include "clip.hpp"
#include "file-ready.hpp"

#include <QList>
#include <QObject>
#include <QString>

// The clips waiting in the inbox, oldest first, saved to a small JSON file after every change.
class ClipStore : public QObject {
	Q_OBJECT

public:
	explicit ClipStore(const QString &filePath, QObject *parent = nullptr);

	void load();
	const QList<Clip> &clips() const { return items; }

	void add(const Clip &clip);
	void remove(const QString &path);

	// Clips OBS finished but that weren't safe to show yet when OBS closed. They get checked again next time.
	void savePending(const QList<PendingClip> &unfinished);
	QList<PendingClip> takeSavedPending();

signals:
	void clipAdded(const Clip &clip);
	void clipRemoved(const QString &path);
	void loadFailed(const QString &error);
	void saveFailed(const QString &error);

private:
	void save();

	QString filePath;
	QList<Clip> items;
	QList<PendingClip> pending;
};
