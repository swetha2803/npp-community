// This file is part of notepad++
// Copyright (C)2009-2010 The Notepad++ Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "precompiled_headers.h"

#include "MISC/PluginsManager/PluginsManager.h"

#include "WinControls/ContextMenu/ContextMenu.h"
#include "WinControls/ColourPicker/WordStyleDlg.h"
#include "WinControls/Grid/ShortcutMapper.h"
#include "WinControls/Preference/preferenceDlg.h"
#include "WinControls/shortcut/RunMacroDlg.h"
#include "WinControls/StaticDialog/RunDlg/RunDlg.h"
#include "WinControls/WindowsDlg/WindowsDlgRc.h"

#include "ScintillaComponent/columnEditor.h"
#include "ScintillaComponent/FindReplaceDlg.h"
#include "ScintillaComponent/GoToLineDlg.h"
#include "ScintillaComponent/ScintillaEditView.h"
#include "ScintillaComponent/UserDefineDialog.h"
#include "ScintillaComponent/UserDefineResource.h"

#include "EncodingMapper.h"
#include "lastRecentFileList.h"
#include "Parameters.h"

//#include "Notepad_plus.h"

#include "localization.h"


NativeLangSpeaker::NativeLangSpeaker():
	_nativeLangA(NULL),
	_nativeLangEncoding(CP_ACP),
	_isRTL(false)
{}

void NativeLangSpeaker::init(TiXmlDocumentA *nativeLangDocRootA, bool loadIfEnglish)
{
	if (nativeLangDocRootA)
	{
		// JOCE: This is terribly dangerous. We have no idea of the lifespan of nativeLangDocRootA
		// on which _nativeLangA depends!!! Needs to find a way to fix this.
		_nativeLangA =  nativeLangDocRootA->FirstChild("NotepadPlus");
		if (_nativeLangA)
		{
			_nativeLangA = _nativeLangA->FirstChild("Native-Langue");
			if (_nativeLangA)
			{
				TiXmlElementA *element = _nativeLangA->ToElement();
				if (const char* rtlAttrib = element->Attribute("RTL"))
				{
					std::basic_string<char> rtl( rtlAttrib );
					// It's all right here. It's used as intended.
					//lint -e864 (Info -- Expression involving variable 'rtl' possibly depends on order of evaluation)
					std::transform(rtl.begin(), rtl.end(), rtl.begin(), ::tolower);
					//lint +e864
					_isRTL = (rtl == "yes");
				}

                // get original file name (defined by Notpad++) from the attribute
                if (const char* filenameAttrib = element->Attribute("filename"))
                {
					_fileName = filenameAttrib;
					std::transform(_fileName.begin(), _fileName.end(), _fileName.begin(), ::tolower);
                }

				if (!loadIfEnglish &&_fileName == "english.xml")
                {
					_nativeLangA = NULL;
					return;
				}

				// get encoding
				if (TiXmlDeclarationA *declaration =  _nativeLangA->GetDocument()->FirstChild()->ToDeclaration())
				{
					const char * encodingStr = declaration->Encoding();
					EncodingMapper *em = EncodingMapper::getInstance();
                    int enc = em->getEncodingFromString(encodingStr);
                    _nativeLangEncoding = (enc != -1)?enc:CP_ACP;
				}
			}
		}
    }
}

void NativeLangSpeaker::changeMenuLang(HMENU menuHandle, generic_string & pluginsTrans, generic_string & windowTrans)
{
	if (!_nativeLangA) return;
	TiXmlNodeA *mainMenu = _nativeLangA->FirstChild("Menu");
	if (!mainMenu) return;
	mainMenu = mainMenu->FirstChild("Main");
	if (!mainMenu) return;
	TiXmlNodeA *entriesRoot = mainMenu->FirstChild("Entries");
	if (!entriesRoot) return;
	const char *idName = NULL;

#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		if (element->Attribute("id", &id))
		{
			const char *name = element->Attribute("name");

#ifdef UNICODE
			const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
			::ModifyMenu(menuHandle, id, MF_BYPOSITION, 0, nameW);
#else
			::ModifyMenu(menuHandle, id, MF_BYPOSITION, 0, name);
#endif
		}
		else
		{
			idName = element->Attribute("idName");
			if (idName)
			{
				const char *name = element->Attribute("name");
				if (!strcmp(idName, "Plugins"))
				{
#ifdef UNICODE
					const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
					pluginsTrans = nameW;
#else
					pluginsTrans = name;
#endif
				}
				else if (!strcmp(idName, "Window"))
				{
#ifdef UNICODE
					const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
					windowTrans = nameW;
#else
					windowTrans = name;
#endif
				}
			}
		}
	}

	TiXmlNodeA *menuCommandsRoot = mainMenu->FirstChild("Commands");
	for (TiXmlNodeA *childNode = menuCommandsRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		element->Attribute("id", &id);
		const char *name = element->Attribute("name");

#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
		::ModifyMenu(menuHandle, id, MF_BYCOMMAND, id, nameW);
#else
		::ModifyMenu(menuHandle, id, MF_BYCOMMAND, id, name);
#endif
	}

	TiXmlNodeA *subEntriesRoot = mainMenu->FirstChild("SubEntries");

	for (TiXmlNodeA *childNode = subEntriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int x, y, z;
		const char *xStr = element->Attribute("posX", &x);
		const char *yStr = element->Attribute("posY", &y);
		const char *name = element->Attribute("name");
		if (!xStr || !yStr || !name)
			continue;

		HMENU hSubMenu = ::GetSubMenu(menuHandle, x);
		if (!hSubMenu)
			continue;
		HMENU hSubMenu2 = ::GetSubMenu(hSubMenu, y);
		if (!hSubMenu2)
			continue;

		HMENU hMenu = hSubMenu;
		int pos = y;

		const char *zStr = element->Attribute("posZ", &z);
		if (zStr)
		{
			HMENU hSubMenu3 = ::GetSubMenu(hSubMenu2, z);
			if (!hSubMenu3)
				continue;
			hMenu = hSubMenu2;
			pos = z;
		}
#ifdef UNICODE

		const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
		::ModifyMenu(hMenu, pos, MF_BYPOSITION, 0, nameW);
#else
		::ModifyMenu(hMenu, pos, MF_BYPOSITION, 0, name);
#endif
	}
}

