#define SS_DEVTYPE 3
#define SS_PLAYER 0
#define SS_SAMPLER 1
#define SS_WAVETABLE 2
#define SS_MIDI 3
#define SS_NEEDPLAYER 4
#define SS_NEEDWAVETABLE 8

struct sounddevice;

struct deviceinfo
{
  sounddevice *dev;
  signed short port;
  signed short port2;
  signed char irq;
  signed char irq2;
  signed char dma;
  signed char dma2;
  unsigned long opt;
  signed char subtype;
  unsigned char chan;
  unsigned long mem;
};

struct sounddevice
{
  char type;
  char name[32];
  int (*Detect)(deviceinfo &c);
  int (*Init)(const deviceinfo &c);
  void (*Close)();
};
