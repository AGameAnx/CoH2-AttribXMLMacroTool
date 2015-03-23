
#include "stdafx.h"
#include "CoH2AttribXMLMacro.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib") 

using namespace std;
using namespace rapidxml;

#define MAX_LOADSTRING 100

#define MM_SELECTATTRIBXML 11001
#define MM_SELECTFOLDER 11002
#define MM_RUNMACRO 11003
#define MM_TOGGLEVIEW 11004

HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];

Editor* editor;
//PrintOutput* printOutput;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

HWND hAttribXMLSelectBtn;
HWND hAttribXMLSelectBtnText;
HWND hFolderSelectBtn;
HWND hFolderSelectBtnText;
HWND hRunMacroBtn;
HWND hMacroTextarea;
HWND hScintilla;
HWND hPrintOutput;
HWND hToggleViewBtn;
HWND hWorkingText;

HBRUSH hbrBkgnd;

WCHAR sAttribFilename[MAX_PATH]; // buffer for attrib xml file name
WCHAR sAttribFolder[MAX_PATH]; // buffer for attrib xml file folder name
WCHAR sFolderName[MAX_PATH]; // buffer for folder name

wofstream logFile;
wstring configFile;
WCHAR configFileName[] = L"config.txt";

char* currentlyParsedFile = NULL;

int GetDir(WCHAR *fullPath, WCHAR *dir) {
	WCHAR buff[MAX_PATH] = { 0 };
	int buffCounter = 0;
	int dirSymbolCounter = 0;

	for (size_t i = 0; i < _tcslen(fullPath); i++) {
		if (fullPath[i] != L'\\') {
			if (buffCounter < MAX_PATH) buff[buffCounter++] = fullPath[i];
			else return -1;
		}
		else {
			for (int i2 = 0; i2 < buffCounter; i2++) {
				dir[dirSymbolCounter++] = buff[i2];
				buff[i2] = 0;
			}

			dir[dirSymbolCounter++] = fullPath[i];
			buffCounter = 0;
		}
	}

	return dirSymbolCounter;
}
const WCHAR* GetFileExt(const WCHAR* filename) {
	const WCHAR* dot = wcschr(filename, L'.');
	if (!dot || dot == filename) return L"";
	return dot + 1;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                       _In_opt_ HINSTANCE hPrevInstance,
                       _In_ LPTSTR    lpCmdLine,
                       _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	InitCommonControls();

	MSG msg;
	HACCEL hAccelTable;

	WCHAR filenameBuf[MAX_PATH];
	GetModuleFileName(NULL, filenameBuf, MAX_PATH);
	wstring::size_type pos = wstring(filenameBuf).find_last_of(L"\\/");
	configFile = wstring(filenameBuf).substr(0, pos).append(L"\\").append(configFileName);

	logFile.open("output.txt", ofstream::binary);
	logFile.clear();

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_COH2ATTRIBXMLMACRO, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_COH2ATTRIBXMLMACRO));
	
	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	logFile.close();

	return (int) msg.wParam;
}
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COH2ATTRIBXMLMACRO));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_COH2ATTRIBXMLMACRO);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowEx(
	   0, szWindowClass, szTitle,
	   WS_SYSMENU | WS_THICKFRAME | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
	   CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
	   NULL, NULL, hInstance, NULL);
   

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}


// ------------------------------------------
// LUA

void luaCF_rebuildXML(lua_State *L, xml_node<char>* node);
int iterateDir(HWND hWnd, lua_State *L, const WCHAR* dir);
void BuildLuaXMLGroupNode(lua_State *L, const xml_node<WCHAR>* curNode, const bool includeNameAttribute = false);
void BuildLuaXMLAttribVariable(lua_State *L, const WCHAR* filePath, const WCHAR* filename, const xml_document<WCHAR>* doc);

// ------------------------------------------
// Helper functions

