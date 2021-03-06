//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "precompiled_headers.h"

#include "Notepad_plus_Window.h"
#include "MISC/Process/Process.h"

#include "MISC/Exception/Win32Exception.h"	//Win32 exception
#include "MISC/Exception/MiniDumper.h"			//Write dump files

#include "resource.h"
#include "ScintillaComponent/Buffer.h"

#include "Parameters.h"
#include "MISC/Common/npp_winver.h"

typedef std::vector<const TCHAR*> ParamVector;

bool checkSingleFile(const TCHAR * commandLine) {
	func_guard(guardWinMain);
	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(commandLine, MAX_PATH, fullpath, NULL);
	if (::PathFileExists(fullpath)) {
		return true;
	}

	return false;
}

//commandLine should contain path to n++ executable running
void parseCommandLine(TCHAR * commandLine, ParamVector & paramVector) {
	func_guard(guardWinMain);
	//params.erase(params.begin());
	//remove the first element, since thats the path the the executable (GetCommandLine does that)
	TCHAR stopChar = TEXT(' ');
	if (commandLine[0] == TEXT('\"')) {
		stopChar = TEXT('\"');
		commandLine++;
	}
	//while this is not really DBCS compliant, space and quote are in the lower 127 ASCII range
	while(commandLine[0] && commandLine[0] != stopChar)
    {
		commandLine++;
    }

    // For unknown reason, the following command :
    // c:\NppDir>notepad++
    // (without quote) will give string "notepad++\0notepad++\0"
    // To avoid the unexpected behaviour we check the end of string before increasing the pointer
    if (commandLine[0] != '\0')
	    commandLine++;	//advance past stopChar

	//kill remaining spaces
	while(commandLine[0] == TEXT(' '))
		commandLine++;

	bool isFile = checkSingleFile(commandLine);	//if the commandline specifies only a file, open it as such
	if (isFile) {
		paramVector.push_back(commandLine);
		return;
	}
	bool isInFile = false;
	bool isInWhiteSpace = true;
	paramVector.clear();
	size_t commandLength = lstrlen(commandLine);
	for(size_t i = 0; i < commandLength; i++) {
		switch(commandLine[i]) {
			case '\"': {										//quoted filename, ignore any following whitespace
				if (!isInFile) {	//" will always be treated as start or end of param, in case the user forgot to add an space
					paramVector.push_back(commandLine+i+1);	//add next param(since zero terminated generic_string original, no overflow of +1)
				}
				isInFile = !isInFile;
				isInWhiteSpace = false;
				//because we dont want to leave in any quotes in the filename, remove them now (with zero terminator)
				commandLine[i] = 0;
				break; }
			case '\t':	//also treat tab as whitespace
			case ' ': {
				isInWhiteSpace = true;
				if (!isInFile)
					commandLine[i] = 0;		//zap spaces into zero terminators, unless its part of a filename
				break; }
			default: {											//default TCHAR, if beginning of word, add it
				if (!isInFile && isInWhiteSpace) {
					paramVector.push_back(commandLine+i);	//add next param
					isInWhiteSpace = false;
				}
				break; }
		}
	}
	//the commandline generic_string is now a list of zero terminated strings concatenated, and the vector contains all the substrings
}

bool isInList(const TCHAR *token2Find, ParamVector & params) {
	int nrItems = params.size();
	func_guard(guardWinMain);

	for (int i = 0; i < nrItems; i++)
	{
		if (!lstrcmp(token2Find, params.at(i))) {
			params.erase(params.begin() + i);
			return true;
		}
	}
	return false;
};

bool getParamVal(TCHAR c, ParamVector & params, generic_string & value) {
	value = TEXT("");
	int nrItems = params.size();
	func_guard(guardWinMain);

	for (int i = 0; i < nrItems; i++)
	{
		const TCHAR * token = params.at(i);
		if (token[0] == '-' && lstrlen(token) >= 2 && token[1] == c) {	//dash, and enough chars
			value = (token+2);
			params.erase(params.begin() + i);
			return true;
		}
	}
	return false;
}

LangType getLangTypeFromParam(ParamVector & params) {
	func_guard(guardWinMain);
	generic_string langStr;
	if (!getParamVal('l', params, langStr))
		return L_EXTERNAL;
	return NppParameters::getLangIDFromStr(langStr.c_str());
};

int getNumberFromParam(char paramName, ParamVector & params, bool & isParamePresent) {
	func_guard(guardWinMain);
	generic_string numStr;
	if (!getParamVal(paramName, params, numStr))
	{
		isParamePresent = false;
		return -1;
	}
	isParamePresent = true;
	return generic_atoi(numStr.c_str());
};

