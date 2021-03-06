
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

#ifndef PANAUTO_H
#define PANAUTO_H

#include "auto.h"
#include "cinelerra.h"
#include "edl.inc"
#include "filexml.inc"
#include "panautos.inc"

class PanAuto : public Auto
{
public:
	PanAuto(EDL *edl, PanAutos *autos);

	int operator==(Auto &that);
	void load(FileXML *file);
	void save_xml(FileXML *file);
	void copy(Auto *that, ptstime start, ptstime end);
	void copy_from(Auto *that);
	size_t get_size();
	void dump(int indent = 0);
	void rechannel();

	double values[MAXCHANNELS];
	int handle_x;
	int handle_y;
};

#endif