void NativeLangSpeaker::changeLangTabContextMenu(HMENU hCM)
{
	const int POS_CLOSE = 0;
	const int POS_CLOSEBUT = 1;
	const int POS_SAVE = 2;
	const int POS_SAVEAS = 3;
	const int POS_RENAME = 4;
	const int POS_REMOVE = 5;
	const int POS_PRINT = 6;
	//------7
	const int POS_READONLY = 8;
	const int POS_CLEARREADONLY = 9;
	//------10
	const int POS_CLIPFULLPATH = 11;
	const int POS_CLIPFILENAME = 12;
	const int POS_CLIPCURRENTDIR = 13;
	//------14
	const int POS_GO2VIEW = 15;
	const int POS_CLONE2VIEW = 16;
	const int POS_GO2NEWINST = 17;
	const int POS_OPENINNEWINST = 18;

	const char *pClose = NULL;
	const char *pCloseBut = NULL;
	const char *pSave = NULL;
	const char *pSaveAs = NULL;
	const char *pPrint = NULL;
	const char *pReadOnly = NULL;
	const char *pClearReadOnly = NULL;
	const char *pGoToView = NULL;
	const char *pCloneToView = NULL;
	const char *pGoToNewInst = NULL;
	const char *pOpenInNewInst = NULL;
	const char *pCilpFullPath = NULL;
	const char *pCilpFileName = NULL;
	const char *pCilpCurrentDir = NULL;
	const char *pRename = NULL;
	const char *pRemove = NULL;
	if (_nativeLangA)
	{
		TiXmlNodeA *tabBarMenu = _nativeLangA->FirstChild("Menu");
		if (tabBarMenu)
		{
			tabBarMenu = tabBarMenu->FirstChild("TabBar");
			if (tabBarMenu)
			{
				for (TiXmlNodeA *childNode = tabBarMenu->FirstChildElement("Item");
					childNode ;
					childNode = childNode->NextSibling("Item") )
				{
					TiXmlElementA *element = childNode->ToElement();
					int ordre;
					element->Attribute("order", &ordre);
					switch (ordre)
					{
						// JOCE Plain numerical values?? makes sense?
					case 0 :
						pClose = element->Attribute("name"); break;
					case 1 :
						pCloseBut = element->Attribute("name"); break;
					case 2 :
						pSave = element->Attribute("name"); break;
					case 3 :
						pSaveAs = element->Attribute("name"); break;
					case 4 :
						pPrint = element->Attribute("name"); break;
					case 5 :
						pGoToView = element->Attribute("name"); break;
					case 6 :
						pCloneToView = element->Attribute("name"); break;
					case 7 :
						pCilpFullPath = element->Attribute("name"); break;
					case 8 :
						pCilpFileName = element->Attribute("name"); break;
					case 9 :
						pCilpCurrentDir = element->Attribute("name"); break;
					case 10 :
						pRename = element->Attribute("name"); break;
					case 11 :
						pRemove = element->Attribute("name"); break;
					case 12 :
						pReadOnly = element->Attribute("name"); break;
					case 13 :
						pClearReadOnly = element->Attribute("name"); break;
					case 14 :
						pGoToNewInst = element->Attribute("name"); break;
					case 15 :
						pOpenInNewInst = element->Attribute("name"); break;

						NO_DEFAULT_CASE;
					}
				}
			}
		}
	}

#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
	if (pGoToView && pGoToView[0])
	{
		const wchar_t *goToViewG = wmc->char2wchar(pGoToView, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
		::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION, cmdID, goToViewG);
	}
	if (pCloneToView && pCloneToView[0])
	{
		const wchar_t *cloneToViewG = wmc->char2wchar(pCloneToView, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
		::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION, cmdID, cloneToViewG);
	}
	if (pGoToNewInst && pGoToNewInst[0])
	{
		const wchar_t *goToNewInstG = wmc->char2wchar(pGoToNewInst, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_GO2NEWINST);
		::ModifyMenu(hCM, POS_GO2NEWINST, MF_BYPOSITION, cmdID, goToNewInstG);
	}
	if (pOpenInNewInst && pOpenInNewInst[0])
	{
		const wchar_t *openInNewInstG = wmc->char2wchar(pOpenInNewInst, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_OPENINNEWINST);
		::ModifyMenu(hCM, POS_OPENINNEWINST, MF_BYPOSITION, cmdID, openInNewInstG);
	}
	if (pClose && pClose[0])
	{
		const wchar_t *closeG = wmc->char2wchar(pClose, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLOSE);
		::ModifyMenu(hCM, POS_CLOSE, MF_BYPOSITION, cmdID, closeG);
	}
	if (pCloseBut && pCloseBut[0])
	{
		const wchar_t *closeButG = wmc->char2wchar(pCloseBut, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLOSEBUT);
		::ModifyMenu(hCM, POS_CLOSEBUT, MF_BYPOSITION, cmdID, closeButG);
	}
	if (pSave && pSave[0])
	{
		const wchar_t *saveG = wmc->char2wchar(pSave, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_SAVE);
		::ModifyMenu(hCM, POS_SAVE, MF_BYPOSITION, cmdID, saveG);
	}
	if (pSaveAs && pSaveAs[0])
	{
		const wchar_t *saveAsG = wmc->char2wchar(pSaveAs, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_SAVEAS);
		::ModifyMenu(hCM, POS_SAVEAS, MF_BYPOSITION, cmdID, saveAsG);
	}
	if (pPrint && pPrint[0])
	{
		const wchar_t *printG = wmc->char2wchar(pPrint, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_PRINT);
		::ModifyMenu(hCM, POS_PRINT, MF_BYPOSITION, cmdID, printG);
	}
	if (pReadOnly && pReadOnly[0])
	{
		const wchar_t *readOnlyG = wmc->char2wchar(pReadOnly, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_READONLY);
		::ModifyMenu(hCM, POS_READONLY, MF_BYPOSITION, cmdID, readOnlyG);
	}
	if (pClearReadOnly && pClearReadOnly[0])
	{
		const wchar_t *clearReadOnlyG = wmc->char2wchar(pClearReadOnly, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLEARREADONLY);
		::ModifyMenu(hCM, POS_CLEARREADONLY, MF_BYPOSITION, cmdID, clearReadOnlyG);
	}
	if (pCilpFullPath && pCilpFullPath[0])
	{
		const wchar_t *cilpFullPathG = wmc->char2wchar(pCilpFullPath, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPFULLPATH);
		::ModifyMenu(hCM, POS_CLIPFULLPATH, MF_BYPOSITION, cmdID, cilpFullPathG);
	}
	if (pCilpFileName && pCilpFileName[0])
	{
		const wchar_t *cilpFileNameG = wmc->char2wchar(pCilpFileName, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPFILENAME);
		::ModifyMenu(hCM, POS_CLIPFILENAME, MF_BYPOSITION, cmdID, cilpFileNameG);
	}
	if (pCilpCurrentDir && pCilpCurrentDir[0])
	{
		const wchar_t * cilpCurrentDirG= wmc->char2wchar(pCilpCurrentDir, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPCURRENTDIR);
		::ModifyMenu(hCM, POS_CLIPCURRENTDIR, MF_BYPOSITION, cmdID, cilpCurrentDirG);
	}
	if (pRename && pRename[0])
	{
		const wchar_t *renameG = wmc->char2wchar(pRename, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_RENAME);
		::ModifyMenu(hCM, POS_RENAME, MF_BYPOSITION, cmdID, renameG);
	}
	if (pRemove && pRemove[0])
	{
		const wchar_t *removeG = wmc->char2wchar(pRemove, _nativeLangEncoding);
		int cmdID = ::GetMenuItemID(hCM, POS_REMOVE);
		::ModifyMenu(hCM, POS_REMOVE, MF_BYPOSITION, cmdID, removeG);
	}
#else
	if (pGoToView && pGoToView[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
		::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION, cmdID, pGoToView);
	}
	if (pCloneToView && pCloneToView[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
		::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION, cmdID, pCloneToView);
	}
	if (pGoToNewInst && pGoToNewInst[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_GO2NEWINST);
		::ModifyMenu(hCM, POS_GO2NEWINST, MF_BYPOSITION, cmdID, pGoToNewInst);
	}
	if (pOpenInNewInst && pOpenInNewInst[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_OPENINNEWINST);
		::ModifyMenu(hCM, POS_OPENINNEWINST, MF_BYPOSITION, cmdID, pOpenInNewInst);
	}
	if (pClose && pClose[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLOSE);
		::ModifyMenu(hCM, POS_CLOSE, MF_BYPOSITION, cmdID, pClose);
	}
	if (pCloseBut && pCloseBut[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLOSEBUT);
		::ModifyMenu(hCM, POS_CLOSEBUT, MF_BYPOSITION, cmdID, pCloseBut);
	}
	if (pSave && pSave[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_SAVE);
		::ModifyMenu(hCM, POS_SAVE, MF_BYPOSITION, cmdID, pSave);
	}
	if (pSaveAs && pSaveAs[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_SAVEAS);
		::ModifyMenu(hCM, POS_SAVEAS, MF_BYPOSITION, cmdID, pSaveAs);
	}
	if (pPrint && pPrint[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_PRINT);
		::ModifyMenu(hCM, POS_PRINT, MF_BYPOSITION, cmdID, pPrint);
	}
	if (pClearReadOnly && pClearReadOnly[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLEARREADONLY);
		::ModifyMenu(hCM, POS_CLEARREADONLY, MF_BYPOSITION, cmdID, pClearReadOnly);
	}
	if (pReadOnly && pReadOnly[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_READONLY);
		::ModifyMenu(hCM, POS_READONLY, MF_BYPOSITION, cmdID, pReadOnly);
	}
	if (pCilpFullPath && pCilpFullPath[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPFULLPATH);
		::ModifyMenu(hCM, POS_CLIPFULLPATH, MF_BYPOSITION, cmdID, pCilpFullPath);
	}
	if (pCilpFileName && pCilpFileName[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPFILENAME);
		::ModifyMenu(hCM, POS_CLIPFILENAME, MF_BYPOSITION, cmdID, pCilpFileName);
	}
	if (pCilpCurrentDir && pCilpCurrentDir[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_CLIPCURRENTDIR);
		::ModifyMenu(hCM, POS_CLIPCURRENTDIR, MF_BYPOSITION, cmdID, pCilpCurrentDir);
	}
	if (pRename && pRename[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_RENAME);
		::ModifyMenu(hCM, POS_RENAME, MF_BYPOSITION, cmdID, pRename);
	}
	if (pRemove && pRemove[0])
	{
		int cmdID = ::GetMenuItemID(hCM, POS_REMOVE);
		::ModifyMenu(hCM, POS_REMOVE, MF_BYPOSITION, cmdID, pRemove);
	}
#endif
}

