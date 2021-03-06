/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2003 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2003 Matthew Kendora and
                            Brad Jorsch (anomie@users.sourceforge.net)
 

                      
  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and
                            Nach (n-a-c-h@users.sourceforge.net)
                                          
  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2003 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman (jweidman@slip.net),
                            neviksti (neviksti@hotmail.com), and
                            Kris Bleakley (stinkfish@bigpond.com)
 
  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2003 zsKnight, pagefault (pagefault@zsnes.com)
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar and Gary Henderson.



 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/
#include <signal.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
/* FIXME: Linux alpha (aristocat) has a broken timercmp. Make testcase */
#if defined(__linux)
# ifndef timercmp
#  define timercmp(tvp, uvp, cmp)\
        ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
        (tvp)->tv_sec == (uvp)->tv_sec &&\
        (tvp)->tv_usec cmp (uvp)->tv_usec)
# endif
#endif
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>

#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif

#ifdef USE_THREADS
#include <pthread.h>
#include <sched.h>

pthread_t thread;
pthread_mutex_t mutex;
#endif

#ifdef __linux
#include <sys/soundcard.h>
#include <sys/mman.h>
#endif

#ifdef __sun
#ifdef __SVR4
#include <sys/audioio.h>
#else
#include <sun/audioio.h>
#endif
#endif

//Breaks sol9 and probably others.
//#if defined(__sun) && defined(__GNUC__)
//typedef void (*SIG_PF)();
//#endif

#include "snes9x.h"
#include "memmap.h"
#include "debug.h"
#include "cpuexec.h"
#include "ppu.h"
#include "snapshot.h"
#include "apu.h"
#include "display.h"
#include "gfx.h"
#include "soundux.h"
#include "spc700.h"

#ifdef SPCTOOL
#include "spctool/spc700.h"
extern "C" void TraceSPC (unsigned char *PC, unsigned short YA, unsigned char X,
			  SPCFlags PS, unsigned char *SP);
#endif

#ifdef _NETPLAY_SUPPORT
#include "netplay.h"
#endif

uint32 joypads [5] = {0};
int NumControllers = 5;

#ifdef JOYSTICK_SUPPORT
#ifdef __linux
#include <linux/joystick.h>
int js_fd [4] = {-1, -1, -1, -1};
int js_map_button [4][16] = {
    {
	SNES_B_MASK, SNES_A_MASK, SNES_A_MASK,
	SNES_Y_MASK, SNES_X_MASK, SNES_B_MASK,
	SNES_TL_MASK, SNES_TR_MASK,
	SNES_START_MASK, SNES_SELECT_MASK, 0, 0, 0, 0, 0, 0
    },
    {
	SNES_B_MASK, SNES_A_MASK, SNES_A_MASK,
	SNES_Y_MASK, SNES_X_MASK, SNES_B_MASK,
	SNES_TL_MASK, SNES_TR_MASK,
	SNES_START_MASK, SNES_SELECT_MASK, 0, 0, 0, 0, 0, 0
    },
    {
	SNES_B_MASK, SNES_A_MASK, SNES_A_MASK,
	SNES_Y_MASK, SNES_X_MASK, SNES_B_MASK,
	SNES_TL_MASK, SNES_TR_MASK,
	SNES_START_MASK, SNES_SELECT_MASK, 0, 0, 0, 0, 0, 0
    },
    {
	SNES_B_MASK, SNES_A_MASK, SNES_A_MASK,
	SNES_Y_MASK, SNES_X_MASK, SNES_B_MASK,
	SNES_TL_MASK, SNES_TR_MASK,
	SNES_START_MASK, SNES_SELECT_MASK, 0, 0, 0, 0, 0, 0
    }
};

#if 0
SNES_A_MASK, SNES_B_MASK, SNES_X_MASK, SNES_Y_MASK,
		            SNES_TL_MASK, SNES_TR_MASK, SNES_START_MASK, SNES_SELECT_MASK,
			    0, 0, 0, 0, 0, 0, 0, 0},
			   {SNES_A_MASK, SNES_B_MASK, SNES_X_MASK, SNES_Y_MASK,
			    SNES_TL_MASK, SNES_TR_MASK, SNES_START_MASK, SNES_SELECT_MASK,
			    0, 0, 0, 0, 0, 0, 0, 0},
			   {SNES_A_MASK, SNES_B_MASK, SNES_X_MASK, SNES_Y_MASK,
			    SNES_TL_MASK, SNES_TR_MASK, SNES_START_MASK, SNES_SELECT_MASK,
			    0, 0, 0, 0, 0, 0, 0, 0},
			   {SNES_A_MASK, SNES_B_MASK, SNES_X_MASK, SNES_Y_MASK,
			    SNES_TL_MASK, SNES_TR_MASK, SNES_START_MASK, SNES_SELECT_MASK,
			    0, 0, 0, 0, 0, 0, 0, 0}};
#endif
char *js_device [4] = {"/dev/js0", "/dev/js1", "/dev/js2", "/dev/js3"};
#endif

void InitJoysticks ();
void ReadJoysticks ();
#endif

void InitTimer ();
void *S9xProcessSound (void *);

char *rom_filename = NULL;
char *snapshot_filename = NULL;
char *SDD1_pack = NULL;

//FIXME: I see no reason not to configureenable this for all Unixen
#if defined(DEBUGGER) && (defined(__linux) || defined(__sun))
static void sigbrkhandler(int)
{
    CPU.Flags |= DEBUG_MODE_FLAG;
    signal(SIGINT, (SIG_PF) sigbrkhandler);
}
#endif

void OutOfMemory ()
{
    fprintf (stderr, "\
Snes9X: Memory allocation failure - not enough RAM/virtual memory available.\n\
        S9xExiting...\n");
    Memory.Deinit ();
    S9xDeinitAPU ();
    
    exit (1);
}

