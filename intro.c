#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <mmsystem.h>
#include <GL/gl.h>
#include <stdio.h>

// relevant glext.h fragment
#define APIENTRYP __stdcall *
typedef char GLchar;
#define GL_FRAGMENT_SHADER                0x8B30
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLUNIFORM1FVPROC) (GLint location, GLsizei count, const GLfloat *value);
// end of glext.h fragment

#define GL_EXT_FUNCTION_COUNT 8

static const char *glExtFunctionNames[] = {
    "glAttachShader",
    "glCompileShader",
    "glCreateProgram",
    "glCreateShader",
    "glLinkProgram",
    "glShaderSource",
    "glUseProgram",
    "glUniform1fv"
};

static void *glExtFunctions[GL_EXT_FUNCTION_COUNT] = { 0 };

#define glAttachShader ((PFNGLATTACHSHADERPROC)glExtFunctions[0])
#define glCompileShader ((PFNGLCOMPILESHADERPROC)glExtFunctions[1])
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)glExtFunctions[2])
#define glCreateShader ((PFNGLCREATESHADERPROC)glExtFunctions[3])
#define glLinkProgram ((PFNGLLINKPROGRAMPROC)glExtFunctions[4])
#define glShaderSource ((PFNGLSHADERSOURCEPROC)glExtFunctions[5])
#define glUseProgram ((PFNGLUSEPROGRAMPROC)glExtFunctions[6])
#define glUniform1fv ((PFNGLUNIFORM1FVPROC)glExtFunctions[7])

static PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0,
    0,
    0,
    0,
    0,
    0,
    8,
    0,
    0,
    0,
    0,
    0,
    0,
    32,
    0,
    0,
    PFD_MAIN_PLANE,
    0,
    0,
    0,
    0
};

//#define CAPTURE_FRAMES

#define UNIFORM_COUNT 6

#include "shader.h"

#define TRACKER_PERIOD 4725 // 140 bpm (44100 * 60 / 140 / 4)
#define TRACKER_PATTERN_LENGTH 16 // 16 periods (16th) per pattern
#define TRACKER_SONG_LENGTH 48 // in patterns
#define AUDIO_SAMPLES (TRACKER_PERIOD * TRACKER_PATTERN_LENGTH * TRACKER_SONG_LENGTH * 2)

#define AUDIO_DEBUG

static const unsigned int riffHeader[11] = {
    0x46464952, /* RIFF */
    AUDIO_SAMPLES * 2 + 36,
    0x45564157, /* WAVE */
    
    /* fmt chunk */
    0x20746D66,
    16,
    0x00020001,
    44100,
    176400,
    0x00100004,
    
    /* data chunk */
    0x61746164,
    AUDIO_SAMPLES * 2
};

#define DELAY_LEFT (TRACKER_PERIOD * 2 * 2)
#define DELAY_RIGHT (TRACKER_PERIOD * 3 * 2)

short audioBuffer[AUDIO_SAMPLES + 22];
short auxBuffer[AUDIO_SAMPLES + DELAY_LEFT + DELAY_RIGHT];

float TAU = 2.0f * 3.14159265f;

void __declspec(naked) _CIpow()
{
	_asm
	{
		fxch st(1)
		fyl2x
		fld st(0)
		frndint
		fsubr st(1),st(0)
		fxch st(1)
		fchs
		f2xm1
		fld1
		faddp st(1),st(0)
		fscale
		fstp st(1)
		ret
	}
}

static float expf(float value)
{
	return pow(2.71828f, value);
}

// returns fractional part but from -0.5f to 0.5f
static float sfract(float f)
{
	return f - (float)(int)f;
}

static float rand(float f)
{
	f = sin(f * 12.9898) * 43758.5453;
	return sfract(f) * 2.0f;
}

typedef float (*Instrument)(float t, float phase);

static float silence(float t, float phase)
{
    return 0.0f;
}

static float kick(float t, float phase)
{
	float out = expf(-t * 0.001f) * sin(phase * expf(-t * 0.0002f));
	out += expf(-t * 0.003f) * rand(phase);
	
    return out;
}

static float saw(float t, float phase)
{
	return sfract(phase / TAU) * 2.0f * 0.1f;
}