void NativeLangSpeaker::changeLangTabDrapContextMenu(HMENU hCM)
{
	const int POS_GO2VIEW = 0;
	const int POS_CLONE2VIEW = 1;
	const char *goToViewA = NULL;
	const char *cloneToViewA = NULL;

	if (_nativeLangA)
	{
		TiXmlNodeA *tabBarMenu = _nativeLangA->FirstChild("Menu");
		if (tabBarMenu)
			tabBarMenu = tabBarMenu->FirstChild("TabBar");
		if (tabBarMenu)
		{
			for (TiXmlNodeA *childNode = tabBarMenu->FirstChildElement("Item");
				childNode ;
				childNode = childNode->NextSibling("Item") )
			{
				TiXmlElementA *element = childNode->ToElement();
				int ordre;
				element->Attribute("order", &ordre);
				if (ordre == 5)
					goToViewA = element->Attribute("name");
				else if (ordre == 6)
					cloneToViewA = element->Attribute("name");
			}
		}
#ifdef UNICODE
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		if (goToViewA && goToViewA[0])
		{
			const wchar_t *goToViewG = wmc->char2wchar(goToViewA, _nativeLangEncoding);
			int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
			::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION|MF_STRING, cmdID, goToViewG);
		}
		if (cloneToViewA && cloneToViewA[0])
		{
			const wchar_t *cloneToViewG = wmc->char2wchar(cloneToViewA, _nativeLangEncoding);
			int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
			::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION|MF_STRING, cmdID, cloneToViewG);
		}
