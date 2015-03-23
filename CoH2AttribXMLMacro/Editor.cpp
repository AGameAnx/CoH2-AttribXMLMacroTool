#include "stdafx.h"
#include "scintilla/SciLexer.h"
#include "scintilla/Scintilla.h"
#include "Editor.h"

Editor::Editor(HWND hWnd, HWND hEditor)
{
	directFn = (int(__cdecl *)(void*, int, int, int))SendMessage(hEditor, SCI_GETDIRECTFUNCTION, 0, 0);
	directPtr = (void*)SendMessage(hEditor, SCI_GETDIRECTPOINTER, 0, 0);

	this->hWnd = hWnd;
	this->hWnd = hEditor;
	InitializeEditor();
}

Editor::~Editor()
{
}

LRESULT Editor::SendEditor(UINT Msg, WPARAM wParam /* = 0 */, LPARAM lParam /* = 0 */)
{
	return directFn(directPtr, Msg, wParam, lParam);
}

void Editor::SetAStyle(int style, COLORREF fore, COLORREF back /* = EDITOR_COLOR_WHITE */, bool bold /* = false */, bool italic /* = false */, int size /* = -1 */, const char *face /* = 0 */) {
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (bold)
		SendEditor(SCI_STYLESETBOLD, style, 1);
	if (italic)
		SendEditor(SCI_STYLESETITALIC, style, 1);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	if (face)
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}

void Editor::Notify(SCNotification* notification)
{
	int tensOfLines;
	
	switch(notification->nmhdr.code) {
	case (SCN_MARGINCLICK) :
		// Folding
		if (notification->margin == MARGIN_SCRIPT_FOLD_INDEX) {
			int lineNr = SendEditor(SCI_LINEFROMPOSITION, (uptr_t)notification->position, 0);
			SendEditor(SCI_TOGGLEFOLD, lineNr, 0);
		}
		break;
	case (SCN_MODIFIED) :
		// Adjust margin size for line numbers
		tensOfLines = (int)max(1, log10(SendEditor(SCI_GETLINECOUNT)));
		SendEditor(SCI_SETMARGINWIDTHN, 0, tensOfLines * 8 + 14);
		break;
	case (SCN_CHARADDED) :
		if (notification->ch == '\n')  { // Auto indentation
			int curLine = SendEditor(SCI_LINEFROMPOSITION, SendEditor(SCI_GETCURRENTPOS));
			if (curLine > 0)
			{
				char linebuf[3000];
				int prevLineLength = SendEditor(SCI_LINELENGTH, curLine - 1);
				if (prevLineLength < sizeof(linebuf)-sizeof(char))
				{
					ZeroMemory(linebuf, sizeof(char)*3000);
					SendEditor(SCI_GETLINE, curLine - 1, reinterpret_cast<LPARAM>(static_cast<char*>(linebuf)));
					for (int pos = 0; linebuf[pos]; pos++)
					{
						if (linebuf[pos] != ' '  &&  linebuf[pos] != '\t')
						{
							linebuf[pos] = '\0';
							break;
						}
					}
					SendEditor(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<char*>(linebuf)));
				}
			}
		}
		break;
	}
}

