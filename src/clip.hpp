#pragma once

#include <QDateTime>
#include <QDir>
#include <QMetaType>
#include <QString>

enum class ClipType { Recording, Replay };

// What the inbox knows about one finished recording or replay.
struct Clip {
	QString path;
	ClipType type = ClipType::Recording;
	QDateTime recordedAt;
};

Q_DECLARE_METATYPE(Clip)

// Windows paths ignore case and slash direction, so compare them the same way the file system does.
inline bool samePath(const QString &a, const QString &b)
{
#ifdef _WIN32
	return QDir::cleanPath(a).compare(QDir::cleanPath(b), Qt::CaseInsensitive) == 0;
#else
	return QDir::cleanPath(a) == QDir::cleanPath(b);
#endif
}
