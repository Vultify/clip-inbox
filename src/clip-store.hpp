#pragma once

#include "clip.hpp"

#include <QList>
#include <QObject>

// The clips waiting in the inbox, oldest first.
class ClipStore : public QObject {
	Q_OBJECT

public:
	explicit ClipStore(QObject *parent = nullptr);

	const QList<Clip> &clips() const { return items; }
	void add(const Clip &clip);

signals:
	void clipAdded(const Clip &clip);

private:
	QList<Clip> items;
};
