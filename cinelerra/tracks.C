
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
#include "atrack.h"
#include "bcpan.h"
#include "automation.h"
#include "bcsignals.h"
#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "localsession.h"
#include "panauto.h"
#include "panautos.h"
#include "plugin.h"
#include "theme.inc"
#include "track.h"
#include "tracks.h"
#include "vtrack.h"

#include <string.h>

Tracks::Tracks(EDL *edl)
 : List<Track>()
{
	this->edl = edl;
}

Tracks::~Tracks()
{
	delete_all_tracks();
}

void Tracks::reset_instance()
{
	delete_all_tracks();
}

void Tracks::reset_plugins()
{
	ptstime selections[4];

	if(edl == master_edl)
	{
		master_edl->local_session->get_selections(selections);
		// no highlight active
		if(!EQUIV(selections[0], selections[1]))
			return;
		for(Track *current = first; current; current = current->next)
			current->reset_plugins(selections[0]);
	}
// FIXIT: reset for rendering should be implemented
}

void Tracks::reset_renderers()
{
	for(Track *current = first; current; current = NEXT)
		current->reset_renderers();
}

void Tracks::equivalent_output(Tracks *tracks, ptstime *result)
{
	if(playable_tracks_of(TRACK_VIDEO) != tracks->playable_tracks_of(TRACK_VIDEO))
	{
		*result = 0;
	}
	else
	{
		Track *current = first;
		Track *that_current = tracks->first;
		while(current || that_current)
		{
// Get next video track
			while(current && current->data_type != TRACK_VIDEO)
				current = NEXT;

			while(that_current && that_current->data_type != TRACK_VIDEO)
				that_current = that_current->next;

// One doesn't exist but the other does
			if((!current && that_current) ||
				(current && !that_current))
			{
				*result = 0;
				break;
			}
			else
// Both exist
			if(current && that_current)
			{
				current->equivalent_output(that_current, result);
				current = NEXT;
				that_current = that_current->next;
			}
		}
	}
}

void Tracks::get_affected_edits(ArrayList<Edit*> *drag_edits, ptstime position, Track *start_track)
{
	drag_edits->remove_all();

	for(Track *track = start_track;
		track;
		track = track->next)
	{
		if(track->record)
		{
			for(Edit *edit = track->edits->first; edit; edit = edit->next)
			{
				ptstime startproject = edit->get_pts();
				if(edl->equivalent(startproject, position))
				{
					drag_edits->append(edit);
					break;
				}
			}
		}
	}

}

void Tracks::get_automation_extents(float *min, 
	float *max,
	ptstime start,
	ptstime end,
	int autogrouptype)
{
	*min = 0;
	*max = 0;
	int coords_undefined = 1;
	for(Track *current = first; current; current = NEXT)
	{
		if(current->record)
		{
			current->automation->get_extents(min,
				max,
				&coords_undefined,
				start,
				end,
				autogrouptype);
		}
	}
}

void Tracks::copy_from(Tracks *tracks)
{
	Track *new_track;

	delete_all_tracks();
	for(Track *current = tracks->first; current; current = NEXT)
	{
		new_track = add_track(current->data_type, 0, 0);
		new_track->copy_from(current);
	}
	init_plugin_pointers_by_ids();
}

Tracks& Tracks::operator=(Tracks &tracks)
{
	copy_from(&tracks);
	return *this;
}

void Tracks::load(FileXML *xml)
{
// add the appropriate type of track
	char string[BCTEXTLEN];
	Track *track = 0;

	string[0] = 0;

	xml->tag.get_property("TYPE", string);

	if(!strcmp(string, "VIDEO"))
		track = add_track(TRACK_VIDEO, 0, 0);
	else
		track = add_track(TRACK_AUDIO, 0, 0);    // default to audio

	if(track)
		track->load(xml);
}

void Tracks::init_shared_pointers()
{
	for(Track *current = first; current; current = NEXT)
		current->init_shared_pointers();
}

