#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>

// Starts COM on the current thread for as long as this object lives. The Windows shell APIs need it.
class ComScope {
public:
	ComScope() : result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {}
	~ComScope()
	{
		if (SUCCEEDED(result))
			CoUninitialize();
	}

	ComScope(const ComScope &) = delete;
	ComScope &operator=(const ComScope &) = delete;

	bool ok() const { return SUCCEEDED(result); }

private:
	HRESULT result;
};
#endif