#define FLAG_MULTI_INSTANCE TEXT("-multiInst")
#define FLAG_NO_PLUGIN TEXT("-noPlugin")
#define FLAG_READONLY TEXT("-ro")
#define FLAG_NOSESSION TEXT("-nosession")
#define FLAG_NOTABBAR TEXT("-notabbar")
#define FLAG_SYSTRAY TEXT("-systemtray")
#define FLAG_LOADINGTIME TEXT("-loadingtime")
#define FLAG_HELP TEXT("--help")

#ifndef SHIPPING
	#define FLAG_RUN_UNITTESTS TEXT("-unittests") // "Secret" option
#endif

#ifdef _DEBUG
	#define FLAG_LEAK_DETECT TEXT("-leakdetect") // "Secret" option
#endif

#define COMMAND_ARG_HELP TEXT("Usage :\n\
\n\
notepad++ [--help] [-multiInst] [-noPlugins] [-lLanguage] [-nLineNumber] [-cColumnNumber] [-xPos] [-yPos] [-nosession] [-notabbar] [-ro] [-systemtray] [fullFilePathName]\n\
\n\
    --help : This help message\n\
    -multiInst : Launch another Notepad++ instance\n\
    -noPlugins : Launch Notepad++ without loading any plugin\n\
    -l : Launch Notepad++ by applying indicated language to the file to open\n\
    -n : Launch Notepad++ by scrolling indicated line on the file to open\n\
    -c : Launch Notepad++ on scrolling indicated column on the file to open\n\
    -x : Launch Notepad++ by indicating its left side position on the screen\n\
    -y : Launch Notepad++ by indicating its top position on the screen\n\
    -nosession : Launch Notepad++ without any session\n\
    -notabbar : Launch Notepad++ without tabbar\n\
    -ro : Launch Notepad++ and make the file to open read only\n\
    -systemtray : Launch Notepad++ directly in system tray\n\
	-loadingTime : Display Notepad++ loading time\n\
    fullFilePathName : file name to open (absolute or relative path name)\n\