/*inline bool FileExists(const std::string& name) {
struct stat buffer;
return (stat(name.c_str(), &buffer) == 0);
}*/
void saveConfig(void)
{
	FILE* fp = _wfopen(configFile.c_str(), L"w, ccs=UTF-8");
	if (fp != NULL)
	{
		fputws(sAttribFilename, fp);
		fputwc(L'\n', fp);
		fputws(sFolderName, fp);
		fclose(fp);
	}
}
void setCurrentAttribXMLFile(const WCHAR* filename)
{
	ZeroMemory(sAttribFilename, sizeof(WCHAR)*MAX_PATH);
	wcscpy(sAttribFilename, filename);

	WCHAR sShortPath[65]; ZeroMemory(sShortPath, sizeof(WCHAR)* 65);
	int iFilenameLength = wcslen(sAttribFilename);
	if (iFilenameLength > 57)
	{
		sShortPath[0] = '.';
		sShortPath[1] = '.';
		sShortPath[2] = '.';
		wcsncpy_s(sShortPath + 3, 60, sAttribFilename + (iFilenameLength - 57), 57);
		SetWindowText(hAttribXMLSelectBtnText, sShortPath);
	}
	else {
		SetWindowText(hAttribXMLSelectBtnText, sAttribFilename);
	}

	size_t pathLength = wcslen(sShortPath);
	char* mbShortPath = new char[pathLength + 5]; ZeroMemory(mbShortPath, sizeof(char)*(pathLength + 5));
	WideCharToMultiByte(CP_ACP, 0, sShortPath, -1, mbShortPath, pathLength, NULL, NULL);

	EnableWindow(hFolderSelectBtn, true);

	ZeroMemory(sAttribFolder, sizeof(WCHAR)*MAX_PATH);
	GetDir(sAttribFilename, sAttribFolder);

	saveConfig();
}
void setCurrentAttribDir(const WCHAR* dirname)
{
	ZeroMemory(sFolderName, sizeof(WCHAR)*MAX_PATH);
	wcscpy(sFolderName, dirname);

	WCHAR sShortPath[65]; ZeroMemory(sShortPath, sizeof(WCHAR)* 65);
	int iFilenameLength = wcslen(sFolderName);
	if (iFilenameLength > 57)
	{
		sShortPath[0] = '.';
		sShortPath[1] = '.';
		sShortPath[2] = '.';
		wcsncpy_s(sShortPath + 3, 60, sFolderName + (iFilenameLength - 57), 57);
		SetWindowText(hFolderSelectBtnText, sShortPath);
	}
	else {
		SetWindowText(hFolderSelectBtnText, sFolderName);
	}

	EnableWindow(hRunMacroBtn, true);

	saveConfig();
}
void printMessage(const WCHAR* msg)
{
	OutputDebugString(msg);
	//size_t msgLength = wcslen(msg);
	//char* printOutputMessage = new char[msgLength + 5]; ZeroMemory(printOutputMessage, sizeof(char)*(msgLength + 5));
	//WideCharToMultiByte(CP_ACP, 0, msg, -1, printOutputMessage, msgLength, NULL, NULL);
	//printOutput->SendEditor(SCI_ADDTEXT, msgLength, reinterpret_cast<LPARAM>(printOutputMessage));
	//delete[] printOutputMessage;
	int ndx = GetWindowTextLength(hPrintOutput);
	SetFocus(hPrintOutput);
	SendMessage(hPrintOutput, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	SendMessage(hPrintOutput, EM_REPLACESEL, 1, reinterpret_cast<LPARAM>(msg));

	logFile << msg;
	logFile.flush();
}
void printMessage(const char* msg)
{
	size_t msgLength = strlen(msg);
	WCHAR* printOutputMessage = new WCHAR[msgLength + 5]; ZeroMemory(printOutputMessage, sizeof(WCHAR)*(msgLength + 5));
	MultiByteToWideChar(CP_ACP, 0, msg, -1, printOutputMessage, msgLength);
	printMessage(printOutputMessage);
	delete[] printOutputMessage;
}

int openXML(xml_document<WCHAR>* doc, WCHAR** sFile, const WCHAR* filePath) {
	FILE* fp;
	if (fp = _wfopen(filePath, L"r, ccs=UTF-8"))
	{
		wstring xmlStr;
		WCHAR sLine[3005] = { 0 };
		int readLength;
		while (readLength = fread_s(sLine, sizeof(WCHAR)* 3005, sizeof(WCHAR), 3000, fp))
		{
			sLine[readLength] = L'\0';
			xmlStr.append(sLine);
		}
		fclose(fp);

		int xmlStrLength = xmlStr.length();
		*sFile = new WCHAR[xmlStrLength + 5]; ZeroMemory(*sFile, sizeof(WCHAR)*(xmlStrLength + 5));
		wcsncpy_s(*sFile, xmlStrLength + 5, xmlStr.c_str(), xmlStrLength);

		try {
			doc->parse<0>(*sFile);
			if (doc->first_node())
			{
				return 1;
			}
		}
		catch (parse_error e)
		{
			wstring errorMsg = L"XML Parse error\r\n  ";
			errorMsg.append(filePath);
			errorMsg.append(L"\r\n\r\n");

			int whatLen = strlen(e.what());
			WCHAR* whatError = new WCHAR[whatLen + 3](); ZeroMemory(whatError, sizeof(WCHAR)*(whatLen + 3));
			MultiByteToWideChar(CP_ACP, 0, e.what(), -1, whatError, whatLen);

			errorMsg.append(whatError);
			errorMsg.append(L" near \"");
			errorMsg.append(e.where<WCHAR>(), 40);
			errorMsg.append(L"\"\r\n");

			printMessage(errorMsg.c_str());

			delete[] whatError;
			return 0;
		}
	}
	else {
		wstring errorMsg = L"Error reading file\r\n  ";
		errorMsg.append(filePath);

		/*MessageBox(hWnd,
		errorMsg,
		L"Error reading file",
		MB_OK | MB_ICONERROR);*/

		printMessage(errorMsg.c_str());
		printMessage(L"\r\n");
	}
	return 0;
}

// ------------------------------------------
// LUA Callables

int luaCF_loadXML(lua_State *L)
{
	const char *filepath;
	size_t length;

	filepath = lua_tolstring(L, -1, &length); // filename arg

	LPWSTR wdir = new WCHAR[length + 1](); ZeroMemory(wdir, sizeof(WCHAR)*(length + 1));
	LPWSTR wfilepath = new WCHAR[length + 1](); ZeroMemory(wfilepath, sizeof(WCHAR)*(length + 1));
	MultiByteToWideChar(CP_ACP, 0, filepath, -1, wfilepath, length);

	//const WCHAR* fileExt = GetFileExt(wfilepath);
	//if (wcscmp(fileExt, L"xml") == 0)
	//{
		int dirLength = GetDir(wfilepath, wdir);
		WCHAR* sFile;
		xml_document<WCHAR> doc;
		if (!openXML(&doc, &sFile, wfilepath))
		{
			return luaL_error(L, "Error loading XML");
		}
		BuildLuaXMLAttribVariable(L, wfilepath, wfilepath+dirLength, &doc);

		delete[] wdir;
		delete[] wfilepath;
		return 1;
	//}

	//delete[] wdir;
	//delete[] wfilepath;
	//return luaL_error(L, "File extension must be xml");
}
int luaCF_saveXML(lua_State *L)
{
	const char* filename = NULL;
	xml_document<char>* doc = new xml_document<char>();

	lua_pushstring(L, "path");
	lua_gettable(L, -2);
	int indexType = lua_type(L, -1);
	if (indexType != LUA_TSTRING || !lua_isstring(L, -1))
	{
		delete doc;
		printMessage(L"Error: Invalid path value type\n");
		return 0;
	}
	filename = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_pushstring(L, "instance");
	lua_gettable(L, -2);
	int valueType = lua_type(L, -1);
	if (valueType != LUA_TTABLE || !lua_istable(L, -1))
	{
		delete doc;
		printMessage(L"Error: Invalid instance value type\n");
		return 0;
	}
	xml_node<char>* node = doc->allocate_node(node_type::node_element, "instance");
	doc->append_node(node);
	luaCF_rebuildXML(L, node);
	lua_pop(L, 1);

	/*lua_pushnil(L);
	while (lua_next(L, -2) != 0)
	{
		int indexType = lua_type(L, -2);
		if (indexType == LUA_TSTRING && lua_isstring(L, -2)) // make sure we don't confuse lua_next
		{
			int valueType = lua_type(L, -1);
			if (valueType == LUA_TTABLE && lua_istable(L, -1))
			{
				const char* index = lua_tostring(L, -2);
				if (strcmp(index, "path") == 0) {
					filename = lua_tostring(L, -1);
				}
				else if (strcmp(index, "instance") == 0) {
					xml_node<char>* node = doc->allocate_node(node_type::node_element, "instance");
					doc->append_node(node);
					luaCF_rebuildXML(L, node);
				}
			}
		}
		
		lua_pop(L, 1);
	}*/

	if (filename != NULL)
	{
		ofstream ofs(filename);
		if (!ofs.good())
		{
			delete doc;
			return 0;
		}
		rapidxml::print<char>(ofs, *doc->last_node());
		ofs.close();
	}
	else {
		delete doc;

		printMessage(L"Error saving XML!\n");

		return 0;
	}

	lua_gc(L, LUA_GCCOLLECT, 0);
	delete doc;

	return 1;
}
int luaCF_print(lua_State *L)
{
	int n = lua_gettop(L); // number of arguments
	int i;
	lua_getglobal(L, "tostring");
	for (i = 1; i <= n; i++)
	{
		const char *s;
		size_t l;
		lua_pushvalue(L, -1); // function to be called
		lua_pushvalue(L, i); // value to print
		lua_call(L, 1, 1);
		s = lua_tolstring(L, -1, &l); // get result
		if (s == NULL)
			return luaL_error(L, LUA_QL("tostring") " must return a string to " LUA_QL("print"));
		if (i > 1)
		{
			printMessage(L"\t");
		}
		int slen = strlen(s);
		LPWSTR ws = new WCHAR[slen + 1]; ZeroMemory(ws, sizeof(WCHAR)*(slen + 1));
		MultiByteToWideChar(CP_ACP, 0, s, -1, ws, slen);
		printMessage(ws);
		delete ws;
		lua_pop(L, 1); // pop result
	}
	printMessage(L"\n");
	return 0;
}
int luaCF_xmlNewIndex(lua_State *L)
{
	return luaL_error(L, "Attempt to modify a read-only value");
}

// ------------------------------------------
// LUA Helpers

void luaCF_stacktrace(lua_State* L)
{
	lua_Debug entry;
	int depth = 0;

	char messageBuf[1000];
	while (lua_getstack(L, depth, &entry))
	{
		int status = lua_getinfo(L, "Sln", &entry);
		assert(status);

		ZeroMemory(messageBuf, 1000 * sizeof(char));
		sprintf(messageBuf, "%s(%d) : %s\n", entry.short_src, entry.currentline, entry.name ? entry.name : " ? ");
		printMessage(messageBuf);
		depth++;
	}
}
void report_lua_errors(const HWND hWnd, lua_State *L, const int status)
{
	if (status != 0)
	{
		const WCHAR* sErrM;
		switch (status)
		{
		case LUA_ERRRUN: sErrM = L"LUA Runtime error : %s"; break;
		case LUA_ERRMEM: sErrM = L"LUA Memory error : %s"; break;
		case LUA_ERRERR: sErrM = L"LUA Error error : %s"; break;
		default: sErrM = L"LUA Unknown error : %s"; break;
		}

		const char* error = lua_tostring(L, -1);
		WCHAR werror[4096];
		MultiByteToWideChar(CP_ACP, 0, error, -1, werror, 4096);

		WCHAR werror2[4096 + 200];
		swprintf_s(werror2, sErrM, werror);

		printMessage(werror2);
		printMessage(L"\n");

		MessageBox(hWnd,
			werror2,
			L"LUA Error",
			MB_OK | MB_ICONERROR);
		lua_pop(L, 1); // remove error message
	}
}

void luaCF_rebuildXMLAttributes(lua_State *L, xml_node<char>* node)
{
	lua_checkstack(L, 2);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0)
	{
		int valueType = lua_type(L, -2);
		if (valueType == LUA_TSTRING) { // Make sure we don't confuse lua_next
			if (lua_isstring(L, -2) && lua_isstring(L, -1))
			{
				xml_attribute<char>* attr = node->document()->allocate_attribute(lua_tostring(L, -2), lua_tostring(L, -1));
				node->append_attribute(attr);
			}
		}

		lua_pop(L, 1);
	}
}
void luaCF_rebuildList(lua_State *L, xml_node<char>* node)
{
	lua_checkstack(L, 3);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0)
	{
		int indexType = lua_type(L, -2);
		if (indexType == LUA_TNUMBER && lua_isnumber(L, -2)) // Lists are indexed with numbers
		{
			int valueType = lua_type(L, -1);
			if (valueType == LUA_TTABLE && lua_istable(L, -1))
			{
				lua_getfield(L, -1, "@type");
				const char* type;
				if (lua_isnoneornil(L, -1)) {
					type = "group";
				}
				else {
					type = lua_tostring(L, -1);
				}
				lua_pop(L, 1);

				xml_node<char>* nextNode = node->document()->allocate_node(node_type::node_element, type);
				node->append_node(nextNode);
				luaCF_rebuildXML(L, nextNode);
			}
		}

		lua_pop(L, 1);
	}
}
void luaCF_rebuildXML(lua_State *L, xml_node<char>* node)
{
	lua_checkstack(L, 3);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0)
	{
		int indexType = lua_type(L, -2);
		if (indexType == LUA_TSTRING && lua_isstring(L, -2)) // make sure we don't confuse lua_next
		{
			int type = lua_type(L, -1);
			if (type == LUA_TNUMBER && lua_isnumber(L, -1))
			{
				xml_node<char>* nextNode = node->document()->allocate_node(node_type::node_element, "float");
				node->append_node(nextNode);

				xml_attribute<char>* nameAttribute = node->document()->allocate_attribute("name", lua_tostring(L, -2));
				nextNode->append_attribute(nameAttribute);

				char valueBuff[FLT_MAX_10_EXP + 2] = { 0 };
				LUA_NUMBER num = lua_tonumber(L, -1);
				sprintf_s(valueBuff, "%G", num);
				xml_attribute<char>* valueAttribute = node->document()->allocate_attribute("value", node->document()->allocate_string(valueBuff));
				nextNode->append_attribute(valueAttribute);
			}
			else if (type == LUA_TBOOLEAN && lua_isboolean(L, -1))
			{
				xml_node<char>* nextNode = node->document()->allocate_node(node_type::node_element, "bool");
				node->append_node(nextNode);

				xml_attribute<char>* nameAttribute = node->document()->allocate_attribute("name", lua_tostring(L, -2));
				nextNode->append_attribute(nameAttribute);

				xml_attribute<char>* valueAttribute = node->document()->allocate_attribute("value", lua_toboolean(L, -1) == 1 ? "True" : "False");
				nextNode->append_attribute(valueAttribute);
			}
			else if (type == LUA_TSTRING && lua_isstring(L, -1))
			{
				const char* name = lua_tostring(L, -2);
				if (strcmp(name, "@type") == 0) {
					const char* value = lua_tostring(L, -1);
					if (strcmp(value, "template_reference") == 0 || strcmp(value, "list") == 0)
						node->value("\n");
				}
				else
				{
					xml_node<char>* nextNode = node->document()->allocate_node(node_type::node_element, "string");
					node->append_node(nextNode);

					xml_attribute<char>* nameAttribute = node->document()->allocate_attribute("name", name);
					nextNode->append_attribute(nameAttribute);

					xml_attribute<char>* valueAttribute = node->document()->allocate_attribute("value", lua_tostring(L, -1));
					nextNode->append_attribute(valueAttribute);
				}
			}
			else if (type == LUA_TTABLE && lua_istable(L, -1))
			{
				const char* name = lua_tostring(L, -2);
				if (strcmp(name, "@attributes") == 0)
				{
					luaCF_rebuildXMLAttributes(L, node);
				}
				else
				{
					lua_getfield(L, -1, "@type");
					const char* type;
					if (lua_isnoneornil(L, -1)) {
						type = "group";
					}
					else {
						type = lua_tostring(L, -1);
					}
					lua_pop(L, 1);

					xml_node<char>* nextNode = node->document()->allocate_node(node_type::node_element, type);
					xml_attribute<char>* nameAttribute = node->document()->allocate_attribute("name", name);
					nextNode->append_attribute(nameAttribute);
					node->append_node(nextNode);
					if (strcmp(type, "list") == 0)
					{
						luaCF_rebuildList(L, nextNode);
					}
					else {
						luaCF_rebuildXML(L, nextNode);
					}
				}
			}
		}

		lua_pop(L, 1);
	}
}


