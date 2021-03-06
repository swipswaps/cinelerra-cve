
/*
 * CINELERRA
 * Copyright (C) 2019 Einar Rünkaru <einarrunkaru@gmail dot com>
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

#ifndef CLIPLIST_H
#define CLIPLIST_H

#include "arraylist.h"
#include "edl.inc"
#include "filexml.inc"
#include "cliplist.inc"


class ClipList : public ArrayList<EDL*>
{
public:
	ClipList();
	~ClipList();

	EDL *add_clip(EDL *new_clip);
	void remove_clip(EDL *clip);
	void remove_from_project(ArrayList<EDL*> *clips);
	void save_xml(FileXML *file, const char *output_path);

	void dump(int indent);
};

#endif
