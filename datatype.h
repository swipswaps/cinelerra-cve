
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

#ifndef DATATYPE_H
#define DATATYPE_H

#include <sys/types.h>
#include <math.h>
// integer media positions
// number of frame
typedef int framenum;
// number of sample
typedef int64_t samplenum;
// variable that can hold either frame or sample number
typedef int64_t posnum;
// timestamp (pts)
typedef double ptstime;

#define pts2str(buf, pts) sprintf((buf), " %16.10e", (pts))
#define str2pts(buf, ptr) strtod((buf), (ptr))

#define EPSILON (2e-5)

#define PTSEQU(x, y) (fabs((x) - (y)) < EPSILON)

#define TRACK_AUDIO 0
#define TRACK_VIDEO 1
#define TRACK_VTRANSITION 2
#define TRACK_ATRANSITION 3

#endif
