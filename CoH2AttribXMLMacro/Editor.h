#pragma once

#include "scintilla/SciLexer.h"

#define MARGIN_SCRIPT_FOLD_INDEX 1

static const COLORREF EDITOR_COLOR_WHITE = RGB(0xFF, 0xFF, 0xFF);

static const int EDITOR_STYLE_DEFAULT = SCE_LUA_DEFAULT;
static const COLORREF EDITOR_COLOR_FG_DEFAULT = RGB(0, 0, 0);
static const COLORREF EDITOR_COLOR_BG_DEFAULT = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_COMMENT = SCE_LUA_COMMENT;
static const COLORREF EDITOR_COLOR_FG_COMMENT = RGB(0x00, 0x80, 0x00);
static const COLORREF EDITOR_COLOR_BG_COMMENT = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_COMMENTLINE = SCE_LUA_COMMENTLINE;
static const COLORREF EDITOR_COLOR_FG_COMMENTLINE = RGB(0x00, 0x80, 0x00);
static const COLORREF EDITOR_COLOR_BG_COMMENTLINE = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_COMMENTDOC = SCE_LUA_COMMENTDOC;
static const COLORREF EDITOR_COLOR_FG_COMMENTDOC = RGB(0x00, 0x80, 0x00);
static const COLORREF EDITOR_COLOR_BG_COMMENTDOC = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_LITERALSTRING = SCE_LUA_LITERALSTRING;
static const COLORREF EDITOR_COLOR_FG_LITERALSTRING = RGB(0x95, 0x00, 0x4A);
static const COLORREF EDITOR_COLOR_BG_LITERALSTRING = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_PREPROCESSOR = SCE_LUA_PREPROCESSOR;
static const COLORREF EDITOR_COLOR_FG_PREPROCESSOR = RGB(0x80, 0x40, 0x00);
static const COLORREF EDITOR_COLOR_BG_PREPROCESSOR = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_NUMBER = SCE_LUA_NUMBER;
static const COLORREF EDITOR_COLOR_FG_NUMBER = RGB(0xFF, 0x80, 0x00);
static const COLORREF EDITOR_COLOR_BG_NUMBER = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_STRING = SCE_LUA_STRING;
static const COLORREF EDITOR_COLOR_FG_STRING = RGB(0x80, 0x80, 0x80);
static const COLORREF EDITOR_COLOR_BG_STRING = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_CHARACTER = SCE_LUA_CHARACTER;
static const COLORREF EDITOR_COLOR_FG_CHARACTER = RGB(0x80, 0x80, 0x80);
static const COLORREF EDITOR_COLOR_BG_CHARACTER = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_OPERATOR = SCE_LUA_OPERATOR;
static const COLORREF EDITOR_COLOR_FG_OPERATOR = RGB(0x00, 0x00, 0x80);
static const COLORREF EDITOR_COLOR_BG_OPERATOR = EDITOR_COLOR_WHITE;

static const int EDITOR_STYLE_INSTRUCTIONWORD = SCE_LUA_WORD;
static const COLORREF EDITOR_COLOR_FG_INSTRUCTIONWORD = RGB(0x00, 0x00, 0xFF);
static const COLORREF EDITOR_COLOR_BG_INSTRUCTIONWORD = EDITOR_COLOR_WHITE;
static const char EDITOR_KEYWORDS_INSTRUCTIONWORD[] = "and break do else elseif end false for function goto if in local nil not or repeat return then true until while";

static const int EDITOR_STYLE_FUNC1 = SCE_LUA_WORD2;
static const COLORREF EDITOR_COLOR_FG_FUNC1 = RGB(0x00, 0x80, 0xC0);
static const COLORREF EDITOR_COLOR_BG_FUNC1 = EDITOR_COLOR_WHITE;
static const char EDITOR_KEYWORDS_FUNC1[] = "_ENV _G _VERSION assert collectgarbage dofile error getfenv getmetatable ipairs load loadfile loadstring module next pairs pcall print require select setfenv setmetatable tonumber tostring type unpack xpcall string table math bit32 coroutine io os debug package __index __newindex __call __add __sub __mul __div __mod __pow __unm __concat __len __eq __lt __le __gc __mode loadXML"; // rawequal rawget rawlen rawset

static const int EDITOR_STYLE_FUNC2 = SCE_LUA_WORD3;
static const COLORREF EDITOR_COLOR_FG_FUNC2 = RGB(0x80, 0x00, 0xFF);
static const COLORREF EDITOR_COLOR_BG_FUNC2 = EDITOR_COLOR_WHITE;
static const char EDITOR_KEYWORDS_FUNC2[] = "byte char dump find format gmatch gsub len lower match rep reverse sub upper abs acos asin atan atan2 ceil cos cosh deg exp floor fmod frexp ldexp log log10 max min modf pow rad random randomseed sin sinh sqrt tan tanh arshift band bnot bor btest bxor extract lrotate lshift replace rrotate rshift shift string.byte string.char string.dump string.find string.format string.gmatch string.gsub string.len string.lower string.match string.rep string.reverse string.sub string.upper table.concat table.insert table.maxn table.pack table.remove table.sort table.unpack math.abs math.acos math.asin math.atan math.atan2 math.ceil math.cos math.cosh math.deg math.exp math.floor math.fmod math.frexp math.huge math.ldexp math.log math.log10 math.max math.min math.modf math.pi math.pow math.rad math.random math.randomseed math.sin math.sinh math.sqrt math.tan math.tanh bit32.arshift bit32.band bit32.bnot bit32.bor bit32.btest bit32.bxor bit32.extract bit32.lrotate bit32.lshift bit32.replace bit32.rrotate bit32.rshift";

static const int EDITOR_STYLE_FUNC3 = SCE_LUA_WORD4;
static const COLORREF EDITOR_COLOR_FG_FUNC3 = RGB(0x80, 0x00, 0xA0);
static const COLORREF EDITOR_COLOR_BG_FUNC3 = EDITOR_COLOR_WHITE;
static const char EDITOR_KEYWORDS_FUNC3[] = "close flush lines read seek setvbuf write clock date difftime execute exit getenv remove rename setlocale time tmpname coroutine.create coroutine.resume coroutine.running coroutine.status coroutine.wrap coroutine.yield io.close io.flush io.input io.lines io.open io.output io.popen io.read io.tmpfile io.type io.write io.stderr io.stdin io.stdout os.clock os.date os.difftime os.execute os.exit os.getenv os.remove os.rename os.setlocale os.time os.tmpname debug.debug debug.getfenv debug.gethook debug.getinfo debug.getlocal debug.getmetatable debug.getregistry debug.getupvalue debug.getuservalue debug.setfenv debug.sethook debug.setlocal debug.setmetatable debug.setupvalue debug.setuservalue debug.traceback debug.upvalueid debug.upvaluejoin package.cpath package.loaded package.loaders package.loadlib package.path package.preload package.seeall";

class Editor
{
private:
	HWND hWnd;
	HWND hEditor;

	int (*directFn)(void*, int, int, int);
	void* directPtr;

	void SetAStyle(int style, COLORREF fore, COLORREF back = EDITOR_COLOR_WHITE, bool bold = false, bool italic = false, int size = -1, const char *face = 0);

public:

	LRESULT SendEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

	void SetWidth(int newWidth);

	void InitializeEditor(void);
	void Notify(SCNotification* notification);

	Editor(HWND wWnd, HWND hEditor);
	~Editor();
};
