/*
this file is part of notepad++
Copyright (C)2003 Don HO < donho@altern.org >

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef WINCONTROLS_COLOURPICKER_COLOURPOPUP_H
#define WINCONTROLS_COLOURPICKER_COLOURPOPUP_H

#ifndef WINCONTROLS_WINDOW_H
#include "WinControls/Window.h"
#endif

#ifndef RESOURCE_H
#include "resource.h"
#endif

#define WM_PICKUP_COLOR (COLOURPOPUP_USER + 1)
#define WM_PICKUP_CANCEL (COLOURPOPUP_USER + 2)

class ColourPopup : public Window
{
public :
    ColourPopup();
	ColourPopup(COLORREF defaultColor);
	void create(int dialogID);

    void doDialog(POINT p);

    COLORREF getSelColour(){return _colour;};

private :
	RECT _rc;
    COLORREF _colour;
	bool isColourChooserLaunched;

	static BOOL CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif //WINCONTROLS_COLOURPICKER_COLOURPOPUP_H


