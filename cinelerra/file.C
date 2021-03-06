// SPDX-License-Identifier: GPL-2.0-or-later

// This file is a part of Cinelerra-CVE
// Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>

#include "asset.h"
#include "aframe.h"
#include "bcsignals.h"
#include "bcresources.h"
#include "byteorder.h"
#include "cache.inc"
#include "condition.h"
#include "edit.h"
#include "file.h"
#include "fileavlibs.h"
#include "filebase.h"
#include "filexml.h"
#include "filejpeg.h"
#include "filepng.h"
#include "filetga.h"
#include "filethread.h"
#include "filetiff.h"
#include "formattools.h"
#include "framecache.h"
#include "language.h"
#include "mainerror.h"
#include "mutex.h"
#include "pluginserver.h"
#include "tmpframecache.h"
#include "vframe.h"

File::File()
{
	cpus = 1;
	asset = 0;
	format_completion = new Mutex("File::format_completion");
	write_lock = new Condition(1, "File::write_lock");
	reset_parameters();
}

File::~File()
{
	if(getting_options)
	{
		if(format_window) format_window->set_done(0);
		format_completion->lock("File::~File");
		format_completion->unlock();
	}

	if(temp_frame) delete temp_frame;

	close_file(0);
	delete format_completion;
	delete write_lock;
	BC_Resources::tmpframes.release_frame(last_frame);
}

void File::reset_parameters()
{
	file = 0;
	writing = 0;
	audio_thread = 0;
	video_thread = 0;
	getting_options = 0;
	format_window = 0;
	temp_frame = 0;
	last_frame = 0;
}

void File::raise_window()
{
	if(getting_options && format_window)
	{
		format_window->raise_window();
		format_window->flush();
	}
}

void File::close_window()
{
	if(getting_options)
	{
		format_window->set_done(1);
		getting_options = 0;
	}
}

void File::get_options(FormatTools *format, int options)
{
	BC_WindowBase *parent_window = format->window;
	Asset *asset = format->asset;

	getting_options = 1;
	format_completion->lock("File::get_options");
	switch(asset->format)
	{
		case FILE_AC3:
		case FILE_AVI:
		case FILE_MOV:
		case FILE_OGG:
		case FILE_WAV:
		case FILE_MP3:
		case FILE_AIFF:
		case FILE_AU:
		case FILE_PCM:
		case FILE_FLAC:
		case FILE_MPEG:
		case FILE_MPEGTS:
		case FILE_RAWDV:
		case FILE_YUV:
		case FILE_MXF:
		case FILE_MKV:
		case FILE_3GP:
		case FILE_MP4:
		case FILE_PSP:
		case FILE_3GPP2:
		case FILE_IPOD:
		case FILE_ISMV:
		case FILE_F4V:
		case FILE_WEBM:
		case FILE_EXR:
			FileAVlibs::get_parameters(parent_window,
				asset,
				format_window,
				options);
			break;
		case FILE_JPEG:
		case FILE_JPEG_LIST:
			FileJPEG::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
		case FILE_PNG:
		case FILE_PNG_LIST:
			FilePNG::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
		case FILE_TGA:
		case FILE_TGA_LIST:
			FileTGA::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;
		case FILE_TIFF:
		case FILE_TIFF_LIST:
			FileTIFF::get_parameters(parent_window, 
				asset, 
				format_window, 
				options);
			break;

		default:
			break;
	}

	if(!format_window)
	{
		const char *s;

		if(options & SUPPORTS_AUDIO)
			s = _("audio");
		else if(options & SUPPORTS_VIDEO)
			s = _("video");
		else
			s = "";
		errorbox(_("No suitable %s codec found."), s);
	}

	getting_options = 0;
	format_window = 0;
	format_completion->unlock();
}

void File::set_processors(int cpus)   // Set the number of cpus for certain codecs
{
	this->cpus = cpus;
}

int File::is_imagelist(int format)
{
	switch(format)
	{
	case FILE_JPEG_LIST:
	case FILE_TGA_LIST:
	case FILE_TIFF_LIST:
	case FILE_PNG_LIST:
		return 1;
	}
	return 0;
}

