/*
	TiMidity -- Experimental MIDI to WAVE converter
	Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef TIMIDITY_H
#define TIMIDITY_H

#include "doomtype.h"
#include "zstring.h"

namespace Timidity
{

/*
config.h
*/

/* Acoustic Grand Piano seems to be the usual default instrument. */
#define DEFAULT_PROGRAM 0

/* 9 here is MIDI channel 10, which is the standard percussion channel.
   Some files (notably C:\WINDOWS\CANYON.MID) think that 16 is one too. 
   On the other hand, some files know that 16 is not a drum channel and
   try to play music on it. This is now a runtime option, so this isn't
   a critical choice anymore. */
#define DEFAULT_DRUMCHANNELS (1<<9)
/*#define DEFAULT_DRUMCHANNELS ((1<<9) | (1<<15))*/

/* Default sampling rate, default polyphony, and maximum polyphony.
   All but the last can be overridden from the command line. */
#define DEFAULT_RATE	32000
#define DEFAULT_VOICES	32
#define MAX_VOICES		256
#define MAXCHAN			16
#define MAXNOTE			128

/* 1000 here will give a control ratio of 22:1 with 22 kHz output.
   Higher CONTROLS_PER_SECOND values allow more accurate rendering
   of envelopes and tremolo. The cost is CPU time. */
#define CONTROLS_PER_SECOND 1000

/* Make envelopes twice as fast. Saves ~20% CPU time (notes decay
   faster) and sounds more like a GUS. There is now a command line
   option to toggle this as well. */
//#define FAST_DECAY

/* How many bits to use for the fractional part of sample positions.
   This affects tonal accuracy. The entire position counter must fit
   in 32 bits, so with FRACTION_BITS equal to 12, the maximum size of
   a sample is 1048576 samples (2 megabytes in memory). The GUS gets
   by with just 9 bits and a little help from its friends...
   "The GUS does not SUCK!!!" -- a happy user :) */
#define FRACTION_BITS 12

/* For some reason the sample volume is always set to maximum in all
   patch files. Define this for a crude adjustment that may help
   equalize instrument volumes. */
#define ADJUST_SAMPLE_VOLUMES

/* The number of samples to use for ramping out a dying note. Affects
   click removal. */
#define MAX_DIE_TIME 20

/**************************************************************************/
/* Anything below this shouldn't need to be changed unless you're porting
   to a new machine with other than 32-bit, big-endian words. */
/**************************************************************************/

/* change FRACTION_BITS above, not these */
#define INTEGER_BITS (32 - FRACTION_BITS)
#define INTEGER_MASK (0xFFFFFFFF << FRACTION_BITS)
#define FRACTION_MASK (~ INTEGER_MASK)
#define MAX_SAMPLE_SIZE (1 << INTEGER_BITS)

/* This is enforced by some computations that must fit in an int */
#define MAX_CONTROL_RATIO 255

#define MAX_AMPLIFICATION 800

/* The TiMiditiy configuration file */
#define CONFIG_FILE	"timidity.cfg"

typedef float sample_t;
typedef float final_volume_t;
#define FINAL_VOLUME(v) (v)

#define FSCALE(a,b) ((a) * (float)(1<<(b)))
#define FSCALENEG(a,b) ((a) * (1.0L / (float)(1<<(b))))

/* Vibrato and tremolo Choices of the Day */
#define SWEEP_TUNING 38
#define VIBRATO_AMPLITUDE_TUNING 1.0L
#define VIBRATO_RATE_TUNING 38
#define TREMOLO_AMPLITUDE_TUNING 1.0L
#define TREMOLO_RATE_TUNING 38

#define SWEEP_SHIFT 16
#define RATE_SHIFT 5

#define VIBRATO_SAMPLE_INCREMENTS 32

#ifndef PI
  #define PI 3.14159265358979323846
#endif

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
// [RH] MinGW's pow() function is terribly slow compared to VC8's
// (I suppose because it's using an old version from MSVCRT.DLL).
// On an Opteron running x86-64 Linux, this also ended up being about
// 100 cycles faster than libm's pow(), which is why I'm using this
// for GCC in general and not just for MinGW.