Track* Tracks::add_track(int track_type, int above, Track *dst_track)
{
	Track* new_track = new VTrack(edl, this);

	switch(track_type)
	{
	case TRACK_VIDEO:
		new_track = new VTrack(edl, this);
		break;

	case TRACK_AUDIO:
		new_track = new ATrack(edl, this);
		break;
	}

	if(!dst_track)
		dst_track = (above ? first : last);

	if(above)
		insert_before(dst_track, new_track);
	else
		insert_after(dst_track, new_track);

	new_track->set_default_title();

	if(track_type == TRACK_AUDIO)
	{
		int current_pan = 0;

		for(Track *current = first;
			current != (Track*)new_track;
			current = NEXT)
		{
			if(current->data_type == TRACK_AUDIO)
				current_pan++;
			if(current_pan >= edlsession->audio_channels)
				current_pan = 0;
		}

		PanAutos* pan_autos =
			(PanAutos*)new_track->automation->autos[AUTOMATION_PAN];

		pan_autos->default_values[current_pan] = 1.0;

		BC_Pan::calculate_stick_position(edlsession->audio_channels,
			edlsession->achannel_positions,
			pan_autos->default_values,
			MAX_PAN,
			PAN_RADIUS,
			pan_autos->default_handle_x,
			pan_autos->default_handle_y);
	}

	return new_track;
}

ptstime Tracks::length()
{
	for(Track *track = first; track; track = track->next)
	{
		if(track->master)
			return track->get_length();
	}
	return 0;
}

ptstime Tracks::append_asset(Asset *asset, ptstime paste_at,
	Track *first_track, int overwrite)
{
	Track *master = 0;
	int atracks, vtracks;
	ptstime alength = 0;
	ptstime start = 0;
	ptstime dur, pasted_length;

	atracks = vtracks = 0;

	if(asset->video_data)
		vtracks = asset->layers;

	if(asset->audio_data)
		atracks = asset->channels;

	alength = asset->total_length_framealigned(edlsession->frame_rate);

	// Determine the start and length
	// If master track is part of the operation, the length
	// is sum of master and asset duration
	// else the final length is the current length of master track
	for(Track *current = first_track ? first_track : first;
			current; current = current->next)
	{
		if(!current->record)
			continue;

		dur = 0;

		if(vtracks && current->data_type == TRACK_VIDEO)
		{
			vtracks--;
			dur = current->get_length();
			if(current->master)
				master = current;
		}

		if(atracks && current->data_type == TRACK_AUDIO)
		{
			atracks--;
			dur = current->get_length();
			if(current->master)
				master = current;
		}
		if(dur > start)
			start = dur;
	}

	if(paste_at < 0)
	{
		// If master is part of operation we append to master
		// If master does not paticipate, append to the longest participating track
		if(master)
			start = master->get_length();
		else if((dur = length()) > 0)
			alength = dur - start;
	}
	else
	{
		dur = length();
		start = MIN(paste_at, dur);

		if(!master && dur > 0)
			alength = dur - start;
	}

	atracks = vtracks = 0;

	if(asset->video_data)
		vtracks = asset->layers;

	if(asset->audio_data)
		atracks = asset->channels;

	pasted_length  = 0;

	start = master_edl->align_to_frame(start);
	alength = master_edl->align_to_frame(alength);

	for(Track *current = first_track ? first_track : first;
		current; current = current->next)
	{
		dur = 0;
		if(!current->record)
			continue;

		if(vtracks && current->data_type == TRACK_VIDEO)
		{
			dur = MIN(alength, asset->video_duration);
			current->insert_asset(asset, dur,
				start, asset->layers - vtracks, overwrite);
			vtracks--;
		}
		if(atracks && current->data_type == TRACK_AUDIO)
		{
			dur = MIN(alength, asset->audio_duration);
			current->insert_asset(asset, MIN(alength, asset->audio_duration),
				start, asset->layers - vtracks, overwrite);
			atracks--;
		}
		if(dur > pasted_length)
			pasted_length = dur;
	}
	return pasted_length;
}

