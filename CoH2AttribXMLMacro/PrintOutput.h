
#pragma once

#include "scintilla/SciLexer.h"

#define MARGIN_SCRIPT_FOLD_INDEX 1

static const COLORREF PRINTOUTPUT_COLOR_WHITE = RGB(0xFF, 0xFF, 0xFF);

static const int PRINTOUTPUT_STYLE_DEFAULT = SCE_LUA_DEFAULT;
static const COLORREF PRINTOUTPUT_COLOR_FG_DEFAULT = RGB(0, 0, 0);
static const COLORREF PRINTOUTPUT_COLOR_BG_DEFAULT = PRINTOUTPUT_COLOR_WHITE;

class PrintOutput
{
private:
	HWND hWnd;
	HWND hEditor;

	int(*directFn)(void*, int, int, int);
	void* directPtr;

	void SetAStyle(int style, COLORREF fore, COLORREF back = PRINTOUTPUT_COLOR_WHITE, bool bold = false, bool italic = false, int size = -1, const char *face = 0);

public:

	LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

	void SetWidth(int newWidth);

	void InitializeEditor(void);
	void Notify(SCNotification* notification);

	PrintOutput(HWND wWnd, HWND hEditor);
	~PrintOutput();
};

