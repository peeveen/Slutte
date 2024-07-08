// Slutte!
// A Bionic Commando emulator by Steven Frew

// All versions are named after famous Captains, in alphabetical order.
// This is the name of the current revision.
char *revision="Beefheart";

// Define this to make the debugging interface open at startup and shutdown.
// The debugging interface requires that the Allegro keyboard handler be
// left out, so you can't really play the game with this defined.
//#define DEBUG

// Define these to create dumps of the RAM and/or ROM on exit.
#define RAMDUMP
//#define ROMDUMP

// Until I can get the player 1 and 2 START controls working correctly, I'm
// having to resort to poking the code. This isn't very impressive, but it
// works for now. Defining this uses this "cheat" rather than the "noble"
// method.
#define STARTCHEAT

// Define this to prefer correct char colors over some sprite colours.
//#define GETCHARPALETTE

// Define this to use 256x240 as the ModeX res, not 320x240. According to
// Allegro docs, it should work just as well for everyone, but it said that
// about the joystick code ... :)
//#define FINESIZE

// Simplify matters: if RAMDUMP or ROMDUMP are defined, define DUMP.
#ifdef RAMDUMP
#define DUMP
#endif
#ifdef ROMDUMP
#ifndef DUMP
#define DUMP
#endif
#endif

// Definitions to make allegro.h miss out stuff we don't want. Apparently
// this makes for slightly speedier compilation.
#define alleg_mouse_unused
#define alleg_timer_unused
#define alleg_flic_unused
#define alleg_math_unused
#define alleg_gui_unused

// Definitions for stuff we DO want.
#include <stdio.h>             // printf(), fopen(), etc
#include <string.h>            // stricmp()
#include <malloc.h>            // memcpy(), malloc() and free()
#include <allegro.h>           // Screen modes, joystick and keyboard.
#include <sys/nearptr.h>       // For easy linear frame buffer access.
#include "Slutte!.h"           // Slutte!'s own stuff.
#include "StarScrm\StarCPU.h"  // The 68000 emulator definitions.
#ifdef DEBUG
#include "StarScrm\CPUDebug.h" // The 68000 emulator debugging commands.
#endif

// Global variables
// All the ROM data goes into these bits of allocated memory.
unsigned char *rom=NULL,*ram=NULL,*sprites=NULL,*chars=NULL,*tiles=NULL,*music=NULL;
// These are the screen buffers we use for drawing graphics to.
unsigned char *scroll1vscreen=NULL,*scroll2vscreen=NULL,*vramvscreen=NULL,*vscreen=NULL;
// These hold the addresses of the last scanlines of each SCROLL layers
// virtual screen.
unsigned char *scroll1vscreenlastline=NULL,*scroll2vscreenlastline=NULL;
// These are the current x/y coordinates of each SCROLL layer.
signed int scroll1xpos,scroll1ypos,scroll2xpos,scroll2ypos;
// The game configuration.
SLUTTECONFIG config;
// Screen dimensions, same as from config, but easier for the assembly
// functions to access if we put them out here in plain view.
unsigned int screenwidth,screenheight;
// When new values are written to tile RAM, this points to the address in the
// tile RAM that the data was written to. Assembly functions use this address
// to draw the correct graphics on the virtual screen.
unsigned newgfxaddress;
// Pointer to the video memory (lfb means linear frame buffer, but it needn't
// be a linear frame buffer. It might be ModeX, for example).
unsigned char *lfb;
// This is the number of clock-cycles-worth of instructions to execute when
// we call 68000exec().
signed int clocktickspercall=100000;
// This is set to TRUE if SCROLL2 is in front of SCROLL1.
unsigned char foregroundscroll2=FALSE;
// This is set to TRUE whenever the palette RAM is modified. This way, we
// can tell when the VGA palette needs updated.
BOOL updatepalette=FALSE;
// If the user calibrates the joystick before playing, there are certain
// joystick-initialisation routines we won't want to call later in the code.
// This tells us whether the calibration was done or not.
BOOL calibrated=FALSE;
// These are the names of the other files that Slutte! uses.
char *configfile="Slutte!.CFG";
char *calibrationfile="Slutte!.CLB";
char *highscorefile="Slutte!.HI";
// To save a few bytes, these are the file access modes. Petty, I know.
char *moderead="rb",*modewrite="wb";
// So that we can create numbered screen-grabs, this keeps count.
unsigned char snapcounter=0;
// If the user runs "Slutte! /?", we don't want to run the game. This is set
// to TRUE if the game should go ahead, FALSE otherwise.
BOOL PlayGame=TRUE;
// There's a certain point where we want to load high scores (not at the very
// start). This is set when that point is reached.
signed char highscorestatus=0;
// The VGA palette, and the address of it (for assembly functions to use).
PALETTE *BCPaletteAddress,BCPalette;

// Assembly functions
extern "C" void buildscreen_(void);
extern "C" void showscreenvesa2l_(void);
extern "C" void showscreenmodex_(void);
extern "C" void buildpalette_(void);
extern "C" void newscroll1tile_(void);
extern "C" void newscroll2tile_(void);
extern "C" void newchartile_(void);

// 68000 callback functions.
void Port801A(unsigned,unsigned);
void DoNothing(unsigned,unsigned);
void WriteScrollWord(unsigned,unsigned);
void WriteCharWord(unsigned,unsigned);
void WritePaletteWord(unsigned,unsigned);

// Dynamic function pointers.
void (*GetControls)(void);
void (*ShowScreen)(void);