ptstime Tracks::append_tracks(Tracks *tracks, ptstime paste_at,
	Track *first_track, int options)
{
	Track *master = 0;
	ptstime alength = 0;
	ptstime start = 0;
	ptstime dur, pasted_length;
	Track *new_track;
	int overwrite = options & TRACKS_OVERWRITE;

	// Determine the start and length
	// If master track is part of the operation, the length
	// is sum of master and asset duration
	// else the final length is the current length of master track
	new_track = tracks->first;
	for(Track *current = first_track ? first_track : first;
			current; current = current->next)
	{
		if(!current->record)
			continue;

		dur = 0;

		if(current->data_type == TRACK_VIDEO)
		{
			for(; new_track; new_track = new_track->next)
				if(new_track->data_type == TRACK_VIDEO)
					break;
			if(new_track)
			{
				dur = current->get_length();
				if(current->master)
				{
					master = current;
					if(options & TRACKS_EFFECTS)
					{
						alength = new_track->get_effects_length(1);

						if(alength < EPSILON)
						{
							alength = new_track->get_effects_length(0);
							if(alength + paste_at > dur)
							{
								alength = dur - paste_at;
								if(alength < 0)
									alength = 0;
							}
						}
					}
					else
						alength = new_track->get_length();
				}
			}
		}
		if(dur > start)
			start = dur;
	}

	new_track = tracks->first;
	for(Track *current = first_track ? first_track : first;
			current; current = current->next)
	{
		if(!current->record)
			continue;

		dur = 0;

		if(current->data_type == TRACK_AUDIO)
		{
			for(; new_track; new_track = new_track->next)
				if(new_track->data_type == TRACK_AUDIO)
					break;
			if(new_track)
			{
				dur = current->get_length();
				if(current->master)
				{
					master = current;
					if(options & TRACKS_EFFECTS)
					{
						alength = new_track->get_effects_length(1);
						if(alength < EPSILON)
						{
							alength = new_track->get_effects_length(0);
							if(alength + paste_at > dur)
							{
								alength = dur - paste_at;
								if(alength < 0)
									alength = 0;
							}
						}
					}
					else
						alength = new_track->get_length();
				}
			}
		}
		if(dur > start)
			start = dur;
	}
	if(paste_at < 0)
	{
		// If master is part of operation we append to master
		// If master does not participate, append to the longest participating track
		if(master)
			start = master->get_length();
		else
			alength = length() - start;
	}
	else
	{
		dur = length();
		start = MIN(paste_at, dur);
		if(!master || alength < EPSILON)
			alength = dur - start;
	}
	pasted_length  = 0;

	start = master_edl->align_to_frame(start);
	alength = master_edl->align_to_frame(alength);

	if(alength < EPSILON)
		return 0;

// Paste video tracks, then audio
	new_track = tracks->first;
	for(Track *current = first_track ? first_track : first;
			current; current = current->next)
	{
		dur = 0;
		if(!current->record)
			continue;

		if(current->data_type == TRACK_VIDEO)
		{
			for(; new_track; new_track = new_track->next)
				if(new_track->data_type == TRACK_VIDEO)
					break;
			if(!new_track)
				break;

			if(options & TRACKS_EFFECTS)
				dur = new_track->get_effects_length(0);
			else
				dur = new_track->get_length();

			dur = MIN(alength, dur);
			current->insert_track(new_track, dur, start, overwrite);
			new_track = new_track->next;
		}
		if(dur > pasted_length)
			pasted_length = dur;
	}

	new_track = tracks->first;
	for(Track *current = first_track ? first_track : first;
		current; current = current->next)
	{
		dur = 0;
		if(!current->record)
			continue;

		if(current->data_type == TRACK_AUDIO)
		{
			for(; new_track; new_track = new_track->next)
				if(new_track->data_type == TRACK_AUDIO)
					break;
			if(!new_track)
				break;

			if(options & TRACKS_EFFECTS)
				dur = new_track->get_effects_length(0);
			else
				dur = new_track->get_length();

			dur = MIN(alength, dur);
			current->insert_track(new_track, dur, start, overwrite);
			new_track = new_track->next;
		}
		if(dur > pasted_length)
			pasted_length = dur;
	}
	init_plugin_pointers_by_ids();
	return pasted_length;
}

