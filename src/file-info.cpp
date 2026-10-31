#include "file-info.hpp"
#include "com-scope.hpp"

#include <QDir>
#include <QFileInfo>

#ifdef _WIN32
#include <initguid.h>
#include <propkey.h>
#include <shobjidl_core.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// Some shell thumbnails come back with every alpha value at zero, which would draw as fully transparent.
static QImage makeOpaqueIfInvisible(const QImage &image)
{
	if (!image.hasAlphaChannel())
		return image;

	QImage argb = image.convertToFormat(QImage::Format_ARGB32);
	for (int y = 0; y < argb.height(); y++) {
		const QRgb *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
		for (int x = 0; x < argb.width(); x++) {
			if (qAlpha(line[x]) != 0)
				return image;
		}
	}

	QImage raw = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
	return QImage(raw.constBits(), raw.width(), raw.height(), raw.bytesPerLine(), QImage::Format_RGB32).copy();
}

static void readShellDetails(const QString &path, const QSize &thumbnailSize, ClipDetails &details)
{
	ComScope com;
	if (!com.ok())
		return;

	std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
	ComPtr<IShellItem2> item;
	if (FAILED(SHCreateItemFromParsingName(nativePath.c_str(), nullptr, IID_PPV_ARGS(&item))))
		return;

	// Windows stores media length in 100-nanosecond units.
	ULONGLONG duration = 0;
	if (SUCCEEDED(item->GetUInt64(PKEY_Media_Duration, &duration)))
		details.durationMs = static_cast<qint64>(duration / 10000);

	// THUMBNAILONLY skips the generic file icon, so clips Windows can't preview get the dock's placeholder instead.
	ComPtr<IShellItemImageFactory> factory;
	HBITMAP bitmap = nullptr;
	SIZE size{thumbnailSize.width(), thumbnailSize.height()};
	if (SUCCEEDED(item.As(&factory)) &&
	    SUCCEEDED(factory->GetImage(size, SIIGBF_THUMBNAILONLY | SIIGBF_BIGGERSIZEOK, &bitmap))) {
		details.thumbnail = makeOpaqueIfInvisible(QImage::fromHBITMAP(bitmap));
		DeleteObject(bitmap);
	}

	// BIGGERSIZEOK can hand back a full-size cached frame, so shrink it here instead of holding megabytes per row.
	if (details.thumbnail.width() > thumbnailSize.width() || details.thumbnail.height() > thumbnailSize.height())
		details.thumbnail =
			details.thumbnail.scaled(thumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
#else
static void readShellDetails(const QString &, const QSize &, ClipDetails &) {}
#endif

ClipDetails readClipDetails(const QString &path, const QSize &thumbnailSize)
{
	ClipDetails details;
	QFileInfo info(path);
	if (info.isFile())
		details.sizeBytes = info.size();

	readShellDetails(path, thumbnailSize, details);
	return details;
}

QString formatDuration(qint64 durationMs)
{
	if (durationMs < 0)
		return {};

	qint64 totalSeconds = durationMs / 1000;
	qint64 hours = totalSeconds / 3600;
	qint64 minutes = (totalSeconds % 3600) / 60;
	qint64 seconds = totalSeconds % 60;

	if (hours > 0)
		return QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
	return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}