int File::open_file(Asset *asset, int open_method)
{
	int probe_result, rs;
	int rd, wr;

	rd = open_method & FILE_OPEN_READ;
	wr = open_method & FILE_OPEN_WRITE;

	this->asset = asset;
	file = 0;

	probe_result = 0;
	if(rd && !is_imagelist(asset->format))
	{
		probe_result = FileAVlibs::probe_input(asset);

		if(probe_result < 0)
			return FILE_NOT_FOUND;
// Probe input fills decoder parameters what
// are not used with some formats
		switch(asset->format)
		{
		case FILE_JPEG:
		case FILE_PNG:
		case FILE_TGA:
		case FILE_TIFF:
		case FILE_SND:
			asset->delete_decoder_parameters();
			break;
		}
	}

	switch(asset->format)
	{
// get the format now
// If you add another format to case 0, you also need to add another case for the
// file format #define.
	case FILE_UNKNOWN:
		FILE *stream;
		if(!(stream = fopen(asset->path, "rb")))
		{
			return FILE_NOT_FOUND;
		}

		char test[16];
		rs = fread(test, 16, 1, stream) != 1;
		fclose(stream);

		if(rs)
			return FILE_NOT_FOUND;

		if(strncmp(test, "TOC ", 4) == 0)
		{
			errormsg(_("Can't open TOC files directly"));
			return FILE_NOT_FOUND;
		}
		if(FilePNG::check_sig(asset))
// PNG list
			file = new FilePNG(asset, this);
		else
		if(FileJPEG::check_sig(asset))
// JPEG list
			file = new FileJPEG(asset, this);
		else
		if(FileTGA::check_sig(asset))
// TGA list
			file = new FileTGA(asset, this);
		else
		if(FileTIFF::check_sig(asset))
// TIFF list
			file = new FileTIFF(asset, this);
		else
		if(test[0] == '<' && test[1] == 'E' && test[2] == 'D' && test[3] == 'L' && test[4] == '>' ||
			test[0] == '<' && test[1] == 'H' && test[2] == 'T' && test[3] == 'A' && test[4] == 'L' && test[5] == '>' ||
			test[0] == '<' && test[1] == '?' && test[2] == 'x' && test[3] == 'm' && test[4] == 'l')
// XML file
			return FILE_IS_XML;
		else
			return FILE_UNRECOGNIZED_CODEC;
		break;

// format already determined
	case FILE_PNG:
	case FILE_PNG_LIST:
		file = new FilePNG(asset, this);
		break;

	case FILE_JPEG:
	case FILE_JPEG_LIST:
		file = new FileJPEG(asset, this);
		break;
	case FILE_TGA_LIST:
	case FILE_TGA:
		file = new FileTGA(asset, this);
		break;

	case FILE_TIFF:
	case FILE_TIFF_LIST:
		file = new FileTIFF(asset, this);
		break;

	case FILE_SVG:
	case FILE_AC3:
	case FILE_AVI:
	case FILE_MOV:
	case FILE_OGG:
	case FILE_MP3:
	case FILE_WAV:
	case FILE_AIFF:
	case FILE_AU:
	case FILE_PCM:
	case FILE_FLAC:
	case FILE_MPEGTS:
	case FILE_MPEG:
	case FILE_RAWDV:
	case FILE_YUV:
	case FILE_MXF:
	case FILE_MKV:
	case FILE_3GP:
	case FILE_MP4:
	case FILE_PSP:
	case FILE_3GPP2:
	case FILE_IPOD:
	case FILE_ISMV:
	case FILE_F4V:
	case FILE_WEBM:
	case FILE_EXR:
		file = new FileAVlibs(asset, this);
		break;

	default:
		errormsg("No suitable codec for format '%s'",
			ContainerSelection::container_to_text(asset->format));
		return 1;
	}

// Reopen file with correct parser and get header.
	if(file->open_file(open_method))
	{
		delete file;
		file = 0;
	}

// Set extra writing parameters to mandatory settings.
	if(file && wr)
	{
		if(asset->dither) file->set_dither();
		writing = 1;
	}

	if(file)
		return FILE_OK;
	else
		return FILE_NOT_FOUND;
}

