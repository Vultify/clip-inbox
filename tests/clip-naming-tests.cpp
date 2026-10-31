#include "clip-naming.hpp"
#include "test-support.hpp"

static void testCleanName()
{
	check(cleanName("clutch 1v3 ridge") == "clutch 1v3 ridge", "A normal name is left alone");
	check(cleanName("  spaced   out  ") == "spaced out", "Extra spaces are trimmed and collapsed");
	check(cleanName("1v3: ridge/church?") == "1v3 ridge church", "Forbidden characters become spaces");
	check(cleanName("a\\b*c\"d<e>f|g") == "a b c d e f g", "Every forbidden character is removed");
	check(cleanName("tab\there\nnewline") == "tab here newline", "Tabs and newlines become spaces");
	check(cleanName("ends with dots...") == "ends with dots", "Trailing dots are removed");
	check(cleanName("").isEmpty(), "Nothing typed means no name");
	check(cleanName("   ").isEmpty(), "Only spaces means no name");
	check(cleanName("???").isEmpty(), "Only forbidden characters means no name");
	check(cleanName(QString(300, 'x')).size() == MAX_NAME_LENGTH, "Very long names are cut short");
}

static void testBuildStem()
{
	QDate date(2026, 10, 30);
	check(buildStem("clutch 1v3 ridge", date) == "clutch 1v3 ridge - 2026-10-30",
	      "The date is added after the name");
	check(buildStem("clutch", date, "") == "clutch", "No date when the date format is empty");
	check(buildStem("clutch", date, "MM/dd/yyyy") == "clutch - 10 30 2026", "Slashes in a date format are cleaned");
	check(buildStem("con", date, "") == "con_", "Names Windows reserves get an underscore");
	check(buildStem("clutch", QDate(), "yyyy-MM-dd") == "clutch", "No date when the recording date is unknown");
}

static void testNumberedFileName()
{
	check(numberedFileName("clutch", "mp4", 1) == "clutch.mp4", "The first try has no number");
	check(numberedFileName("clutch", "mp4", 2) == "clutch (2).mp4", "The second try adds (2)");
	check(numberedFileName("50%2 off", "mp4", 3) == "50%2 off (3).mp4", "Percent signs in names stay intact");
	check(numberedFileName("clutch", "", 1) == "clutch", "Files without an extension stay that way");
}

void runClipNamingTests()
{
	testCleanName();
	testBuildStem();
	testNumberedFileName();
}
