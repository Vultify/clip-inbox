#include "clip-settings.hpp"
#include "test-support.hpp"

#include <QDate>
#include <QFile>

static void testDefaults(const QString &dir)
{
	ClipSettings settings = loadSettings(dir + "/settings/missing.json");
	check(settings.dateFormat == DEFAULT_DATE_FORMAT && settings.quickTags.isEmpty(),
	      "A missing settings file gives the defaults");

	QString broken = dir + "/settings/broken.json";
	saveSettings(broken, {});
	QFile file(broken);
	file.open(QIODevice::WriteOnly);
	file.write("{ not json");
	file.close();
	check(loadSettings(broken).dateFormat == DEFAULT_DATE_FORMAT, "A damaged settings file gives the defaults");
}

static void testSaveAndLoad(const QString &dir)
{
	QString file = dir + "/settings/settings.json";
	ClipSettings saved;
	saved.dateFormat = "";
	saved.quickTags = {"Valorant", "Apex Legends"};

	check(saveSettings(file, saved), "Settings save");
	ClipSettings loaded = loadSettings(file);
	check(loaded.dateFormat.isEmpty(), "Turning the date off survives a restart");
	check(loaded.quickTags == QStringList({"Valorant", "Apex Legends"}), "Quick tags survive a restart in order");
}

static void testParseQuickTags()
{
	check(parseQuickTags("Valorant\n\n  Apex  \nvalorant\n") == QStringList({"Valorant", "Apex"}),
	      "Tags are trimmed, and blank lines and repeats are dropped");
	check(parseQuickTags("Halo: Infinite") == QStringList({"Halo Infinite"}),
	      "Tags can't hold forbidden characters");
	check(parseQuickTags("Windows line\r\nendings") == QStringList({"Windows line", "endings"}),
	      "Windows line endings don't leave stray characters");

	QStringList many;
	for (int i = 0; i < 50; i++)
		many.append(QString("tag %1").arg(i));
	check(parseQuickTags(many.join('\n')).size() == MAX_QUICK_TAGS, "The number of tags is capped");
}

// Every offered date format has to produce text that's already safe in a file name.
static void testDateFormatChoices()
{
	QDate date(2026, 10, 30);
	bool allSafe = true;
	for (const QString &format : dateFormatChoices()) {
		QString text = date.toString(format);
		if (cleanName(text) != text)
			allSafe = false;
	}
	check(allSafe, "Every date format choice is safe in a file name");
	check(buildStem("clutch", date, "MMM d yyyy") == "clutch - Oct 30 2026", "The written-out date format works");
	check(buildStem("clutch", date, "MM-dd-yyyy") == "clutch - 10-30-2026", "The US date format works");
}

void runClipSettingsTests(const QString &dir)
{
	testDefaults(dir);
	testSaveAndLoad(dir);
	testParseQuickTags();
	testDateFormatChoices();
}
