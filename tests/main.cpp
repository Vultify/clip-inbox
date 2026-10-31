#include "test-support.hpp"

#include <QCoreApplication>
#include <QTemporaryDir>

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QTemporaryDir dir;

	runFileReadyTests(dir.path());
	runClipNamingTests();
	runFileActionsTests(dir.path());
	runClipStoreTests(dir.path());

	std::printf("\n%d failed\n", failures);
	return failures == 0 ? 0 : 1;
}