#else
		if (goToViewA && goToViewA[0])
		{
			int cmdID = ::GetMenuItemID(hCM, POS_GO2VIEW);
			::ModifyMenu(hCM, POS_GO2VIEW, MF_BYPOSITION, cmdID, goToViewA);
		}
		if (cloneToViewA && cloneToViewA[0])
		{
			int cmdID = ::GetMenuItemID(hCM, POS_CLONE2VIEW);
			::ModifyMenu(hCM, POS_CLONE2VIEW, MF_BYPOSITION, cmdID, cloneToViewA);
		}
#endif
	}
}

void NativeLangSpeaker::changeConfigLang(HWND hDlg)
{
	if (!_nativeLangA) return;

	TiXmlNodeA *styleConfDlgNode = _nativeLangA->FirstChild("Dialog");
	if (!styleConfDlgNode) return;

	styleConfDlgNode = styleConfDlgNode->FirstChild("StyleConfig");
	if (!styleConfDlgNode) return;


#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	// Set Title
	const char *titre = (styleConfDlgNode->ToElement())->Attribute("title");

	if ((titre && titre[0]) && hDlg)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
#else
		::SetWindowText(hDlg, titre);
#endif
	}
	for (TiXmlNodeA *childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
#else
				::SetWindowText(hItem, name);
#endif
			}
		}
	}
	styleConfDlgNode = styleConfDlgNode->FirstChild("SubDialog");

	for (TiXmlNodeA *childNode = styleConfDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
#else
				::SetWindowText(hItem, name);
#endif
			}
		}
	}
}