// Only one area of memory from which code is executed: ROM!
struct S68000MEMORYFETCH memoryfetch[]=
{
  {0x00000000,0x0003FFFF,NULL},
  {0xFFFFFFFF,0xFFFFFFFF,NULL}  // This signifies the end of the array
};

// The 68000 can read bytes from the ROM and from the RAM.
struct S68000MEMORYIO memoryreadbyte[]=
{
  {0x00000000,0x0003FFFF,NULL,NULL},
  {0x00FE0000,0x00FFFFFF,NULL,NULL},
  {0xFFFFFFFF,0xFFFFFFFF,NULL,NULL}
};

// The 68000 can also read words from the ROM and from the RAM.
struct S68000MEMORYIO memoryreadword[]=
{
  {0x00000000,0x0003FFFF,NULL,NULL},
  {0x00FE0000,0x00FFFFFF,NULL,NULL},
  {0xFFFFFFFF,0xFFFFFFFF,NULL,NULL}
};

// It can only write words to RAM (not ROM). However, the code often tries
// to write to the area of RAM which contains dipswitch values. If this
// happens, don't do it. Chances are that the code which does this isn't
// trying to write over the dipswitch values. The address is probably R/W
// mapped (different purposes for both reading and writing). Note that when
// an address is read from or written to, the 68000 code checks down this
// list, and if it finds a matching address range, it goes no further, so
// we can put the whole RAM at the end of a list of special entries.
struct S68000MEMORYIO memorywritebyte[]=
{
  {0x00FE4002,0x00FE4003,DoNothing,NULL},  // Overruled!
  {0x00FE0000,0x00FFFFFF,NULL,NULL},     // The whole RAM
  {0xFFFFFFFF,0xFFFFFFFF,NULL,NULL}
};

// Words can only be written to RAM. However, if the Palette RAM, the SCROLL
// RAM, or the Character RAM are written to, we want to know so that we can
// update the graphics in the relevant layer, or update the palette. Also,
// if address 00FE801A is written to, this is our cue to update the screen
// and read the controls.
struct S68000MEMORYIO memorywriteword[]=
{
  {0x00FE801A,0x00FE801A,Port801A,NULL},
  {0x00FEC000,0x00FECFFF,WriteCharWord,NULL},
  {0x00FF0000,0x00FF7FFF,WriteScrollWord,NULL},
  {0x00FE0000,0x00FFFFFF,NULL,NULL}, // replaced later
  {0x00FE0000,0x00FFFFFF,NULL,NULL},
  {0xFFFFFFFF,0xFFFFFFFF,NULL,NULL}
};

/*---------------------------------------------------------
            Slutte! - Bionic Commando Emulator
---------------------------------------------------------*/

void WritePaletteWord(unsigned address,unsigned data)
{
  address&=0x1FFFE; // only write to even addresses.
  // If the palette entry is changing, set updatepalette to TRUE, so that
  // we know to rebuild the VGA palette.
  if((ram[address+1]!=((data>>8)&255))||(ram[address]!=(data&255)))
    updatepalette=TRUE;
  // Change the palette entry.
  ram[address+1]=(data>>8)&255;
  ram[address]=data&255;
}

void DoNothing(unsigned address,unsigned data)
{
  // This is an empty stub for memory writes that we want to ignore.
}

void WriteCharWord(unsigned address,unsigned data)
{
  address&=0x1FFFE; // only write to even addresses.
  ram[address+1]=(data>>8)&255;
  ram[address]=data&255;
  address&=0xC7FF;  // Just get the relevant bit of the address.
  newgfxaddress=address;
  newchartile_();
}

void WriteScrollWord(unsigned address,unsigned data)
{
  address&=0x1FFFE; // only write to even addresses.
  ram[address+1]=(data>>8)&255;
  ram[address]=data&255;
  newgfxaddress=address;
  if(address&0x04000)
    newscroll2tile_();
  else
    newscroll1tile_();
}

void Port801A(unsigned address,unsigned data)
{
  // If this "port" is written to, do all the hard work.
  buildscreen_();
  if(updatepalette)
  {
    buildpalette_();
    set_palette(BCPalette);
    updatepalette=FALSE;
  }
  else if(config.waitvbl)
    vsync();
  ShowScreen();
  ram[0x1FFFA]=ram[0x1FFFE]=0;
  GetControls();
  if(key[KEY_1])
    ram[0x1FFFA]|=2;
  if(key[KEY_2])
    ram[0x1FFFA]|=1;
  if(key[KEY_3])
    ram[0x1FFFA]|=8;
  if(key[KEY_4])
    ram[0x1FFFA]|=4;
  ram[0x1FFFC]=ram[0x1FFFE];
  ram[0x1FFFD]=ram[0x1FFFF]=~ram[0x1FFFE];
}

#ifdef DUMP
void Dump(char *filename,unsigned char *mstart,unsigned char *mend)
{
  // For dumping memory to a file.
  FILE *fileID;
  size_t LengthActuallyWritten;
  if(fileID=fopen(filename,modewrite))
  {
    LengthActuallyWritten=fwrite(mstart,(mend-mstart)+1,1,fileID);
    fclose(fileID);
  }
}
#endif