void Tracks::init_plugin_pointers_by_ids()
{
	for(Track *current = first; current; current = current->next)
	{
		for(int i = 0; i < current->plugins.total; i++)
			current->plugins.values[i]->init_pointers_by_ids();
	}
}

void Tracks::create_new_tracks(Asset *asset)
{
	ptstime master_length = length();
	ptstime len;
	Track *new_track;
	int atracks, vtracks;

	atracks = vtracks = 0;

	if(asset->video_data)
		vtracks = asset->layers;

	if(asset->audio_data)
		atracks = asset->channels;

	if(master_length < EPSILON)
		master_length = MIN(asset->video_duration, asset->audio_duration);

	if(!atracks && !vtracks || master_length < EPSILON)
		return;

	for(int i = 0; i < vtracks; i++)
	{
		new_track = add_track(TRACK_VIDEO, 0, 0);
		len = asset->total_length_framealigned(edlsession->frame_rate);
		if(len > master_length)
			len = master_length;
		new_track->insert_asset(asset, len, 0, i);
	}

	for(int i = 0; i < atracks; i++)
	{
		new_track = add_track(TRACK_AUDIO, 0, 0);
		len = asset->total_length_framealigned(edlsession->frame_rate);
		if(len > master_length)
			len = master_length;
		new_track->insert_asset(asset, len, 0, i);
	}
}

void Tracks::create_new_tracks(Tracks *tracks)
{
	ptstime master_length = length();
	ptstime len;
	Track *new_track;

	if(master_length < EPSILON)
		master_length = tracks->length();

	if(!tracks->total() || master_length < EPSILON)
		return;

	for(Track *track = tracks->first; track; track = track->next)
	{
		new_track = add_track(track->data_type, 0, 0);
		len = track->get_length();
		if(len > master_length)
			len = master_length;
		new_track->insert_track(track, len, 0);
	}
	init_plugin_pointers_by_ids();
	edl->check_master_track();
}

void Tracks::delete_track(Track *track)
{
	if(!track)
		return;

	detach_shared_effects(0, track);

	if(track) delete track;
	edl->check_master_track();
}

void Tracks::detach_shared_effects(Plugin *plugin, Track *track)
{
	for(Track *current = first; current; current = NEXT)
		current->detach_shared_effects(plugin, track);
}

int Tracks::recordable_tracks_of(int type)
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == type && current->record)
			result++;
	}
	return result;
}

int Tracks::playable_tracks_of(int type)
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == type && current->play)
		{
			result++;
		}
	}
	return result;
}

int Tracks::total_tracks_of(int type)
{
	int result = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == type)
			result++;
	}
	return result;
}

ptstime Tracks::total_length()
{
	ptstime total = 0;

	for(Track *current = first; current; current = NEXT)
	{
		ptstime length = current->get_length();
		if(length > total) total = length;
	}
	return total;
}

ptstime Tracks::total_length_of(int type)
{
	ptstime len, total = 0;

	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == type)
		{
			len = current->get_length();
			if(len > total)
				total = len;
		}
	}
	return total;
}

ptstime Tracks::total_length_framealigned(double fps)
{
	int atracks, vtracks;
	ptstime alen, vlen;

	alen = vlen = 0;

	if(atracks = total_tracks_of(TRACK_AUDIO))
		alen = total_length_of(TRACK_AUDIO);

	if(vtracks = total_tracks_of(TRACK_VIDEO))
		vlen = total_length_of(TRACK_VIDEO);

	if(atracks && vtracks)
		return MIN(floor(alen * fps), floor(vlen * fps)) / fps;

	if(atracks)
		return floor(alen * fps) / fps;

	if(vtracks)
		return floor(vlen * fps) / fps;

	return 0;
}

