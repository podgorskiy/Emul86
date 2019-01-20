#include <string>
#include <stdarg.h>
#include "_assert.h"

#ifdef _WIN32
#include <windows.h>
__declspec(thread) HHOOK DebugMessageBoxHook;

void(*Assert::OpPause)() = nullptr;

static LRESULT CALLBACK CustomMessageBoxProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HCBT_ACTIVATE) {
		HWND hChildWnd = (HWND)wParam;
		UINT result;
		if (GetDlgItem(hChildWnd, IDYES))
		{
			result = SetDlgItemTextA(hChildWnd, IDYES, "Debug");
		}
		if (GetDlgItem(hChildWnd, IDNO))
		{
			result = SetDlgItemTextA(hChildWnd, IDNO, "Ignore");
		}
		if (GetDlgItem(hChildWnd, IDCANCEL))
		{
			result = SetDlgItemTextA(hChildWnd, IDCANCEL, "Continue");
		}
		UnhookWindowsHookEx(DebugMessageBoxHook);
	}
	else
	{
		CallNextHookEx(DebugMessageBoxHook, nCode, wParam, lParam);
	}
	return 0;
}
int DebugMessageBox(HWND hwnd, const char* lpText, const char* lpCaption, UINT uType)
{
	DebugMessageBoxHook = SetWindowsHookEx(WH_CBT, &CustomMessageBoxProc, 0, GetCurrentThreadId());
	return MessageBoxA(hwnd, lpText, lpCaption, uType);
}
#endif

static Assert::result ShowMessageBox(const char *file, int line, const char *condition, const char *fmt, va_list ap)
{
#ifdef _WIN32
	char buf[2048];
	va_list apCopy;
	va_copy(apCopy, ap);
	int count = vsnprintf(&buf[0], 2048, fmt, apCopy);
	char caption[256];
	std::string message = buf;
	message += "\n";
	if (condition != nullptr)
	{
		message += condition;
		message += " - is false\n";
	}
	sprintf(caption, "Assert failed at: %s(%d)", file, line);
	int value = DebugMessageBox(0, message.c_str(), caption, MB_ICONWARNING | MB_YESNOCANCEL | MB_SYSTEMMODAL);
	if (value == IDYES)
	{
		return Assert::result_break;
	}
	else if (value == IDNO)
	{
		return Assert::result_ignore_always;
	}
	else if (value == IDCANCEL)
	{
		return Assert::result_ignore_once;
	}
	return Assert::result_break;
#else
	return Assert::result_break;
#endif
}

Assert::result Assert::message(const char *file, int line, const char *condition, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	result ab = ShowMessageBox(file, line, condition, fmt, ap);
	va_end(ap);
	return ab;
}