")
void doException(Notepad_plus_Window & notepad_plus_plus);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR /*cmdLineAnsi*/, int /*nCmdShow*/)
{
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	LPTSTR cmdLine = ::GetCommandLine();
	ParamVector params;
	parseCommandLine(cmdLine, params);

#ifndef SHIPPING
	if (isInList(FLAG_RUN_UNITTESTS, params))
	{
		int argc = params.size();

		TCHAR argv[4096];
		TCHAR* argvPtr = &argv[0];
		int spaceLeft = 4096;
		for (int i = 0; i< argc; i++)
		{
			_tcscpy_s(argvPtr, spaceLeft, params[i]);
			int len = _tcslen(params[i]) + 1;
			argvPtr += len;
			spaceLeft -= len;
		}
		*argvPtr = '\0';
		argvPtr = &argv[0];
		::testing::InitGoogleTest(&argc, &argvPtr);
		::testing::GTEST_FLAG(print_time) = true;
		return RUN_ALL_TESTS();
	}
#endif

	func_guard(guardWinMain); // added after GTest check to not interfere with tests.

	MiniDumper mdump;	//for debugging purposes.

	bool TheFirstOne = true;
	::SetLastError(NO_ERROR);
	::CreateMutex(NULL, false, TEXT("nppInstance"));
	if (::GetLastError() == ERROR_ALREADY_EXISTS)
		TheFirstOne = false;

	bool isParamePresent;
	bool showHelp = isInList(FLAG_HELP, params);
	bool isMultiInst = isInList(FLAG_MULTI_INSTANCE, params);

#ifdef _DEBUG
	bool isLeakDetect = isInList(FLAG_LEAK_DETECT, params);
#endif

	CmdLineParams cmdLineParams;
	cmdLineParams._isNoTab = isInList(FLAG_NOTABBAR, params);
	cmdLineParams._isNoPlugin = isInList(FLAG_NO_PLUGIN, params);
	cmdLineParams._isReadOnly = isInList(FLAG_READONLY, params);
	cmdLineParams._isNoSession = isInList(FLAG_NOSESSION, params);
	cmdLineParams._isPreLaunch = isInList(FLAG_SYSTRAY, params);
	cmdLineParams._showLoadingTime = isInList(FLAG_LOADINGTIME, params);
	cmdLineParams._langType = getLangTypeFromParam(params);
	cmdLineParams._line2go = getNumberFromParam('n', params, isParamePresent);
    cmdLineParams._column2go = getNumberFromParam('c', params, isParamePresent);
	cmdLineParams._point.x = getNumberFromParam('x', params, cmdLineParams._isPointXValid);
	cmdLineParams._point.y = getNumberFromParam('y', params, cmdLineParams._isPointYValid);

	if (showHelp)
	{
		::MessageBox(NULL, COMMAND_ARG_HELP, TEXT("Notepad++ Command Argument Help"), MB_OK);
	}

	NppParameters *pNppParameters = NppParameters::getInstance();
	// override the settings if notepad style is present
	if (pNppParameters->asNotepadStyle())
	{
		isMultiInst = true;
		cmdLineParams._isNoTab = true;
		cmdLineParams._isNoSession = true;
	}

	generic_string quotFileName = TEXT("");
	// tell the running instance the FULL path to the new files to load
	size_t nrFilesToOpen = params.size();
	const TCHAR * currentFile;
	TCHAR fullFileName[MAX_PATH];

	for(size_t i = 0; i < nrFilesToOpen; i++)
	{
		currentFile = params.at(i);
		//check if relative or full path. Relative paths dont have a colon for driveletter
		BOOL isRelative = ::PathIsRelative(currentFile);
		quotFileName += TEXT("\"");
		if (isRelative)
		{
			::GetFullPathName(currentFile, MAX_PATH, fullFileName, NULL);
			quotFileName += fullFileName;
		}
		else
		{
			quotFileName += currentFile;
		}
		quotFileName += TEXT("\" ");
	}

	//Only after loading all the file paths set the working directory
	::SetCurrentDirectory(NppParameters::getInstance()->getNppPath().c_str());	//force working directory to path of module, preventing lock

	if ((!isMultiInst) && (!TheFirstOne))
	{
		HWND hNotepad_plus = ::FindWindow(Notepad_plus_Window::getClassName(), NULL);
		for (int i = 0 ;!hNotepad_plus && i < 5 ; i++)
		{
			Sleep(100);
			hNotepad_plus = ::FindWindow(Notepad_plus_Window::getClassName(), NULL);
		}

		if (hNotepad_plus)
		{
			// First of all, destroy static object NppParameters
			NppParameters::destroyInstance();
			FileManager::destroyInstance();

			int sw;

			if (::IsZoomed(hNotepad_plus))
				sw = SW_MAXIMIZE;
			else if (::IsIconic(hNotepad_plus))
				sw = SW_RESTORE;
			else
				sw = SW_SHOW;

			// IMPORTANT !!!
			::ShowWindow(hNotepad_plus, sw);

			::SetForegroundWindow(hNotepad_plus);

			if (params.size() > 0)	//if there are files to open, use the WM_COPYDATA system
			{
				COPYDATASTRUCT paramData;
				paramData.dwData = COPYDATA_PARAMS;
				paramData.lpData = &cmdLineParams;
				paramData.cbData = sizeof(cmdLineParams);

				COPYDATASTRUCT fileNamesData;
				fileNamesData.dwData = COPYDATA_FILENAMES;
				fileNamesData.lpData = (void *)quotFileName.c_str();
				fileNamesData.cbData = long(quotFileName.length() + 1)*(sizeof(TCHAR));

				::SendMessage(hNotepad_plus, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&paramData);
				::SendMessage(hNotepad_plus, WM_COPYDATA, (WPARAM)hInstance, (LPARAM)&fileNamesData);
			}
			return 0;
		}
	}

	pNppParameters->load();
	Notepad_plus_Window notepad_plus_plus;

	NppGUI & nppGui = pNppParameters->getNppGUI();

	generic_string updaterDir = pNppParameters->getNppPath();
	updaterDir += TEXT("\\updater\\");

	generic_string updaterFullPath = updaterDir + TEXT("gup.exe");

	generic_string version = TEXT("-v");
	// nul char is expected in version strings.
	//lint -e840 (Info -- Use of nul character in a string literal)
	version += VERSION_VALUE;
	//lint +e840

	winVer curWinVer = getWinVersion();

	bool isUpExist = nppGui._doesExistUpdater = (::PathFileExists(updaterFullPath.c_str()) == TRUE);
	bool winSupported = (curWinVer >= WV_W2K);
	bool doUpdate = nppGui._autoUpdateOpt._doAutoUpdate;

	if (doUpdate) // check more detail
	{
		Date today(0);

		if (today < nppGui._autoUpdateOpt._nextUpdateDate)
			doUpdate = false;
	}

	// Vista/Win7 UAC issue
	bool isVista = (curWinVer >= WV_VISTA);

	if (!winSupported)
		nppGui._doesExistUpdater = false;

	if (TheFirstOne && isUpExist && doUpdate && winSupported && !isVista)
	{
		Process updater(updaterFullPath.c_str(), version.c_str(), updaterDir.c_str());
		updater.run();

		// Update next update date
		if (nppGui._autoUpdateOpt._intervalDays < 0) // Make sure interval days value is positive
		{
			nppGui._autoUpdateOpt._intervalDays = 0 - nppGui._autoUpdateOpt._intervalDays;
		}
		nppGui._autoUpdateOpt._nextUpdateDate = Date(nppGui._autoUpdateOpt._intervalDays);
	}

	MSG msg;
	msg.wParam = 0;
	Win32Exception::installHandler();
	try {
		notepad_plus_plus.init(hInstance, NULL, quotFileName.c_str(), &cmdLineParams);
		bool unicodeSupported = getWinVersion() >= WV_NT;
#ifdef _DEBUG
		// mem leak check output for build-testing.
		if (isLeakDetect)
		{
			::SendMessage(Notepad_plus_Window::gNppHWND, WM_CLOSE, 0, 0);
			return 0;
		};
#endif
		bool going = true;
		while (going)
		{
			going = (unicodeSupported?(::GetMessageW(&msg, NULL, 0, 0)):(::GetMessageA(&msg, NULL, 0, 0))) != 0;
			if (going)
			{
				// if the message doesn't belong to the notepad_plus_plus's dialog
				if (!notepad_plus_plus.isDlgsMsg(&msg, unicodeSupported))
				{
					if (::TranslateAccelerator(notepad_plus_plus.getHSelf(), notepad_plus_plus.getAccTable(), &msg) == 0)
					{
						::TranslateMessage(&msg);
						if (unicodeSupported)
							::DispatchMessageW(&msg);
						else
							::DispatchMessage(&msg);
					}
				}
			}
		}
	} catch(int i) {
		TCHAR str[50] = TEXT("God Damned Exception : ");
		TCHAR code[10];
		wsprintf(code, TEXT("%d"), i);
		::MessageBox(Notepad_plus_Window::gNppHWND, lstrcat(str, code), TEXT("Int Exception"), MB_OK);
		doException(notepad_plus_plus);
	} catch(std::runtime_error & ex) {
		::MessageBoxA(Notepad_plus_Window::gNppHWND, ex.what(), "Runtime Exception", MB_OK);
		doException(notepad_plus_plus);
	} catch (const Win32Exception & ex) {
		TCHAR message[1024];	//TODO: sane number
		wsprintf(message, TEXT("An exception occured. Notepad++ cannot recover and must be shut down.\r\nThe exception details are as follows:\r\n")
		TEXT("Code:\t0x%08X\r\nType:\t%S\r\nException address: 0x%08X"), ex.code(), ex.what(), ex.where());
		::MessageBox(Notepad_plus_Window::gNppHWND, message, TEXT("Win32Exception"), MB_OK | MB_ICONERROR);
		mdump.writeDump(ex.info());
		doException(notepad_plus_plus);
	} catch(std::exception & ex) {
		::MessageBoxA(Notepad_plus_Window::gNppHWND, ex.what(), "General Exception", MB_OK);
		doException(notepad_plus_plus);
	} catch(...) {	//this shouldnt ever have to happen
		::MessageBoxA(Notepad_plus_Window::gNppHWND, "An exception that we did not yet found its name is just caught", "Unknown Exception", MB_OK);
		doException(notepad_plus_plus);
	}

	return (UINT)msg.wParam;
}

