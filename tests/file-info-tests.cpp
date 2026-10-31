#include "file-info.hpp"
#include "test-support.hpp"

#include <QFile>

static void testFormatDuration()
{
	check(formatDuration(0) == "0:00", "Zero seconds reads 0:00");
	check(formatDuration(42'000) == "0:42", "Short clips show minutes and seconds");
	check(formatDuration(725'500) == "12:05", "Partial seconds are dropped");
	check(formatDuration(3'723'000) == "1:02:03", "Long recordings show hours");
	check(formatDuration(-1).isEmpty(), "An unknown length shows nothing");
}

static void testNotAVideo(const QString &dir)
{
	QString path = dir + "/notes.txt";
	QFile file(path);
	file.open(QIODevice::WriteOnly);
	file.write("not a video");
	file.close();

	ClipDetails details = readClipDetails(path, QSize(192, 192));
	check(details.thumbnail.isNull() && details.durationMs < 0, "A file Windows can't preview has no details");
}

// Set CLIP_INBOX_SAMPLE_VIDEO to a real recording to check thumbnails and lengths against Windows itself.
static void testSampleVideo()
{
	QString path = qEnvironmentVariable("CLIP_INBOX_SAMPLE_VIDEO");
	if (path.isEmpty())
		return;

	ClipDetails details = readClipDetails(path, QSize(192, 192));
	check(!details.thumbnail.isNull(), "Windows makes a thumbnail for the sample video");
	check(details.thumbnail.width() <= 192 && details.thumbnail.height() <= 192,
	      "The thumbnail is shrunk to the size the dock asked for");
	check(details.durationMs > 0, "Windows knows the sample video's length");
	std::printf("      thumbnail %dx%d, length %s\n", details.thumbnail.width(), details.thumbnail.height(),
		    qPrintable(formatDuration(details.durationMs)));
}

void runFileInfoTests(const QString &dir)
{
	testFormatDuration();
	testNotAVideo(dir);
	testSampleVideo();
}