void ResetStub(void)
{
  // "clocktickspercall" is provided to s68000exec, to tell it how long to
  // run for. To begin with, during the ROM/RAM checks, we want to run for a
  // fairly short period of time inbetween screen updates (10000, the
  // starting value), because we want to see the messages. After the checks,
  // BC performs a RESET instruction, at which point we set the clockticks
  // per call to the maximum available, because BC uses some kind of event
  // driven mechanism, and never processes more than one frames worth of
  // instructions before reaching a STOP instruction, which causes the
  // s68000exec function to return anyway. By doing this we're telling it to
  // run for as long as it takes to process the frame.
  clocktickspercall=2147483647;
  highscorestatus=-1; // Now we can load the high scores.
  // We now want to start paying attention to the palette, so we write a new
  // entry into the memorywriteword s68000MEMORYIO array to take care of
  // that. Currently, there are two duplicate entries for the whole RAM. We
  // rewrite one of them.
  memorywriteword[3].lowaddr=0x00FF8000;
  memorywriteword[3].highaddr=0x00FF867F;
  memorywriteword[3].memorycall=WritePaletteWord;
  memorywriteword[3].userdata=NULL;
}

// ROM loading routines

BOOL LoadLinearROM(char *filename,unsigned char *memlocation,size_t length)
{
  // For normal ROMs.
  FILE *fileID;
  size_t LengthActuallyRead;
  if(fileID=fopen(filename,moderead))
  {
    LengthActuallyRead=fread(memlocation,length,1,fileID);
    fclose(fileID);
    if(LengthActuallyRead==1)
      return TRUE;
  }
  return FALSE;
}

BOOL LoadSpriteROM(char *filename,unsigned char *memlocation,size_t length,unsigned char ppick)
{
  // Sprites are in bitplane formation.
  unsigned char *buffer=NULL;
  unsigned long f=0;
  unsigned char h;
  unsigned long temp;
  FILE *fileID;
  size_t LengthActuallyRead;
  buffer=(unsigned char *)malloc(length);
  if(buffer)
  {
    if(fileID=fopen(filename,moderead))
    {
      LengthActuallyRead=fread(buffer,length,1,fileID);
      fclose(fileID);
      if(LengthActuallyRead==1)
      {
        for(;f<length;f+=32)    // Every 32 bytes there is a 16x16 bitplane
        {
          for(h=0;h<16;++h)     // Through the 16 rows ...
          {
            if(!ppick)          // Clear the chunky bytes of this row
            {
              // Each row is 16 bytes
              memlocation[(f*8)+(16*h)+0]=memlocation[(f*8)+(16*h)+1]=
              memlocation[(f*8)+(16*h)+2]=memlocation[(f*8)+(16*h)+3]=
              memlocation[(f*8)+(16*h)+4]=memlocation[(f*8)+(16*h)+5]=
              memlocation[(f*8)+(16*h)+6]=memlocation[(f*8)+(16*h)+7]=
              memlocation[(f*8)+(16*h)+8]=memlocation[(f*8)+(16*h)+9]=
              memlocation[(f*8)+(16*h)+10]=memlocation[(f*8)+(16*h)+11]=
              memlocation[(f*8)+(16*h)+12]=memlocation[(f*8)+(16*h)+13]=
              memlocation[(f*8)+(16*h)+14]=memlocation[(f*8)+(16*h)+15]=0;
            }
            temp=(unsigned long)(buffer[f+h]<<16)|(unsigned long)(buffer[f+16+h]<<8); // the bits of this row
            memlocation[(f*8)+(16*h)+0]|=(temp&8388608)>>(23-ppick);
            memlocation[(f*8)+(16*h)+1]|=(temp&4194304)>>(22-ppick);
            memlocation[(f*8)+(16*h)+2]|=(temp&2097152)>>(21-ppick);
            memlocation[(f*8)+(16*h)+3]|=(temp&1048576)>>(20-ppick);
            memlocation[(f*8)+(16*h)+4]|=(temp&524288)>>(19-ppick);
            memlocation[(f*8)+(16*h)+5]|=(temp&262144)>>(18-ppick);
            memlocation[(f*8)+(16*h)+6]|=(temp&131072)>>(17-ppick);
            memlocation[(f*8)+(16*h)+7]|=(temp&65536)>>(16-ppick);
            memlocation[(f*8)+(16*h)+8]|=(temp&32768)>>(15-ppick);
            memlocation[(f*8)+(16*h)+9]|=(temp&16384)>>(14-ppick);
            memlocation[(f*8)+(16*h)+10]|=(temp&8192)>>(13-ppick);
            memlocation[(f*8)+(16*h)+11]|=(temp&4096)>>(12-ppick);
            memlocation[(f*8)+(16*h)+12]|=(temp&2048)>>(11-ppick);
            memlocation[(f*8)+(16*h)+13]|=(temp&1024)>>(10-ppick);
            memlocation[(f*8)+(16*h)+14]|=(temp&512)>>(9-ppick);
            memlocation[(f*8)+(16*h)+15]|=(temp&256)>>(8-ppick);
          }
        }
        return TRUE;
      }
    }
    free(buffer);
  }
  return FALSE;
}

