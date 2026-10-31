#pragma once

#include "clip.hpp"

#include <QHash>
#include <QWidget>

class ClipRow;
class ClipStore;
class FileActions;
class QLabel;
class QScrollArea;
class QVBoxLayout;

// The panel in OBS's Docks menu that lists clips waiting for a name.
class InboxDock : public QWidget {
	Q_OBJECT

public:
	InboxDock(ClipStore *store, FileActions *fileActions, QWidget *parent = nullptr);

private:
	void addRow(const Clip &clip);
	void removeRow(const QString &path);
	void requestRename(const Clip &clip, const QString &typed);
	void showStatus(const QString &path, const QString &message);
	void updateEmptyState();

	ClipStore *store;
	FileActions *fileActions;
	QHash<QString, ClipRow *> rows;
	QLabel *emptyLabel;
	QScrollArea *scrollArea;
	QVBoxLayout *rowsLayout;
};
