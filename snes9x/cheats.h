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
#ifndef _CHEATS_H_
#define _CHEATS_H_

struct SCheat
{
    uint32  address;
    uint8   byte;
    uint8   saved_byte;
    bool8   enabled;
    bool8   saved;
    char    name [22];
};

#define MAX_CHEATS 75

struct SCheatData
{
    struct SCheat   c [MAX_CHEATS];
    uint32	    num_cheats;
    uint8	    CWRAM [0x20000];
    uint8	    CSRAM [0x10000];
    uint8	    CIRAM [0x2000];
    uint8           *RAM;
    uint8           *FillRAM;
    uint8           *SRAM;
    uint32	    WRAM_BITS [0x20000 >> 3];
    uint32	    SRAM_BITS [0x10000 >> 3];
    uint32	    IRAM_BITS [0x2000 >> 3];
};

typedef enum
{
    S9X_LESS_THAN, S9X_GREATER_THAN, S9X_LESS_THAN_OR_EQUAL,
    S9X_GREATER_THAN_OR_EQUAL, S9X_EQUAL, S9X_NOT_EQUAL
} S9xCheatComparisonType;

typedef enum
{
    S9X_8_BITS, S9X_16_BITS, S9X_24_BITS, S9X_32_BITS
} S9xCheatDataSize;

void S9xInitCheatData ();

const char *S9xGameGenieToRaw (const char *code, uint32 &address, uint8 &byte);
const char *S9xProActionReplayToRaw (const char *code, uint32 &address, uint8 &byte);
const char *S9xGoldFingerToRaw (const char *code, uint32 &address, bool8 &sram,
				uint8 &num_bytes, uint8 bytes[3]);
void S9xApplyCheats ();
void S9xApplyCheat (uint32 which1);
void S9xRemoveCheats ();
void S9xRemoveCheat (uint32 which1);
void S9xEnableCheat (uint32 which1);
void S9xDisableCheat (uint32 which1);
void S9xAddCheat (bool8 enable, bool8 save_current_value, uint32 address,
		  uint8 byte);
void S9xDeleteCheats ();
void S9xDeleteCheat (uint32 which1);
bool8 S9xLoadCheatFile (const char *filename);
bool8 S9xSaveCheatFile (const char *filename);

void S9xStartCheatSearch (SCheatData *);
void S9xSearchForChange (SCheatData *, S9xCheatComparisonType cmp,
                         S9xCheatDataSize size, bool8 is_signed, bool8 update);
void S9xSearchForValue (SCheatData *, S9xCheatComparisonType cmp,
                        S9xCheatDataSize size, uint32 value,
                        bool8 is_signed, bool8 update);
void S9xOutputCheatSearchResults (SCheatData *);

#endif