void S9xParseArg (char **argv, int &i, int argc)
{
#ifdef JOYSTICK_SUPPORT
    if (strcmp (argv [i], "-j") == 0 ||
	     strcasecmp (argv [i], "-nojoy") == 0)
	Settings.JoystickEnabled = FALSE;
    else if (strcasecmp (argv [i], "-joydev1") == 0)
    {
	if (i + 1 < argc)
	    js_device[0] = argv[++i];
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joydev2") == 0)
    {
	if (i + 1 < argc)
	    js_device[1] = argv[++i];
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joymap1") == 0)
    {
	if (i + 8 < argc)
	{
	    int t;

	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_A_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_B_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_X_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_Y_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_TL_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_TR_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_START_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [0][t] = SNES_SELECT_MASK;
	}
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joymap2") == 0)
    {
	if (i + 8 < argc)
	{
	    int t;

	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_A_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_B_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_X_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_Y_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_TL_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_TR_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_START_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [1][t] = SNES_SELECT_MASK;
	}
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joymap3") == 0)
    {
	if (i + 8 < argc)
	{
	    int t;

	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_A_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_B_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_X_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_Y_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_TL_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_TR_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_START_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [2][t] = SNES_SELECT_MASK;
	}
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-joymap4") == 0)
    {
	if (i + 8 < argc)
	{
	    int t;

	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_A_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_B_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_X_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_Y_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_TL_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_TR_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_START_MASK;
	    if ((t = atoi (argv [++i])) < 15) js_map_button [3][t] = SNES_SELECT_MASK;
	}
	else
	    S9xUsage ();
    }
    else 
#endif
    if (strcasecmp (argv [i], "-b") == 0 ||
	     strcasecmp (argv [i], "-bs") == 0 ||
	     strcasecmp (argv [i], "-buffersize") == 0)
    {
	if (i + 1 < argc)
	    Settings.SoundBufferSize = atoi (argv [++i]);
	else
	    S9xUsage ();
    }
    else if (strcmp (argv [i], "-l") == 0 ||
	     strcasecmp (argv [i], "-loadsnapshot") == 0)
    {
	if (i + 1 < argc)
	    snapshot_filename = argv [++i];
	else
	    S9xUsage ();
    }
    else if (strcasecmp (argv [i], "-sdd1-pack") == 0)
    {
	if (i + 1 < argc)
	    SDD1_pack = argv [++i];
	else
	    S9xUsage ();
    }
    else
	S9xParseDisplayArg (argv, i, argc);
}

#include "cheats.h"

int main (int argc, char **argv)
{
    if (argc < S9xMinCommandLineArgs ())
	S9xUsage ();
	
    ZeroMemory (&Settings, sizeof (Settings));

#ifdef JOYSTICK_SUPPORT
    Settings.JoystickEnabled = TRUE;
#else
    Settings.JoystickEnabled = FALSE;
#endif

    Settings.SoundPlaybackRate = 4;
    Settings.Stereo = TRUE;
    Settings.SoundBufferSize = 0;
    Settings.CyclesPercentage = 100;
    Settings.DisableSoundEcho = FALSE;
#ifndef NOSOUND
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
#else
    Settings.APUEnabled = Settings.NextAPUEnabled = FALSE;
#endif
    Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
    Settings.SkipFrames = AUTO_FRAMERATE;
    Settings.ShutdownMaster = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.DisableSampleCaching = FALSE;
    Settings.DisableMasterVolume = FALSE;
    Settings.Mouse = TRUE;
    Settings.SuperScope = TRUE;
    Settings.MultiPlayer5 = TRUE;
    Settings.ControllerOption = SNES_JOYPAD;
    Settings.Transparency = FALSE;
    Settings.SixteenBit = FALSE;
    Settings.SupportHiRes = FALSE;
    Settings.NetPlay = FALSE;
    Settings.ServerName [0] = 0;
    Settings.ThreadSound = FALSE;
    Settings.AutoSaveDelay = 30;
#ifdef _NETPLAY_SUPPORT
    Settings.Port = NP_DEFAULT_PORT;
#endif
    Settings.ApplyCheats = TRUE;
    Settings.TurboMode = FALSE;
    Settings.TurboSkipFrames = 15;
    Settings.StretchScreenshots = 1;
    rom_filename = S9xParseArgs (argv, argc);

    Settings.Transparency = Settings.ForceTransparency;
    if (Settings.ForceNoTransparency)
	Settings.Transparency = FALSE;

    if (Settings.Transparency)
	Settings.SixteenBit = TRUE;

    Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;

    if (!Memory.Init () || !S9xInitAPU())
	OutOfMemory ();

    (void) S9xInitSound (Settings.SoundPlaybackRate, Settings.Stereo,
			 Settings.SoundBufferSize);

    if (!Settings.APUEnabled)
	S9xSetSoundMute (TRUE);

    uint32 saved_flags = CPU.Flags;

#ifdef GFX_MULTI_FORMAT
    S9xSetRenderPixelFormat (RGB565);
#endif

    if (rom_filename)
    {
	if (!Memory.LoadROM (rom_filename))
	{
	    char dir [_MAX_DIR + 1];
	    char drive [_MAX_DRIVE + 1];
	    char name [_MAX_FNAME + 1];
	    char ext [_MAX_EXT + 1];
	    char fname [_MAX_PATH + 1];

	    _splitpath (rom_filename, drive, dir, name, ext);
	    _makepath (fname, drive, dir, name, ext);

	    strcpy (fname, S9xGetROMDirectory ());
	    strcat (fname, SLASH_STR);
	    strcat (fname, name);
	    if (ext [0])
	    {
		strcat (fname, ".");
		strcat (fname, ext);
	    }
	    _splitpath (fname, drive, dir, name, ext);
	    _makepath (fname, drive, dir, name, ext);
	    if (!Memory.LoadROM (fname))
	    {
		printf ("Error opening: %s\n", rom_filename);
		exit (1);
	    }
	}
	Memory.LoadSRAM (S9xGetFilename (".srm"));
	S9xLoadCheatFile (S9xGetFilename (".cht"));
    }
    else
    {
	S9xReset ();
	Settings.Paused |= 2;
    }
    CPU.Flags = saved_flags;

#if !defined(__MSDOS) && defined(DEBUGGER)
#if defined(__unix) && !defined(__NeXT__)
    struct sigaction sa;
#if defined(__linux)
    sa.sa_handler = sigbrkhandler;
#else
    sa.sa_handler = (SIG_PF) sigbrkhandler;
#endif

#ifdef SA_RESTART
    sa.sa_flags = SA_RESTART;
#else
    sa.sa_flags = 0;
#endif

    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
#else
    signal(SIGINT, (SIG_PF) sigbrkhandler);
#endif
#endif

    S9xInitInputDevices ();

    S9xInitDisplay (argc, argv);
    if (!S9xGraphicsInit ())
	OutOfMemory ();
	
    S9xTextMode ();

#ifdef _NETPLAY_SUPPORT
    if (strlen (Settings.ServerName) == 0)
    {
	char *server = getenv ("S9XSERVER");
	if (server)
	{
	    strncpy (Settings.ServerName, server, 127);
	    Settings.ServerName [127] = 0;
	}
    }
    char *port = getenv ("S9XPORT");
    if (Settings.Port >= 0 && port)
	Settings.Port = atoi (port);
    else
    if (Settings.Port < 0)
	Settings.Port = -Settings.Port;

    if (Settings.NetPlay)
    {
	int player;

	if (!S9xNetPlayConnectToServer (Settings.ServerName, Settings.Port,
					Memory.ROMName, player))
	{
	    fprintf (stderr, "\
Failed to connected to Snes9x netplay server \"%s\" on port %d.\n",
		 Settings.ServerName, Settings.Port);
	    S9xExit ();
	}
	fprintf (stderr, "\
Connected to \"%s\" on port %d as player #%d playing \"%s\"\n",
		 Settings.ServerName, Settings.Port, player, Memory.ROMName);
    }
    
#endif

    if (snapshot_filename)
    {
	int Flags = CPU.Flags & (DEBUG_MODE_FLAG | TRACE_FLAG);
	if (!S9xLoadSnapshot (snapshot_filename))
	    exit (1);
	CPU.Flags |= Flags;
    }

    S9xGraphicsMode ();

    sprintf (String, "\"%s\" %s: %s", Memory.ROMName, TITLE, VERSION);
    S9xSetTitle (String);
    
#ifdef JOYSTICK_SUPPORT
    uint32 JoypadSkip = 0;
#endif

    InitTimer ();
    if (!Settings.APUEnabled)
	S9xSetSoundMute (FALSE);

#if 0
    {
	FILE *fs = fopen ("test.bin", "r");
	if (fs)
	{
	    memset (IAPU.RAM, 0, 1024 * 64);
	    int bytes = fread (IAPU.RAM + 1024, 1, 13, fs);
	    bytes = fread (IAPU.RAM + 1024, 1, 1024 * 63, fs);
	    fclose (fs);
#ifdef SPCTOOL
	    _FixSPC (1024, 0, 0, 0, 0, 0xff);
#else
	    IAPU.PC = IAPU.RAM + 1024;
#endif
	    APU.Flags ^= TRACE_FLAG;
	    extern FILE *apu_trace;
	    if (APU.Flags & TRACE_FLAG)
	    {
#ifdef SPCTOOL
		printf ("ENABLED\n");
		_SetSPCDbg (TraceSPC);                   //Install debug handler
#endif
		if (apu_trace == NULL)
		    apu_trace = fopen ("aputrace.log", "wb");
	    }
	    CPU.Cycles = 1024 * 10;
	    APU_EXECUTE ();
	    exit (0);
	}
    }
#endif

    while (1)
    {
	if (!Settings.Paused
#ifdef DEBUGGER
	    || (CPU.Flags & (DEBUG_MODE_FLAG | SINGLE_STEP_FLAG))
#endif
           )
	    S9xMainLoop ();

	if (Settings.Paused
#ifdef DEBUGGER
	    || (CPU.Flags & DEBUG_MODE_FLAG)
#endif
           )
	{
	    S9xSetSoundMute (TRUE);
	}

#ifdef DEBUGGER
	if (CPU.Flags & DEBUG_MODE_FLAG)
	{
	    S9xDoDebug ();
	}
	else
#endif
	if (Settings.Paused)
	    S9xProcessEvents (TRUE);

#ifdef JOYSTICK_SUPPORT
	if (Settings.JoystickEnabled && (JoypadSkip++ & 1) == 0)
	    ReadJoysticks ();
#endif
	S9xProcessEvents (FALSE);
	
	if (!Settings.Paused
#ifdef DEBUGGER
	    && !(CPU.Flags & DEBUG_MODE_FLAG)
#endif	    
           )
	{
	    S9xSetSoundMute (FALSE);
	}
    }
    return (0);
}

void S9xAutoSaveSRAM ()
{
    Memory.SaveSRAM (S9xGetFilename (".srm"));
}

void S9xExit ()
{
    S9xSetSoundMute (TRUE);
    S9xDeinitDisplay ();
    Memory.SaveSRAM (S9xGetFilename (".srm"));
    S9xSaveCheatFile (S9xGetFilename (".cht"));
    Memory.Deinit ();
    S9xDeinitAPU ();

#ifdef _NETPLAY_SUPPORT
    if (Settings.NetPlay)
	S9xNetPlayDisconnect ();
#endif
    
    exit (0);
}

void S9xInitInputDevices ()
{
#ifdef JOYSTICK_SUPPORT
    InitJoysticks ();
#endif
}

#ifdef JOYSTICK_SUPPORT
void InitJoysticks ()
{
#ifdef JSIOCGVERSION
    int version;
    unsigned char axes, buttons;

    if ((js_fd [0] = open (js_device [0], O_RDONLY | O_NONBLOCK)) < 0)
    {
	perror (js_device [0]);
	return;
    }

    if (ioctl (js_fd [0], JSIOCGVERSION, &version))
    {
        puts("joystick: You need at least driver version 1.0 for joystick support");
	close (js_fd [0]);
	return;
    }
    js_fd [1] = open (js_device [1], O_RDONLY | O_NONBLOCK);
    js_fd [2] = open (js_device [2], O_RDONLY | O_NONBLOCK);
    js_fd [3] = open (js_device [3], O_RDONLY | O_NONBLOCK);

#ifdef JSIOCGNAME
    char name [130];
    bzero (name, 128);
    if (ioctl (js_fd [0], JSIOCGNAME(128), name) > 0) 
    {
        printf ("Using %s (%s) as pad1", name, js_device [0]);
        if (js_fd [1] > 0)
	{
	    ioctl (js_fd [1], JSIOCGNAME(128), name);
	    printf ("and %s (%s) as pad2", name, js_device [1]);
	}
    } 
    else
#endif
    {
	ioctl (js_fd [0], JSIOCGAXES, &axes);
	ioctl (js_fd [0], JSIOCGBUTTONS, &buttons);
	printf ("Using %d-axis %d-button joystick (%s) as pad1", axes, buttons, js_device [0]);
	if (js_fd [1] > 0)
	{
	    ioctl (js_fd [0], JSIOCGAXES, &axes);
	    ioctl (js_fd [0], JSIOCGBUTTONS, &buttons);
	    printf (" and %d-axis %d-button (%s) as pad2", axes, buttons, js_device [1]);
	}
    }

    puts (".");
#endif
}

void ReadJoysticks ()
{
#ifdef JSIOCGVERSION
    struct js_event js_ev;
    int i;

    for (i = 0; i < 4 && js_fd [i] >= 0; i++)
    {
	while (read (js_fd[i], &js_ev, sizeof (struct js_event)) == sizeof (struct js_event) )
	{
	    switch (js_ev.type & ~JS_EVENT_INIT)
	    {
	    case JS_EVENT_AXIS:
		if (js_ev.number == 0)
		{
		    if(js_ev.value < -16384)
		    {
			joypads [i] |= SNES_LEFT_MASK; 
			joypads [i] &= ~SNES_RIGHT_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			joypads [i] &= ~SNES_LEFT_MASK;
			joypads [i] |= SNES_RIGHT_MASK; 
			break;
		    }
		    joypads [i] &= ~SNES_LEFT_MASK;
		    joypads [i] &= ~SNES_RIGHT_MASK;
		    break;	
		}

		if (js_ev.number == 1)
		{
		    if (js_ev.value < -16384)
		    {
			joypads [i] |= SNES_UP_MASK; 
			joypads [i] &= ~SNES_DOWN_MASK;
			break;
		    }
		    if (js_ev.value > 16384)
		    {
			joypads [i] &= ~SNES_UP_MASK;
			joypads [i] |= SNES_DOWN_MASK; 
			break;
		    }
		    joypads [i] &= ~SNES_UP_MASK;
		    joypads [i] &= ~SNES_DOWN_MASK;
		    break;	
		}

		break;

	    case JS_EVENT_BUTTON:
		if (js_ev.number > 15)
		    break;

		if (js_ev.value)
		    joypads [i] |= js_map_button [i][js_ev.number];
		else
		    joypads [i] &= ~js_map_button [i][js_ev.number];

		break;
	    }
	}
    }
#endif
}
#endif // defined (JOYSTICK_SUPPORT)

const char *GetHomeDirectory ()
{
    return (getenv ("HOME"));
}

START_EXTERN_C
char* osd_GetPackDir()
{
  static char filename[_MAX_PATH];
  memset(filename, 0, _MAX_PATH);
  
  if(strlen(S9xGetSnapshotDirectory())!=0)
    strcpy (filename, S9xGetSnapshotDirectory());
  else
  {
    char dir [_MAX_DIR + 1];
    char drive [_MAX_DRIVE + 1];
    char name [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    _splitpath(Memory.ROMFilename, drive, dir, name, ext);
    _makepath(filename, drive, dir, NULL, NULL);
  }
  
  if(!strncmp((char*)&Memory.ROM [0xffc0], "SUPER POWER LEAG 4   ", 21))
  {
    if (getenv("SPL4PACK"))
      return getenv("SPL4PACK");
    else 
      strcat(filename, "/SPL4-SP7");
  }
  else if(!strncmp((char*)&Memory.ROM [0xffc0], "MOMOTETSU HAPPY      ",21))
  {
    if (getenv("MDHPACK"))
      return getenv("MDHPACK");
    else 
      strcat(filename, "/SMHT-SP7");
  }
  else if(!strncmp((char*)&Memory.ROM [0xffc0], "HU TENGAI MAKYO ZERO ", 21))
  {
    if (getenv("FEOEZPACK"))
      return getenv("FEOEZPACK");
    else 
      strcat(filename, "/FEOEZSP7");
  }
  else if(!strncmp((char*)&Memory.ROM [0xffc0], "JUMP TENGAIMAKYO ZERO",21))
  {
    if (getenv("SJNSPACK"))
      return getenv("SJNSPACK");
    else 
      strcat(filename, "/SJUMPSP7");
  } else strcat(filename, "/MISC-SP7");
  return filename;
}
END_EXTERN_C

const char *S9xGetSnapshotDirectory ()
{
    static char filename [PATH_MAX];
    const char *snapshot;
    
    if (!(snapshot = getenv ("SNES9X_SNAPSHOT_DIR")) &&
	!(snapshot = getenv ("SNES96_SNAPSHOT_DIR")))
    {
	const char *home = GetHomeDirectory ();
	strcpy (filename, home);
	strcat (filename, SLASH_STR);
	strcat (filename, ".snes96_snapshots");
	mkdir (filename, 0777);
	chown (filename, getuid (), getgid ());
    }
    else
	return (snapshot);

    return (filename);
}

const char *S9xGetFilename (const char *ex)
{
    static char filename [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
    strcpy (filename, S9xGetSnapshotDirectory ());
    strcat (filename, SLASH_STR);
    strcat (filename, fname);
    strcat (filename, ex);

    return (filename);
}

const char *S9xGetFilenameInc (const char *e)
{
    static char filename [_MAX_PATH + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];
    char *ptr;
    struct stat buf;

    if (strlen (S9xGetSnapshotDirectory()))
    {
        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
        strcpy (filename, S9xGetSnapshotDirectory());
        strcat (filename, "/");
        strcat (filename, fname);
        ptr = filename + strlen (filename);
        strcat (filename, "00/");
        strcat (filename, e);
    }
    else
    {
        _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
        strcat (fname, "00/");
        _makepath (filename, drive, dir, fname, e);
        ptr = strstr (filename, "00/");
    }

    do
    {
        if (++*(ptr + 2) > '9')
        {
            *(ptr + 2) = '0';
            if (++*(ptr + 1) > '9')
            {
                *(ptr + 1) = '0';
                if (++*ptr > '9')
                    break;
            }
        }
    } while( stat(filename, &buf) == 0 );

    return (filename);
}

const char *S9xGetROMDirectory ()
{
    const char *roms;
    
    if (!(roms = getenv ("SNES9X_ROM_DIR")) &&
	!(roms = getenv ("SNES96_ROM_DIR")))
	return ("." SLASH_STR "roms");
    else
	return (roms);
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
	return (p + 1);

    return (f);
}

const char *S9xChooseFilename (bool8 read_only)
{
    char def [PATH_MAX + 1];
    char title [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (Memory.ROMFilename, drive, dir, def, ext);
    strcat (def, ".s96");
    sprintf (title, "%s snapshot filename",
	    read_only ? "Select load" : "Choose save");
    const char *filename;

    S9xSetSoundMute (TRUE);
    filename = S9xSelectFilename (def, S9xGetSnapshotDirectory (), "s96", title);
    S9xSetSoundMute (FALSE);
    return (filename);
}

bool8 S9xOpenSnapshotFile (const char *fname, bool8 read_only, STREAM *file)
{
    char filename [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (fname, drive, dir, filename, ext);

    if (*drive || *dir == '/' ||
	(*dir == '.' && (*(dir + 1) == '/'
        )))
    {
	strcpy (filename, fname);
	if (!*ext)
	    strcat (filename, ".s96");
    }
    else
    {
	strcpy (filename, S9xGetSnapshotDirectory ());
	strcat (filename, SLASH_STR);
	strcat (filename, fname);
	if (!*ext)
	    strcat (filename, ".s96");
    }
    
#ifdef ZLIB
    if (read_only)
    {
	if ((*file = OPEN_STREAM (filename, "rb")))
	    return (TRUE);
    }
    else
    {
	if ((*file = OPEN_STREAM (filename, "wb")))
	{
	    chown (filename, getuid (), getgid ());
	    return (TRUE);
	}
    }
#else
    char command [PATH_MAX];
    
    if (read_only)
    {
	sprintf (command, "gzip -d <\"%s\"", filename);
	if (*file = popen (command, "r"))
	    return (TRUE);
    }
    else
    {
	sprintf (command, "gzip --best >\"%s\"", filename);
	if (*file = popen (command, "wb"))
	    return (TRUE);
    }
#endif
    return (FALSE);
}

void S9xCloseSnapshotFile (STREAM file)
{
#ifdef ZLIB
    CLOSE_STREAM (file);
#else
    pclose (file);
#endif
}

bool8 S9xInitUpdate ()
{
    return (TRUE);
}

bool8 S9xDeinitUpdate (int Width, int Height, bool8 /*sixteen_bit*/)
{
    S9xPutImage (Width, Height);
    return (TRUE);
}

static unsigned long now ()
{
    static unsigned long seconds_base = 0;
    struct timeval tp;
    gettimeofday (&tp, NULL);
    if (!seconds_base)
	seconds_base = tp.tv_sec;

    return ((tp.tv_sec - seconds_base) * 1000 + tp.tv_usec / 1000);
}

void OutputFrameRate ()
{
    static int FrameCount = 0;
    static unsigned long then = now ();

    if (++FrameCount % 60 == 0)
    {
	unsigned long here = now ();
//	printf ("\rFrame rate: %.2lfms", (double) (here - then) / 60);
//	fflush (stdout);
	then = here;
    }
}

void _makepath (char *path, const char *, const char *dir,
		const char *fname, const char *ext)
{
    if (dir && *dir)
    {
	strcpy (path, dir);
	strcat (path, "/");
    }
    else
	*path = 0;
    strcat (path, fname);
    if (ext && *ext)
    {
        strcat (path, ".");
        strcat (path, ext);
    }
}

void _splitpath (const char *path, char *drive, char *dir, char *fname,
		 char *ext)
{
    *drive = 0;

    char *slash = strrchr (path, '/');
    if (!slash)
	slash = strrchr (path, '\\');

    char *dot = strrchr (path, '.');

    if (dot && slash && dot < slash)
	dot = NULL;

    if (!slash)
    {
	strcpy (dir, "");
	strcpy (fname, path);
        if (dot)
        {
	    *(fname + (dot - path)) = 0;
	    strcpy (ext, dot + 1);
        }
	else
	    strcpy (ext, "");
    }
    else
    {
	strcpy (dir, path);
	*(dir + (slash - path)) = 0;
	strcpy (fname, slash + 1);
        if (dot)
	{
	    *(fname + (dot - slash) - 1) = 0;
    	    strcpy (ext, dot + 1);
	}
	else
	    strcpy (ext, "");
    }
}

void S9xToggleSoundChannel (int c)
{
    if (c == 8)
	so.sound_switch = 255;
    else
	so.sound_switch ^= 1 << c;
    S9xSetSoundControl (so.sound_switch);
}

static void SoundTrigger ()
{
    if (Settings.APUEnabled && !so.mute_sound)
	S9xProcessSound (NULL);
}

void StopTimer ()
{
    struct itimerval timeout;

    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_usec = 0;
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_usec = 0;
    if (setitimer (ITIMER_REAL, &timeout, NULL) < 0)
	perror ("setitimer");
}

void InitTimer ()
{
    struct itimerval timeout;
    struct sigaction sa;
    
#ifdef USE_THREADS
    if (Settings.ThreadSound)
    {
	pthread_mutex_init (&mutex, NULL);
	pthread_create (&thread, NULL, S9xProcessSound, NULL);
	return;
    }
#endif

#if defined(__linux)
    sa.sa_handler = (SIG_PF) SoundTrigger;
#else
    sa.sa_handler = (SIG_PF) SoundTrigger;
#endif

#if defined (SA_RESTART)
    sa.sa_flags = SA_RESTART;
#else
    sa.sa_flags = 0;
#endif

    sigemptyset (&sa.sa_mask);
    sigaction (SIGALRM, &sa, NULL);
    
    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_usec = 10000;
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_usec = 10000;
    if (setitimer (ITIMER_REAL, &timeout, NULL) < 0)
	perror ("setitimer");
}

void S9xSyncSpeed ()
{
#ifdef _NETPLAY_SUPPORT
    if (Settings.NetPlay)
    {
	// XXX: Send joypad position update to server
	// XXX: Wait for heart beat from server
	S9xNetPlaySendJoypadUpdate (joypads [0]);
	if (!S9xNetPlayCheckForHeartBeat ())
	{
	    do
	    {
		CHECK_SOUND ();
		S9xProcessEvents (FALSE);
	    } while (!S9xNetPlayCheckForHeartBeat ());
	    IPPU.RenderThisFrame = TRUE;
	    IPPU.SkippedFrames = 0;
	}
	else
	{
	    if (IPPU.SkippedFrames < 10)
	    {
		IPPU.SkippedFrames++;
		IPPU.RenderThisFrame = FALSE;
	    }
	    else
	    {
		IPPU.RenderThisFrame = TRUE;
		IPPU.SkippedFrames = 0;
	    }
	}
    }
    else
#endif
#if 0
    if (Settings.SoundSync == 2)
    {
	IPPU.RenderThisFrame = TRUE;
	IPPU.SkippedFrames = 0;
    }
    else
#endif
    if (!Settings.TurboMode && Settings.SkipFrames == AUTO_FRAMERATE)
    {
	static struct timeval next1 = {0, 0};
	struct timeval now;

#ifdef __sgi
	/* BS: saves on CPU usage */
	sginap(1);
#endif
	while (gettimeofday (&now, NULL) < 0) ;
	if (next1.tv_sec == 0)
	{
	    next1 = now;
	    next1.tv_usec++;
	}

	if (timercmp(&next1, &now, >))
	{
	    if (IPPU.SkippedFrames == 0)
	    {
		do
		{
		    CHECK_SOUND ();
		    S9xProcessEvents (FALSE);
		    while (gettimeofday (&now, NULL) < 0) ;
		} while (timercmp(&next1, &now, >));
	    }
	    IPPU.RenderThisFrame = TRUE;
	    IPPU.SkippedFrames = 0;
	}
	else
	{
	    if (IPPU.SkippedFrames < 10)
	    {
		IPPU.SkippedFrames++;
		IPPU.RenderThisFrame = FALSE;
	    }
	    else
	    {
		IPPU.RenderThisFrame = TRUE;
		IPPU.SkippedFrames = 0;
		next1 = now;
	    }
	}
	next1.tv_usec += Settings.FrameTime;
	if (next1.tv_usec >= 1000000)
	{
	    next1.tv_sec += next1.tv_usec / 1000000;
	    next1.tv_usec %= 1000000;
	}
    }
    else
    {
	if (++IPPU.FrameSkip >= (Settings.TurboMode ? Settings.TurboSkipFrames
						    : Settings.SkipFrames))
	{
	    IPPU.FrameSkip = 0;
	    IPPU.SkippedFrames = 0;
	    IPPU.RenderThisFrame = TRUE;
	}
	else
	{
	    IPPU.SkippedFrames++;
	    IPPU.RenderThisFrame = FALSE;
	}
    }
}

static long log2 (long num)
{
    long n = 0;

    while (num >>= 1)
	n++;

    return (n);
}

#if 0 /* Dead code  20031117 */
static long power (int num, int pow)
{
    long val = num;
    int i;
    
    if (pow == 0)
	return (1);

    for (i = 1; i < pow; i++)
	val *= num;

    return (val);
}
#endif 

/* BS: the SGI sound routines. */
#ifdef __sgi
char	*sgi_sound = "SGI/IRIX sound by Sherman";

#include <audio.h>

static int Rates[8] =
{
    0, 8000, 11025, 16000, 22050, 32000, 44100, 48000
};

static int BufferSizes [8] =
{
#if 1 /*BS: double buffer size */
    0, 512, 512, 512, 1024, 1024, 2048, 2048
#else
    0, 256, 256, 256, 512, 512, 1024, 1024
#endif
};


bool8 S9xOpenSoundDevice (int mode, bool8 stereo, int buffer_size)
{
	long	channels = -1,
		sampsize = -1,
		samprate = -1,
		compress = -1,
		device = -1,
		interleave = -1,
		outsize = -1;
	ALconfig al_config;
	long	al_params[2] = {
			AL_OUTPUT_RATE, -1
		};

	al_config = ALnewconfig();

#if 1 /* by not doing this, num channels will remain at current setting */
	/* number of channels can only be changed before port is open. */
	channels = stereo ? 2 : 1;
	if (ALsetchannels(al_config, channels)) {
		perror("ERROR with ALsetchannels");
	}

	channels = ALgetchannels(al_config);
#  if 0
	printf("channels now = %ld\n", channels);
#  endif
#endif
	so.stereo = channels - 1;

	/* unforunately, this must be called before opening the port.  It'd */
	/*   be nice to have a little more info before calling this, but    */
	/*   we'll have to guess for now.  Not reducing the queue size      */
	/*   results in a delay between events and the corresponding sounds.*/
	if (ALsetqueuesize(al_config, (long)(BufferSizes[mode&7]*channels*3))) {
		perror("ERROR with ALsetqueuesize");
	}

	/* open the audio port */
	so.al_port = ALopenport("Snes9x audio", "w", al_config);
	if (so.al_port == 0) {
		return (FALSE);
	}

	/* get the current settings of the audio port */
	al_config = ALgetconfig(so.al_port);
	channels = ALgetchannels(al_config);
	sampsize = ALgetwidth(al_config);
	outsize = ALgetfillable(so.al_port);
	ALgetparams(AL_DEFAULT_DEVICE, al_params, 2);
	samprate = al_params[1];

#if 0
	/* print machines current settings */
	printf("channels = %ld\n", channels);
	printf("sampsize = %ld\n", sampsize);
	printf("samprate = %ld\n", samprate);
	printf("outsize =  %ld\n", outsize);
	printf("compress = %ld\n", compress);
	printf("device =   %ld\n", device);
	printf("interleave=%ld\n", interleave);
#endif


	/* do not encode */
	so.encoded = 0;

#if 1 /* by not doing this, rate will be left at current setting */
	samprate = Rates [mode & 7];
	al_params[1] = samprate;
	ALsetparams(AL_DEFAULT_DEVICE, al_params, 2);

	ALgetparams(AL_DEFAULT_DEVICE, al_params, 2);
	samprate = al_params[1];
# if 0
	printf("samprate now = %ld\n", samprate);
#endif
#endif
	so.playback_rate = samprate;

#if 0 /* by not doing this, sample size will be left at current setting */
	if (ALsetwidth(al_config, AL_SAMPLE_8)) {
		perror("ERROR with ALsetwidth");
	}
	if (ALsetconfig(so.al_port, al_config)) {
		perror("ERROR with ALsetconfig");
	}

	sampsize = ALgetwidth(al_config);
	printf("sampsize now = %ld\n", sampsize);
#endif


	/* set the sample size */
	switch (sampsize) {
	case AL_SAMPLE_8:
		so.sixteen_bit = 0;
		break;
	case AL_SAMPLE_16:
		so.sixteen_bit = 1;
		break;
	case AL_SAMPLE_24:
	default:
		so.sixteen_bit = 1;
		break;
	}

	/* choose a buffer size based on the sample rate, and increase as nece*/
	for (int i = 1; i < 7; i++)
	if (samprate <= Rates [i])
	    break;
	so.buffer_size = BufferSizes [i];
	if (so.stereo)
		so.buffer_size *= 2;
	if (so.sixteen_bit)
		so.buffer_size *= 2;
	if (so.buffer_size > MAX_BUFFER_SIZE*4)
		so.buffer_size = MAX_BUFFER_SIZE*4;

	printf("SGI sound successfully opened\n");
	printf ("  Rate: %d, Buffer size: %d, 16-bit: %s, Stereo: %s, Encoded: %s\n",
	    so.playback_rate, so.buffer_size, so.sixteen_bit ? "yes" : "no",
	    so.stereo ? "yes" : "no", so.encoded ? "yes" : "no");

	return (TRUE);
}


void S9xUnixProcessSound (void)
{
    uint8		Buf[MAX_BUFFER_SIZE*4];
    int			buf_size;
    signed short	*audio_buf;

    CPU.Flags &= ~PROCESS_SOUND_FLAG;

    if (so.al_port == 0)
	return;
    if (Settings.APUEnabled == FALSE) {
	printf("why am I hear? alport = %p\n", so.al_port);
	return;
    }

    int sample_count = so.buffer_size;
    if (so.sixteen_bit)
	sample_count >>= 1;

    /* BS: return if buffer is full */
    if (ALgetfillable(so.al_port) < so.buffer_size) {
#if 0
	printf("error: fillable space only = %ld\n", ALgetfillable(so.al_port));
#endif
	return;
    }

    S9xMixSamples (Buf, sample_count);

    if (!so.mute_sound) {
	    buf_size = sample_count;
	    audio_buf = (signed short *)Buf;

#if 0
	    printf("about to write buffer %p size %d\n", audio_buf, buf_size);
#endif
	    if (ALwritesamps(so.al_port, audio_buf, buf_size)) {
		perror("ERROR with ALwritesamps");
	    }
    }
}
#endif

#ifdef __sun
static int Rates[8] =
{
    0, 8000, 11025, 16000, 22050, 32000, 37800, 44100
};

static int BufferSizes [8] =
{
    0, 256, 256, 256, 512, 512, 1024, 1024
};

bool8 S9xOpenSoundDevice (int mode, bool8 stereo, int buffer_size)
{
    int i;

    if ((so.sound_fd = open ("/dev/audio", O_WRONLY)) < 0)
	return (FALSE);

    audio_info_t audio;

    AUDIO_INITINFO (&audio);
    audio.play.sample_rate = Rates [mode & 7];
    audio.play.channels = stereo ? 2 : 1;
    audio.play.precision = 16;
    audio.play.encoding = AUDIO_ENCODING_LINEAR;

    ioctl (so.sound_fd, AUDIO_SETINFO, &audio);
    if (ioctl (so.sound_fd, AUDIO_GETINFO, &audio) != 0)
	return (FALSE);

    so.stereo = audio.play.channels - 1;
    so.playback_rate = audio.play.sample_rate;
    so.encoded = audio.play.encoding != AUDIO_ENCODING_LINEAR;
    so.sixteen_bit = audio.play.precision == 16;

    for (i = 1; i < 7; i++)
	if (audio.play.sample_rate <= Rates [i])
	    break;
    so.buffer_size = BufferSizes [i];
    if (buffer_size > 0)
    if (so.stereo)
	so.buffer_size *= 2;
    if (so.sixteen_bit)
	so.buffer_size *= 2;
    if (so.buffer_size > MAX_BUFFER_SIZE)
	so.buffer_size = MAX_BUFFER_SIZE;
    so.last_eof = -1;

    printf ("Rate: %d, Buffer size: %d, 16-bit: %s, Stereo: %s, Encoded: %s\n",
	    so.playback_rate, so.buffer_size, so.sixteen_bit ? "yes" : "no",
	    so.stereo ? "yes" : "no", so.encoded ? "yes" : "no");

    return (TRUE);
}
#endif

#if defined(__linux)
static int Rates[8] =
{
    0, 8000, 11025, 16000, 22050, 32000, 44100, 48000
};

static int BufferSizes [8] =
{
    0, 256, 256, 256, 512, 512, 1024, 1024
};

bool8 S9xOpenSoundDevice (int mode, bool8 stereo, int buffer_size)
{
    int J, K;

    if ((so.sound_fd = open ("/dev/dsp", O_WRONLY)) < 0)
    {
	perror ("/dev/dsp");
	return (FALSE);
    }

#ifdef MMAP_SOUND 
   if (ioctl (so.sound_fd, SNDCTL_DSP_GETCAPS, &J) < 0)
    {
	perror ("ioctl SNDCTL_DSP_GETCAPS");
    }
    else
    {
	if (J & DSP_CAP_MMAP)
	    printf ("DSP_CAP_MMAP supported\n");
    }
    void *ptr;

    if ((ptr = mmap (0, so.buffer_size * 3, PROT_WRITE, 0, so.sound_fd, 0)) == 0)
	printf ("mmap failed\n");

    J = 0;
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETTRIGGER, &J) < 0)
	perror ("ioctl SNDCTL_DSP_SETTRIGGER");
#endif

    //Test and check back to http://snes9x.com/forum/topic.asp?TOPIC_ID=8512
//    J = AFMT_U8;
    J = AFMT_S16_NE;
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETFMT, &J) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SETFMT");
	return (FALSE);
    }

    if (J != AFMT_S16_NE)
    {
	so.sixteen_bit = FALSE;
	J = AFMT_U8;
	if (ioctl (so.sound_fd, SNDCTL_DSP_SETFMT, &J) < 0)
	{
	    perror ("ioctl SNDCTL_DSP_SETFMT");
	    return (FALSE);
	}
    }
    else
	so.sixteen_bit = TRUE;

    so.stereo = stereo;
    if (ioctl (so.sound_fd, SNDCTL_DSP_STEREO, &so.stereo) < 0)
    {
	perror ("ioctl SNDCTL_DSP_STEREO");
	return (FALSE);
    }
    
    so.playback_rate = Rates[mode & 0x07];
    if (ioctl (so.sound_fd, SNDCTL_DSP_SPEED, &so.playback_rate) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SPEED");
	return (FALSE);
    }

    S9xSetPlaybackRate (so.playback_rate);

    if (buffer_size == 0)
	buffer_size = BufferSizes [mode & 7];

    if (buffer_size > MAX_BUFFER_SIZE / 4)
	buffer_size = MAX_BUFFER_SIZE / 4;
    if (so.sixteen_bit)
	buffer_size *= 2;
    if (so.stereo)
	buffer_size *= 2;

    int power2 = log2 (buffer_size);
    J = K = power2 | (3 << 16);
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETFRAGMENT, &J) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SETFRAGMENT");
	return (FALSE);
    }
    ioctl (so.sound_fd, SNDCTL_DSP_GETBLKSIZE, &so.buffer_size);
    
#ifdef MMAP_SOUND
    J = PCM_ENABLE_OUTPUT;
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETTRIGGER, &J) < 0)
	perror ("ioctl SNDCTL_DSP_SETTRIGGER");
#endif

#if 0
    buffmem_desc buff;
    buff.size = so.buffer_size * 3;
    buff.buffer = ptr;
    if (ioctl (so.sound_fd, SNDCTL_DSP_MAPOUTBUF, &buff) < 0)
	perror ("ioctl SNDCTL_DSP_MAPOUTBUF");
#endif

    printf ("Rate: %d, Buffer size: %d, 16-bit: %s, Stereo: %s, Encoded: %s\n",
	    so.playback_rate, so.buffer_size, so.sixteen_bit ? "yes" : "no",
	    so.stereo ? "yes" : "no", so.encoded ? "yes" : "no");

    return (TRUE);
}
#endif


#if defined (__linux) || defined (__sun) || defined(NOSOUND)
void S9xUnixProcessSound (void)
{
}

static uint8 Buf[MAX_BUFFER_SIZE];

#define FIXED_POINT 0x10000
#define FIXED_POINT_SHIFT 16
#define FIXED_POINT_REMAINDER 0xffff

static volatile bool8 block_signal = FALSE;
static volatile bool8 block_generate_sound = FALSE;
static volatile bool8 pending_signal = FALSE;
#endif

#if defined(NOSOUND)
static int Rates[8] =
{
    0, 8000, 11025, 16000, 22050, 32000, 44100, 48000
};

static int BufferSizes [8] =
{
    0, 256, 256, 256, 512, 512, 1024, 1024
};

bool8 S9xOpenSoundDevice (int mode, bool8 stereo, int buffer_size)
{
  return false;
}

void S9xGenerateSound ()
{
}

void *S9xProcessSound (void *)
{
}
#endif

#if defined (__linux) || defined (__sun)
void S9xGenerateSound ()
{
    int bytes_so_far = so.sixteen_bit ? (so.samples_mixed_so_far << 1) :
				        so.samples_mixed_so_far;
    if (Settings.SoundSync == 2)
    {
	// Assumes sound is signal driven
	while (so.samples_mixed_so_far >= so.buffer_size && !so.mute_sound)
	    pause ();
    }
    else
    if (bytes_so_far >= so.buffer_size)
	return;

#ifdef USE_THREADS
    if (Settings.ThreadSound)
    {
	if (block_generate_sound || pthread_mutex_trylock (&mutex))
	    return;
    }
#endif

    block_signal = TRUE;

    so.err_counter += so.err_rate;
    if (so.err_counter >= FIXED_POINT)
    {
        int sample_count = so.err_counter >> FIXED_POINT_SHIFT;
	int byte_offset;
	int byte_count;

        so.err_counter &= FIXED_POINT_REMAINDER;
	if (so.stereo)
	    sample_count <<= 1;
	byte_offset = bytes_so_far + so.play_position;
	    
	do
	{
	    int sc = sample_count;
	    byte_count = sample_count;
	    if (so.sixteen_bit)
		byte_count <<= 1;
	    
	    if ((byte_offset & SOUND_BUFFER_SIZE_MASK) + byte_count > SOUND_BUFFER_SIZE)
	    {
		sc = SOUND_BUFFER_SIZE - (byte_offset & SOUND_BUFFER_SIZE_MASK);
		byte_count = sc;
		if (so.sixteen_bit)
		    sc >>= 1;
	    }
	    if (bytes_so_far + byte_count > so.buffer_size)
	    {
		byte_count = so.buffer_size - bytes_so_far;
		if (byte_count == 0)
		    break;
		sc = byte_count;
		if (so.sixteen_bit)
		    sc >>= 1;
	    }
	    S9xMixSamplesO (Buf, sc,
			    byte_offset & SOUND_BUFFER_SIZE_MASK);
	    so.samples_mixed_so_far += sc;
	    sample_count -= sc;
	    bytes_so_far = so.sixteen_bit ? (so.samples_mixed_so_far << 1) :
	 	           so.samples_mixed_so_far;
	    byte_offset += byte_count;
	} while (sample_count > 0);
    }
    block_signal = FALSE;

#ifdef USE_THREADS
    if (Settings.ThreadSound)
	pthread_mutex_unlock (&mutex);
    else
#endif    
    if (pending_signal)
    {
	S9xProcessSound (NULL);
	pending_signal = FALSE;
    }
}

void *S9xProcessSound (void *)
{
#ifdef __linux
    audio_buf_info info;

    if (!Settings.ThreadSound &&
	(ioctl (so.sound_fd, SNDCTL_DSP_GETOSPACE, &info) == -1 ||
	 info.bytes < so.buffer_size))
    {
	return (NULL);
    }
#ifdef MMAP_SOUND
    count_info count;

    if (ioctl (so.sound_fd, SNDCTL_DSP_GETOPTR, &count) < 0)
    {
	printf ("F"); fflush (stdout);
    }
    else
    {
	printf ("<%d,%d>", count.blocks, count.bytes); fflush (stdout);
    	return (NULL);
    }
#endif

#endif
#ifdef __sun
    audio_info_t audio;
    if (!Settings.ThreadSound)
    {
	if (ioctl (so.sound_fd, AUDIO_GETINFO, &audio) < 0)
	    return (NULL);

	if (audio.play.eof == 0)
	    so.last_eof = -2;
	else
	if (audio.play.eof == so.last_eof)
	    return (NULL);

	so.last_eof++;
    }
#endif

#ifdef USE_THREADS
    do
    {
#endif

    int sample_count = so.buffer_size;
    int byte_offset;

    if (so.sixteen_bit)
	sample_count >>= 1;
 
#ifdef USE_THREADS
    if (Settings.ThreadSound)
	pthread_mutex_lock (&mutex);
    else
#endif
    if (block_signal)
    {
	pending_signal = TRUE;
	return (NULL);
    }

    block_generate_sound = TRUE;

    if (so.samples_mixed_so_far < sample_count)
    {
	byte_offset = so.play_position + 
		      (so.sixteen_bit ? (so.samples_mixed_so_far << 1)
				      : so.samples_mixed_so_far);

//printf ("%d:", sample_count - so.samples_mixed_so_far); fflush (stdout);
	if (Settings.SoundSync == 2)
	{
	    /*memset (Buf + (byte_offset & SOUND_BUFFER_SIZE_MASK), 0,
		    sample_count - so.samples_mixed_so_far);*/
	}
	else
	    S9xMixSamplesO (Buf, sample_count - so.samples_mixed_so_far,
			    byte_offset & SOUND_BUFFER_SIZE_MASK);
	so.samples_mixed_so_far = 0;
    }
    else
	so.samples_mixed_so_far -= sample_count;
    
//    if (!so.mute_sound)
    {
	int I;
	int J = so.buffer_size;

	byte_offset = so.play_position;
	so.play_position = (so.play_position + so.buffer_size) & SOUND_BUFFER_SIZE_MASK;

#ifdef USE_THREADS
	if (Settings.ThreadSound)
	    pthread_mutex_unlock (&mutex);
#endif
	block_generate_sound = FALSE;
	do
	{
	    if (byte_offset + J > SOUND_BUFFER_SIZE)
	    {
		I = write (so.sound_fd, (char *) Buf + byte_offset,
			   SOUND_BUFFER_SIZE - byte_offset);
		if (I > 0)
		{
		    J -= I;
		    byte_offset = (byte_offset + I) & SOUND_BUFFER_SIZE_MASK;
		}
	    }
	    else
	    {
		I = write (so.sound_fd, (char *) Buf + byte_offset, J);
		if (I > 0)
		{
		    J -= I;
		    byte_offset = (byte_offset + I) & SOUND_BUFFER_SIZE_MASK;
		}
	    }
	} while ((I < 0 && errno == EINTR) || J > 0);
    }

#ifdef USE_THREADS
    } while (Settings.ThreadSound);
#endif

#ifdef __sun
    if (!Settings.ThreadSound)
	write (so.sound_fd, NULL, 0);
#endif

    return (NULL);
}
#endif

uint32 S9xReadJoypad (int which1)
{
#ifdef _NETPLAY_SUPPORT
    if (Settings.NetPlay)
	return (S9xNetPlayGetJoypad (which1));
#endif
    if (which1 < NumControllers)
	return (0x80000000 | joypads [which1]);
    return (0);
}

#ifdef __sun
uint8 int2ulaw(int ch)
{
    int mask;

    if (ch < 0) {
      ch = -ch;
      mask = 0x7f;
    }
    else {
      mask = 0xff;
    }

    if (ch < 32) {
	ch = 0xF0 | ( 15 - (ch/2) );
    } else if (ch < 96) {
        ch = 0xE0 | ( 15 - (ch-32)/4 );
    } else if (ch < 224) {
	ch = 0xD0 | ( 15 - (ch-96)/8 );
    } else if (ch < 480) {
	ch = 0xC0 | ( 15 - (ch-224)/16 );
    } else if (ch < 992 ) {
	ch = 0xB0 | ( 15 - (ch-480)/32 );
    } else if (ch < 2016) {
	ch = 0xA0 | ( 15 - (ch-992)/64 );
    } else if (ch < 4064) {
	ch = 0x90 | ( 15 - (ch-2016)/128 );
    } else if (ch < 8160) {
	ch = 0x80 | ( 15 - (ch-4064)/256 );
    } else {
	ch = 0x80;
    }
    return (uint8)(mask & ch);
}
#endif

#if 0
void S9xParseConfigFile ()
{
    int i, t = 0;
    char *b, buf[10];
    struct ffblk f;

    set_config_file("SNES9X.CFG");

    if (findfirst("SNES9X.CFG", &f, 0) != 0)
    {
        set_config_int("Graphics", "VideoMode", -1);
        set_config_int("Graphics", "AutoFrameskip", 1);
        set_config_int("Graphics", "Frameskip", 0);
        set_config_int("Graphics", "Shutdown", 1);
        set_config_int("Graphics", "FrameTimePAL", 20000);
        set_config_int("Graphics", "FrameTimeNTSC", 16667);
        set_config_int("Graphics", "Transparency", 0);
        set_config_int("Graphics", "HiColor", 0);
        set_config_int("Graphics", "Hi-ResSupport", 0);
        set_config_int("Graphics", "CPUCycles", 100);
        set_config_int("Graphics", "Scale", 0);
        set_config_int("Graphics", "VSync", 0);
        set_config_int("Sound", "APUEnabled", 1);
        set_config_int("Sound", "SoundPlaybackRate", 4);
        set_config_int("Sound", "Stereo", 1);
        set_config_int("Sound", "SoundBufferSize", 256);
        set_config_int("Sound", "SPCToCPURatio", 2);
        set_config_int("Sound", "Echo", 1);
        set_config_int("Sound", "SampleCaching", 1);
        set_config_int("Sound", "MasterVolume", 1);
        set_config_int("Peripherals", "Mouse", 1);
        set_config_int("Peripherals", "SuperScope", 1);
        set_config_int("Peripherals", "MultiPlayer5", 1);
        set_config_int("Peripherals", "Controller", 0);
        set_config_int("Controllers", "Type", JOY_TYPE_AUTODETECT);
        set_config_string("Controllers", "Button1", "A");
        set_config_string("Controllers", "Button2", "B");
        set_config_string("Controllers", "Button3", "X");
        set_config_string("Controllers", "Button4", "Y");
        set_config_string("Controllers", "Button5", "TL");
        set_config_string("Controllers", "Button6", "TR");
        set_config_string("Controllers", "Button7", "START");
        set_config_string("Controllers", "Button8", "SELECT");
        set_config_string("Controllers", "Button9", "NONE");
        set_config_string("Controllers", "Button10", "NONE");
    }

    mode = get_config_int("Graphics", "VideoMode", -1);
    Settings.SkipFrames = get_config_int("Graphics", "AutoFrameskip", 1);
    if (!Settings.SkipFrames)
       Settings.SkipFrames = get_config_int("Graphics", "Frameskip", AUTO_FRAMERATE);
    else
       Settings.SkipFrames = AUTO_FRAMERATE;
    Settings.ShutdownMaster = get_config_int("Graphics", "Shutdown", TRUE);
    Settings.FrameTimePAL = get_config_int("Graphics", "FrameTimePAL", 20000);
    Settings.FrameTimeNTSC = get_config_int("Graphics", "FrameTimeNTSC", 16667);
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.Transparency = get_config_int("Graphics", "Transparency", FALSE);
    Settings.SixteenBit = get_config_int("Graphics", "HiColor", FALSE);
    Settings.SupportHiRes = get_config_int("Graphics", "Hi-ResSupport", FALSE);
    i = get_config_int("Graphics", "CPUCycles", 100);
    Settings.H_Max = (i * SNES_CYCLES_PER_SCANLINE) / i;
    stretch = get_config_int("Graphics", "Scale", 0);
    _vsync = get_config_int("Graphics", "VSync", 0);

    Settings.APUEnabled = get_config_int("Sound", "APUEnabled", TRUE);
    Settings.SoundPlaybackRate = get_config_int("Sound", "SoundPlaybackRate", 4);
    Settings.Stereo = get_config_int("Sound", "Stereo", TRUE);
    Settings.SoundBufferSize = get_config_int("Sound", "SoundBufferSize", 256);
    Settings.SPCTo65c816Ratio = get_config_int("Sound", "SPCToCPURatio", 2);
    Settings.DisableSoundEcho = get_config_int("Sound", "Echo", TRUE) ? FALSE : TRUE;
    Settings.DisableSampleCaching = get_config_int("Sound", "SampleCaching", TRUE) ? FALSE : TRUE;
    Settings.DisableMasterVolume = get_config_int("Sound", "MasterVolume", TRUE) ? FALSE : TRUE;

    Settings.Mouse = get_config_int("Peripherals", "Mouse", TRUE);
    Settings.SuperScope = get_config_int("Peripherals", "SuperScope", TRUE);
    Settings.MultiPlayer5 = get_config_int("Peripherals", "MultiPlayer5", TRUE);
    Settings.ControllerOption = (uint32)get_config_int("Peripherals", "Controller", SNES_MULTIPLAYER5);

    joy_type = get_config_int("Controllers", "Type", JOY_TYPE_AUTODETECT);
    for (i = 0; i < 10; i++)
    {
        sprintf(buf, "Button%d", i+1);
        b = get_config_string("Controllers", buf, "NONE");
        if (!strcasecmp(b, "A"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_A_MASK;}
        else if (!strcasecmp(b, "B"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_B_MASK;}
        else if (!strcasecmp(b, "X"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_X_MASK;}
        else if (!strcasecmp(b, "Y"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_Y_MASK;}
        else if (!strcasecmp(b, "TL"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_TL_MASK;}
        else if (!strcasecmp(b, "TR"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_TR_MASK;}
        else if (!strcasecmp(b, "START"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_START_MASK;}
        else if (!strcasecmp(b, "SELECT"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_SELECT_MASK;}
    }
}
#endif

static int S9xCompareSDD1IndexEntries (const void *p1, const void *p2)
{
    return (*(uint32 *) p1 - *(uint32 *) p2);
}

void S9xLoadSDD1Data ()
{
    char filename [_MAX_PATH + 1];
    char index [_MAX_PATH + 1];
    char data [_MAX_PATH + 1];
    char patch [_MAX_PATH + 1];

    Memory.FreeSDD1Data ();

    strcpy (filename, S9xGetSnapshotDirectory ());

    Settings.SDD1Pack=FALSE;
    if (strncmp (Memory.ROMName, "Star Ocean", 10) == 0){
        if(SDD1_pack) strcpy (filename, SDD1_pack);
#ifdef SDD1_DECOMP
        else Settings.SDD1Pack=TRUE;
#else
	strcat (filename, "/socnsdd1");
#endif
    } else if(strncmp(Memory.ROMName, "STREET FIGHTER ALPHA2", 21)==0){
        if(SDD1_pack) strcpy (filename, SDD1_pack);
#ifdef SDD1_DECOMP
        else Settings.SDD1Pack=TRUE;
#else
	strcat (filename, "/sfa2sdd1");
#endif
    } else {
        if(SDD1_pack) strcpy (filename, SDD1_pack);
#ifdef SDD1_DECOMP
        else Settings.SDD1Pack=TRUE;
#else
	S9xMessage(S9X_WARNING, S9X_ROM_INFO, "WARNING: No default SDD1 pack for this ROM");
#endif
    }

    if(Settings.SDD1Pack) return;

    DIR *dir = opendir (filename);

    index [0] = 0;
    data [0] = 0;
    patch [0] = 0;

    if (dir)
    {
	struct dirent *d;
	
	while ((d = readdir (dir)))
	{
	    if (strcasecmp (d->d_name, "SDD1GFX.IDX") == 0)
	    {
		strcpy (index, filename);
		strcat (index, "/");
		strcat (index, d->d_name);
	    }
	    else
	    if (strcasecmp (d->d_name, "SDD1GFX.DAT") == 0)
	    {
		strcpy (data, filename);
		strcat (data, "/");
		strcat (data, d->d_name);
	    }
	    if (strcasecmp (d->d_name, "SDD1GFX.PAT") == 0)
	    {
		strcpy (patch, filename);
		strcat (patch, "/");
		strcat (patch, d->d_name);
	    }
	}
	closedir (dir);

	if (strlen (index) && strlen (data))
	{
	    FILE *fs = fopen (index, "rb");
	    int len = 0;

	    if (fs)
	    {
		// Index is stored as a sequence of entries, each entry being
		// 12 bytes consisting of:
		// 4 byte key: (24bit address & 0xfffff * 16) | translated block
		// 4 byte ROM offset
		// 4 byte length
		fseek (fs, 0, SEEK_END);
		len = ftell (fs);
		rewind (fs);
		Memory.SDD1Index = (uint8 *) malloc (len);
		fread (Memory.SDD1Index, 1, len, fs);
		fclose (fs);
		Memory.SDD1Entries = len / 12;

		if (!(fs = fopen (data, "rb")))
		{
		    free ((char *) Memory.SDD1Index);
		    Memory.SDD1Index = NULL;
		    Memory.SDD1Entries = 0;
		}
		else
		{
		    fseek (fs, 0, SEEK_END);
		    len = ftell (fs);
		    rewind (fs);
		    Memory.SDD1Data = (uint8 *) malloc (len);
		    fread (Memory.SDD1Data, 1, len, fs);
		    fclose (fs);

		    if (strlen (patch) > 0 &&
			(fs = fopen (patch, "rb")))
		    {
			fclose (fs);
		    }
#ifdef MSB_FIRST
		    // Swap the byte order of the 32-bit value triplets on
		    // MSBFirst machines.
		    uint8 *ptr = Memory.SDD1Index;
		    for (int i = 0; i < Memory.SDD1Entries; i++, ptr += 12)
		    {
			SWAP_DWORD ((*(uint32 *) (ptr + 0)));
			SWAP_DWORD ((*(uint32 *) (ptr + 4)));
			SWAP_DWORD ((*(uint32 *) (ptr + 8)));
		    }
#endif
		    qsort (Memory.SDD1Index, Memory.SDD1Entries, 12,
			   S9xCompareSDD1IndexEntries);
		}
	    }
	}
	else
	{
	    printf ("Decompressed data pack not found in '%s'.\n", filename);
	}
    }
}

