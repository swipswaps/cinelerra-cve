
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
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "localsession.h"
#include "transportcommand.h"

#include <string.h>

TransportCommand::TransportCommand()
{
	edl = 0;
	command = 0;
	change_type = 0;
	reset();
}

void TransportCommand::reset()
{
	playbackstart = 0;
	start_position = 0;
	end_position = 0;
	realtime = 0;
	loop_playback = 0;
// Don't reset the change type for commands which don't perform the change
	if(command != STOP) change_type = 0;
	command = COMMAND_NONE;
}

EDL* TransportCommand::get_edl()
{
	return edl;
}

void TransportCommand::set_edl(EDL *edl)
{
	this->edl = edl;
}

void TransportCommand::copy_from(TransportCommand *cmd)
{
	command = cmd->command;
	change_type = cmd->change_type;
	edl = cmd->edl;
	start_position = cmd->start_position;
	end_position = cmd->end_position;
	playbackstart = cmd->playbackstart;
	realtime = cmd->realtime;
	loop_playback = cmd->loop_playback;
}

TransportCommand& TransportCommand::operator=(TransportCommand &command)
{
	copy_from(&command);
	return *this;
}

int TransportCommand::single_frame()
{
	return (command == SINGLE_FRAME_FWD ||
		command == SINGLE_FRAME_REWIND ||
		command == CURRENT_FRAME);
}


int TransportCommand::get_direction()
{
	switch(command)
	{
	case SINGLE_FRAME_FWD:
	case NORMAL_FWD:
	case FAST_FWD:
	case SLOW_FWD:
	case CURRENT_FRAME:
		return PLAY_FORWARD;

	case SINGLE_FRAME_REWIND:
	case NORMAL_REWIND:
	case FAST_REWIND:
	case SLOW_REWIND:
		return PLAY_REVERSE;

	default:
		return PLAY_FORWARD;
	}
}

double TransportCommand::get_speed()
{
	switch(command)
	{
	case SLOW_FWD:
	case SLOW_REWIND:
		return 0.5;

	case NORMAL_FWD:
	case NORMAL_REWIND:
	case SINGLE_FRAME_FWD:
	case SINGLE_FRAME_REWIND:
	case CURRENT_FRAME:
		return 1;

	case FAST_FWD:
	case FAST_REWIND:
		return 2;
	}
	return 0.0;
}

void TransportCommand::set_playback_range(int use_inout)
{
	ptstime totlen;

	if(!edl || command == STOP)
	{
		start_position = end_position = playbackstart = 0;
		return;
	}
	totlen = edl->total_length();
	loop_playback = 0;

	switch(command)
	{
	case SLOW_FWD:
	case FAST_FWD:
	case NORMAL_FWD:
		if(edl->local_session->loop_playback)
		{
			start_position = edl->local_session->loop_start;
			end_position = edl->local_session->loop_end;
			loop_playback = 1;
		}
		else
		{
			start_position = edl->local_session->get_selectionstart(1);
			if(EQUIV(edl->local_session->get_selectionend(1),
					edl->local_session->get_selectionstart(1)))
				end_position = totlen;
			else
				end_position = edl->local_session->get_selectionend(1);
		}
		playbackstart = start_position;
		break;

	case SLOW_REWIND:
	case FAST_REWIND:
	case NORMAL_REWIND:
		if(edl->local_session->loop_playback)
		{
			start_position = edl->local_session->loop_start;
			end_position = edl->local_session->loop_end;
			loop_playback = 1;
		}
		else
		{
			end_position = edl->local_session->get_selectionend(1);
			if(EQUIV(edl->local_session->get_selectionend(1), edl->local_session->get_selectionstart(1)))
				start_position = 0;
			else
				start_position = edl->local_session->get_selectionstart(1);
		}
		playbackstart = end_position;
		break;

	case CURRENT_FRAME:
		start_position = edl->local_session->get_selectionstart(1);
		end_position = start_position +
			1.0 / edlsession->frame_rate;
		playbackstart = start_position;
		break;
	case SINGLE_FRAME_FWD:
		start_position = edl->local_session->get_selectionstart(1);
		end_position = start_position +
			1.0 / edlsession->frame_rate;
		playbackstart = end_position;
		break;

	case SINGLE_FRAME_REWIND:
		start_position = edl->local_session->get_selectionend(1);
		end_position = start_position -
			1.0 / edlsession->frame_rate;
		playbackstart = end_position;
		break;
	}

	if(use_inout)
	{
		if(edl->local_session->inpoint_valid())
			start_position = edl->local_session->get_inpoint();
		if(edl->local_session->outpoint_valid())
			end_position = edl->local_session->get_outpoint();
	}

	if(end_position > totlen)
		end_position = totlen;
	if(start_position > totlen)
		start_position = totlen;
	if(end_position < 0)
		end_position = 0;
	if(start_position < 0)
		start_position = 0;
	if(playbackstart > totlen)
		playbackstart = totlen;
	if(playbackstart < 0)
		playbackstart = 0;
}

void TransportCommand::playback_range_inout()
{
	if(edl->local_session->inpoint_valid())
		start_position = edl->local_session->get_inpoint();
	else
		start_position = 0;

	if(edl->local_session->outpoint_valid())
		end_position = edl->local_session->get_outpoint();
	else
		end_position = edl->total_length();
}

void TransportCommand::playback_range_project()
{
	start_position = 0;
	end_position = edl->total_length();
}

// Debug
const char* TransportCommand::commandstr(int cmd)
{
	static const char* cmdmemo[] =
	{
		"None", 		// 0
		"Frame Forward", 	// 1
		"Normal Forward", 	// 2
		"Fast Forward", 	// 3
		"Frame Rewind", 	// 4
		"Normal Rewind", 	// 5
		"Fast Rewind", 		// 6
		"Stop", 		// 7
		"Unused", 		// 8
		"Slow Forward", 	// 9
		"Slow Rewind", 		// 10
		"Rewind", 		// 11
		"Goto End",		// 12
		"Current Frame"		// 13
	};
	if(cmd == -1)
		cmd = command;
	if(cmd < 0 || cmd >= (sizeof(cmdmemo)/sizeof(const char *)))
		return "Unknown";
	return cmdmemo[cmd];
}

// Debug
void TransportCommand::dump(int indent)
{
	const char *tps;
	char b[64];

	printf("%*sTransportCommand %p dump: '%s' %s\n", indent, "",
		this, commandstr(), realtime ? "rt" : "--");
	indent += 2;
	printf("%*splayback %.3f; positions start=%.3f, end=%.3f loop %d\n", indent, "",
		playbackstart, start_position, end_position, loop_playback);
	if(change_type == CHANGE_ALL)
		tps = " All";
	else if(change_type == CHANGE_NONE)
		tps = " None";
	else
	{
		b[0] = 0;
		if(change_type & CHANGE_EDL)
			strcat(b, " EDL");
		if(change_type & CHANGE_PARAMS)
			strcat(b, " PAR");
		tps = b;
	}
	printf("%*schange_type:%s, edl: %p\n",  indent, "", tps, edl);
}

