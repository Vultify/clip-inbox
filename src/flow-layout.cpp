#include "flow-layout.hpp"

FlowLayout::FlowLayout(QWidget *parent, int spacing) : QLayout(parent), gap(spacing)
{
	setContentsMargins(0, 0, 0, 0);
}

FlowLayout::~FlowLayout()
{
	while (QLayoutItem *item = takeAt(0))
		delete item;
}

void FlowLayout::addItem(QLayoutItem *item)
{
	items.append(item);
}

int FlowLayout::count() const
{
	return items.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
	return items.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
	return index >= 0 && index < items.size() ? items.takeAt(index) : nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const
{
	return {};
}

bool FlowLayout::hasHeightForWidth() const
{
	return true;
}

int FlowLayout::heightForWidth(int width) const
{
	return arrange(QRect(0, 0, width, 0), false);
}

QSize FlowLayout::minimumSize() const
{
	QSize size;
	for (const QLayoutItem *item : items)
		size = size.expandedTo(item->minimumSize());

	QMargins margins = contentsMargins();
	return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

QSize FlowLayout::sizeHint() const
{
	return minimumSize();
}

void FlowLayout::setGeometry(const QRect &rect)
{
	QLayout::setGeometry(rect);
	arrange(rect, true);
}

// Places each item after the previous one and starts a new line when the next one wouldn't fit.
// Returns the height all the lines need.
int FlowLayout::arrange(const QRect &rect, bool apply) const
{
	QRect area = rect.marginsRemoved(contentsMargins());
	int x = area.x();
	int y = area.y();
	int lineHeight = 0;

	for (QLayoutItem *item : items) {
		if (item->isEmpty())
			continue;

		QSize size = item->sizeHint();
		if (x > area.x() && x + size.width() > area.right() + 1) {
			x = area.x();
			y += lineHeight + gap;
			lineHeight = 0;
		}

		if (apply)
			item->setGeometry(QRect(QPoint(x, y), size));

		x += size.width() + gap;
		lineHeight = qMax(lineHeight, size.height());
	}

	return y + lineHeight - rect.y() + contentsMargins().bottom();
}