void NativeLangSpeaker::changeStyleCtrlsLang(HWND hDlg, int *idArray, const char **translatedText)
{
	const int iColorStyle = 0;
	const int iUnderline = 8;

	HWND hItem;
	for (int i = iColorStyle ; i < (iUnderline + 1) ; i++)
	{
		if (translatedText[i] && translatedText[i][0])
		{
			hItem = ::GetDlgItem(hDlg, idArray[i]);
			if (hItem)
			{
#ifdef UNICODE
				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t *nameW = wmc->char2wchar(translatedText[i], _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
#else
				::SetWindowText(hItem, translatedText[i]);
#endif

			}
		}
	}
}

void NativeLangSpeaker::changeUserDefineLang(UserDefineDialog *userDefineDlg)
{
	if (!_nativeLangA) return;

	TiXmlNodeA *userDefineDlgNode = _nativeLangA->FirstChild("Dialog");
	if (!userDefineDlgNode) return;

	userDefineDlgNode = userDefineDlgNode->FirstChild("UserDefine");
	if (!userDefineDlgNode) return;

	HWND hDlg = userDefineDlg->getHSelf();
#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	// Set Title
	const char *titre = (userDefineDlgNode->ToElement())->Attribute("title");
	if (titre && titre[0])
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
#else
		::SetWindowText(hDlg, titre);
#endif
	}
	// pour ses propres controls
	const int nbControl = 9;
	const char *translatedText[nbControl];
	for (int i = 0 ; i < nbControl ; i++)
		translatedText[i] = NULL;

	for (TiXmlNodeA *childNode = userDefineDlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");

		if (sentinel && (name && name[0]))
		{
			if (id > 30)
			{
				HWND hItem = ::GetDlgItem(hDlg, id);
				if (hItem)
				{
#ifdef UNICODE
					const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
					::SetWindowText(hItem, nameW);
#else
					::SetWindowText(hItem, name);
#endif
				}
			}
			else
			{
				switch(id)
				{
					// JOCE Plain numerical values?? makes sense?
				case 0: case 1: case 2: case 3: case 4:
				case 5: case 6: case 7: case 8:
					translatedText[id] = name; break;

					NO_DEFAULT_CASE;
				}
			}
		}
	}

	const int nbDlg = 4;
	HWND hDlgArrary[nbDlg];
	hDlgArrary[0] = userDefineDlg->getFolderHandle();
	hDlgArrary[1] = userDefineDlg->getKeywordsHandle();
	hDlgArrary[2] = userDefineDlg->getCommentHandle();
	hDlgArrary[3] = userDefineDlg->getSymbolHandle();

	const int nbGrpFolder = 3;
	int folderID[nbGrpFolder][nbControl] = {\
	{IDC_DEFAULT_COLORSTYLEGROUP_STATIC, IDC_DEFAULT_FG_STATIC, IDC_DEFAULT_BG_STATIC, IDC_DEFAULT_FONTSTYLEGROUP_STATIC, IDC_DEFAULT_FONTNAME_STATIC, IDC_DEFAULT_FONTSIZE_STATIC, IDC_DEFAULT_BOLD_CHECK, IDC_DEFAULT_ITALIC_CHECK, IDC_DEFAULT_UNDERLINE_CHECK},\
	{IDC_FOLDEROPEN_COLORSTYLEGROUP_STATIC, IDC_FOLDEROPEN_FG_STATIC, IDC_FOLDEROPEN_BG_STATIC, IDC_FOLDEROPEN_FONTSTYLEGROUP_STATIC, IDC_FOLDEROPEN_FONTNAME_STATIC, IDC_FOLDEROPEN_FONTSIZE_STATIC, IDC_FOLDEROPEN_BOLD_CHECK, IDC_FOLDEROPEN_ITALIC_CHECK, IDC_FOLDEROPEN_UNDERLINE_CHECK},\
	{IDC_FOLDERCLOSE_COLORSTYLEGROUP_STATIC, IDC_FOLDERCLOSE_FG_STATIC, IDC_FOLDERCLOSE_BG_STATIC, IDC_FOLDERCLOSE_FONTSTYLEGROUP_STATIC, IDC_FOLDERCLOSE_FONTNAME_STATIC, IDC_FOLDERCLOSE_FONTSIZE_STATIC, IDC_FOLDERCLOSE_BOLD_CHECK, IDC_FOLDERCLOSE_ITALIC_CHECK, IDC_FOLDERCLOSE_UNDERLINE_CHECK}\
	};

	const int nbGrpKeywords = 4;
	int keywordsID[nbGrpKeywords][nbControl] = {\
	{IDC_KEYWORD1_COLORSTYLEGROUP_STATIC, IDC_KEYWORD1_FG_STATIC, IDC_KEYWORD1_BG_STATIC, IDC_KEYWORD1_FONTSTYLEGROUP_STATIC, IDC_KEYWORD1_FONTNAME_STATIC, IDC_KEYWORD1_FONTSIZE_STATIC, IDC_KEYWORD1_BOLD_CHECK, IDC_KEYWORD1_ITALIC_CHECK, IDC_KEYWORD1_UNDERLINE_CHECK},\
	{IDC_KEYWORD2_COLORSTYLEGROUP_STATIC, IDC_KEYWORD2_FG_STATIC, IDC_KEYWORD2_BG_STATIC, IDC_KEYWORD2_FONTSTYLEGROUP_STATIC, IDC_KEYWORD2_FONTNAME_STATIC, IDC_KEYWORD2_FONTSIZE_STATIC, IDC_KEYWORD2_BOLD_CHECK, IDC_KEYWORD2_ITALIC_CHECK, IDC_KEYWORD2_UNDERLINE_CHECK},\
	{IDC_KEYWORD3_COLORSTYLEGROUP_STATIC, IDC_KEYWORD3_FG_STATIC, IDC_KEYWORD3_BG_STATIC, IDC_KEYWORD3_FONTSTYLEGROUP_STATIC, IDC_KEYWORD3_FONTNAME_STATIC, IDC_KEYWORD3_FONTSIZE_STATIC, IDC_KEYWORD3_BOLD_CHECK, IDC_KEYWORD3_ITALIC_CHECK, IDC_KEYWORD3_UNDERLINE_CHECK},\
	{IDC_KEYWORD4_COLORSTYLEGROUP_STATIC, IDC_KEYWORD4_FG_STATIC, IDC_KEYWORD4_BG_STATIC, IDC_KEYWORD4_FONTSTYLEGROUP_STATIC, IDC_KEYWORD4_FONTNAME_STATIC, IDC_KEYWORD4_FONTSIZE_STATIC, IDC_KEYWORD4_BOLD_CHECK, IDC_KEYWORD4_ITALIC_CHECK, IDC_KEYWORD4_UNDERLINE_CHECK}\
	};

	const int nbGrpComment = 3;
	int commentID[nbGrpComment][nbControl] = {\
	{IDC_COMMENT_COLORSTYLEGROUP_STATIC, IDC_COMMENT_FG_STATIC, IDC_COMMENT_BG_STATIC, IDC_COMMENT_FONTSTYLEGROUP_STATIC, IDC_COMMENT_FONTNAME_STATIC, IDC_COMMENT_FONTSIZE_STATIC, IDC_COMMENT_BOLD_CHECK, IDC_COMMENT_ITALIC_CHECK, IDC_COMMENT_UNDERLINE_CHECK},\
	{IDC_NUMBER_COLORSTYLEGROUP_STATIC, IDC_NUMBER_FG_STATIC, IDC_NUMBER_BG_STATIC, IDC_NUMBER_FONTSTYLEGROUP_STATIC, IDC_NUMBER_FONTNAME_STATIC, IDC_NUMBER_FONTSIZE_STATIC, IDC_NUMBER_BOLD_CHECK, IDC_NUMBER_ITALIC_CHECK, IDC_NUMBER_UNDERLINE_CHECK},\
	{IDC_COMMENTLINE_COLORSTYLEGROUP_STATIC, IDC_COMMENTLINE_FG_STATIC, IDC_COMMENTLINE_BG_STATIC, IDC_COMMENTLINE_FONTSTYLEGROUP_STATIC, IDC_COMMENTLINE_FONTNAME_STATIC, IDC_COMMENTLINE_FONTSIZE_STATIC, IDC_COMMENTLINE_BOLD_CHECK, IDC_COMMENTLINE_ITALIC_CHECK, IDC_COMMENTLINE_UNDERLINE_CHECK}\
	};

	const int nbGrpOperator = 3;
	int operatorID[nbGrpOperator][nbControl] = {\
	{IDC_SYMBOL_COLORSTYLEGROUP_STATIC, IDC_SYMBOL_FG_STATIC, IDC_SYMBOL_BG_STATIC, IDC_SYMBOL_FONTSTYLEGROUP_STATIC, IDC_SYMBOL_FONTNAME_STATIC, IDC_SYMBOL_FONTSIZE_STATIC, IDC_SYMBOL_BOLD_CHECK, IDC_SYMBOL_ITALIC_CHECK, IDC_SYMBOL_UNDERLINE_CHECK},\
	{IDC_SYMBOL_COLORSTYLEGROUP2_STATIC, IDC_SYMBOL_FG2_STATIC, IDC_SYMBOL_BG2_STATIC, IDC_SYMBOL_FONTSTYLEGROUP2_STATIC, IDC_SYMBOL_FONTNAME2_STATIC, IDC_SYMBOL_FONTSIZE2_STATIC, IDC_SYMBOL_BOLD2_CHECK, IDC_SYMBOL_ITALIC2_CHECK, IDC_SYMBOL_UNDERLINE2_CHECK},\
	{IDC_SYMBOL_COLORSTYLEGROUP3_STATIC, IDC_SYMBOL_FG3_STATIC, IDC_SYMBOL_BG3_STATIC, IDC_SYMBOL_FONTSTYLEGROUP3_STATIC, IDC_SYMBOL_FONTNAME3_STATIC, IDC_SYMBOL_FONTSIZE3_STATIC, IDC_SYMBOL_BOLD3_CHECK, IDC_SYMBOL_ITALIC3_CHECK, IDC_SYMBOL_UNDERLINE3_CHECK}
	};

	int nbGpArray[nbDlg] = {nbGrpFolder, nbGrpKeywords, nbGrpComment, nbGrpOperator};
	const char nodeNameArray[nbDlg][16] = {"Folder", "Keywords", "Comment", "Operator"};

	for (int i = 0 ; i < nbDlg ; i++)
	{

		for (int j = 0 ; j < nbGpArray[i] ; j++)
		{
			switch (i)
			{
			case 0 : changeStyleCtrlsLang(hDlgArrary[i], folderID[j], translatedText); break;
			case 1 : changeStyleCtrlsLang(hDlgArrary[i], keywordsID[j], translatedText); break;
			case 2 : changeStyleCtrlsLang(hDlgArrary[i], commentID[j], translatedText); break;
			case 3 : changeStyleCtrlsLang(hDlgArrary[i], operatorID[j], translatedText); break;

				NO_DEFAULT_CASE;
			}
		}
		TiXmlNodeA *node = userDefineDlgNode->FirstChild(nodeNameArray[i]);

		if (node)
		{
			// Set Title
			titre = (node->ToElement())->Attribute("title");
			if (titre &&titre[0])
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(titre, _nativeLangEncoding);
				userDefineDlg->setTabName(i, nameW);
#else
				userDefineDlg->setTabName(i, titre);
#endif
			}
			for (TiXmlNodeA *childNode = node->FirstChildElement("Item");
				childNode ;
				childNode = childNode->NextSibling("Item") )
			{
				TiXmlElementA *element = childNode->ToElement();
				int id;
				const char *sentinel = element->Attribute("id", &id);
				const char *name = element->Attribute("name");
				if (sentinel && (name && name[0]))
				{
					HWND hItem = ::GetDlgItem(hDlgArrary[i], id);
					if (hItem)
					{
#ifdef UNICODE
						const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
						::SetWindowText(hItem, nameW);
#else
						::SetWindowText(hItem, name);
#endif
					}
				}
			}
		}
	}
}