BOOL LoadTileROM(char *filename,unsigned char *memlocation,size_t length,unsigned char ppick)
{
  // Tiles are in some wacky switched-nibble formation.
  unsigned char *buffer=NULL;
  unsigned long f=0;
  unsigned short temp;
  FILE *fileID;
  size_t LengthActuallyRead;
  buffer=(unsigned char *)malloc(length);
  if(buffer)
  {
    if(fileID=fopen(filename,moderead))
    {
      LengthActuallyRead=fread(buffer,length,1,fileID);
      fclose(fileID);
      if(LengthActuallyRead==1)
      {
        for(;f<length;f+=2)
        {
          if(!ppick)
          {
            memlocation[(f*4)]=0;
            memlocation[(f*4)+1]=0;
            memlocation[(f*4)+2]=0;
            memlocation[(f*4)+3]=0;
            memlocation[(f*4)+4]=0;
            memlocation[(f*4)+5]=0;
            memlocation[(f*4)+6]=0;
            memlocation[(f*4)+7]=0;
          }
          temp=buffer[f]<<8;
          memlocation[(f*4)+0]|=(temp&32768)>>(15-ppick);
          memlocation[(f*4)+0]|=(temp&2048)>>(10-ppick);
          memlocation[(f*4)+1]|=(temp&16384)>>(14-ppick);
          memlocation[(f*4)+1]|=(temp&1024)>>(9-ppick);
          memlocation[(f*4)+2]|=(temp&8192)>>(13-ppick);
          memlocation[(f*4)+2]|=(temp&512)>>(8-ppick);
          memlocation[(f*4)+3]|=(temp&4096)>>(12-ppick);
          memlocation[(f*4)+3]|=(temp&256)>>(7-ppick);
          temp=buffer[f+1]<<8;
          memlocation[(f*4)+4]|=(temp&32768)>>(15-ppick);
          memlocation[(f*4)+4]|=(temp&2048)>>(10-ppick);
          memlocation[(f*4)+5]|=(temp&16384)>>(14-ppick);
          memlocation[(f*4)+5]|=(temp&1024)>>(9-ppick);
          memlocation[(f*4)+6]|=(temp&8192)>>(13-ppick);
          memlocation[(f*4)+6]|=(temp&512)>>(8-ppick);
          memlocation[(f*4)+7]|=(temp&4096)>>(12-ppick);
          memlocation[(f*4)+7]|=(temp&256)>>(7-ppick);
        }
        return TRUE;
      }
    }
    free(buffer);
  }
  return FALSE;
}

BOOL LoadSwitchedROM(char *filename,unsigned char *memlocation,size_t length,unsigned char switchgap)
{
  // Switched ROMs need to be loaded into alternate bytes.
  unsigned char *buffer=NULL,temp;
  unsigned long f=0;
  FILE *fileID;
  size_t LengthActuallyRead;
  buffer=(unsigned char *)malloc(length);
  if(buffer)
  {
    if(fileID=fopen(filename,moderead))
    {
      LengthActuallyRead=fread(buffer,length,1,fileID);
      fclose(fileID);
      if(LengthActuallyRead==1)
      {
        for(;f<length;++f)
          memlocation[f*switchgap]=buffer[f];
        return TRUE;
      }
    }
    free(buffer);
  }
  return FALSE;
}

BOOL LoadROMs(ROM *romlist)
{
  int f=0;
  for(;f<NUMBEROFROMS;++f)
  {
    if(romlist[f].LoadMethod==ROMLOAD_SWITCHED)
      if(!LoadSwitchedROM(romlist[f].FileName,rom+romlist[f].MemLocation,romlist[f].FileLength,romlist[f].SwitchGap))
        return FALSE;
    if(romlist[f].LoadMethod==ROMLOAD_LINEAR)
      if(!LoadLinearROM(romlist[f].FileName,rom+romlist[f].MemLocation,romlist[f].FileLength))
        return FALSE;
    if(romlist[f].LoadMethod==ROMLOAD_SOUND)
      if(!LoadLinearROM(romlist[f].FileName,music+romlist[f].MemLocation,romlist[f].FileLength))
        return FALSE;
    if(romlist[f].LoadMethod==ROMLOAD_SPRITES)
      if(!LoadSpriteROM(romlist[f].FileName,sprites+romlist[f].MemLocation,romlist[f].FileLength,romlist[f].PlanePick))
        return FALSE;
    if(romlist[f].LoadMethod==ROMLOAD_TILES)
      if(!LoadTileROM(romlist[f].FileName,tiles+romlist[f].MemLocation,romlist[f].FileLength,romlist[f].PlanePick))
        return FALSE;
    if(romlist[f].LoadMethod==ROMLOAD_CHARS)
      if(!LoadTileROM(romlist[f].FileName,chars+romlist[f].MemLocation,romlist[f].FileLength,romlist[f].PlanePick))
        return FALSE;
  }
  return TRUE;
}

BOOL AllocateMemory(void)
{
  if((rom=(unsigned char *)malloc(ROM_K_NEEDED*1024))&&
    (ram=(unsigned char *)malloc(RAM_K_NEEDED*1024))&&
    (chars=(unsigned char *)malloc(CHAR_K_NEEDED*1024))&&
    (sprites=(unsigned char *)malloc(SPRITE_K_NEEDED*1024))&&
    (tiles=(unsigned char *)malloc(TILE_K_NEEDED*1024))&&
    (music=(unsigned char *)malloc(SOUND_K_NEEDED*1024))&&
    (vramvscreen=(unsigned char *)malloc(VRAM_K_NEEDED*1024))&&
    (scroll1vscreen=(unsigned char *)malloc(SCROLL1_K_NEEDED*1024))&&
    (scroll2vscreen=(unsigned char *)malloc(SCROLL2_K_NEEDED*1024))&&
    (vscreen=(unsigned char *)malloc(VSCREEN_K_NEEDED*1024)))
    return TRUE;
  return FALSE;
}

void DeallocateMemory(void)
{
  if(vscreen)
    free(vscreen);
  if(scroll1vscreen)
    free(scroll1vscreen);
  if(scroll2vscreen)
    free(scroll2vscreen);
  if(vramvscreen)
    free(vramvscreen);
  if(music)
    free(music);
  if(tiles)
    free(tiles);
  if(sprites)
    free(sprites);
  if(chars)
    free(chars);
  if(ram)
    free(ram);
  if(rom)
    free(rom);
} 

