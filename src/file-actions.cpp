#include "file-actions.hpp"
#include "clip-naming.hpp"
#include "clip.hpp"
#include "com-scope.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

#ifdef _WIN32
#include <shellapi.h>
#include <shobjidl_core.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
#endif

static constexpr int MAX_NUMBERED_ATTEMPTS = 999;
static const QSize THUMBNAIL_REQUEST_SIZE(192, 192);

enum class MoveResult { Moved, TargetExists, SourceBusy, SourceMissing, Error };

#ifdef _WIN32
// Without MOVEFILE_REPLACE_EXISTING, Windows itself refuses to touch a file that already has the new name.
static MoveResult moveWithoutReplacing(const QString &from, const QString &to, QString &error)
{
	std::wstring source = QDir::toNativeSeparators(from).toStdWString();
	std::wstring target = QDir::toNativeSeparators(to).toStdWString();
	if (MoveFileExW(source.c_str(), target.c_str(), 0))
		return MoveResult::Moved;

	DWORD code = GetLastError();
	switch (code) {
	case ERROR_ALREADY_EXISTS:
	case ERROR_FILE_EXISTS:
		return MoveResult::TargetExists;
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
		return MoveResult::SourceBusy;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
		return MoveResult::SourceMissing;
	default:
		error = qt_error_string(static_cast<int>(code));
		return MoveResult::Error;
	}
}
#else
static MoveResult moveWithoutReplacing(const QString &from, const QString &to, QString &error)
{
	if (QFileInfo::exists(to))
		return MoveResult::TargetExists;
	if (!QFileInfo::exists(from))
		return MoveResult::SourceMissing;

	QFile file(from);
	if (file.rename(to))
		return MoveResult::Moved;

	error = file.errorString();
	return MoveResult::Error;
}
#endif

RenameResult renameClipFile(const QString &path, const QString &stem)
{
	QFileInfo info(path);
	if (!info.isFile())
		return {RenameOutcome::Missing};

	QDir folder = info.dir();
	QString extension = info.suffix();

	for (int attempt = 1; attempt <= MAX_NUMBERED_ATTEMPTS; attempt++) {
		QString target = folder.filePath(numberedFileName(stem, extension, attempt));
		if (samePath(target, path))
			return {RenameOutcome::Renamed, path};

		QString error;
		switch (moveWithoutReplacing(path, target, error)) {
		case MoveResult::Moved:
			return {RenameOutcome::Renamed, target};
		case MoveResult::TargetExists:
			continue;
		case MoveResult::SourceBusy:
			return {RenameOutcome::Busy};
		case MoveResult::SourceMissing:
			return {RenameOutcome::Missing};
		case MoveResult::Error:
			return {RenameOutcome::Failed, {}, error};
		}
	}

	return {RenameOutcome::Failed, {}, QStringLiteral("every numbered version of this name is taken")};
}

#ifdef _WIN32
// Windows only lets a file be deleted when every app that has it open allowed that.
static bool isLockedAgainstDelete(const QString &path)
{
	std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
	HANDLE handle = CreateFileW(nativePath.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				    nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (handle == INVALID_HANDLE_VALUE)
		return GetLastError() == ERROR_SHARING_VIOLATION;

	CloseHandle(handle);
	return false;
}

// Windows asks this sink before each delete. Items it can't recycle (too big for the Recycle Bin, or on a drive
// without one) arrive without the recycle flag, and returning E_ABORT cancels them instead of deleting for good.
class RecycleOnlySink final : public IFileOperationProgressSink {
public:
	bool refusedPermanentDelete = false;
	HRESULT deleteResult = S_OK;

	// The sink lives on the stack for a single operation, so reference counting has nothing to do.
	IFACEMETHODIMP QueryInterface(REFIID riid, void **object) override
	{
		if (riid == IID_IUnknown || riid == __uuidof(IFileOperationProgressSink)) {
			*object = static_cast<IFileOperationProgressSink *>(this);
			return S_OK;
		}
		*object = nullptr;
		return E_NOINTERFACE;
	}
	IFACEMETHODIMP_(ULONG) AddRef() override { return 1; }
	IFACEMETHODIMP_(ULONG) Release() override { return 1; }

	IFACEMETHODIMP PreDeleteItem(DWORD flags, IShellItem *) override
	{
		if (flags & TSF_DELETE_RECYCLE_IF_POSSIBLE)
			return S_OK;

		refusedPermanentDelete = true;
		return E_ABORT;
	}

	IFACEMETHODIMP PostDeleteItem(DWORD, IShellItem *, HRESULT result, IShellItem *) override
	{
		deleteResult = result;
		return S_OK;
	}

	IFACEMETHODIMP StartOperations() override { return S_OK; }
	IFACEMETHODIMP FinishOperations(HRESULT) override { return S_OK; }
	IFACEMETHODIMP PreRenameItem(DWORD, IShellItem *, LPCWSTR) override { return S_OK; }
	IFACEMETHODIMP PostRenameItem(DWORD, IShellItem *, LPCWSTR, HRESULT, IShellItem *) override { return S_OK; }
	IFACEMETHODIMP PreMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) override { return S_OK; }
	IFACEMETHODIMP PostMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT, IShellItem *) override
	{
		return S_OK;
	}
	IFACEMETHODIMP PreCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) override { return S_OK; }
	IFACEMETHODIMP PostCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT, IShellItem *) override
	{
		return S_OK;
	}
	IFACEMETHODIMP PreNewItem(DWORD, IShellItem *, LPCWSTR) override { return S_OK; }
	IFACEMETHODIMP PostNewItem(DWORD, IShellItem *, LPCWSTR, LPCWSTR, DWORD, HRESULT, IShellItem *) override
	{
		return S_OK;
	}
	IFACEMETHODIMP UpdateProgress(UINT, UINT) override { return S_OK; }
	IFACEMETHODIMP ResetTimer() override { return S_OK; }
	IFACEMETHODIMP PauseTimer() override { return S_OK; }
	IFACEMETHODIMP ResumeTimer() override { return S_OK; }
};