extern __inline__ double pow_x87_inline(double x,double y)
{
	double result;

	if (y == 0)
	{
		return 1;
	}
	if (x == 0)
	{
		if (y > 0)
		{
			return 0;
		}
		else
		{
			union { double fp; long long ip; } infinity;
			infinity.ip = 0x7FF0000000000000ll;
			return infinity.fp;
		}
	}
	__asm__ (
		"fyl2x\n\t"
		"fld %%st(0)\n\t"
		"frndint\n\t"
		"fxch\n\t"
		"fsub %%st(1),%%st(0)\n\t"
		"f2xm1\n\t"
		"fld1\n\t"
		"faddp\n\t"
		"fxch\n\t"
		"fld1\n\t"
		"fscale\n\t"
		"fstp %%st(1)\n\t"
		"fmulp\n\t"
		: "=t" (result)
		: "0" (x), "u" (y)
		: "st(1)", "st(7)", "%3", "%4" );
	return result;
}
#define pow pow_x87_inline
#endif

/*
common.h
*/

extern FString current_filename;

/* Noise modes for open_file */
#define OF_SILENT	0
#define OF_NORMAL	1
#define OF_VERBOSE	2

extern FILE *open_file(const char *name, int decompress, int noise_mode);
extern void add_to_pathlist(const char *s);
extern void close_file(FILE *fp);
extern void skip(FILE *fp, size_t len);
extern void *safe_malloc(size_t count);

/*
controls.h
*/

#define CMSG_INFO			0
#define CMSG_WARNING		1
#define CMSG_ERROR			2
#define CMSG_FATAL			3
#define CMSG_TRACE			4
#define CMSG_TIME			5
#define CMSG_TOTAL			6
#define CMSG_FILE			7
#define CMSG_TEXT			8

#define VERB_NORMAL			0
#define VERB_VERBOSE		1
#define VERB_NOISY			2
#define VERB_DEBUG			3
#define VERB_DEBUG_SILLY	4

struct ControlMode
{
	virtual ~ControlMode();
	void cmsg(int type, int verbosity_level, const char *fmt, ...);
};


/*
instrum.h
*/

struct Sample
{
	SDWORD
		loop_start, loop_end, data_length,
		sample_rate, low_vel, high_vel, low_freq, high_freq, root_freq;
	SDWORD
		envelope_rate[7], envelope_offset[7];
	float
		modulation_rate[7], modulation_offset[7];
	float
		volume, resonance,
		modEnvToFilterFc, modEnvToPitch, modLfoToFilterFc;
	sample_t *data;
	SDWORD 
		tremolo_sweep_increment, tremolo_phase_increment,
		lfo_sweep_increment, lfo_phase_increment,
		vibrato_sweep_increment, vibrato_control_ratio,
		cutoff_freq;
	BYTE
		reverberation, chorusdepth,
		tremolo_depth, vibrato_depth,
		modes,
		attenuation;
	WORD
		freq_center, panning;
	SBYTE
		note_to_use, exclusiveClass;
	SWORD
		keyToModEnvHold, keyToModEnvDecay,
		keyToVolEnvHold, keyToVolEnvDecay;
	SDWORD
		freq_scale;
};

void convert_sample_data(Sample *sample, const void *data);
void free_instruments();

/* Bits in modes: */
#define MODES_16BIT			(1<<0)
#define MODES_UNSIGNED		(1<<1)
#define MODES_LOOPING		(1<<2)
#define MODES_PINGPONG		(1<<3)
#define MODES_REVERSE		(1<<4)
#define MODES_SUSTAIN		(1<<5)
#define MODES_ENVELOPE		(1<<6)
#define MODES_FAST_RELEASE	(1<<7)

#define INST_GUS			0
#define INST_SF2			1
#define INST_DLS			2

struct Instrument
{
	int type;
	int samples;
	Sample *sample;
	int left_samples;
	Sample *left_sample;
	int right_samples;
	Sample *right_sample;
};

struct InstrumentLayer
{
	BYTE lo, hi;
	Instrument *instrument;
	InstrumentLayer *next;
};

struct cfg_type
{
	int font_code;
	int num;
	const char *name;
};

#define FONT_NORMAL		0
#define FONT_FFF		1
#define FONT_SBK		2
#define FONT_TONESET	3
#define FONT_DRUMSET	4
#define FONT_PRESET		5

struct ToneBankElement
{
	ToneBankElement() : layer(NULL), font_type(0), sf_ix(0), tuning(0),
		note(0), amp(0), pan(0), strip_loop(0), strip_envelope(0), strip_tail(0)
	{}

	FString name;
	InstrumentLayer *layer;
	int font_type, sf_ix, tuning;
	int note, amp, pan, strip_loop, strip_envelope, strip_tail;
};

/* A hack to delay instrument loading until after reading the
entire MIDI file. */
#define MAGIC_LOAD_INSTRUMENT ((InstrumentLayer *)(-1))