void Tracks::translate_projector(float offset_x, float offset_y)
{
	for(Track *current = first; current; current = NEXT)
	{
		if(current->data_type == TRACK_VIDEO)
		{
			((VTrack*)current)->translate(offset_x, offset_y, 0);
		}
	}
}

void Tracks::update_y_pixels(Theme *theme)
{
	int y = -edl->local_session->track_start;
	for(Track *current = first; current; current = NEXT)
	{
		current->y_pixel = y;
		y += current->vertical_span(theme);
	}
}

void Tracks::dump(int indent)
{
	printf("%*sTracks %p dump(%d):\n", indent, "", this, total());
	indent += 2;
	for(Track* current = first; current; current = NEXT)
	{
		current->dump(indent);
	}
}

int Tracks::totalpixels()
{
	int result = 0;

	for(Track* current = first; current; current = NEXT)
		result += edl->local_session->zoom_track;

	return result;
}

void Tracks::cleanup()
{
	for(Track* current = first; current; current = NEXT)
		current->cleanup();
}

void Tracks::cleanup_plugins()
{
	int have_shared_track = 0;

	for(Track *track = first; track; track = track->next)
	{
		for(int i = 0; i < track->plugins.total; i++)
		{
			Plugin *plugin = track->plugins.values[i];
			Track *cur;
			Plugin *slave;
			int found;

			if(plugin->plugin_type == PLUGIN_SHAREDMODULE)
				have_shared_track = 1;

			if(plugin->plugin_type != PLUGIN_STANDALONE ||
					!plugin->plugin_server ||
					!plugin->plugin_server->multichannel)
				continue;

			ptstime start = plugin->get_pts();
			ptstime end = plugin->end_pts();

			for(cur = first; cur; cur = cur->next)
			{
				found = 0;
				for(int j = 0; j < cur->plugins.total; j++)
				{
					slave = cur->plugins.values[j];

					if(slave->plugin_type == PLUGIN_SHAREDPLUGIN &&
						slave->shared_plugin == plugin)
					{
					// Force slave pts equivalent of masters
						slave->set_pts(start);
						slave->set_end(end);
						found = 1;
						break;
					}
				}
				if(found)
				{
				// Remove all other shared plugin slaves active same time
					for(int j = 0; j < cur->plugins.total; j++)
					{
						Plugin *curplugin = cur->plugins.values[j];

						if(curplugin->plugin_type == PLUGIN_SHAREDPLUGIN &&
							curplugin->shared_plugin &&
							curplugin->shared_plugin != plugin &&
							curplugin->shared_plugin->plugin_server &&
							curplugin->shared_plugin->plugin_server->multichannel)
						{
							ptstime cur_start = curplugin->get_pts();
							ptstime cur_end = curplugin->end_pts();

							if(cur_start < end && cur_end > start)
							{
								// Remove shared slave that overlaps
								// with current plugin
								cur->plugins.remove_object(curplugin);
								j--;
							}
						}
					}
				}
			}
		}
	}
	if(have_shared_track)
	{
		for(Track *track = first; track; track = track->next)
		{
			for(int i = 0; i < track->plugins.total; i++)
			{
				Plugin *plugin = track->plugins.values[i];

				if(plugin->plugin_type != PLUGIN_SHAREDMODULE)
					continue;

				ptstime start = plugin->get_pts();
				ptstime end = plugin->end_pts();

				if(track->get_shared_multichannel(start, end))
				{
					// Remove shared track plugin
					track->plugins.remove_object(plugin);
					i--;
					continue;
				}
				if(plugin->shared_track)
				{
					// Remove shared plugin from src track
					Plugin *src = plugin->shared_track->get_shared_multichannel(start, end);

					if(src)
						plugin->shared_track->plugins.remove_object(src);
				}
			}
		}
	}
}

size_t Tracks::get_size()
{
	size_t size = sizeof(*this);

	for(Track* current = first; current; current = NEXT)
		size += current->get_size();
	return size;
}