static RecycleResult recycleFailure(HRESULT result)
{
	return {RecycleOutcome::Failed, qt_error_string(static_cast<int>(result))};
}

RecycleResult recycleClipFile(const QString &path)
{
	if (!QFileInfo(path).isFile())
		return {RecycleOutcome::Missing};
	if (isLockedAgainstDelete(path))
		return {RecycleOutcome::Busy};

	ComScope com;
	if (!com.ok())
		return {RecycleOutcome::Failed, QStringLiteral("Windows shell services aren't available")};

	ComPtr<IFileOperation> operation;
	HRESULT result = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&operation));
	if (FAILED(result))
		return recycleFailure(result);

	result = operation->SetOperationFlags(FOF_ALLOWUNDO | FOF_NO_UI | FOFX_RECYCLEONDELETE);
	if (FAILED(result))
		return recycleFailure(result);

	std::wstring nativePath = QDir::toNativeSeparators(path).toStdWString();
	ComPtr<IShellItem> item;
	result = SHCreateItemFromParsingName(nativePath.c_str(), nullptr, IID_PPV_ARGS(&item));
	if (FAILED(result))
		return recycleFailure(result);

	RecycleOnlySink sink;
	result = operation->DeleteItem(item.Get(), &sink);
	if (SUCCEEDED(result))
		result = operation->PerformOperations();

	if (sink.refusedPermanentDelete)
		return {RecycleOutcome::NotRecyclable};
	if (FAILED(result))
		return recycleFailure(result);
	if (FAILED(sink.deleteResult))
		return recycleFailure(sink.deleteResult);
	if (QFileInfo::exists(path))
		return {RecycleOutcome::Failed, QStringLiteral("Windows left the file in place")};

	return {RecycleOutcome::Recycled};
}
#else
// The macOS and Linux trash never deletes for good, so Qt's built-in move is safe there.
RecycleResult recycleClipFile(const QString &path)
{
	if (!QFileInfo(path).isFile())
		return {RecycleOutcome::Missing};

	QFile file(path);
	if (file.moveToTrash())
		return {RecycleOutcome::Recycled};
	return {RecycleOutcome::Failed, file.errorString()};
}
#endif

FileActions::FileActions(int retryIntervalMs, QObject *parent) : QObject(parent), retryTimer(new QTimer(this))
{
	retryTimer->setInterval(retryIntervalMs);
	connect(retryTimer, &QTimer::timeout, this, &FileActions::retryWaiting);
}

void FileActions::rename(const QString &path, const QString &stem)
{
	// A newer name replaces one that was still waiting for the file.
	waiting.remove(path);
	if (tryRename(path, stem))
		return;

	waiting.insert(path, stem);
	emit renameWaiting(path);
	if (!retryTimer->isActive())
		retryTimer->start();
}

void FileActions::recycle(const QString &path)
{
	cancel(path);

	RecycleResult result = recycleClipFile(path);
	switch (result.outcome) {
	case RecycleOutcome::Recycled:
		emit recycled(path);
		break;
	case RecycleOutcome::Missing:
		emit missing(path);
		break;
	case RecycleOutcome::Busy:
		emit recycleBusy(path);
		break;
	case RecycleOutcome::NotRecyclable:
		emit recycleRefused(path);
		break;
	case RecycleOutcome::Failed:
		emit recycleFailed(path, result.error);
		break;
	}
}

// Forgets a rename that was still waiting for the file, for when the clip leaves the inbox another way.
void FileActions::cancel(const QString &path)
{
	waiting.remove(path);
	if (waiting.isEmpty())
		retryTimer->stop();
}

void FileActions::loadDetails(const QString &path)
{
	emit detailsLoaded(path, readClipDetails(path, THUMBNAIL_REQUEST_SIZE));
}

void FileActions::dropMissing(const QStringList &paths)
{
	for (const QString &path : paths) {
		if (!QFileInfo(path).isFile())
			emit missing(path);
	}
}

void FileActions::stop()
{
	retryTimer->stop();
	waiting.clear();
}

// Returns false only when another app is holding the file, so the caller knows to try again later.
bool FileActions::tryRename(const QString &path, const QString &stem)
{
	RenameResult result = renameClipFile(path, stem);
	switch (result.outcome) {
	case RenameOutcome::Renamed:
		emit renamed(path, result.newPath);
		return true;
	case RenameOutcome::Missing:
		emit missing(path);
		return true;
	case RenameOutcome::Failed:
		emit renameFailed(path, result.error);
		return true;
	case RenameOutcome::Busy:
		return false;
	}
	return true;
}

void FileActions::retryWaiting()
{
	const auto snapshot = waiting;
	for (auto it = snapshot.cbegin(); it != snapshot.cend(); ++it) {
		if (tryRename(it.key(), it.value()))
			waiting.remove(it.key());
	}

	if (waiting.isEmpty())
		retryTimer->stop();
}