void Set68kMemoryAccessArrays(void)
{
  memoryfetch[0].off=(unsigned)rom;
  memoryreadbyte[0].userdata=memoryreadword[0].userdata=(void *)rom;
  memoryreadbyte[1].userdata=memoryreadword[1].userdata=(void *)ram;
  memorywritebyte[1].userdata=memorywriteword[3].userdata=memorywriteword[4].userdata=(void *)ram;
  s68000context.memoryfetch=memoryfetch;
  s68000context.memoryreadbyte=memoryreadbyte;
  s68000context.memoryreadword=memoryreadword;
  s68000context.memorywritebyte=memorywritebyte;
  s68000context.memorywriteword=memorywriteword;
}

unsigned char *GetLinearFrameBuffer(void)
// I have no idea what this means. I copied it from somewhere. It all looks
// hellishly stupid and unacceptable. All I know is that it returns a pointer
// to a linear frame buffer, which is an area of video memory that you can
// access like mode 0x13, but is available for any mode.
{
  unsigned long screen_base_addr;
  __djgpp_nearptr_enable();
  __dpmi_get_segment_base_address(screen->seg,&screen_base_addr);
  return (unsigned char *)(screen_base_addr+screen->line[0]-__djgpp_base_address);
}

void MakeEasyGraphics(void)
// Since there are so many palettes in BC, it makes the drawing process
// easier if we tinker with the graphics data beforehand.
{
  unsigned int f=0;
  // All foreground tiles use palette values 0-63. In this range are four
  // 16-colour palettes. The first 16-colour palette uses colours 0, 4, 8,
  // 12, etc. The second uses colours 1, 5, 9, 13, etc. In this way, the
  // lower 2 bits of a byte specify the palette number to use, and we can
  // identify transparent colours more efficiently (they're > 60).
  for(;f<0x80000;++f)
    tiles[f]=(tiles[f]<<2)|0x00;
  // All background tiles use palette values 64-127, in exactly the same
  // way as before, but there are no transparent background tiles.
  for(;f<0xA0000;++f)
    tiles[f]=(tiles[f]<<2)|0x40;
  for(f=0;f<0x20000;++f)
    chars[f]=(chars[f]<<4)|0xC0;
  for(f=0;f<0x80000;++f)
#ifdef GETCHARPALETTE
    sprites[f]=(sprites[f]<<2)|0x80;
#else
    sprites[f]=(sprites[f]<<3)|0x80;
#endif
}

void InitRAM(void)
// We clear the RAM at startup.
{
  unsigned int f=0x0000;
  for(;f<0x20000;++f)
    ram[f]=0;
  // Set the dipswitches to starting values.
  ram[0x4002]=config.dipswitch4002;
  ram[0x4003]=config.dipswitch4003;
}

void GetJoystickControls(void)
{
  poll_joystick();
  if(joy_up)
    ram[0x1FFFE]|=32;
  if(joy_down)
    ram[0x1FFFE]|=16;
  if(joy_left)
    ram[0x1FFFE]|=8;
  if(joy_right)
    ram[0x1FFFE]|=4;
  if(joy_b1)
    ram[0x1FFFE]|=2;
  if(joy_b2)
    ram[0x1FFFE]|=1;
}

void GetKeyboardControls(void)
{
  if(key[config.keyup])
    ram[0x1FFFE]|=32;
  if(key[config.keydown])
    ram[0x1FFFE]|=16;
  if(key[config.keyleft])
    ram[0x1FFFE]|=8;
  if(key[config.keyright])
    ram[0x1FFFE]|=4;
  if(key[config.keygun])
    ram[0x1FFFE]|=2;
  if(key[config.keyarm])
    ram[0x1FFFE]|=1;
}

void Initialise(void)
{
  Set68kMemoryAccessArrays();
  s68000context.resethandler=ResetStub;
  scroll1vscreenlastline=(scroll1vscreen+(SCROLL1_K_NEEDED*1024))-2048;
  scroll2vscreenlastline=(scroll2vscreen+(SCROLL2_K_NEEDED*1024))-1024;
  s68000init();
  s68000reset();
  InitRAM();
  MakeEasyGraphics();
  set_color_depth(8);
  BCPaletteAddress=&BCPalette;
  if(config.joystick)
  {
    joy_type=JOY_TYPE_STANDARD;
    if(load_joystick_data(calibrationfile))
      initialise_joystick();
    GetControls=GetJoystickControls;
  }
  else
    GetControls=GetKeyboardControls;
  ShowScreen=(config.modex?showscreenmodex_:showscreenvesa2l_);
}

void PlayerStart(unsigned char playernumber)
{
  // This is the only way I can get the START buttons to work at the moment.
  // It replaces a BEQ with a BNE, makes the jump, then rewrites the BEQ.
  rom[0x220C+(playernumber==1?0x0A:0x00)]=0x00;
  rom[0x220D+(playernumber==1?0x0A:0x00)]=0x66;
  s68000interrupt(2);
  s68000exec(clocktickspercall);
  rom[0x220C+(playernumber==1?0x0A:0x00)]=0x00;
  rom[0x220D+(playernumber==1?0x0A:0x00)]=0x67;
}

int GetKey(void)
{
  int f,input=0;
  clear_keybuf();
  do
  {
    for(f=KEY_ESC;f<KEY_PAUSE;++f)
      if(key[f])
        input=f;
  }
  while(!input);
  clear_keybuf();
  return input;
}