void File::close_file(int ignore_thread)
{
	if(!ignore_thread)
	{
		stop_audio_thread();
		stop_video_thread();
	}

	if(file) 
	{
		file->close_file();
		delete file;
	}
	reset_parameters();
}


int File::get_index(const char *index_path)
{
	if(file)
	{
		return file->get_index(index_path);
	}
	return 1;
}

void File::start_audio_thread(int buffer_size, int ring_buffers)
{
	if(!audio_thread)
	{
		audio_thread = new FileThread(this, SUPPORTS_AUDIO);
		audio_thread->start_writing(buffer_size, 0, ring_buffers);
	}
}

void File::start_video_thread(int buffer_size, 
	int color_model, 
	int ring_buffers)
{
	if(!video_thread)
	{
		video_thread = new FileThread(this, SUPPORTS_VIDEO);
		video_thread->start_writing(buffer_size, 
			color_model, 
			ring_buffers);
	}
}

void File::stop_audio_thread()
{
	if(audio_thread)
	{
		audio_thread->stop_writing();
		delete audio_thread;
		audio_thread = 0;
	}
}

void File::stop_video_thread()
{
	if(video_thread)
	{
		video_thread->stop_writing();
		delete video_thread;
		video_thread = 0;
	}
}

samplenum File::get_audio_length()
{
	return asset->audio_length;
}

// No resampling here.
int File::write_aframes(AFrame **frames)
{
	int result;

	if(file)
	{
		write_lock->lock("File::write_aframes");
		for(int i = 0; i < asset->channels; i++)
		{
			if(frames[i])
			{
				asset->audio_length += frames[i]->get_length();
				break;
			}
		}
		result = file->write_aframes(frames);
		write_lock->unlock();
		return result;
	}
	return 1;
}

// Can't put any cmodel abstraction here because the filebase couldn't be
// parallel.
int File::write_frames(VFrame ***frames, int len)
{
// Store the counters in temps so the filebase can choose to overwrite them.
	int result;

	int video_length_temp = asset->video_length;

	write_lock->lock("File::write_frames");

	result = file->write_frames(frames, len);

	asset->video_length = video_length_temp + len;
	write_lock->unlock();

	return result;
}

int File::write_audio_buffer(int len)
{
	int result = 0;
	if(audio_thread)
	{
		result = audio_thread->write_buffer(len);
	}
	return result;
}

int File::write_video_buffer(int len)
{
	int result = 0;
	if(video_thread)
	{
		result = video_thread->write_buffer(len);
	}

	return result;
}

AFrame** File::get_audio_buffer()
{
	if(audio_thread) return audio_thread->get_audio_buffer();
	return 0;
}

VFrame*** File::get_video_buffer()
{
	if(video_thread) return video_thread->get_video_buffer();
	return 0;
}

int File::get_samples(AFrame *aframe)
{
	if(!file)
		return 0;

	if(aframe->get_samplerate() == 0)
		aframe->set_samplerate(asset->sample_rate);
	if(aframe->get_buffer_length() <= 0)
		return 0;
	if(aframe->get_source_duration() <= 0)
		return 0;

	aframe->position = round(aframe->get_source_pts() * aframe->get_samplerate());
	if(aframe->get_length() + aframe->get_source_length() > aframe->get_buffer_length())
		aframe->set_source_length(aframe->get_buffer_length() - aframe->get_length());
	if(aframe->get_source_length() <= 0)
		return 0;
// Never try to read more samples than exist in the file
	if(aframe->get_source_pts() + aframe->get_source_duration() > asset->audio_duration)
		aframe->set_source_duration(asset->audio_duration - aframe->get_source_pts());

	return file->read_aframe(aframe);
}

