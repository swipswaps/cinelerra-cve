
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "levelwindow.h"
#include "levelwindowgui.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "theme.h"

LevelWindowGUI::LevelWindowGUI(MWindow *mwindow)
 : BC_Window(MWindow::create_title(N_("Levels")),
	mainsession->lwindow_x,
	mainsession->lwindow_y,
	mainsession->lwindow_w,
	mainsession->lwindow_h,
	10,
	10,
	1,
	0, 
	1)
{
	this->mwindow = mwindow;
	set_icon(mwindow->get_window_icon());
	mwindow->theme->draw_lwindow_bg(this);
	panel = new MeterPanel(mwindow,
		this,
		5,
		5,
		get_h() - 10,
		edlsession->audio_channels,
		1);
}

LevelWindowGUI::~LevelWindowGUI()
{
	delete panel;
}

void LevelWindowGUI::resize_event(int w, int h)
{
	mainsession->lwindow_x = get_x();
	mainsession->lwindow_y = get_y();
	mainsession->lwindow_w = w;
	mainsession->lwindow_h = h;

	mwindow->theme->draw_lwindow_bg(this);

	panel->reposition_window(panel->x,
		panel->y,
		h - 10);

	BC_WindowBase::resize_event(w, h);
}

void LevelWindowGUI::translation_event()
{
	mainsession->lwindow_x = get_x();
	mainsession->lwindow_y = get_y();
}

void LevelWindowGUI::close_event()
{
	hide_window();
	mainsession->show_lwindow = 0;
	mwindow->gui->mainmenu->show_lwindow->set_checked(0);
	mwindow->save_defaults();
}

int LevelWindowGUI::keypress_event()
{
	if(get_keypress() == 'w' || get_keypress() == 'W')
	{
		close_event();
		return 1;
	}
	return 0;
}
