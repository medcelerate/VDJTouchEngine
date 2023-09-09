//////////////////////////////////////////////////////////////////////////
//
// VirtualDJ
// Plugin SDK
// (c)Atomix Productions 2011-2013
//
//////////////////////////////////////////////////////////////////////////
//
// This file defines the real-time DSP sound effect plugins.
// In addition to all the elements supported from the base IVdjPlugin class,
// it defines additional DSP-specific functions and variables:
//
//////////////////////////////////////////////////////////////////////////


#ifndef VdjDsp8H
#define VdjDsp8H

#include "vdjPlugin8.h"

//////////////////////////////////////////////////////////////////////////
// DSP plugin class

class IVdjPluginDsp8 : public IVdjPlugin8
{
public:
	// called when the plugin is started or stopped
	virtual HRESULT VDJ_API OnStart() {return 0;}
	virtual HRESULT VDJ_API OnStop() {return 0;}

	// This function will be called each time VirtualDJ needs your plugin
	// to be applied on a new sound buffer
	// NOTE: samples are stereo, so you need to process up to buffer[2*nb]
	virtual HRESULT VDJ_API OnProcessSamples(float *buffer, int nb)=0;

	// Some useful variables
	int SampleRate;			// samplerate of the audio engine
	int SongBpm;			// number of samples between two consecutive beats for this song
	double SongPosBeats;	// number of beats from the first beat in the song
};

//////////////////////////////////////////////////////////////////////////
// Buffer plugin class

class IVdjPluginBufferDsp8 : public IVdjPlugin8
{
public:
	// called when the plugin is started or stopped
	virtual HRESULT VDJ_API OnStart() {return 0;}
	virtual HRESULT VDJ_API OnStop() {return 0;}

	// This function will be called each time VirtualDJ needs your plugin
	// to be applied on a new sound buffer
	// NOTE: samples are stereo, so you need to process up to buffer[2*nb]
	virtual short * VDJ_API OnGetSongBuffer(int songPos, int nb)=0;

	// Call this to get the buffer at the specified position
	HRESULT GetSongBuffer(int pos, int nb, short **buffer) {return cb->GetSongBuffer(pos, nb, buffer);}

	// Some useful variables
	int SampleRate;			// samplerate of the song
	int SongBpm;			// number of samples between two consecutive beats for this song
	int SongPos;			// number of samples from beginning of song
	double SongPosBeats;	// number of beats from the first beat in the song
};

class IVdjPluginPositionDsp8 : public IVdjPlugin8
{
public:
	// called when the plugin is started or stopped
	virtual HRESULT VDJ_API OnStart() { return 0; }
	virtual HRESULT VDJ_API OnStop() { return 0; }

	//When called, songPos can be modified
	virtual HRESULT VDJ_API OnTransformPosition(double *songPos, double *videoPos, float *volume, float *srcVolume) = 0;

	// This function will be called each time VirtualDJ needs your plugin
	// to be applied on a new sound buffer
	// NOTE: samples are stereo, so you need to process up to buffer[2*nb]
	virtual HRESULT VDJ_API OnProcessSamples(float *buffer, int nb) { return 0; }

	// Some useful variables
	int SampleRate;			// samplerate of the audio engine
	int SongBpm;			// number of samples between two consecutive beats for this song
	int SongPos;			// number of samples from beginning of song
	double SongPosBeats;	// number of beats from the first beat in the song
};

//////////////////////////////////////////////////////////////////////////
// GUID definitions

#ifndef VDJDSP8GUID_DEFINED
#define VDJDSP8GUID_DEFINED 
static const GUID IID_IVdjPluginDsp8 = { 0x7cfcf3f5, 0x6fb9, 0x434c, { 0xb6, 0x3, 0xd7, 0x3a, 0x88, 0xf6, 0x72, 0x26 } };
#else
extern static const GUID IID_IVdjPluginDsp8;
#endif

#ifndef VDJBUFFER8GUID_DEFINED
#define VDJBUFFER8GUID_DEFINED 
static const GUID IID_IVdjPluginBuffer8 = { 0x1d00e65f, 0x44c7, 0x41bf, { 0xa3, 0x6b, 0x04, 0xda, 0xf2, 0x67, 0x3b, 0x98 } };
#else
extern static const GUID IID_IVdjPluginBuffer8;
#endif

//////////////////////////////////////////////////////////////////////////

#endif
