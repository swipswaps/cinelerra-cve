
/*
 * CINELERRA
 * Copyright (C) 2006 Pierre Dumuid
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef LABELEDIT_H
#define LABELEDIT_H

#include "awindow.inc"
#include "bctextbox.h"
#include "bcwindow.h"
#include "mwindow.inc"
#include "thread.h"
#include "vwindow.inc"


class LabelEdit : public Thread
{
public:
	LabelEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow);

	void run();
	void edit_label(Label *label);

// If it is being created or edited
	MWindow *mwindow;
	AWindow *awindow;
	VWindow *vwindow;

	Label *label;
};


class LabelEditWindow : public BC_Window
{
public:
	LabelEditWindow(MWindow *mwindow, LabelEdit *thread, int absx, int absy);

// Use this copy of the pointer in LabelEdit since multiple windows are possible
	Label *label;
	MWindow *mwindow;
	BC_TextBox *textbox;
};


class LabelEditTitle : public BC_TextBox
{
public:
	LabelEditTitle(LabelEditWindow *window, int x, int y, int w);

	int handle_event();
	LabelEditWindow *window;
};


class LabelEditComments : public BC_TextBox
{
public:
	LabelEditComments(LabelEditWindow *window, int x, int y, int w, int rows);
};

#endif