void LoadHighScores(void)
{
  unsigned char value,f,nospaces=0;
  FILE *fileID;
  if(fileID=fopen(highscorefile,moderead))
  {
    fread(&(ram[0x1F9E2]),80,1,fileID);
    memcpy(&(ram[0x1C57A]),&(ram[0x1F9E2]),4);
    memcpy(&(ram[0x1C58C]),&(ram[0x1F9E2]),4);
    fclose(fileID);
    // Because I haven't bothered to find the optimum point for loading
    // the highscores, I also have to draw the new top score onto the
    // screen manually. :) I'm just plain lazy.
    for(f=0;f<8;f++)
    {
      value=ram[0x1F9E2+((f&2)?(f>>1)-1:(f>>1)+1)];
      nospaces|=value=((f&1)?(value&15):(value>>4));
      WriteCharWord(0xC0D8+(f*2),(nospaces?value:0x20));
    }
  }
  highscorestatus=1;
}

void SaveHighScores(void)
{
  FILE *fileID;
  if(fileID=fopen(highscorefile,modewrite))
  {
    fwrite(&(ram[0x1F9E2]),80,1,fileID);
    fclose(fileID);
  }
}

void vramprintfcolour(unsigned char x,unsigned char y,unsigned char length,unsigned char colour)
{
  int position=0x0C800+(y*64)+(x*2);
  for(;length;--length,position+=2)
    WriteCharWord(position,colour);
}

void vramprintf(unsigned char x,unsigned char y,char *string,unsigned char colour)
{
  int position=0x0C000+(y*64)+(x*2),f=0;
  for(;f<strlen(string);++f,position+=2)
  {
    WriteCharWord(position,string[f]);
    WriteCharWord(position+0x800,colour);
  }
}

void ShowSwitchValues(int selected)
{
  unsigned char bank1=ram[0x4002];
  unsigned char bank2=ram[0x4003];
  char *coinvalues[8]={"1/4","1/3","1/2","6/1","4/1","3/1","2/1","1/1"};
  char *startlives[4]={"7","5","4","3"};
  char *bonusvalues[4]={"   30K AND 60K ONLY","   20K AND 60K ONLY","30K, 50K, EVERY 70K","20K, 40K, EVERY 60K"};
  char *difficultyvalues[4]={"VERY DIFFICULT","     DIFFICULT","          EASY","        NORMAL"};
  char *cabinettypes[2]={"  TABLE","UPRIGHT"};
  char *offstring="OFF",*onstring=" ON";
  char *string=coinvalues[bank1&7];
  vramprintf(27,7,string,(selected==0?12:2));
  string=coinvalues[(bank1&56)>>3];
  vramprintf(27,9,string,(selected==1?12:2));
  string=startlives[bank2&3];
  vramprintf(29,11,string,(selected==2?12:2));
  string=bonusvalues[(bank2&24)>>3];
  vramprintf(11,13,string,(selected==3?12:2));
  string=difficultyvalues[(bank2&96)>>5];
  vramprintf(16,15,string,(selected==4?12:2));
  string=(bank1&64?offstring:onstring);
  vramprintf(27,17,string,(selected==5?12:2));
  string=(bank1&128?offstring:onstring);
  vramprintf(27,19,string,(selected==6?12:2));
  string=cabinettypes[(bank2&4)>>2];
  vramprintf(23,21,string,(selected==7?12:2));
}

void ChangeSwitchValue(int position,int direction)
{
  unsigned char bank;
  switch(position)
  {
    case 0:
      bank=ram[0x4002];
      ram[0x4002]=(bank&248)|((bank-direction)&7);
      break;
    case 1:
      bank=ram[0x4002];
      ram[0x4002]=(bank&199)|((bank-(direction*8))&56);
      break;
    case 2:
      bank=ram[0x4003];
      ram[0x4003]=(bank&252)|((bank-direction)&3);
      break;
    case 3:
      bank=ram[0x4003];
      ram[0x4003]=(bank&231)|((bank-(direction*8))&24);
      break;
    case 4:
      bank=ram[0x4003];
      ram[0x4003]=(bank&159)|((bank-(direction*32))&96);
      break;
    case 5:
      bank=ram[0x4002];
      ram[0x4002]=(bank&191)|((bank-(direction*64))&64);
      break;
    case 6:
      bank=ram[0x4002];
      ram[0x4002]=(bank&127)|((bank-(direction*128))&128);
      break;
    case 7:
      bank=ram[0x4003];
      ram[0x4003]=(bank&251)|((bank-(direction*4))&4);
      break;
    default:
      break;
  }
  clear_keybuf();
}

void MoveSwitchCursor(unsigned int *currentposition,int direction)
{
  vramprintfcolour(0,7+(*currentposition*2),32,2);
  *currentposition+=direction;
  *currentposition=(*currentposition)%8;
  vramprintfcolour(0,7+(*currentposition*2),32,12);
  clear_keybuf();
}