void BuildLuaListNode(lua_State *L, const xml_node<WCHAR>* curNode)
{
	lua_checkstack(L, 2);
	lua_newtable(L);
	int i = 1;
	for (xml_node<WCHAR>* node = curNode->first_node(); node; node = node->next_sibling(), ++i)
	{
		char* nodeType = new char[node->name_size() + 5]; ZeroMemory(nodeType, sizeof(char)*(node->name_size() + 5));
		WideCharToMultiByte(CP_ACP, 0, node->name(), -1, nodeType, node->name_size(), NULL, NULL);

		char* nodeName = new char[node->first_attribute()->value_size() + 5]; ZeroMemory(nodeName, sizeof(char)*(node->first_attribute()->value_size() + 5));
		WideCharToMultiByte(CP_ACP, 0, node->first_attribute()->value(), -1, nodeName, node->first_attribute()->value_size(), NULL, NULL);

		BuildLuaXMLGroupNode(L, node, true);
		if (strcmp(nodeType, "group") != 0)
		{
			lua_pushstring(L, nodeType);
			lua_setfield(L, -2, "@type");
		}
		lua_rawseti(L, -2, i);

		delete[] nodeType;
		delete[] nodeName;
	}
}
void BuildLuaXMLGroupNode(lua_State *L, const xml_node<WCHAR>* curNode, const bool includeNameAttribute/* = false*/)
{
	lua_checkstack(L, 3);
	lua_newtable(L);
	lua_newtable(L);
	for (xml_attribute<WCHAR>* attribute = curNode->first_attribute(); attribute; attribute = attribute->next_attribute())
	{
		char* attributeName = new char[attribute->name_size() + 5]; ZeroMemory(attributeName, sizeof(char)*(attribute->name_size() + 5));
		WideCharToMultiByte(CP_ACP, 0, attribute->name(), -1, attributeName, attribute->name_size(), NULL, NULL);

		if (includeNameAttribute || strcmp(attributeName, "name") != 0)
		{
			char* attributeValue = new char[attribute->value_size() + 5]; ZeroMemory(attributeValue, sizeof(char)*(attribute->value_size() + 5));
			WideCharToMultiByte(CP_ACP, 0, attribute->value(), -1, attributeValue, attribute->value_size(), NULL, NULL);
			lua_pushstring(L, attributeValue);
			lua_setfield(L, -2, attributeName);
			delete[] attributeValue;
		}
		delete[] attributeName;
	}
	lua_checkstack(L, 2);
	lua_setfield(L, -2, "@attributes");
	for (xml_node<WCHAR>* node = curNode->first_node(); node; node = node->next_sibling())
	{
		char* nodeType = new char[node->name_size() + 5]; ZeroMemory(nodeType, sizeof(char)*(node->name_size() + 5));
		WideCharToMultiByte(CP_ACP, 0, node->name(), -1, nodeType, node->name_size(), NULL, NULL);

		char* nodeName = new char[node->first_attribute()->value_size() + 5]; ZeroMemory(nodeName, sizeof(char)*(node->first_attribute()->value_size() + 5));
		WideCharToMultiByte(CP_ACP, 0, node->first_attribute()->value(), -1, nodeName, node->first_attribute()->value_size(), NULL, NULL);

		/*if (strcmp(nodeType, "int") == 0)
		{
			lua_pushinteger(L, (int)wcstod(node->last_attribute()->value(), NULL));
		}
		else */if (strcmp(nodeType, "float") == 0)
		{
			lua_pushnumber(L, wcstod(node->last_attribute()->value(), NULL));
		}
		else if (strcmp(nodeType, "string") == 0)
		{
			char* nodeValue = new char[node->last_attribute()->value_size() + 5]; ZeroMemory(nodeValue, sizeof(char)*(node->last_attribute()->value_size() + 5));
			WideCharToMultiByte(CP_ACP, 0, node->last_attribute()->value(), -1, nodeValue, node->last_attribute()->value_size(), NULL, NULL);
			lua_pushstring(L, nodeValue);
			delete[] nodeValue;
		}
		else if (strcmp(nodeType, "bool") == 0)
		{
			char* nodeValue = new char[node->last_attribute()->value_size() + 5]; ZeroMemory(nodeValue, sizeof(char)*(node->last_attribute()->value_size() + 5));
			WideCharToMultiByte(CP_ACP, 0, node->last_attribute()->value(), -1, nodeValue, node->last_attribute()->value_size(), NULL, NULL);
			for (int i = 0; nodeValue[i]; i++)
				nodeValue[i] = tolower(nodeValue[i]);
			lua_pushboolean(L, strcmp(nodeValue, "true") == 0);
			delete[] nodeValue;
		}
		else
		{
			if (strcmp(nodeType, "list") == 0)
			{
				BuildLuaListNode(L, node);
				lua_pushstring(L, nodeType);
				lua_setfield(L, -2, "@type");
			}
			else
			{
				BuildLuaXMLGroupNode(L, node);
				if (strcmp(nodeType, "group") != 0)
				{
					lua_pushstring(L, nodeType);
					lua_setfield(L, -2, "@type");
				}
			}
		}
		lua_setfield(L, -2, nodeName);

		delete[] nodeType;
		delete[] nodeName;
	}
}
void BuildLuaXMLTable(lua_State *L, const xml_document<WCHAR>* doc)
{
	lua_checkstack(L, 1);
	lua_newtable(L);
	BuildLuaXMLGroupNode(L, doc->first_node());
	lua_setfield(L, -2, "instance");
}
void BuildLuaXMLAttribVariable(lua_State *L, const WCHAR* filePath, const WCHAR* filename, const xml_document<WCHAR>* doc)
{
	lua_checkstack(L, 2);
	lua_newtable(L); // the xml variable metatable
	lua_newtable(L); // proxy

	// index >
	BuildLuaXMLTable(L, doc); // pushes table onto stack

	size_t filePathLen = wcslen(filePath);
	char* cFilepath = new char[filePathLen + 5]; ZeroMemory(cFilepath, sizeof(char)*(filePathLen + 5));
	WideCharToMultiByte(CP_ACP, 0, filePath, -1, cFilepath, filePathLen, NULL, NULL);

	lua_pushstring(L, cFilepath);
	lua_setfield(L, -2, "path");
	currentlyParsedFile = cFilepath;

	size_t filenameLen = wcslen(filename);
	char* cFilename = new char[filePathLen + 5]; ZeroMemory(cFilename, sizeof(char)*(filenameLen + 5));
	WideCharToMultiByte(CP_ACP, 0, filename, -1, cFilename, filenameLen, NULL, NULL);
	lua_pushstring(L, cFilename);
	lua_setfield(L, -2, "name");

	lua_pushcfunction(L, luaCF_saveXML);
	lua_setfield(L, -2, "save");

	lua_setfield(L, -2, "__index");
	// index <

	lua_pushcfunction(L, luaCF_xmlNewIndex);
	lua_setfield(L, -2, "__newindex");

	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");

	lua_setmetatable(L, -2);

	delete[] cFilepath;
	delete[] cFilename;
}

