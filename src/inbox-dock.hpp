#pragma once

#include <QWidget>

class QLabel;

// The panel in OBS's Docks menu that lists clips waiting for a name.
class InboxDock : public QWidget {
	Q_OBJECT

public:
	explicit InboxDock(QWidget *parent = nullptr);

private:
	QLabel *emptyLabel;
};
