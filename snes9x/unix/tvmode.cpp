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
void TVMode (uint8 *srcPtr, uint32 srcPitch, uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height)
{
    uint8 *nextLine, *finish;
    uint32 colorMask = ~(RGB_LOW_BITS_MASK | (RGB_LOW_BITS_MASK << 16));
    uint32 lowPixelMask = RGB_LOW_BITS_MASK;

    nextLine = dstPtr + dstPitch;

    if (width == 256)
    {
	do
	{
	    uint32 *bP = (uint32 *) srcPtr;
	    uint32 *xP = (uint32 *) deltaPtr;
	    uint32 *dP = (uint32 *) dstPtr;
	    uint32 *nL = (uint32 *) nextLine;
	    uint32 currentPixel;
	    uint32 nextPixel;
	    uint32 currentDelta;
	    uint32 nextDelta;

	    finish = (uint8 *) bP + ((width + 2) << 1);
	    nextPixel = *bP++;
	    nextDelta = *xP++;

	    do
	    {
		currentPixel = nextPixel;
		currentDelta = nextDelta;
		nextPixel = *bP++;
		nextDelta = *xP++;

		if ((nextPixel != nextDelta) || (currentPixel != currentDelta))
		{
		    uint32 colorA, colorB, product, darkened;

		    *(xP - 2) = currentPixel;
#ifdef LSB_FIRST
		    colorA = currentPixel & 0xffff;
#else
		    colorA = (currentPixel & 0xffff0000) >> 16;
#endif

#ifdef LSB_FIRST
		    colorB = (currentPixel & 0xffff0000) >> 16;
		    *(dP) = product = colorA |
				      ((((colorA & colorMask) >> 1) +
					((colorB & colorMask) >> 1) +
					(colorA & colorB & lowPixelMask)) << 16);
#else
		    colorB = currentPixel & 0xffff;
		    *(dP) = product = (colorA << 16) | 
				      (((colorA & colorMask) >> 1) +
				       ((colorB & colorMask) >> 1) +
				       (colorA & colorB & lowPixelMask));
#endif
		    darkened = (product = ((product & colorMask) >> 1));
		    darkened += (product = ((product & colorMask) >> 1));
		    darkened += (product & colorMask) >> 1;
		    *(nL) = darkened;

#ifdef LSB_FIRST
		    colorA = nextPixel & 0xffff;
		    *(dP + 1) = product = colorB |
					  ((((colorA & colorMask) >> 1) +
					    ((colorB & colorMask) >> 1) +
					    (colorA & colorB & lowPixelMask)) << 16);
#else
		    colorA = (nextPixel & 0xffff0000) >> 16;
		    *(dP + 1) = product = (colorB << 16) | 
					   (((colorA & colorMask) >> 1) +
					    ((colorB & colorMask) >> 1) + 
					    (colorA & colorB & lowPixelMask));
#endif
		    darkened = (product = ((product & colorMask) >> 1));
		    darkened += (product = ((product & colorMask) >> 1));
		    darkened += (product & colorMask) >> 1;
		    *(nL + 1) = darkened;
		}

		dP += 2;
		nL += 2;
	    }
	    while ((uint8 *) bP < finish);

	    deltaPtr += srcPitch;
	    srcPtr += srcPitch;
	    dstPtr += dstPitch * 2;
	    nextLine += dstPitch * 2;
	}
	while (--height);
    }
    else
    {
	do
	{
	    uint32 *bP = (uint32 *) srcPtr;
	    uint32 *xP = (uint32 *) deltaPtr;
	    uint32 *dP = (uint32 *) dstPtr;
	    uint32 currentPixel;

	    finish = (uint8 *) bP + ((width + 2) << 1);

	    do
	    {
		currentPixel = *bP++;

		if (currentPixel != *xP++)
		{
		    uint32 product, darkened;

		    *(xP - 1) = currentPixel;
		    *dP = currentPixel;
		    darkened = (product = ((currentPixel & colorMask) >> 1));
		    darkened += (product = ((product & colorMask) >> 1));
		    darkened += (product & colorMask) >> 1;
		    *(uint32 *) ((uint8 *) dP + dstPitch) = darkened;
		}

		dP++;
	    }
	    while ((uint8 *) bP < finish);

	    deltaPtr += srcPitch;
	    srcPtr += srcPitch;
	    dstPtr += dstPitch * 2;
	}
	while (--height);
    }
}
