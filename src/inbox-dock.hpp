#pragma once

#include "clip.hpp"

#include <QWidget>

class ClipStore;
class QLabel;
class QScrollArea;
class QVBoxLayout;

// The panel in OBS's Docks menu that lists clips waiting for a name.
class InboxDock : public QWidget {
	Q_OBJECT

public:
	explicit InboxDock(ClipStore *store, QWidget *parent = nullptr);

private:
	void addRow(const Clip &clip);
	void updateEmptyState();

	ClipStore *store;
	QLabel *emptyLabel;
	QScrollArea *scrollArea;
	QVBoxLayout *rowsLayout;
};