int File::get_frame(VFrame *frame)
{
	ptstime rqpts, srcrqpts;

	if(file)
	{
		int supported_colormodel = colormodel_supported(frame->get_color_model());
		srcrqpts = frame->get_source_pts();
		rqpts = frame->get_pts();

		if(asset->single_image)
			frame->set_source_pts(0);
// Test cache
		if(last_frame && last_frame->pts_in_frame_source(srcrqpts, FRAME_ACCURACY) &&
			last_frame->next_source_pts() - srcrqpts > FRAME_ACCURACY)
		{
			if(frame->equivalent(last_frame))
			{
				frame->copy_from(last_frame);
				frame->copy_pts(last_frame);
				frame->set_pts(rqpts);
				adjust_times(frame, rqpts, srcrqpts);
				return 0;
			}
			BC_Resources::tmpframes.release_frame(last_frame);
			last_frame = 0;
		}
// Need temp
		if(frame->get_color_model() != BC_COMPRESSED &&
			!file->converts_frame() &&
			(supported_colormodel != frame->get_color_model() ||
			frame->get_w() != asset->width ||
			frame->get_h() != asset->height))
		{
			if(temp_frame)
			{
				if(!temp_frame->params_match(asset->width, asset->height, supported_colormodel))
				{
					delete temp_frame;
					temp_frame = 0;
				}
			}

			if(!temp_frame)
			{
				temp_frame = new VFrame(0,
					asset->width,
					asset->height,
					supported_colormodel);
			}
			temp_frame->copy_pts(frame);
			file->read_frame(temp_frame);
			frame->transfer_from(temp_frame);
			frame->copy_pts(temp_frame);
		}
		else
			file->read_frame(frame);

		if(asset->single_image)
		{
			frame->set_source_pts(0);
			frame->set_duration(asset->video_duration);
			frame->set_frame_number(0);
		}

		if(last_frame && !last_frame->equivalent(frame))
		{
			BC_Resources::tmpframes.release_frame(last_frame);
			last_frame = 0;
		}
		if(!last_frame)
		{
			last_frame = BC_Resources::tmpframes.get_tmpframe(
				frame->get_w(), frame->get_h(),
				frame->get_color_model());
		}
		last_frame->copy_from(frame);
		last_frame->copy_pts(frame);
		adjust_times(frame, rqpts, srcrqpts);
		return 0;
	}
	else
		return 1;
}

void File::adjust_times(VFrame *frame, ptstime pts, ptstime src_pts)
{
	frame->set_pts(pts);
	if(frame->get_source_pts() < src_pts)
		frame->set_duration(frame->get_duration() -
			(src_pts - frame->get_source_pts()));
}

int File::supports(int format)
{
	switch(format)
	{
	case FILE_JPEG:
	case FILE_PNG:
	case FILE_TIFF:
	case FILE_TGA:
		return SUPPORTS_VIDEO | SUPPORTS_STILL;

	case FILE_JPEG_LIST:
	case FILE_PNG_LIST:
	case FILE_TIFF_LIST:
	case FILE_TGA_LIST:
		return SUPPORTS_VIDEO;

	case FILE_SND:
		return SUPPORTS_AUDIO;

	case FILE_YUV:
	case FILE_AC3:
	case FILE_AVI:
	case FILE_MOV:
	case FILE_OGG:
	case FILE_MP3:
	case FILE_WAV:
	case FILE_AIFF:
	case FILE_AU:
	case FILE_PCM:
	case FILE_FLAC:
	case FILE_MPEG:
	case FILE_MPEGTS:
	case FILE_RAWDV:
	case FILE_MXF:
	case FILE_MKV:
	case FILE_3GP:
	case FILE_MP4:
	case FILE_PSP:
	case FILE_3GPP2:
	case FILE_IPOD:
	case FILE_ISMV:
	case FILE_F4V:
	case FILE_WEBM:
	case FILE_EXR:
		return FileAVlibs::supports(format, 0);
	}
	return 0;
}

int File::colormodel_supported(int colormodel)
{
	if(file)
		return file->colormodel_supported(colormodel);

	return BC_RGB888;
}

size_t File::get_memory_usage() 
{
	size_t result = 0;
	if(temp_frame) result += temp_frame->get_data_size();
	if(file) result += file->get_memory_usage();
	if(last_frame) result += last_frame->get_data_size();
	if(result < MIN_CACHEITEM_SIZE) result = MIN_CACHEITEM_SIZE;
	return result;
}