#define MAXPROG			128
#define MAXBANK			130
#define SFXBANK			(MAXBANK-1)
#define SFXDRUM1		(MAXBANK-2)
#define SFXDRUM2		(MAXBANK-1)
#define XGDRUM			1

struct ToneBank
{
	FString name;
	ToneBankElement tone[MAXPROG];
};


#define SPECIAL_PROGRAM -1

extern void pcmap(int *b, int *v, int *p, int *drums);

/*
mix.h
*/

extern void mix_voice(struct Renderer *song, float *buf, struct Voice *v, int c);
extern int recompute_envelope(struct Voice *v);
extern void apply_envelope_to_amp(struct Voice *v);

/*
playmidi.h
*/

/* Midi events */
#define ME_NOTEOFF			0x80
#define ME_NOTEON			0x90
#define ME_KEYPRESSURE		0xA0
#define ME_CONTROLCHANGE	0xB0
#define ME_PROGRAM			0xC0
#define ME_CHANNELPRESSURE	0xD0
#define ME_PITCHWHEEL		0xE0

/* Controllers */
#define CTRL_BANK_SELECT		0
#define CTRL_DATA_ENTRY			6
#define CTRL_VOLUME				7
#define CTRL_PAN				10
#define CTRL_EXPRESSION			11
#define CTRL_SUSTAIN			64
#define CTRL_HARMONICCONTENT	71
#define CTRL_RELEASETIME		72
#define CTRL_ATTACKTIME			73
#define CTRL_BRIGHTNESS			74
#define CTRL_REVERBERATION		91
#define CTRL_CHORUSDEPTH		93
#define CTRL_NRPN_LSB			98
#define CTRL_NRPN_MSB			99
#define CTRL_RPN_LSB			100
#define CTRL_RPN_MSB			101
#define CTRL_ALL_SOUNDS_OFF		120
#define CTRL_RESET_CONTROLLERS	121
#define CTRL_ALL_NOTES_OFF		123

/* NRPNs */
#define NRPN_BRIGHTNESS			0x00A0
#define NRPN_HARMONICCONTENT	0x00A1
#define NRPN_DRUMVOLUME			(26<<7)		// drum number in low 7 bits
#define NRPN_DRUMPANPOT			(28<<7)		// "
#define NRPN_DRUMREVERBERATION	(29<<7)		// "
#define NRPN_DRUMCHORUSDEPTH	(30<<7)		// "

/* RPNs */
#define RPN_PITCH_SENS			0x0000
#define RPN_FINE_TUNING			0x0001
#define RPN_COARSE_TUNING		0x0002
#define RPN_RESET				0x3fff

#define SFX_BANKTYPE	64

struct Channel
{
	int
		bank, program, sustain, pitchbend, 
		mono, /* one note only on this channel -- not implemented yet */
		/* new stuff */
		variationbank, reverberation, chorusdepth, harmoniccontent,
		releasetime, attacktime, brightness, kit, sfx,
		/* end new */
		pitchsens;
	WORD
		volume, expression;
	SWORD
		panning;
	WORD
		rpn, nrpn;
	bool
		nrpn_mode;
	char
		transpose;
	float
		pitchfactor; /* precomputed pitch bend factor to save some fdiv's */
};

/* Causes the instrument's default panning to be used. */
#define NO_PANNING -1
/* envelope points */
#define MAXPOINT 7

struct Voice
{
	BYTE
		status, channel, note, velocity, clone_type;
	Sample *sample;
	Sample *left_sample;
	Sample *right_sample;
	int clone_voice;
	float
		orig_frequency, frequency;
	int
		sample_offset, loop_start, loop_end;
	int
		envelope_volume, modulation_volume;
	int
		envelope_target, modulation_target;
	int
		tremolo_sweep, tremolo_sweep_position, tremolo_phase,
		lfo_sweep, lfo_sweep_position, lfo_phase,
		vibrato_sweep, vibrato_sweep_position, vibrato_depth,
		echo_delay_count;
	int
		echo_delay,
		sample_increment,
		envelope_increment,
		modulation_increment,
		tremolo_phase_increment,
		lfo_phase_increment;

	final_volume_t left_mix, right_mix;

	float
		left_amp, right_amp,
		volume, tremolo_volume, lfo_volume;
	int
		vibrato_sample_increment[VIBRATO_SAMPLE_INCREMENTS];
	int
		envelope_rate[MAXPOINT], envelope_offset[MAXPOINT];
	int
		vibrato_phase, vibrato_control_ratio, vibrato_control_counter,
		envelope_stage, modulation_stage, control_counter,
		modulation_delay, modulation_counter, panning, panned;

};

