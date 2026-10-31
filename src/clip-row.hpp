#pragma once

#include "clip.hpp"
#include "file-info.hpp"

#include <QFrame>
#include <QStringList>

class FlowLayout;
class QLabel;
class QLineEdit;

// One clip in the inbox: its thumbnail and details, a box to name it, quick tags, and buttons to skip or delete it.
class ClipRow : public QFrame {
	Q_OBJECT

public:
	explicit ClipRow(const Clip &clip, QWidget *parent = nullptr);

	void setDetails(const ClipDetails &details);
	void setQuickTags(const QStringList &tags);
	void showStatus(const QString &message);

signals:
	void nameEntered(const QString &text);
	void skipClicked();
	void deleteClicked();

private:
	void addTagToName(const QString &tag);

	QString typeText;
	QString recordedText;
	QLabel *thumbnailLabel;
	QLabel *detailsLabel;
	QLineEdit *nameEdit;
	QWidget *tagsWidget;
	FlowLayout *tagsLayout;
	QLabel *statusLabel;
};
