#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef BOOL
#define BOOL signed char
#endif

#define NUMBEROFROMS    24

#define ROM_K_NEEDED     256
#define RAM_K_NEEDED     128
#define CHAR_K_NEEDED    128
#define TILE_K_NEEDED    640
#define SPRITE_K_NEEDED  512
#define SOUND_K_NEEDED   32
#define SCROLL1_K_NEEDED 2048
#define SCROLL2_K_NEEDED 512
#define VRAM_K_NEEDED    64
// We need a 288x272 virtual screen. This is because we are drawing sprites
// directly to it (not using a sprite layer buffer). Sprites can exist
// overlapping screen-edge boundaries, so this is why the dimensions are
// not 256x240 (we need 16 spare pixels at each edge, since sprite "tiles"
// are 16x16).
#define VSCREEN_K_NEEDED 76

#define ROMLOAD_SWITCHED    0
#define ROMLOAD_LINEAR      1
#define ROMLOAD_SPRITES     2
#define ROMLOAD_CHARS       3
#define ROMLOAD_TILES       4
#define ROMLOAD_SOUND       5

typedef struct
{
  char *FileName;
  unsigned char LoadMethod;
  unsigned long MemLocation;
  unsigned char SwitchGap;
  unsigned char PlanePick;
  unsigned long FileLength;
} ROM;

typedef struct
{
  BOOL joystick,waitvbl;
  unsigned int screenwidth,screenheight;
  unsigned char updatepaletteframefrequency;  // obsolete
  int keyup,keydown,keyleft,keyright,keygun,keyarm;
  BOOL modex;
  unsigned char dipswitch4002,dipswitch4003;
} SLUTTECONFIG;