void NativeLangSpeaker::changeFindReplaceDlgLang(FindReplaceDlg* findReplaceDlg)
{
	assert(findReplaceDlg);
	if (_nativeLangA)
	{
		TiXmlNodeA *dlgNode = _nativeLangA->FirstChild("Dialog");
		if (dlgNode)
		{
			NppParameters *pNppParam = NppParameters::getInstance();
			dlgNode = searchDlgNode(dlgNode, "Find");
			if (dlgNode)
			{
				const char *titre1 = (dlgNode->ToElement())->Attribute("titleFind");
				const char *titre2 = (dlgNode->ToElement())->Attribute("titleReplace");
				const char *titre3 = (dlgNode->ToElement())->Attribute("titleFindInFiles");
				if (titre1 && titre2 && titre3)
				{
#ifdef UNICODE
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();

					std::basic_string<wchar_t> nameW = wmc->char2wchar(titre1, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._find = nameW;

					nameW = wmc->char2wchar(titre2, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._replace = nameW;

					nameW = wmc->char2wchar(titre3, _nativeLangEncoding);
					pNppParam->getFindDlgTabTitiles()._findInFiles = nameW;
#else
					pNppParam->getFindDlgTabTitiles()._find = titre1;
					pNppParam->getFindDlgTabTitiles()._replace = titre2;
					pNppParam->getFindDlgTabTitiles()._findInFiles = titre3;
#endif
				}
			}

			findReplaceDlg->changeTabName(FIND_DLG, pNppParam->getFindDlgTabTitiles()._find.c_str());
			findReplaceDlg->changeTabName(REPLACE_DLG, pNppParam->getFindDlgTabTitiles()._replace.c_str());
			findReplaceDlg->changeTabName(FINDINFILES_DLG, pNppParam->getFindDlgTabTitiles()._findInFiles.c_str());
		}
	}
	changeDlgLang(findReplaceDlg->getHSelf(), "Find");
}

#define TITLE_BUF_LEN 128
void NativeLangSpeaker::changePrefereceDlgLang(PreferenceDlg* preferenceDlg)
{
	// JOCE: This method looks like it could have two implementations
	// Or maybe more simply: get rid of the non unicode code path.
	assert(preferenceDlg);
	changeDlgLang(preferenceDlg->getHSelf(), "Preference");

	char title[TITLE_BUF_LEN];

#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	changeDlgLang(preferenceDlg->_barsDlg->getHSelf(), "Global", title, TITLE_BUF_LEN);
	if (*title)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(title, _nativeLangEncoding);
		preferenceDlg->_ctrlTab->renameTab(TEXT("Global"), nameW);
#else
		preferenceDlg->_ctrlTab->renameTab("Global", title);
#endif
	}
	changeDlgLang(preferenceDlg->_marginsDlg->getHSelf(), "Scintillas", title, TITLE_BUF_LEN);
	if (*title)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(title, _nativeLangEncoding);
		preferenceDlg->_ctrlTab->renameTab(TEXT("Scintillas"), nameW);
#else
		preferenceDlg->_ctrlTab->renameTab("Scintillas", title);
#endif
	}

	changeDlgLang(preferenceDlg->_defaultNewDocDlg->getHSelf(), "NewDoc", title, TITLE_BUF_LEN);
	if (*title)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(title, _nativeLangEncoding);
		preferenceDlg->_ctrlTab->renameTab(TEXT("NewDoc"), nameW);
#else
		preferenceDlg->_ctrlTab->renameTab("NewDoc", title);
#endif
	}

	changeDlgLang(preferenceDlg->_fileAssocDlg->getHSelf(), "FileAssoc", title, TITLE_BUF_LEN);
	if (*title)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(title, _nativeLangEncoding);
		preferenceDlg->_ctrlTab->renameTab(TEXT("FileAssoc"), nameW);