void Dipswitches(void)
{
  unsigned int f,g;
  unsigned short *vrambackup;
  if(vrambackup=(unsigned short *)malloc(0x1000))
  {
    memcpy(vrambackup,&(ram[0x0C000]),0x1000);
    for(f=0x0C000;f<0x0C800;f+=2)
      WriteCharWord(f,0x0020);
    for(f=0x0C800;f<0x0D000;f+=2)
      WriteCharWord(f,0x0002);
    vramprintf(10,3,"DIPSWITCHES",3);
    vramprintf(2,7,"P1 CREDITS/COINS",12);
    vramprintf(2,9,"P2 CREDITS/COINS",2);
    vramprintf(2,11,"START LIVES",2);
    vramprintf(2,13,"BONUS",2);
    vramprintf(2,15,"DIFFICULTY",2);
    vramprintf(2,17,"TEST MODE",2);
    vramprintf(2,19,"Y FLIP",2);
    vramprintf(2,21,"CABINET TYPE",2);
    vramprintf(2,25,"USE CURSORS TO CHANGE VALUES",3);
    clear_keybuf();
    f=0;
    do
    {
      buildscreen_();
      ShowScreen();
      ShowSwitchValues(f);
      if(key[KEY_UP])
        MoveSwitchCursor(&f,-1);
      if(key[KEY_DOWN])
        MoveSwitchCursor(&f,1);
      if(key[KEY_LEFT])
        ChangeSwitchValue(f,-1);
      if(key[KEY_RIGHT])
        ChangeSwitchValue(f,1);
    }
    while(!key[KEY_TAB]);
    clear_keybuf();
    for(f=0;f<0x0800;++f)
      WriteCharWord(0x0C000+(f*2),vrambackup[f]);
    config.dipswitch4002=ram[0x4002];
    config.dipswitch4003=ram[0x4003];
    free(vrambackup);
  }
}

void Snapshot(void)
{
  char filename[15];
  sprintf(filename,"snap%04d.pcx",snapcounter++);
  save_pcx(filename,screen,BCPalette);
}

void Emulate(void)
// This is the main loop.
{
  unsigned long f=0;
  do
  {
    s68000interrupt(2);
    s68000exec(clocktickspercall);
    if(!~highscorestatus)
      LoadHighScores();
    if(key[KEY_F12])
      Snapshot();
    if(key[KEY_PAUSE])
      GetKey();
    if(key[KEY_TAB])
      Dipswitches();
#ifdef STARTCHEAT
    if(key[KEY_1])
      PlayerStart(1);
    if(key[KEY_2])
      PlayerStart(2);
#endif
  }
  while(!(key[KEY_ESC]));
  if(highscorestatus)
    SaveHighScores();
}

void DefineKeys(void)
{
  printf("Press a key for UP ...\n");
  config.keyup=GetKey();
  printf("Press a key for DOWN ...\n");
  config.keydown=GetKey();
  printf("Press a key for LEFT ...\n");
  config.keyleft=GetKey();
  printf("Press a key for RIGHT ...\n");
  config.keyright=GetKey();
  printf("Press a key for SHOOT ...\n");
  config.keygun=GetKey();
  printf("Press a key for ARM ...\n");
  config.keyarm=GetKey();
  printf("Thankyou ...\n");
}

void CalibrateJoystick(void)
{
  printf("Centre your joystick then press a key ...\n");
  GetKey();
  initialise_joystick();
  printf("Move your joystick to the top-left position then press a key ...\n");
  GetKey();
  calibrate_joystick_tl();
  printf("Move your joystick to the bottom-right position then press a key ...\n");
  GetKey();
  calibrate_joystick_br();
  save_joystick_data(calibrationfile);
  printf("Thankyou ...\n");
}

void CommandLineInfo(void)
{
  printf("Slutte! (%s version) by Steven Frew\n\n",revision);
  printf("-res xxx yyy    Specify resolution. This option uses VESA 2.0 linear frame\n");
  printf("                buffers, so you must have a VESA 2.0 driver.\n");
  printf("-modex          Play game in ModeX resolution.\n");
  printf("-j              Use joystick.\n");
  printf("-k              Use keyboard.\n");
  printf("-v ON/OFF       Enable or disable vsync.\n");
  printf("-definekeys     Prompt for keys before playing.\n");
  printf("-calibrate      Calibrate joystick before playing.\n");
  PlayGame=FALSE;
}

void ParseOptions(int argc,char **argv)
{
  int argcounter=1;
  for(argcounter=1;argcounter<argc;++argcounter)
  {
    if(!stricmp(argv[argcounter],"/?"))
      CommandLineInfo();
    else if(!stricmp(argv[argcounter],"-?"))
      CommandLineInfo();
    else if(!stricmp(argv[argcounter],"-MODEX"))
    {
      config.modex=TRUE;
#ifdef FINESIZE
      config.screenwidth=screenwidth=256;
#else
      config.screenwidth=screenwidth=320;
#endif
      config.screenheight=240;
    }
    else if((!stricmp(argv[argcounter],"-RES"))&&(argcounter+2<argc))
    {
      config.modex=FALSE;
      config.screenwidth=screenwidth=atol(argv[argcounter+1]);
      config.screenheight=screenheight=atol(argv[argcounter+2]);
    }
    else if(!stricmp(argv[argcounter],"-J"))
      config.joystick=TRUE;
    else if(!stricmp(argv[argcounter],"-K"))
      config.joystick=FALSE;
    else if((!stricmp(argv[argcounter],"-V"))&&(argcounter+1<argc))
    {
      if(!stricmp(argv[argcounter+1],"ON"))
        config.waitvbl=TRUE;
      else
        config.waitvbl=FALSE;
    }
    else if(!stricmp(argv[argcounter],"-DEFINEKEYS"))
      DefineKeys();
    else if(!stricmp(argv[argcounter],"-CALIBRATE"))
      CalibrateJoystick();
  }
}