// ------------------------------------------
// Window message interpreters

void MCreate(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (LoadLibrary(L"SciLexer.DLL") == NULL)
	{
		MessageBox(hWnd,
			L"The Scintilla DLL could not be loaded.",
			L"Error loading Scintilla",
			MB_OK | MB_ICONERROR);
	}

	hAttribXMLSelectBtnText = CreateWindow(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY, 10, 10, 500, 21, hWnd, NULL, hInst, NULL);
	hAttribXMLSelectBtn = CreateWindow(L"Button", L"Select Attrib XML File", WS_VISIBLE | WS_CHILD | WS_BORDER, 470, 10, 200, 25, hWnd, (HMENU)MM_SELECTATTRIBXML, hInst, NULL);
	hFolderSelectBtnText = CreateWindow(L"Edit", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY, 10, 35, 500, 21, hWnd, NULL, hInst, NULL);
	hFolderSelectBtn = CreateWindow(L"Button", L"Select XML Folder", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_DISABLED, 470, 35, 200, 25, hWnd, (HMENU)MM_SELECTFOLDER, hInst, NULL);
	hRunMacroBtn = CreateWindow(L"Button", L"Run Macro", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_DISABLED, 570, 400, 100, 25, hWnd, (HMENU)MM_RUNMACRO, hInst, NULL);
	hPrintOutput = CreateWindow(L"Edit", L"", WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_BORDER, 10, 70, 660, 370, hWnd, NULL, hInst, NULL);
	hScintilla = CreateWindow(L"Scintilla", L"", WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN | WS_BORDER, 10, 70, 660, 370, hWnd, NULL, hInst, NULL);
	hToggleViewBtn = CreateWindow(L"Button", L"Show Macro", WS_CHILD | WS_BORDER, 467, 400, 100, 25, hWnd, (HMENU)MM_TOGGLEVIEW, hInst, NULL);
	hWorkingText = CreateWindow(L"Static", L"Working...", WS_CHILD, 390, 415, 95, 16, hWnd, NULL, hInst, NULL);

	HFONT printFont = CreateFont(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, L"Courier New");
	SendMessage(hPrintOutput, WM_SETFONT, WPARAM(printFont), TRUE);

	SendMessage(hPrintOutput, EM_SETLIMITTEXT, 500000, 0);

	hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));

	editor = new Editor(hWnd, hScintilla);
	editor->SetWidth(660);

	//printOutput = new PrintOutput(hWnd, hPrintOutput);
	//printOutput->SetWidth(660);

	const char initText[] = "\n-- This function runs for every xml file in the selected directory\nfunction each_file(xml)\n\tprint(xml.name)\n\t--xml:save()\nend\n\n-- This function runs at the end of execution\nfunction at_end()\n\tprint('Done!')\nend\n";
	editor->SendEditor(SCI_ADDTEXT, sizeof(initText)-1, reinterpret_cast<LPARAM>(initText));
	ShowWindow(hScintilla, SW_SHOW);
	SetFocus(hScintilla);

	// Read config
	FILE* fpConfig = _wfopen(configFile.c_str(), L"r, ccs=UTF-8");
	if (fpConfig != NULL)
	{
		WCHAR sConfigAttribFilename[MAX_PATH] = { 0 };
		WCHAR sConfigFolderName[MAX_PATH] = { 0 };

		fwscanf(fpConfig, L"%[^\n]\n%[^\n]", sConfigAttribFilename, sConfigFolderName);
		fclose(fpConfig);

		if (wcslen(sConfigAttribFilename) > 2)
		{
			setCurrentAttribXMLFile(sConfigAttribFilename);
			if (wcslen(sConfigFolderName) > 2)
			{
				setCurrentAttribDir(sConfigFolderName);
			}
		}
	}
}
void MCSelectAttribXML(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WCHAR sNewAttribFilename[MAX_PATH];
	ZeroMemory(sNewAttribFilename, sizeof(WCHAR)*MAX_PATH);
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = sNewAttribFilename;
	ofn.nMaxFile = sizeof(sNewAttribFilename);
	ofn.lpstrFile[0] = L'\0';
	ofn.lpstrFilter = L"All\0*.*\0Attribute XML\0*.XML\0";
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn))
	{
		setCurrentAttribXMLFile(sNewAttribFilename);
	}
}
void MCSelectFolder(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WCHAR sNewFolderName[MAX_PATH];
	ZeroMemory(sNewFolderName, sizeof(WCHAR)*MAX_PATH);
	BROWSEINFO bi = { 0 };
	bi.hwndOwner = hWnd;
	bi.lpszTitle = L"Select instance XML directory";
	bi.ulFlags = BIF_RETURNONLYFSDIRS;

	LPITEMIDLIST rootPidl;
	SFGAOF out;
	HRESULT hr = ERROR_CANCELLED; 
	hr = SHParseDisplayName(sAttribFolder, NULL, &rootPidl, SFGAO_FILESYSTEM, &out);
	if (SUCCEEDED(hr))
	{
		bi.pidlRoot = rootPidl;
		LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
		if (pidl != 0)
		{
			if (SHGetPathFromIDList(pidl, sNewFolderName))
			{
				setCurrentAttribDir(sNewFolderName);
			}

			// free memory used
			IMalloc * imalloc = 0;
			if (SUCCEEDED(SHGetMalloc(&imalloc)))
			{
				imalloc->Free(pidl);
				imalloc->Release();
			}
		}
	}
	else {
		MessageBox(hWnd,
			L"An error has occurred while opening attribute XML directory",
			L"Error",
			MB_OK | MB_ICONERROR);

		printMessage(L"An error has occurred while opening attribute XML directory\r\n");
	}
}
void ShowPrint()
{
	SetWindowText(hToggleViewBtn, L"Show macro");
	ShowWindow(hScintilla, SW_HIDE);
	ShowWindow(hPrintOutput, SW_SHOW);
}
void HidePrint()
{
	SetWindowText(hToggleViewBtn, L"Show output");
	ShowWindow(hScintilla, SW_SHOW);
	ShowWindow(hPrintOutput, SW_HIDE);
}
void MCToggleView(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (IsWindowVisible(hScintilla))
		ShowPrint();
	else
		HidePrint();
}
void MWindSize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int w, h;
	w = LOWORD(lParam);
	h = HIWORD(lParam);
	SetWindowPos(hAttribXMLSelectBtnText, NULL, 10, 12, max(100, (int)(w - 230)), 21, NULL);
	SetWindowPos(hAttribXMLSelectBtn, NULL, (int)(w - 210), 10, 200, 25, NULL);
	SetWindowPos(hFolderSelectBtnText, NULL, 10, 40, max(100, (int)(w - 230)), 21, NULL);
	SetWindowPos(hFolderSelectBtn, NULL, (int)(w - 210), 38, 200, 25, NULL);
	SetWindowPos(hScintilla, NULL, 10, 70, (int)(w - 20), h - 110, NULL);
	SetWindowPos(hPrintOutput, NULL, 10, 70, (int)(w - 20), h - 110, NULL);
	SetWindowPos(hRunMacroBtn, NULL, (int)(w - 110), h - 33, 100, 25, NULL);
	SetWindowPos(hToggleViewBtn, NULL, (int)(w - 213), h - 33, 100, 25, NULL);
	SetWindowPos(hWorkingText, NULL, (int)(w - 290), h - 30, 67, 18, NULL);
	editor->SetWidth((int)(w - 20));
	//printOutput->SetWidth((int)(w - 20));
}
void MCRunMacro(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ShowWindow(hWorkingText, SW_SHOW);
	ShowWindow(hToggleViewBtn, SW_SHOW);
	EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVEOUTPUT, MF_ENABLED);
	//printOutput->SendEditor(SCI_CLEARALL);
	SetWindowText(hPrintOutput, L"");
	ShowPrint();

	RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE);
	RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW);

	currentlyParsedFile = NULL;

	//printMessage(L"Reading attribute XML file...\r\n\r\n");

	// Read attrib XML file
	FILE* attribXMLfp;
	if (attribXMLfp = _wfopen(sAttribFilename, L"r, ccs=UTF-8"))
	{
		wstring xmlStr;
		WCHAR sLine[3005] = { 0 };
		while (fread_s(sLine, sizeof(WCHAR)*3005, sizeof(WCHAR), 3000, attribXMLfp))
		{
			xmlStr.append(sLine);
		}

		int xmlStrLength = xmlStr.length();
		WCHAR* sFile = new WCHAR[xmlStrLength + 5]; ZeroMemory(sFile, sizeof(WCHAR)*(xmlStrLength + 5));
		wcsncpy_s(sFile, xmlStrLength + 5, xmlStr.c_str(), _TRUNCATE);

		xml_document<WCHAR> doc;
		try {
			doc.parse<0>(sFile);
		}
		catch (parse_error e)
		{
			printMessage(L"Attrib XML Parse error\r\n\r\n");

			int whatLen = strlen(e.what());
			int slen = strlen(e.what()) + wcslen(e.where<WCHAR>());
			LPWSTR errMessage = new WCHAR[slen + 1](); ZeroMemory(errMessage, sizeof(WCHAR)*(slen + 1));
			MultiByteToWideChar(CP_ACP, 0, e.what(), -1, errMessage, whatLen);

			StringCchCat(errMessage, slen, L" (");
			StringCchCat(errMessage, slen, e.where<WCHAR>());
			StringCchCat(errMessage, slen, L" )");

			printMessage(errMessage);
			printMessage(L"\r\n");
			delete[] errMessage;
			return;
		}
		if (!doc.first_node())
		{
			printMessage(L"Error: XML document is empty\r\n");
			return;
		}
		size_t guidLen = wcslen(doc.first_node()->first_attribute()->value());
		size_t overrideInstancesdLen = wcslen(doc.first_node()->last_attribute()->value());
		WCHAR* attribXML_guid = new WCHAR[guidLen + 1](); ZeroMemory(attribXML_guid, sizeof(WCHAR)*(guidLen + 1));
		WCHAR* attribXML_overrideInstances = new WCHAR[overrideInstancesdLen + 1](); ZeroMemory(attribXML_overrideInstances, sizeof(WCHAR)*(overrideInstancesdLen + 1));
		wcscpy(attribXML_guid, doc.first_node()->first_attribute()->value());
		wcscpy(attribXML_overrideInstances, doc.first_node()->last_attribute()->value());

		/*printMessage(L"  ");
		printMessage(doc.first_node()->first_attribute()->name());
		printMessage(L" = \"");
		printMessage(doc.first_node()->first_attribute()->value());
		printMessage(L"\"\r\n  ");
		printMessage(doc.first_node()->last_attribute()->name());
		printMessage(L" = \"");
		printMessage(doc.first_node()->last_attribute()->value());
		printMessage(L"\"\r\n\r\n");*/

		fclose(attribXMLfp);
		delete[] sFile;

		lua_State* L = luaL_newstate();

		luaL_openlibs(L);
		luaopen_io(L);
		luaopen_base(L);
		luaopen_table(L);
		luaopen_string(L);
		luaopen_math(L);

		//printMessage(L"Running LUA script...\r\n\r\n");

		lua_register(L, "loadXML", luaCF_loadXML);
		lua_register(L, "print", luaCF_print);

		char* a_attribXML_guid = new char[guidLen + 1](); ZeroMemory(a_attribXML_guid, sizeof(char)*(guidLen + 1));
		WideCharToMultiByte(CP_ACP, 0, attribXML_guid, -1, a_attribXML_guid, guidLen, NULL, NULL);
		lua_pushstring(L, a_attribXML_guid);
		lua_setglobal(L, "guid");

		char* a_attribXML_overrideInstances = new char[overrideInstancesdLen + 1](); ZeroMemory(a_attribXML_overrideInstances, sizeof(char)*(overrideInstancesdLen + 1));
		WideCharToMultiByte(CP_ACP, 0, attribXML_overrideInstances, -1, a_attribXML_overrideInstances, overrideInstancesdLen, NULL, NULL);
		lua_pushstring(L, a_attribXML_overrideInstances);
		lua_setglobal(L, "override_instances");

		int scriptLength = editor->SendEditor(SCI_GETLENGTH);
		char* scintillaText = new char[scriptLength + 3]; ZeroMemory(scintillaText, sizeof(char)*(scriptLength+3));
		editor->SendEditor(SCI_GETTEXT, scriptLength + 1, reinterpret_cast<LPARAM>(static_cast<char*>(scintillaText)));

		int status = luaL_dostring(L, scintillaText);
		report_lua_errors(hWnd, L, status);
		delete[] scintillaText;
		if (status == 0) {
			lua_getglobal(L, "each_file");
			if (lua_isfunction(L, -1))
			{
				//printMessage(L"Iterating XML files...\r\n\r\n");
				iterateDir(hWnd, L, sFolderName);

				lua_getglobal(L, "at_end");
				if (lua_isfunction(L, -1))
				{
					status = lua_pcall(L, 0, 0, 0);
					report_lua_errors(hWnd, L, status);
				}
				lua_pop(L, 1);
			}
			else {
				MessageBox(hWnd,
					L"Macro requires \"each_file\" function to iterate over attribute xml",
					L"Error",
					MB_OK | MB_ICONERROR);
			}
			lua_pop(L, 1);
		}

		lua_close(L);
	}
	else {
		printMessage(L"Error opening XML file\n");
		MessageBox(hWnd,
			L"Error opening XML file",
			L"Error",
			MB_OK | MB_ICONERROR);
	}
	ShowWindow(hWorkingText, SW_HIDE);
}
int iterateDir(HWND hWnd, lua_State *L, const WCHAR* dir)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	TCHAR szDir[MAX_PATH];

	size_t dirLength;
	StringCchLength(dir, MAX_PATH, &dirLength);
	if (dirLength > MAX_PATH - 3)
	{
		wstring errorMsg = L"Error: Directory name too long\r\n  ";
		errorMsg.append(dir);

		MessageBox(hWnd,
			errorMsg.c_str(),
			L"Error",
			MB_OK | MB_ICONERROR);

		printMessage(errorMsg.c_str());
		printMessage(L"\r\n");
		return 0;
	}

	StringCchCopy(szDir, MAX_PATH, dir);
	StringCchCat(szDir, MAX_PATH, L"\\*");

	hFind = FindFirstFile(szDir, &FindFileData);

	//printMessage(dir);
	//printMessage(L"\r\n");
	int ret = 1;
	if (hFind != INVALID_HANDLE_VALUE)
	{
		vector<wstring> directories;
		do
		{
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (wcscmp(FindFileData.cFileName, L".") != 0 && wcscmp(FindFileData.cFileName, L"..") != 0)
				{
					wstring dirName(dir);
					dirName.append(L"\\");
					dirName.append(FindFileData.cFileName);
					directories.push_back(dirName);
				}
			}
			else
			{
				const WCHAR* fileExt = GetFileExt(FindFileData.cFileName);
				if (wcscmp(fileExt, L"xml") == 0)
				{
					WCHAR filePath[MAX_PATH];
					StringCchCopy(filePath, MAX_PATH, dir);
					StringCchCat(filePath, MAX_PATH, L"\\");
					StringCchCat(filePath, MAX_PATH, FindFileData.cFileName);

					WCHAR* sFile = NULL;
					xml_document<WCHAR> doc;
					if (!openXML(&doc, &sFile, filePath))
					{
						if (sFile != NULL)
							delete[] sFile;
						return 0;
					}
					lua_getglobal(L, "each_file");
					if (lua_isfunction(L, -1))
					{
						BuildLuaXMLAttribVariable(L, filePath, FindFileData.cFileName, &doc);

						int status = lua_pcall(L, 1, 0, 0);
						report_lua_errors(hWnd, L, status);

						if (status != 0)
						{
							delete[] sFile;
							return 0;
						}
					}
					else {
						lua_pop(L, 1);

						MessageBox(hWnd,
							L"Macro requires \"each_file\" function to iterate over attribute xml",
							L"Error",
							MB_OK | MB_ICONERROR);
						
						delete[] sFile;
						return 0;
					}

					delete[] sFile;

					lua_gc(L, LUA_GCCOLLECT, 0);
				}
			}
		} while (FindNextFile(hFind, &FindFileData) != 0);
		currentlyParsedFile = NULL;

		for (std::vector<wstring>::iterator it = directories.begin(); it != directories.end(); ++it)
		{
			ret = iterateDir(hWnd, L, ((wstring)*it).c_str());
			if (ret == 0)
				break;
		}
	}
	else
	{
		printMessage(L"Error looking in XML directory\r\n  ");
		printMessage(dir);
		printMessage(L"\r\n");
		MessageBox(hWnd,
			L"Error looking in XML directory",
			L"Error",
			MB_OK | MB_ICONERROR);
		return 0;
	}

	FindClose(hFind);
	return ret;
}

