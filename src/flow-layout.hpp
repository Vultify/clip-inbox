#pragma once

#include <QLayout>
#include <QList>

// Lays widgets out left to right and wraps onto a new line when the dock gets narrow, like words in a paragraph.
class FlowLayout : public QLayout {
public:
	explicit FlowLayout(QWidget *parent = nullptr, int spacing = 4);
	~FlowLayout() override;

	void addItem(QLayoutItem *item) override;
	int count() const override;
	QLayoutItem *itemAt(int index) const override;
	QLayoutItem *takeAt(int index) override;

	Qt::Orientations expandingDirections() const override;
	bool hasHeightForWidth() const override;
	int heightForWidth(int width) const override;
	QSize minimumSize() const override;
	QSize sizeHint() const override;
	void setGeometry(const QRect &rect) override;

private:
	int arrange(const QRect &rect, bool apply) const;

	QList<QLayoutItem *> items;
	int gap;
};