/* Voice status options: */
#define VOICE_FREE 0
#define VOICE_ON 1
#define VOICE_SUSTAINED 2
#define VOICE_OFF 3
#define VOICE_DIE 4

/* Voice panned options: */
#define PANNED_MYSTERY 0
#define PANNED_LEFT 1
#define PANNED_RIGHT 2
#define PANNED_CENTER 3
/* Anything but PANNED_MYSTERY only uses the left volume */

/* Envelope stages: */
#define ATTACK 0
#define HOLD 1
#define DECAY 2
#define RELEASE 3
#define RELEASEB 4
#define RELEASEC 5
#define DELAY 6

#define ISDRUMCHANNEL(c) ((drumchannels & (1<<(c))))

/*
resample.h
*/

extern sample_t *resample_voice(struct Renderer *song, Voice *v, int *countptr);
extern void pre_resample(struct Renderer *song, Sample *sp);

/* 
tables.h
*/

#define sine(x)			(sin((2*PI/1024.0) * (x)))
#define note_to_freq(x)	(float(8175.7989473096690661233836992789 * pow(2.0, (x) / 12.0)))

// Use TiMidity++'s volume equation rather than TiMidity's, since it's louder.
//#define calc_vol(x)	(pow(2.0,((x)*6.0 - 6.0)))
#define calc_vol(x)		(pow((double)(x), (double)1.66096404744))

#define XMAPMAX 800

/*
timidity.h
*/
struct DLS_Data;
extern int LoadConfig();
extern void FreeAll();

extern ToneBank *tonebank[MAXBANK];
extern ToneBank *drumset[MAXBANK];

struct Renderer
{
	ControlMode *ctl;
	float rate;
	DLS_Data *patches;
	InstrumentLayer *default_instrument;
	int default_program;
	bool fast_decay;
	int resample_buffer_size;
	sample_t *resample_buffer;
	Channel channel[16];
	Voice voice[MAX_VOICES];
	signed char drumvolume[MAXCHAN][MAXNOTE];
	signed char drumpanpot[MAXCHAN][MAXNOTE];
	signed char drumreverberation[MAXCHAN][MAXNOTE];
	signed char drumchorusdepth[MAXCHAN][MAXNOTE];
	int control_ratio, amp_with_poly;
	int drumchannels;
	int adjust_panning_immediately;
	int voices;
	int GM_System_On;
	int XG_System_On;
	int GS_System_On;
	int XG_System_reverb_type;
	int XG_System_chorus_type;
	int XG_System_variation_type;
	int lost_notes, cut_notes;

	Renderer(float sample_rate);
	~Renderer();

	void HandleEvent(int status, int parm1, int parm2);
	void HandleLongMessage(const BYTE *data, int len);
	void HandleController(int chan, int ctrl, int val);
	void ComputeOutput(float *buffer, int num_samples);
	void MarkInstrument(int bank, int percussion, int instr);
	void Reset();

	int load_missing_instruments();
	int set_default_instrument(const char *name);
	int convert_tremolo_sweep(BYTE sweep);
	int convert_vibrato_sweep(BYTE sweep, int vib_control_ratio);
	int convert_tremolo_rate(BYTE rate);
	int convert_vibrato_rate(BYTE rate);

	void recompute_amp(Voice *v);
	int vc_alloc(int not_this_voice);
	void kill_others(int voice);
	void clone_voice(Instrument *ip, int v, int note, int vel, int clone_type, int variationbank);
	void xremap(int *banknumpt, int *this_notept, int this_kit);
	void start_note(int chan, int note, int vel, int voice);

	void note_on(int chan, int note, int vel);
	void note_off(int chan, int note, int vel);
	void all_notes_off(int chan);
	void all_sounds_off(int chan);
	void adjust_pressure(int chan, int note, int amount);
	void adjust_panning(int chan);
	void drop_sustain(int chan);
	void adjust_pitchbend(int chan);
	void adjust_volume(int chan);

	void reset_voices();
	void reset_controllers(int chan);
	void reset_midi();

	void select_sample(int voice, Instrument *instr);
	void select_stereo_samples(int voice, InstrumentLayer *layer);
	void recompute_freq(int voice);

	void kill_note(int voice);
	void finish_note(int voice);

	void DataEntryCoarseRPN(int chan, int rpn, int val);
	void DataEntryFineRPN(int chan, int rpn, int val);
	void DataEntryCoarseNRPN(int chan, int nrpn, int val);
	void DataEntryFineNRPN(int chan, int nrpn, int val);
};

}
#endif
