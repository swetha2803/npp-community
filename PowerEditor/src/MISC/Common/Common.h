//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#ifndef MISC_COMMON_COMMON_H
#define MISC_COMMON_COMMON_H

#define dirUp true
#define dirDown false

#define NPP_CP_WIN_1252           1252
#define NPP_CP_DOS_437            437
#define NPP_CP_BIG5               950

void folderBrowser(HWND parent, int outputCtrlID, const TCHAR *defaultStr = NULL);

void printInt(int int2print);
void printStr(const TCHAR *str2print);

void writeLog(const TCHAR *logFileName, const char *log2write);
int filter(unsigned int code);
generic_string purgeMenuItemString(const TCHAR * menuItemStr, bool keepAmpersand = false);
std::vector<generic_string> tokenizeString(const generic_string & tokenString, const char delim);

void ClientRectToScreenRect(HWND hWnd, RECT* rect);
void ScreenRectToClientRect(HWND hWnd, RECT* rect);

std::wstring string2wstring(const std::string & rString, UINT codepage);
std::string wstring2string(const std::wstring & rwString, UINT codepage);
bool isInList(const TCHAR *token, const TCHAR *list);
TCHAR *BuildMenuFileName(TCHAR *buffer, int len, int pos, const TCHAR *filename);

int strVal(const TCHAR *str, int base);
int decStrVal(const TCHAR *str);
int hexStrVal(const TCHAR *str);

// test string case insensitive
// 0 if equal, <0 if name comes before, >0 if it comes after
int CompareNoCase(const generic_string& name1, const generic_string& name2);

// MS reimplementations with N++'s generic strings.
BOOL PathRemoveFileSpec(generic_string & path);
BOOL PathAppend(generic_string& path, const generic_string& more);
BOOL PathCanonicalize(generic_string& path);
BOOL PathCanonicalize(generic_string& path, generic_string& output);

class WcharMbcsConvertor
{
public:
	static WcharMbcsConvertor * getInstance();
	static void destroyInstance();

	const wchar_t * char2wchar(const char *mbStr, UINT codepage, int lenIn=-1, int *pLenOut=NULL, int *pBytesNotProcessed=NULL);
	const wchar_t * char2wchar(const char *mbcs2Convert, UINT codepage, int *mstart, int *mend);
	const char * wchar2char(const wchar_t *wcStr, UINT codepage, int lenIn=-1, int *pLenOut=NULL);
	const char * wchar2char(const wchar_t *wcStr, UINT codepage, long *mstart, long *mend);

	const char * encode(UINT fromCodepage, UINT toCodepage, const char *txt2Encode, int lenIn=-1, int *pLenOut=NULL, int *pBytesNotProcessed=NULL) {
		int lenWc = 0;
        const wchar_t * strW = char2wchar(txt2Encode, fromCodepage, lenIn, &lenWc, pBytesNotProcessed);
        return wchar2char(strW, toCodepage, lenWc, pLenOut);
    };

	template <class T>
	class StringBuffer {
	public:
		StringBuffer() : _str(0), _allocLen(0) {}
		~StringBuffer()
		{
			if(_allocLen > 0)
			{
				delete [] _str;
			}
		}

		void sizeTo(size_t size)
		{
			if(_allocLen < size)
			{
				if(_allocLen > 0)
				{
					delete[] _str;
				}
				_allocLen = max(size, initSize);
				_str = new T[_allocLen];
			}
		}

		void empty()
		{
			static T nullStr = 0; // routines may return an empty string, with null terminator, without allocating memory; a pointer to this null character will be returned in that case
			if(_allocLen == 0)
			{
				_str = &nullStr;
			}
			else
			{
				_str[0] = 0;
			}
		}

		operator T*() { return _str; }

	protected:
		static const int initSize = 1024;
		size_t _allocLen;
		T* _str;
	};

protected:
	WcharMbcsConvertor(){}
	~WcharMbcsConvertor(){}
	static WcharMbcsConvertor * _pSelf;

	StringBuffer<char> _multiByteStr;
	StringBuffer<wchar_t> _wideCharStr;

private:
	// Since there's no public ctor, we need to void the default assignment operator.
	const WcharMbcsConvertor& operator= (const WcharMbcsConvertor&);
};

#define MACRO_RECORDING_IN_PROGRESS 1
#define MACRO_RECORDING_HAS_STOPPED 2

#define ERROR_MSG_SIZE 1024

#if _MSC_VER > 1400 // MS Compiler > VS 2005
#define REBARBAND_SIZE REBARBANDINFO_V3_SIZE
#else
#define REBARBAND_SIZE sizeof(REBARBANDINFO)
#endif

#endif //MISC_COMMON_COMMON_H
