#pragma once

#include <QString>

#include <cstdio>

inline int failures = 0;

inline void check(bool condition, const char *description)
{
	std::printf("%s  %s\n", condition ? "PASS" : "FAIL", description);
	if (!condition)
		failures++;
}

// Each test file runs its checks in a scratch folder that's deleted afterwards.
void runFileReadyTests(const QString &dir);
void runClipNamingTests();
void runFileActionsTests(const QString &dir);
void runClipStoreTests(const QString &dir);