#else
		preferenceDlg->_ctrlTab->renameTab("FileAssoc", title);
#endif
	}

	changeDlgLang(preferenceDlg->_langMenuDlg->getHSelf(), "LangMenu", title, TITLE_BUF_LEN);
	if (*title)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(title, _nativeLangEncoding);
		preferenceDlg->_ctrlTab->renameTab(TEXT("LangMenu"), nameW);
#else
		preferenceDlg->_ctrlTab->renameTab("LangMenu", title);
#endif
	}

	changeDlgLang(preferenceDlg->_printSettingsDlg->getHSelf(), "Print", title, TITLE_BUF_LEN);
	if (*title)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(title, _nativeLangEncoding);
		preferenceDlg->_ctrlTab->renameTab(TEXT("Print"), nameW);
#else
		preferenceDlg->_ctrlTab->renameTab("Print", title);
#endif
	}

	changeDlgLang(preferenceDlg->_settingsDlg->getHSelf(), "MISC", title, TITLE_BUF_LEN);
	if (*title)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(title, _nativeLangEncoding);
		preferenceDlg->_ctrlTab->renameTab(TEXT("MISC"), nameW);
#else
		preferenceDlg->_ctrlTab->renameTab("MISC", title);
#endif
	}
	changeDlgLang(preferenceDlg->_backupDlg->getHSelf(), "Backup", title, TITLE_BUF_LEN);
	if (*title)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(title, _nativeLangEncoding);
		preferenceDlg->_ctrlTab->renameTab(TEXT("Backup"), nameW);
