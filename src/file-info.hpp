#pragma once

#include <QImage>
#include <QMetaType>
#include <QSize>
#include <QString>

// What Windows knows about a clip: the same thumbnail, length, and size File Explorer shows.
struct ClipDetails {
	QImage thumbnail;
	qint64 durationMs = -1;
	qint64 sizeBytes = -1;
};

Q_DECLARE_METATYPE(ClipDetails)

// Empty fields mean Windows couldn't tell, for example a codec without its Microsoft Store extension.
ClipDetails readClipDetails(const QString &path, const QSize &thumbnailSize);

// "0:42", "12:05", or "1:02:03".
QString formatDuration(qint64 durationMs);
