
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

#include "bcsignals.h"
#include "bcdragwindow.h"
#include "bcpixmap.h"
#include "bcresources.h"

#include "vframe.h"
#include <unistd.h>

BC_DragWindow::BC_DragWindow(BC_WindowBase *parent_window, 
	BC_Pixmap *pixmap, 
	int icon_x, 
	int icon_y)
 : BC_Popup(parent_window, 
	icon_x, 
	icon_y,
	pixmap->get_w(),
	pixmap->get_h(),
	-1,
	0,
	pixmap)
{
	int cx, cy;

	init_x = icon_x;
	init_y = icon_y;
	end_x = BC_INFINITY;
	end_y = BC_INFINITY;
	BC_Resources::get_abs_cursor(&cx, &cy);
	icon_offset_x = init_x - cx;
	icon_offset_y = init_y - cy;
	do_animation = 1;
}


BC_DragWindow::BC_DragWindow(BC_WindowBase *parent_window, 
	VFrame *frame, 
	int icon_x, 
	int icon_y)
 : BC_Popup(parent_window, 
	icon_x, 
	icon_y,
	frame->get_w(),
	frame->get_h(),
	-1,
	0,
	prepare_frame(frame, parent_window))
{
	int cx, cy;

	delete temp_frame;  // created in prepare_frame inside constructor
	init_x = icon_x;
	init_y = icon_y;
	end_x = BC_INFINITY;
	end_y = BC_INFINITY;
	BC_Resources::get_abs_cursor(&cx, &cy);
	icon_offset_x = init_x - cx;
	icon_offset_y = init_y - cy;
	do_animation = 1;
}

BC_DragWindow::~BC_DragWindow()
{
}

int BC_DragWindow::get_init_x(BC_WindowBase *parent_window, int icon_x)
{
	int output_x, temp = 0;
	Window tempwin;

	parent_window->top_level->lock_window("BC_DragWindow::get_init_x");
	XTranslateCoordinates(parent_window->top_level->display, 
		parent_window->win, 
		parent_window->top_level->rootwin, 
		icon_x, 
		temp, 
		&output_x, 
		&temp, 
		&tempwin);
	parent_window->top_level->unlock_window();
	return output_x;
}

int BC_DragWindow::get_init_y(BC_WindowBase *parent_window, int icon_y)
{
	int output_y, temp = 0;
	Window tempwin;

	parent_window->top_level->lock_window("BC_DragWindow::get_init_y");
	XTranslateCoordinates(parent_window->top_level->display, 
		parent_window->win, 
		parent_window->top_level->rootwin, 
		temp, 
		icon_y, 
		&temp, 
		&output_y, 
		&tempwin);
	parent_window->top_level->unlock_window();
	return output_y;
}

int BC_DragWindow::cursor_motion_event()
{
	int cx, cy;

	BC_Resources::get_abs_cursor(&cx, &cy);
	reposition_window(cx + icon_offset_x,
		cy + icon_offset_y,
		get_w(), 
		get_h());
	flush();
	return 1;
}

int BC_DragWindow::get_offset_x()
{
	return icon_offset_x;
}

int BC_DragWindow::get_offset_y()
{
	return icon_offset_y;
}

int BC_DragWindow::drag_failure_event()
{
	if(!do_animation) return 0;

	if(end_x == BC_INFINITY)
	{
		end_x = get_x();
		end_y = get_y();
	}

	for(int i = 0; i < 10; i++)
	{
		int new_x = end_x + (init_x - end_x) * i / 10;
		int new_y = end_y + (init_y - end_y) * i / 10;

		reposition_window(new_x, 
			new_y, 
			get_w(), 
			get_h());
		flush();
		usleep(1000);
	}
	return 0;
}

void BC_DragWindow::set_animation(int value)
{
	this->do_animation = value;
}

BC_Pixmap *BC_DragWindow::prepare_frame(VFrame *frame, BC_WindowBase *parent_window)
{
	temp_frame = 0;
	
	if(frame->get_color_model() == BC_RGBA8888)
	{
		temp_frame = new VFrame(*frame);
	}
	else
	{
		temp_frame = new VFrame(0, 
					frame->get_w(), 
					frame->get_h(), 
					BC_RGBA8888); 
		temp_frame->transfer_from(frame);
	}
	temp_frame->get_row_ptr(temp_frame->get_h() / 2)[(temp_frame->get_w() / 2) * 4 + 3] = 0;
	my_pixmap = new BC_Pixmap(parent_window,
			temp_frame,
			PIXMAP_ALPHA);

	return (my_pixmap);
}