static float lead(float t, float phase)
{
	/*float detune = 1.01f;
	float out = sin(phase) + 0.5f * sin(phase * 2.0) + 0.33f * sin(phase * 3.0)
	          + sin(phase * detune) + 0.5f * sin(phase * 2.0 * detune) + 0.33f * sin(phase * 3.0 * detune);*/
	
	//float out = sin(phase * sin(t / 0.0000001f));
	//float out = saw(t, phase + sin(phase * 1.12f) * t * 0.00001f);
	
	/*float detune = 1.01f - t * 0.0000002f;
	float out = sin(phase + sin(phase * 1.12f) * t * 0.000001f) + sin(phase * detune + sin(phase * detune * 0.87f) * t * 0.000003f);*/
	
	float adsr = expf(-(float)t * 0.00004f);
	float out = saw(t, phase) + saw(t, phase * 1.02f) * t * 0.00004f;
	out += sin(phase);
	
    return adsr * out * 0.7f;
}

static float snare(float t, float phase)
{
	float adsr = expf(-t * 0.0004f);
	float out = sin(phase + sin(phase * 1.01f)) * 0.9f;
	out += rand(phase) * 0.6f;
	
    return adsr * out * 0.25f;
}

#define SAW2_VOLUME_DIVIDER 10.0f
static float saw2(float t, float phase)
{
	float adsr = expf(-(float)t * 0.001f);
	return adsr * sfract(phase / TAU) * 2.0f / SAW2_VOLUME_DIVIDER;
}

static float square(float t, float phase)
{
	float adsr = expf(-(float)t * 0.0004f);
	float out = sfract(phase / TAU) >= 0.0f;
	
	return adsr * out * 0.1f;
}

static float bass(float t, float phase)
{
	return square(t, phase) + square(t, phase * 1.02f);
}

static float pad(float t, float phase)
{
	float adsr = expf(-t * 0.0002f);
	return adsr * sin(phase + sin(phase * 2.0f)) * 0.25f;
}

#define SINE_VOLUME_DIVIDER 6.0f
static float sine(float t, float phase)
{
	float adsr = expf(-(float)t * 0.001f);
	float out = adsr * sin(phase);// + 0.2f * sin(phase * 1.1f);
    return out / SINE_VOLUME_DIVIDER;
}

static float reese(float t, float phase)
{
	float adsr = expf(-(float)t * 0.0004f);
	float detune = 1.01f;
	float out = sin(phase) + 0.5f * sin(phase * 2.0) + 0.33f * sin(phase * 3.0)
	          + sin(phase * detune) + 0.5f * sin(phase * 2.0 * detune) + 0.33f * sin(phase * 3.0 * detune);
	
    return adsr * 0.5f * out;
}

static float noise(float t, float phase)
{
	float adsr = expf(-phase * 0.006f);
	float out = adsr * rand(phase);
	
    return out * 0.2f;
}

Instrument instruments[] = {
    /* 0 */ silence,
    /* 1 */ kick,
    /* 2 */ reese,
    /* 3 */ lead,
    /* 4 */ snare,
    /* 5 */ pad,
    /* 6 */ noise,
    /* 7 */ saw,
};

#define CHANNELS 8

typedef struct
{
    unsigned int frame;
    unsigned int period;
    unsigned char instrument;
} ChannelState;

ChannelState channels[CHANNELS];

// effect bitfield

// 0x8: left output
// 0x4: right output
// 0x2: stereo delay
// 0x1: pitch fall

