#pragma once

#include "clip.hpp"
#include "clip-settings.hpp"

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
	InboxDock(ClipStore *store, FileActions *fileActions, const QString &settingsPath, QWidget *parent = nullptr);

	void updateCount();

private:
	void addRow(const Clip &clip);
	void removeRow(const QString &path);
	void requestRename(const Clip &clip, const QString &typed);
	void skip(const Clip &clip);
	void confirmDelete(const Clip &clip);
	void openSettings();
	void showStatus(const QString &path, const QString &message);

	ClipStore *store;
	FileActions *fileActions;
	QString settingsPath;
	ClipSettings settings;
	QHash<QString, ClipRow *> rows;
	QLabel *emptyLabel;
	QScrollArea *scrollArea;
	QVBoxLayout *rowsLayout;
};
