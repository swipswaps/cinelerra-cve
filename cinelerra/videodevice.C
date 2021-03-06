
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

#include "asset.h"
#include "bccapture.h"
#include "bcsignals.h"
#include "file.inc"
#include "mutex.h"
#include "mwindow.h"
#include "playbackconfig.h"
#include "playbackengine.h"
#include "preferences.h"
#include "vdevicex11.h"
#include "videodevice.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

VideoDevice::VideoDevice(MWindow *mwindow)
{
	this->mwindow = mwindow;
	out_config = 0;
	irate = 0;
	out_w = out_h = 0;
	writing = 0;
	output_base = 0;
	interrupt = 0;
	adevice = 0;
	single_frame = 0;
}

VideoDevice::~VideoDevice()
{
	close_all();
}


VDeviceX11 *VideoDevice::get_output_base()
{
	return output_base;
}

const char* VideoDevice::drivertostr(int driver)
{
	switch(driver)
	{
	case PLAYBACK_X11:
		return PLAYBACK_X11_TITLE;
	case PLAYBACK_X11_XV:
		return PLAYBACK_X11_XV_TITLE;
	case PLAYBACK_X11_GL:
		return PLAYBACK_X11_GL_TITLE;
	case VIDEO4LINUX:
		return VIDEO4LINUX_TITLE;
	case VIDEO4LINUX2:
		return VIDEO4LINUX2_TITLE;
	case VIDEO4LINUX2JPEG:
		return VIDEO4LINUX2JPEG_TITLE;
	case SCREENCAPTURE:
		return SCREENCAPTURE_TITLE;
	}
	return "";
}

void VideoDevice::close_all()
{
	if(writing)
	{
		if(output_base)
		{
			delete output_base;
			output_base = 0;
		}
		writing = 0;
	}
}

void VideoDevice::set_adevice(AudioDevice *adevice)
{
	this->adevice = adevice;
}

// ================================= OUTPUT ==========================================


int VideoDevice::open_output(VideoOutConfig *config,
			int out_w,
			int out_h,
			int colormodel,
			Canvas *output,
			int single_frame)
{
	writing = 1;
	out_config = config;
	this->out_w = out_w;
	this->out_h = out_h;
	this->single_frame = single_frame;
	interrupt = 0;

	switch(config->driver)
	{
	case PLAYBACK_X11:
	case PLAYBACK_X11_XV:
	case PLAYBACK_X11_GL:
		output_base = new VDeviceX11(this, output);
		break;
	}

	if(output_base->open_output(colormodel))
	{
		delete output_base;
		output_base = 0;
	}

	if(output_base) 
		return 0;
	else
		return 1;
}

void VideoDevice::interrupt_playback()
{
	interrupt = 1;
}

int VideoDevice::write_buffer(VFrame *output, EDL *edl)
{
	if(output_base) return output_base->write_buffer(output, edl);
	return 1;
}

int VideoDevice::output_visible()
{
	if(output_base) return output_base->output_visible();
	return 0;
}

int VideoDevice::set_cloexec_flag(int desc, int value)
{
	int oldflags = fcntl(desc, F_GETFD, 0);
	if(oldflags < 0) return oldflags;
	if(value != 0) 
		oldflags |= FD_CLOEXEC;
	else
		oldflags &= ~FD_CLOEXEC;
	return fcntl(desc, F_SETFD, oldflags);
}
