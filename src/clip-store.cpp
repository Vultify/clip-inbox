#include "clip-store.hpp"

ClipStore::ClipStore(QObject *parent) : QObject(parent) {}

void ClipStore::add(const Clip &clip)
{
	for (const Clip &existing : items) {
		if (samePath(existing.path, clip.path))
			return;
	}

	items.append(clip);
	emit clipAdded(clip);
}