#else
		preferenceDlg->_ctrlTab->renameTab("Backup", title);
#endif
	}
}

void NativeLangSpeaker::changeShortcutLang()
{
	if (!_nativeLangA) return;

	NppParameters * pNppParam = NppParameters::getInstance();
	std::vector<CommandShortcut> & mainshortcuts = pNppParam->getUserShortcuts();
	std::vector<ScintillaKeyMap> & scinshortcuts = pNppParam->getScintillaKeyList();
	int mainSize = (int)mainshortcuts.size();
	int scinSize = (int)scinshortcuts.size();

	TiXmlNodeA *shortcuts = _nativeLangA->FirstChild("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("Main");
	if (!shortcuts) return;

	TiXmlNodeA *entriesRoot = shortcuts->FirstChild("Entries");
	if (!entriesRoot) return;

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index, id;
		if (element->Attribute("index", &index) && element->Attribute("id", &id))
		{
			if (index > -1 && index < mainSize) { //valid index only
				const char *name = element->Attribute("name");
				CommandShortcut & csc = mainshortcuts[index];
				if (csc.getID() == id)
				{
#ifdef UNICODE
					WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
					const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
					csc.setName(nameW);
#else
					csc.setName(name);
#endif
				}
			}
		}
	}

	//Scintilla
	shortcuts = _nativeLangA->FirstChild("Shortcuts");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("Scintilla");
	if (!shortcuts) return;

	entriesRoot = shortcuts->FirstChild("Entries");
	if (!entriesRoot) return;

	for (TiXmlNodeA *childNode = entriesRoot->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index;
		if (element->Attribute("index", &index))
		{
			if (index > -1 && index < scinSize) { //valid index only
				const char *name = element->Attribute("name");
				ScintillaKeyMap & skm = scinshortcuts[index];
#ifdef UNICODE
				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
				skm.setName(nameW);
#else
				skm.setName(name);
#endif
			}
		}
	}
}

void NativeLangSpeaker::changeShortcutmapperLang(ShortcutMapper* sm)
{
	if (!_nativeLangA) return;

	TiXmlNodeA *shortcuts = _nativeLangA->FirstChild("Dialog");
	if (!shortcuts) return;

	shortcuts = shortcuts->FirstChild("ShortcutMapper");
	if (!shortcuts) return;

	for (TiXmlNodeA *childNode = shortcuts->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int index;
		if (element->Attribute("index", &index))
		{
			if (index > -1 && index < 5)  //valid index only
			{
				const char *name = element->Attribute("name");

#ifdef UNICODE
				WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
				const wchar_t * nameW = wmc->char2wchar(name, _nativeLangEncoding);
				sm->translateTab(index, nameW);
#else
				sm->translateTab(index, name);
#endif
			}
		}
	}
}


TiXmlNodeA * NativeLangSpeaker::searchDlgNode(TiXmlNodeA *node, const char *dlgTagName)
{
	TiXmlNodeA *dlgNode = node->FirstChild(dlgTagName);
	if (dlgNode) return dlgNode;
	for (TiXmlNodeA *childNode = node->FirstChildElement();
		childNode ;
		childNode = childNode->NextSibling() )
	{
		dlgNode = searchDlgNode(childNode, dlgTagName);
		if (dlgNode) return dlgNode;
	}
	return NULL;
}

bool NativeLangSpeaker::changeDlgLang(HWND hDlg, const char *dlgTagName, char *title, int titleBufLen)
{
	if (title && titleBufLen > 0)
		title[0] = '\0';

	if (!_nativeLangA) return false;

	TiXmlNodeA *dlgNode = _nativeLangA->FirstChild("Dialog");
	if (!dlgNode) return false;

	dlgNode = searchDlgNode(dlgNode, dlgTagName);
	if (!dlgNode) return false;

#ifdef UNICODE
	WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
#endif

	// Set Title
	const char *titleAttrib = (dlgNode->ToElement())->Attribute("title");
	if ((titleAttrib && titleAttrib[0]) && hDlg)
	{
#ifdef UNICODE
		const wchar_t *nameW = wmc->char2wchar(titleAttrib, _nativeLangEncoding);
		::SetWindowText(hDlg, nameW);
#else
		::SetWindowText(hDlg, titleAttrib);
#endif
		if (title)
			strcpy_s(title, titleBufLen, titleAttrib);
	}

	// Set the text of child control
	for (TiXmlNodeA *childNode = dlgNode->FirstChildElement("Item");
		childNode ;
		childNode = childNode->NextSibling("Item") )
	{
		TiXmlElementA *element = childNode->ToElement();
		int id;
		const char *sentinel = element->Attribute("id", &id);
		const char *name = element->Attribute("name");
		if (sentinel && (name && name[0]))
		{
			HWND hItem = ::GetDlgItem(hDlg, id);
			if (hItem)
			{
#ifdef UNICODE
				const wchar_t *nameW = wmc->char2wchar(name, _nativeLangEncoding);
				::SetWindowText(hItem, nameW);
#else
				::SetWindowText(hItem, name);
#endif
			}
		}
	}
	return true;
}