BOOL LoadConfig(void)
{
  FILE *fileID;
  size_t LengthActuallyRead;
  if(fileID=fopen(configfile,moderead))
  {
    LengthActuallyRead=fread(&config,sizeof(config),1,fileID);
    fclose(fileID);
    if(LengthActuallyRead==1)
    {
      screenwidth=config.screenwidth;
      screenheight=config.screenheight;
      return TRUE;
    }
  }
  return FALSE;
}

BOOL SaveConfig(void)
{
  FILE *fileID;
  size_t LengthActuallyWritten;
  if(fileID=fopen(configfile,modewrite))
  {
    LengthActuallyWritten=fwrite(&config,sizeof(config),1,fileID);
    fclose(fileID);
    if(LengthActuallyWritten==1)
      return TRUE;
  }
  return FALSE;
}

void DefaultConfig(void)
{
  config.keyup=0;
  config.waitvbl=config.joystick=FALSE;
#ifdef FINESIZE
  config.screenwidth=screenwidth=256;
#else
  config.screenwidth=screenwidth=320;
#endif
  config.screenheight=screenheight=240;
  config.modex=FALSE;
  config.dipswitch4002=config.dipswitch4003=0xFF;
}

void GetConfig(int argc,char **argv)
{
  DefaultConfig();
  LoadConfig();
  if(argc>1)
    ParseOptions(argc,argv);
  if((!config.keyup)&&(!config.joystick))
  {
    printf("You have no default keys defined. Please enter the keyboard controls that\n");
    printf("you want to use. These will be saved in Slutte!.CFG for next time.\n\n");
    DefineKeys();
  }
}

void main(int argc,char **argv)
{
  ROM ROMs[NUMBEROFROMS]=
  {
    // Code
    {"BionicC\\TSU_04B.ROM",ROMLOAD_SWITCHED,0x00000,2,0,65536},
    {"BionicC\\TSU_02B.ROM",ROMLOAD_SWITCHED,0x00001,2,0,65536},
    {"BionicC\\TSU_05B.ROM",ROMLOAD_SWITCHED,0x20000,2,0,65536},
    {"BionicC\\TSU_03B.ROM",ROMLOAD_SWITCHED,0x20001,2,0,65536},
    // Music? (not sure yet)
    {"BionicC\\TSU_01B.ROM",ROMLOAD_SOUND,0x00000,0,0,32768},
    // Sprites
    {"BionicC\\TSU_10.ROM",ROMLOAD_SPRITES,0x00000,0,0,32768},
    {"BionicC\\TSU_15.ROM",ROMLOAD_SPRITES,0x00000,0,1,32768},
    {"BionicC\\TSU_20.ROM",ROMLOAD_SPRITES,0x00000,0,2,32768},
    {"BionicC\\TSU_22.ROM",ROMLOAD_SPRITES,0x00000,0,3,32768},
    {"BionicC\\TSU_09.ROM",ROMLOAD_SPRITES,0x40000,0,0,32768},
    {"BionicC\\TSU_14.ROM",ROMLOAD_SPRITES,0x40000,0,1,32768},
    {"BionicC\\TSU_19.ROM",ROMLOAD_SPRITES,0x40000,0,2,32768},
    {"BionicC\\TSU_21.ROM",ROMLOAD_SPRITES,0x40000,0,3,32768},
    // Chars
    {"BionicC\\TSU_08.ROM",ROMLOAD_CHARS,0x00000,0,0,32768},
    // Scroll1 tiles
    {"BionicC\\TS_12.ROM",ROMLOAD_TILES,0x00000,0,0,32768},
    {"BionicC\\TS_13.ROM",ROMLOAD_TILES,0x00000,0,2,32768},
    {"BionicC\\TS_11.ROM",ROMLOAD_TILES,0x20000,0,0,32768},
    {"BionicC\\TS_18.ROM",ROMLOAD_TILES,0x20000,0,2,32768},
    {"BionicC\\TS_17.ROM",ROMLOAD_TILES,0x40000,0,0,32768},
    {"BionicC\\TS_23.ROM",ROMLOAD_TILES,0x40000,0,2,32768},
    {"BionicC\\TS_16.ROM",ROMLOAD_TILES,0x60000,0,0,32768},
    {"BionicC\\TS_24.ROM",ROMLOAD_TILES,0x60000,0,2,32768},
    // Scroll2 tiles
    {"BionicC\\TSU_07.ROM",ROMLOAD_TILES,0x80000,0,0,32768},
    {"BionicC\\TSU_06.ROM",ROMLOAD_TILES,0x80000,0,2,32768}
  };
  allegro_init();
#ifndef DEBUG
  install_keyboard();
#endif
  GetConfig(argc,argv);
  if(PlayGame)
    if(AllocateMemory())
      if(LoadROMs(ROMs))
      {
        Initialise();
#ifdef DEBUG
        cpudebug_interactive(1);
#endif
        if(!set_gfx_mode((config.modex?GFX_MODEX:GFX_VESA2L),config.screenwidth,config.screenheight,0,0))
        {
          lfb=GetLinearFrameBuffer();
          Emulate();
          SaveConfig();
#ifdef RAMDUMP
          Dump("DATA.RAM",&(ram[0x00000]),&(ram[0x1FFFF]));
#endif
#ifdef ROMDUMP
          Dump("CODE.ROM",&(rom[0x00000]),&(rom[0x3FFFF]));
#endif
          set_gfx_mode(GFX_TEXT,0,0,0,0);
#ifdef DEBUG
          cpudebug_interactive(1);
#endif
        }
        else
          printf(allegro_error);
      }
      else
        printf("Couldn't load ROMs\n");
    else
      printf("Couldn't allocate memory\n");
  DeallocateMemory();
  exit(0);
}