unsigned short patterns[][TRACKER_PATTERN_LENGTH * 2] = {
    // note, effects (4 bits) + instrument (4 bits)
    {
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 1 - note off
	{
        NOTE(0), 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 2 - kick
    {
        NOTE(25), 0xc1,
        0, 0,
        0, 0,
        0, 0,
        NOTE(25), 0xc1,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 3 - bass
    {
        NOTE(13), 0xc2,
        0, 0,
        0, 0,
        0, 0,
        NOTE(13), 0xc2,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 4 - lead
    {
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(49), 0xe3,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 5 - snare
    {
        NOTE(25), 0xc4,
        0, 0,
        NOTE(25), 0xc4,
        0, 0,
        NOTE(28), 0xc4,
        0, 0,
        NOTE(25), 0xc4,
        0, 0,
        NOTE(25), 0xc4,
        0, 0,
        NOTE(28), 0xc4,
        0, 0,
        NOTE(25), 0xc4,
        0, 0,
        NOTE(25), 0xc4,
        0, 0
    },
	
	// 6 - bass
    {
        NOTE(16), 0xc2,
        0, 0,
        0, 0,
        0, 0,
        NOTE(13), 0xc2,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(11), 0xc2,
        0, 0,
        NOTE(13), 0xc2,
        0, 0
    },
	
	// 7 - chord
    {
        NOTE(37), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 8 - chord
    {
        NOTE(42), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },

	// 9 - chord 2
    {
        NOTE(37), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(35), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 10 - chord 2
    {
        NOTE(42), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(40), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 11 - chord 3
    {
        NOTE(37), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(40), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 12 - chord 3
    {
        NOTE(42), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(44), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 13 - chord 4
    {
        NOTE(43), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(42), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 14 - chord 4
    {
        NOTE(48), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(47), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 15 - break
    {
        NOTE(25), 0xc1,
        0, 0,
        NOTE(25), 0xc1,
        0, 0,
        NOTE(37), 0xc4,
        0, 0,
        NOTE(49), 0xc4,
        NOTE(48), 0xc4,
        NOTE(25), 0xc1,
        NOTE(49), 0xc4,
        NOTE(25), 0xc1,
        0, 0,
        NOTE(37), 0xc4,
        0, 0,
        NOTE(49), 0xc4,
        NOTE(49), 0xc4
    },
	
	// 16 - break bass
    {
        NOTE(13), 0xc2,
        NOTE(16), 0xc2,
        NOTE(13), 0xc2,
        0, 0,
        NOTE(13), 0xc2,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(11), 0xc2,
        0, 0,
        NOTE(13), 0xc2,
        0, 0
    },
	
	// 17 - break pad
    {
        NOTE(37), 0xe5,
        0, 0,
        NOTE(37), 0xe5,
        0, 0,
        NOTE(40), 0xe5,
        0, 0,
        NOTE(37), 0xe5,
        0, 0,
        NOTE(40), 0xe5,
        0, 0,
        NOTE(37), 0xe5,
        0, 0,
        NOTE(42), 0xe5,
        0, 0,
        NOTE(44), 0xe5,
        0, 0
    },
	
	// 18 - break pad
    {
        NOTE(49), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        NOTE(44), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        NOTE(47), 0xe5,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 19 - kick ramp up
    {
        NOTE(25), 0xc1,
        0, 0,
        0, 0,
        0, 0,
        NOTE(25), 0xc1,
        0, 0,
        0, 0,
        0, 0,
        NOTE(25), 0xc1,
        0, 0,
        0, 0,
        0, 0,
        NOTE(25), 0xc1,
        0, 0,
        NOTE(25), 0xc1,
        0, 0,
    },
	
	// 20 - snare ramp up
    {
        NOTE(25), 0xc4,
        0, 0,
        NOTE(25), 0xc4,
        0, 0,
        NOTE(28), 0xc4,
        0, 0,
        NOTE(25), 0xc4,
        0, 0,
        NOTE(25), 0xc4,
        NOTE(25), 0xc4,
        NOTE(25), 0xc4,
        NOTE(25), 0xc4,
        NOTE(25), 0xc4,
        NOTE(25), 0xc4,
        NOTE(25), 0xc4,
        NOTE(25), 0xc4,
    },
	
	// 21 - noise FX
	{
        NOTE(30), 0xe6,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
    },
	
	// 22 - saw saw saw
    {
        NOTE(13), 0xe7,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 23 - saw saw saw 2
    {
        NOTE(16), 0xe7,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
	
	// 24 - saw saw saw 3
    {
        NOTE(15), 0xe7,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        NOTE(14), 0xe7,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0
    },
};

unsigned char song[TRACKER_SONG_LENGTH][CHANNELS] = {
    /*{ 22, 0, 0, 0, 0, 0, 0, 0 },
    { 22, 0, 0, 0, 0, 0, 0, 0 },
    { 22, 0, 0, 0, 0, 0, 0, 0 },
    { 22, 0, 0, 0, 0, 0, 0, 0 },*/
	
	// 0 - pre-intro (32)
    { 0, 0, 4, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 4, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 4, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 0, 0, 0, 0 },
    { 0, 0, 4, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 20, 21, 0, 0, 0 },
	
	// 32 - intro (32)
    { 2, 3, 4, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 0, 0, 0, 0 },
    { 2, 3, 4, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 0, 0, 0, 0 },
    { 2, 3, 4, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 0, 0, 0, 0 },
    { 2, 3, 4, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 21, 0, 0, 0 },
	
	// 64 - bass (32)
    { 2, 6, 4, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 7, 8, 0, 0 },
    { 2, 6, 0, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 9, 10, 0, 0 },
    { 2, 6, 4, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 7, 8, 0, 0 },
    { 2, 6, 0, 5, 0, 0, 0, 0 },
    { 0, 0, 0, 5, 9, 10, 21, 0 },
	
	// 96 - weird (32)
    { 2, 6, 4, 5, 11, 12, 0, 0 },
    { 0, 0, 0, 5, 13, 14, 0, 0 },
    { 2, 6, 0, 5, 11, 12, 0, 0 },
    { 0, 0, 0, 5, 13, 14, 0, 0 },
    { 2, 6, 4, 5, 11, 12, 0, 0 },
    { 0, 0, 0, 5, 13, 14, 0, 0 },
    { 2, 6, 0, 5, 11, 12, 0, 0 },
    { 19, 0, 0, 20, 13, 14, 21, 0 },
	
	// 128 - break (64)
    { 15, 16, 17, 4, 22, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 0, 0 },
    { 15, 16, 17, 4, 23, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 0, 0 },
    { 15, 16, 17, 0, 24, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 21, 0 },
	
    { 15, 16, 17, 4, 22, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 0, 0 },
    { 15, 16, 17, 4, 23, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 0, 0 },
    { 15, 16, 17, 0, 24, 0, 0, 0 },
    { 15, 16, 17, 0, 0, 0, 21, 0 },
	
	// 192
};

static __forceinline void renderAudio()
{
    int i, j, k, l, delay;
    short *buffer = audioBuffer + 22;
    short *aux = auxBuffer + DELAY_LEFT + DELAY_RIGHT;
    char *songPosition;
    
    for (songPosition = (char *)song; songPosition != (char *)song + TRACKER_SONG_LENGTH * CHANNELS; songPosition += CHANNELS)
    {
        for (k = 0; k < TRACKER_PATTERN_LENGTH; k++)
        {
            for (j = 0; j < CHANNELS; j++)
            {
                short *periodBuffer = buffer;
                short *auxPeriodBuffer = aux;
                
                unsigned short *pattern = patterns[songPosition[j]];
                unsigned short period = pattern[k * 2 + 0];
                
                if (period)
                {
                    channels[j].frame = 0;
                    channels[j].period = period;
                    channels[j].instrument = pattern[k * 2 + 1];
                }
                
                for (l = 0; l < TRACKER_PERIOD; l++)
                {
					float t = (float)channels[j].frame;
					float phase = TAU * t / (float)channels[j].period;
					float sf = instruments[channels[j].instrument & 0x0f](t, phase) * 0.3f;
                    short s = (short)(sf * 32767.0f);
                    short l = (channels[j].instrument & 0x80) ? s : 0;
                    short r = (channels[j].instrument & 0x40) ? s : 0;
                    
                    *periodBuffer++ += l;
                    *periodBuffer++ += r;
                    
                    *auxPeriodBuffer++ += (channels[j].instrument & 0x20) ? l : 0;
                    *auxPeriodBuffer++ += (channels[j].instrument & 0x20) ? r : 0;
                    
                    if (!(channels[j].frame % 1000) && (channels[j].instrument & 0x10))
                        channels[j].period++;
                    
                    channels[j].frame++;
                }
            }
            
            buffer += TRACKER_PERIOD * 2;
            aux += TRACKER_PERIOD * 2;
        }
    }
    
    delay = DELAY_LEFT;
    for (buffer = audioBuffer + 22, aux = auxBuffer + DELAY_LEFT + DELAY_RIGHT; aux != auxBuffer + AUDIO_SAMPLES + DELAY_LEFT + DELAY_RIGHT;)
    {
        short s = aux[-delay] * 3 / 4;
        *aux += s;
        
        delay ^= DELAY_LEFT;
        delay ^= DELAY_RIGHT;
        
        *buffer++ += *aux++;
    }
}

#ifdef CAPTURE_FRAMES
char *frameFilename(int n)
{
    static char *name = "frame00000.raw";
    char *ptr = name + 9;
    
    while (n > 0)
    {
        *ptr-- = (n - (n / 10) * 10) + '0';
        n /= 10;
    }
    
    return name;
}
#endif

void entry()
{
    HWND hwnd;
    HDC hdc;
    unsigned int i;
    GLint program;
    GLint fragmentShader;
    DWORD startTime;
	DWORD width;
	DWORD height;
    float u[UNIFORM_COUNT];
    
#ifdef AUDIO_DEBUG
    HANDLE audioFile;
    DWORD bytesWritten;
#endif
    
#ifdef CAPTURE_FRAMES
    int frameNumber;
    HANDLE frameFile;
    DWORD frameBytesWritten;
    char *frameBuffer;
#endif

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);
	
#ifdef CAPTURE_FRAMES
    frameNumber = 0;
    frameBuffer = HeapAlloc(GetProcessHeap(), 0, width * height * 3 /* RGB8 */);
#endif

    hwnd = CreateWindow("static", NULL, WS_POPUP | WS_VISIBLE, 0, 0, width, height, NULL, NULL, NULL, 0);
    hdc = GetDC(hwnd);
    SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);
    wglMakeCurrent(hdc, wglCreateContext(hdc));
    
    for (i = 0; i < GL_EXT_FUNCTION_COUNT; i++)
        glExtFunctions[i] = wglGetProcAddress(glExtFunctionNames[i]);
    
    program = glCreateProgram();
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fs, 0);
    glCompileShader(fragmentShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glUseProgram(program);
    
    renderAudio();
    memcpy(audioBuffer, &riffHeader, sizeof(riffHeader));
    
#ifdef AUDIO_DEBUG
    // debug audio output
    audioFile = CreateFile("audio.wav", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(audioFile, audioBuffer, sizeof(audioBuffer), &bytesWritten, NULL);
    CloseHandle(audioFile);
#endif
    
    sndPlaySound((LPCSTR)audioBuffer, SND_ASYNC | SND_MEMORY);
    
	ShowCursor(FALSE);
	
    startTime = timeGetTime();
    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        float time;
        
		// avoid 'not responding' system messages
		PeekMessage(NULL, NULL, 0, 0, PM_REMOVE);
		
#ifdef CAPTURE_FRAMES
        // capture at a steady 60fps
        time = (float)frameNumber / 60.0f * 140.0f / 60.0f;
        
        // stop at the end of the music
        if (((float)frameNumber / 60.0f) > (AUDIO_SAMPLES / 2 / 44100.0f))
            break;
#else
        time = (float)(timeGetTime() - startTime) * 0.001f * 140.0f / 60.0f;
#endif
        
		//time += 164.0f;
		
        u[0] = time; // time
		u[1] = (float)(time < 4.0f); // black
		u[1] += (time >= 324.0f) ? (time - 324.0f) * 0.25f : 0.0f;
		u[2] = 1.0f - (float)(time >= 68.0f && time < 260.0f); // spheres
		
		u[3] = 0.0f;
		if (time >= 164.0f && time < 168.0f)
			u[3] = 4.0 - (time - 164.0f);
		else if (time >= 180.0f && time < 184.0f)
			u[3] = 4.0 - (time - 180.0f);

        u[4] = (float)width;
        u[5] = (float)height;
        
        // hack - assume that the uniforms u[] will always be linked to locations [0-n]
        // given that they are the only uniforms in the shader, it is likely to work on all drivers
		glUniform1fv(0, UNIFORM_COUNT, u);
        
        glRects(-1, -1, 1, 1);
        
#ifdef CAPTURE_FRAMES
        // read back pixels
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer);
        
        // write ouput frame (skip existing ones)
        frameFile = CreateFile(frameFilename(frameNumber), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (frameFile)
		{
			WriteFile(frameFile, frameBuffer, width * height * 3, &frameBytesWritten, NULL);
			CloseHandle(frameFile);
		}
        
        frameNumber++;
#endif
        
        wglSwapLayerBuffers(hdc, WGL_SWAP_MAIN_PLANE);
    }
    
	ExitProcess(0);
}