void Editor::SetWidth(int newWidth)
{
	SendEditor(SCI_SETSCROLLWIDTH, newWidth - SendEditor(SCI_GETMARGINWIDTHN, 0) - SendEditor(SCI_GETMARGINWIDTHN, MARGIN_SCRIPT_FOLD_INDEX));
}
void Editor::InitializeEditor(void)
{
	SendEditor(SCI_SETLEXER, SCLEX_LUA);

	SendEditor(SCI_SETEOLMODE, SC_EOL_LF);
	
	SendEditor(SCI_SETTABWIDTH, 4);
	//SendEditor(SCI_SETSCROLLWIDTHTRACKING, 1);
	SendEditor(SCI_SETMARGINWIDTHN, 0, 22); // enable line numbers
	SendEditor(SCI_SETSCROLLWIDTH, 660);
	//SendEditor(SCI_SETHSCROLLBAR, 0);
	SendEditor(SCI_SETWRAPMODE, SC_WRAP_WORD);
	SendEditor(SCI_SETWRAPINDENTMODE, SC_WRAPINDENT_INDENT);

	SendEditor(SCI_SETPROPERTY, (WPARAM)"fold", (LPARAM)"1");
	SendEditor(SCI_SETMARGINWIDTHN, MARGIN_SCRIPT_FOLD_INDEX, 0);
	SendEditor(SCI_SETMARGINTYPEN, MARGIN_SCRIPT_FOLD_INDEX, SC_MARGIN_SYMBOL);
	SendEditor(SCI_SETMARGINMASKN, MARGIN_SCRIPT_FOLD_INDEX, SC_MASK_FOLDERS);
	SendEditor(SCI_SETMARGINWIDTHN, MARGIN_SCRIPT_FOLD_INDEX, 20);
	SendEditor(SCI_SETMARGINSENSITIVEN, MARGIN_SCRIPT_FOLD_INDEX, 1);

	SendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
	SendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDER, RGB(255, 255, 255));
	SendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDER, RGB(100, 100, 100));

	SendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
	SendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPEN, RGB(255, 255, 255));
	SendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPEN, RGB(100, 100, 100));

	SendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUS);
	SendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEREND, RGB(255, 255, 255));
	SendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEREND, RGB(100, 100, 100));

	SendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);
	SendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERMIDTAIL, RGB(255, 255, 255));
	SendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERMIDTAIL, RGB(100, 100, 100));

	SendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUS);
	SendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDEROPENMID, RGB(255, 255, 255));
	SendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPENMID, RGB(100, 100, 100));

	SendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
	SendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERSUB, RGB(255, 255, 255));
	SendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERSUB, RGB(100, 100, 100));

	SendEditor(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
	SendEditor(SCI_MARKERSETFORE, SC_MARKNUM_FOLDERTAIL, RGB(255, 255, 255));
	SendEditor(SCI_MARKERSETBACK, SC_MARKNUM_FOLDERTAIL, RGB(100, 100, 100));
	
	SendEditor(SCI_SETFOLDFLAGS, SC_FOLDFLAG_LINEAFTER_CONTRACTED, 0);

	SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(EDITOR_KEYWORDS_INSTRUCTIONWORD));
	SendEditor(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(EDITOR_KEYWORDS_FUNC1));
	SendEditor(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(EDITOR_KEYWORDS_FUNC2));
	SendEditor(SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(EDITOR_KEYWORDS_FUNC3));

	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(STYLE_DEFAULT, EDITOR_COLOR_FG_DEFAULT, EDITOR_COLOR_BG_DEFAULT, false, false, 10, "Courier New");
	SendEditor(SCI_STYLECLEARALL);	// Copies global style to all others

	SetAStyle(EDITOR_STYLE_DEFAULT, EDITOR_COLOR_FG_DEFAULT, EDITOR_COLOR_BG_DEFAULT, false, false, 10, "Courier New");

	SetAStyle(EDITOR_STYLE_COMMENT, EDITOR_COLOR_FG_COMMENT, EDITOR_COLOR_BG_COMMENT);
	SetAStyle(EDITOR_STYLE_COMMENTLINE, EDITOR_COLOR_FG_COMMENTLINE, EDITOR_COLOR_BG_COMMENTLINE);
	SetAStyle(EDITOR_STYLE_COMMENTDOC, EDITOR_COLOR_FG_COMMENTDOC, EDITOR_COLOR_BG_COMMENTDOC);
	SetAStyle(EDITOR_STYLE_LITERALSTRING, EDITOR_STYLE_LITERALSTRING, EDITOR_COLOR_BG_LITERALSTRING);
	SetAStyle(EDITOR_STYLE_PREPROCESSOR, EDITOR_COLOR_FG_PREPROCESSOR, EDITOR_COLOR_BG_PREPROCESSOR);
	SetAStyle(EDITOR_STYLE_NUMBER, EDITOR_COLOR_FG_NUMBER, EDITOR_COLOR_BG_NUMBER);
	SetAStyle(EDITOR_STYLE_STRING, EDITOR_COLOR_FG_STRING, EDITOR_COLOR_BG_STRING);
	SetAStyle(EDITOR_STYLE_CHARACTER, EDITOR_COLOR_FG_CHARACTER, EDITOR_COLOR_BG_CHARACTER);
	SetAStyle(EDITOR_STYLE_OPERATOR, EDITOR_COLOR_FG_OPERATOR, EDITOR_COLOR_BG_OPERATOR, true);
	SetAStyle(EDITOR_STYLE_INSTRUCTIONWORD, EDITOR_COLOR_FG_INSTRUCTIONWORD, EDITOR_COLOR_BG_INSTRUCTIONWORD, true);
	SetAStyle(EDITOR_STYLE_FUNC1, EDITOR_COLOR_FG_FUNC1, EDITOR_COLOR_BG_FUNC1, true);
	SetAStyle(EDITOR_STYLE_FUNC2, EDITOR_COLOR_FG_FUNC2, EDITOR_COLOR_BG_FUNC2, true);
	SetAStyle(EDITOR_STYLE_FUNC3, EDITOR_COLOR_FG_FUNC3, EDITOR_COLOR_BG_FUNC3, true, true);
}
