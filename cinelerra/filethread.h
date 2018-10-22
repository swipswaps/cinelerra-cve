
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

#ifndef FILETHREAD_H
#define FILETHREAD_H

#include "aframe.inc"
#include "condition.inc"
#include "datatype.h"
#include "file.inc"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"


// This allows the file hander to write in the background without 
// blocking the write commands.
// Used for recording.

class FileThread : public Thread
{
public:
	FileThread(File *file, int do_audio, int do_video);
	~FileThread();

// ============================== writing section ==============================
// Allocate the buffers and start loop for writing.
	void start_writing(int buffer_size, 
			int color_model, 
			int ring_buffers);
	void stop_writing();

// ================================ reading section ============================
// Allocate buffers and start loop for reading
	void stop_reading();

	int read_frame(VFrame *frame);
	int read_buffer();
	size_t get_memory_usage();

// write data into next available buffer
	int write_buffer(int size);
// get pointer to next buffer to be written and lock it
	AFrame** get_audio_buffer();
// get pointer to next frame to be written and lock it
	VFrame*** get_video_buffer();

	void run();
	int swap_buffer();

	AFrame ***audio_buffer;
// (VFrame*)(VFrame array *)(Track *)[ring buffer]
	VFrame ****video_buffer;
	int *output_size;  // Number of frames or samples to write
// Not used
	int *is_compressed; // Whether to use the compressed data in the frame
	Condition **output_lock, **input_lock;
// Lock access to the file to allow it to be changed without stopping the loop
	Mutex *file_lock;
	int current_buffer;
	int local_buffer;
	int *last_buffer;  // last_buffer[ring buffer]
	int return_value;
	int do_audio;
	int do_video;
	File *file;
	int ring_buffers;
	int buffer_size;    // Frames or samples per ring buffer

// Mode of operation
	int is_reading;
	int is_writing;
	int done;

// For the reading mode, the thread reads continuously from the given
// point until stopped.
// Maximum frames to preload
#define MAX_READ_FRAMES 4
// Total number of frames preloaded
	int total_frames;
// Allocated frames
	VFrame *read_frames[MAX_READ_FRAMES];
// If the seeking pattern isn't optimal for asynchronous reading, this is

// set to 1 to stop reading.
	int disable_read;

// Thread waits on this if the maximum frames have been read.

	Condition *read_wait_lock;
// read_frame waits on this if the thread is running.
	Condition *user_wait_lock;

// Lock access to read_frames
	Mutex *frame_lock;

// Duration of clip already decoded
	ptstime start_pts;
	ptstime end_pts;

// Layer of the frame
	int layer;
};

#endif
