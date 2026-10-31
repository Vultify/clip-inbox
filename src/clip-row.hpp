#pragma once

#include "clip.hpp"

#include <QFrame>

class QLabel;

// One clip in the inbox: what kind it is, its file name, and a box to name it.
class ClipRow : public QFrame {
	Q_OBJECT

public:
	explicit ClipRow(const Clip &clip, QWidget *parent = nullptr);

	void showStatus(const QString &message);

signals:
	void nameEntered(const QString &text);

private:
	QLabel *statusLabel;
};