void MCSave(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WCHAR wsFilename[MAX_PATH];
	ZeroMemory(wsFilename, sizeof(WCHAR)*MAX_PATH);
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = wsFilename;
	ofn.nMaxFile = sizeof(wsFilename);
	ofn.lpstrFile[0] = L'\0';
	ofn.lpstrFilter = L"All\0*.*\0Lua (*.lua)\0*.lua";
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn))
	{
		int i;
		int filenameLen = wcslen(wsFilename);
		bool extNotSet = true;
		for (i = filenameLen-1; i >= 0; --i)
		{
			if (wsFilename[i] == '.')
				extNotSet = false;
		}
		if (extNotSet && filenameLen < MAX_PATH-5)
		{
			wcsncpy(wsFilename + filenameLen, L".lua", 4);
		}

		char cFilename[MAX_PATH] = { 0 };
		WideCharToMultiByte(CP_ACP, 0, wsFilename, -1, cFilename, MAX_PATH, NULL, NULL);

		FILE* fp = fopen(cFilename, "w");
		if (fp)
		{
			char line[3000];
			int curLine = 0;
			int linesTotal = editor->SendEditor(SCI_GETLINECOUNT);
			while (curLine < linesTotal)
			{
				ZeroMemory(line, sizeof(char)* 3000);
				editor->SendEditor(SCI_GETLINE, curLine, reinterpret_cast<LPARAM>(static_cast<char*>(line)));

				fputs(line, fp);
				fflush(fp);

				++curLine;
			}
			fclose(fp);
		}
		else {
			MessageBox(hWnd,
				L"Error opening file",
				L"Error",
				MB_OK | MB_ICONERROR);
		}
	}
}
void MCOpen(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WCHAR wsFilename[MAX_PATH];
	ZeroMemory(wsFilename, sizeof(WCHAR)*MAX_PATH);
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = wsFilename;
	ofn.nMaxFile = sizeof(wsFilename);
	ofn.lpstrFile[0] = L'\0';
	ofn.lpstrFilter = L"All\0*.*\0Lua (*.lua)\0*.lua";
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn))
	{
		char cFilename[MAX_PATH] = { 0 };
		WideCharToMultiByte(CP_ACP, 0, wsFilename, -1, cFilename, MAX_PATH, NULL, NULL);

		FILE* fp = fopen(cFilename, "r");
		if (fp)
		{
			editor->SendEditor(SCI_CLEARALL);
			editor->SendEditor(SCI_SETCURRENTPOS, 0);

			int len;
			char buff[3000];
			while (len = fread(buff, sizeof(char), 10, fp))
			{
				editor->SendEditor(SCI_ADDTEXT, len, reinterpret_cast<LPARAM>(buff));
			}

			fclose(fp);
		}
		else {
			MessageBox(hWnd,
				L"Error opening file",
				L"Error",
				MB_OK | MB_ICONERROR);
		}
	}
}
void MCSaveOutput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WCHAR wsFilename[MAX_PATH];
	ZeroMemory(wsFilename, sizeof(WCHAR)*MAX_PATH);
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = wsFilename;
	ofn.nMaxFile = sizeof(wsFilename);
	ofn.lpstrFile[0] = L'\0';
	ofn.lpstrFilter = L"All\0*.*\0Text\0*.txt";
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofn))
	{
		int i;
		int filenameLen = wcslen(wsFilename);
		bool extNotSet = true;
		for (i = filenameLen - 1; i >= 0; --i)
		{
			if (wsFilename[i] == '.')
				extNotSet = false;
		}
		if (extNotSet && filenameLen < MAX_PATH - 5)
		{
			wcsncpy(wsFilename + filenameLen, L".txt", 4);
		}

		//char cFilename[MAX_PATH] = { 0 };
		//WideCharToMultiByte(CP_ACP, 0, wsFilename, -1, cFilename, MAX_PATH, NULL, NULL);

		FILE* fp = _wfopen(wsFilename, L"w, ccs=UTF-8");
		if (fp)
		{
			WCHAR line[3002];
			int curLine = 0;
			int linesTotal = SendMessage(hPrintOutput, EM_GETLINECOUNT, NULL, NULL);
			while (curLine < linesTotal)
			{
				ZeroMemory(line, sizeof(WCHAR) * 3002);
				((WORD*)line)[0] = 3000;
				int size = SendMessage(hPrintOutput, EM_GETLINE, curLine, (LPARAM)line);
				line[size] = L'\0';

				fputws(line, fp);

				++curLine;
			}
			fclose(fp);
		}
		else {
			MessageBox(hWnd,
				L"Error opening file",
				L"Error",
				MB_OK | MB_ICONERROR);
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		MCreate(hWnd, message, wParam, lParam);
		break;
	case WM_NOTIFY:
		LPNMHDR lpnmhdr;
		lpnmhdr = (LPNMHDR)lParam;
		if (lpnmhdr->hwndFrom == hScintilla)
		{
			editor->Notify(reinterpret_cast<SCNotification*>(lParam));
		}
		//else if (lpnmhdr->hwndFrom == hPrintOutput)
		//{
		//	printOutput->Notify(reinterpret_cast<SCNotification*>(lParam));
		//}
		break;
	case WM_COMMAND:
		int wmId;
		int wmEvent;
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case MM_RUNMACRO:
			MCRunMacro(hWnd, message, wParam, lParam);
			break;
		case MM_SELECTATTRIBXML:
			MCSelectAttribXML(hWnd, message, wParam, lParam);
			break;
		case MM_SELECTFOLDER:
			MCSelectFolder(hWnd, message, wParam, lParam);
			break;
		case MM_TOGGLEVIEW:
			MCToggleView(hWnd, message, wParam, lParam);
			break;

		case ID_FILE_SAVE:
			MCSave(hWnd, message, wParam, lParam);
			break;
		case ID_FILE_OPEN:
			MCOpen(hWnd, message, wParam, lParam);
			break;
		case ID_FILE_SAVEOUTPUT:
			MCSaveOutput(hWnd, message, wParam, lParam);
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_SIZE:
		MWindSize(hWnd, message, wParam, lParam);
		break;
	/*case WM_PAINT:
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);
		break;*/
	/*case WM_CTLCOLORSTATIC: // Set print output colors
		HDC hdcStatic;
		hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkColor(hdcStatic, RGB(255, 255, 255));
		if (hbrBkgnd == 0)
			hbrBkgnd = CreateSolidBrush(RGB(255, 255, 255));

		return (INT_PTR)hbrBkgnd;*/
	case WM_GETMINMAXINFO:
		MINMAXINFO* mmi;
		mmi = (MINMAXINFO*)lParam;
		mmi->ptMinTrackSize.x = 450;
		mmi->ptMinTrackSize.y = 250;
		return 0;
	case WM_DESTROY:
		DeleteObject(hbrBkgnd);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_NOTIFY:
		LPNMHDR lpnmhdr;
		lpnmhdr = (LPNMHDR)lParam;
		if (lpnmhdr->idFrom == IDC_SYSLINK_FORUMTHREAD)
		{
			if ((lpnmhdr->code == NM_CLICK) || (lpnmhdr->code == NM_RETURN)) {
				ShellExecute(NULL, L"open", L"http://community.companyofheroes.com/forum/company-of-heroes-2/company-of-heroes-2-general-discussion/modding/119838-tool-attribute-xml-macro-tool", NULL, NULL, SW_SHOWNORMAL);
			}
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
