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
#ifndef _SIDEWINDER_H
#define _SIDEWINDER_H

#define SW_MAX_SIDEWINDERS  10

struct SW_READ_DATA
{
    unsigned int count;
    unsigned int buttons [SW_MAX_SIDEWINDERS];
    unsigned char valid [SW_MAX_SIDEWINDERS];
};

#define SW_TRUE		    1
#define SW_FALSE	    0

#define SW_NUM_SIDEWINDER_BUTTONS   14

#define SW_UP_MASK		    (1 << 0)
#define SW_DOWN_MASK		    (1 << 1)
#define SW_RIGHT_MASK		    (1 << 2)
#define SW_LEFT_MASK		    (1 << 3)
#define SW_A_MASK		    (1 << 4)
#define SW_B_MASK		    (1 << 5)
#define SW_C_MASK		    (1 << 6)
#define SW_X_MASK		    (1 << 7)
#define SW_Y_MASK		    (1 << 8)
#define SW_Z_MASK		    (1 << 9)
#define SW_L_MASK		    (1 << 10)
#define SW_R_MASK		    (1 << 11)
#define SW_START_MASK		    (1 << 12)
#define SW_M_MASK		    (1 << 13)
#define SW_PARITY_MASK		    (1 << 14)

#endif /* _SIDEWINDER_H */