void doException(Notepad_plus_Window & notepad_plus_plus) {
	func_guard(guardWinMain);
	Win32Exception::removeHandler();	//disable exception handler after excpetion, we dont want corrupt data structurs to crash the exception handler
	::MessageBox(Notepad_plus_Window::gNppHWND, TEXT("Notepad++ will attempt to save any unsaved data. However, dataloss is very likely."), TEXT("Recovery initiating"), MB_OK | MB_ICONINFORMATION);

	TCHAR tmpDir[1024];
	GetTempPath(1024, tmpDir);
	generic_string emergencySavedDir = tmpDir;
	emergencySavedDir += TEXT("\\N++RECOV");

	bool res = notepad_plus_plus.emergency(emergencySavedDir);
	if (res) {
		generic_string displayText = TEXT("Notepad++ was able to successfully recover some unsaved documents, or nothing to be saved could be found.\r\nYou can find the results at :\r\n");
		displayText += emergencySavedDir;
		::MessageBox(Notepad_plus_Window::gNppHWND, displayText.c_str(), TEXT("Recovery success"), MB_OK | MB_ICONINFORMATION);
	} else {
		::MessageBox(Notepad_plus_Window::gNppHWND, TEXT("Unfortunatly, Notepad++ was not able to save your work. We are sorry for any lost data."), TEXT("Recovery failure"), MB_OK | MB_ICONERROR);
	}
}
