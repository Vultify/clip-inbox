#pragma once

#include "clip.hpp"
#include "file-info.hpp"

#include <QFrame>

class QLabel;

// One clip in the inbox: its thumbnail and details, a box to name it, and buttons to keep or delete it.
class ClipRow : public QFrame {
	Q_OBJECT

public:
	explicit ClipRow(const Clip &clip, QWidget *parent = nullptr);

	void setDetails(const ClipDetails &details);
	void showStatus(const QString &message);

signals:
	void nameEntered(const QString &text);
	void keepClicked();
	void deleteClicked();

private:
	QString typeText;
	QLabel *thumbnailLabel;
	QLabel *typeLabel;
	QLabel *statusLabel;
};
