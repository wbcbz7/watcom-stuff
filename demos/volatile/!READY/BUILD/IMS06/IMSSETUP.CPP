// (c) 1994-1997 Niklas Beisert, this file is part of "Indoor Music System".
// you must not read, modify, print, compile or copy this file or parts of
// it unless you have accepted the license in the accompanying file IMS.TXT.

#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "imssetup.h"

void vgamode(int);
#pragma aux vgamode parm [eax] = "int 10h"
int ekbhit();
#pragma aux ekbhit value [eax] = "mov ah,11h" "int 16h" "mov eax,0" "jz nohit" "inc eax" "nohit:"
int egetch();
#pragma aux egetch value [eax] = "mov ah,10h" "int 16h" "movzx eax,ax"
void setcurpos(int y, int x);
#pragma aux setcurpos parm [eax] [edx] modify [eax ebx] = "mov dh,al" "mov ah,2" "mov bh,0" "int 10h"
void setcurshape(int);
#pragma aux setcurshape parm [ecx] modify [eax] = "mov ax,0103h" "int 10h"

static unsigned char screenbuf[25][80][2];

static void clearscreen()
{
  memset(screenbuf, 0, 80*25*2);
}

static void putscreen()
{
  memcpy((void*)0xB8000, screenbuf, 80*25*2);
}

static void drawstring(int y, int x, char attr, char* str, int l)
{
  int i;
  for (i=0; i<l; i++)
  {
    screenbuf[y][x][0]=*str?*str++:0;
    screenbuf[y][x++][1]=attr;
  }
}

struct selectitem
{
  char *name;
  int type;
  int npredefs;
  char **strings;
  int *values;
  int value;
  int valmin;
  int valmax;
  char **helptext;
};

static int edititem(int y, int x, int len, selectitem &it, int key)
{
  setcurshape(0xD0E);
  int sign=1;
  unsigned int num=0;
  int radix=(it.type==3)?0x10:10;
  int max=(it.type==3)?0x1000000:10000000;
  int stop=0;
  while (1)
  {
    switch (key)
    {
    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8':
    case '9': case '0':
      if (num>=max)
        break;
      num=num*radix+key-'0';
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      if ((it.type!=3)||(num>=max))
        break;
      num=num*0x10+(key&~0x20)-'A'+10;
      break;
    case 8:
    case 0x4B00:
      if (num)
        num=num/radix;
      else
        sign=1;
      break;
    case '-':
      sign=-sign;
      break;

    case 13:
    case 0x4800:
    case 0x5000:
    case 0x4D00:
      sign*=num;
      if (sign<it.valmin)
        sign=it.valmin;
      if (sign>it.valmax)
        sign=it.valmax;
      it.value=sign;
      setcurshape(0x2000);
      return (key==0x4800)?0x4800:(key==0x5000)?0x5000:-1;
    case 27:
      setcurshape(0x2000);
      return -1;
    }
    char str[30];
    *str=(sign>0)?'+':'-';
    ltoa(num, str+1, radix);
    strupr(str);
    drawstring(y, x, 0x8F, str, len);
    putscreen();
    setcurpos(y, x+(num?strlen(str):1));
    while (!ekbhit());
    key=egetch();
    if ((key&0xFF)==0xE0)
      key&=0xFF00;
    if (key&0xFF)
      key&=0x00FF;
  }
}

static const int maxlist=20;
static char portstrbuf[maxlist][5];
static char port2strbuf[maxlist][5];
static char dmastrbuf[maxlist][2];
static char dma2strbuf[maxlist][2];
static char irqstrbuf[maxlist][3];
static char irq2strbuf[maxlist][3];
static char ratestrbuf[maxlist][6];
static char *portstrs[maxlist];
static char *port2strs[maxlist];
static char *dmastrs[maxlist];
static char *dma2strs[maxlist];
static char *irqstrs[maxlist];
static char *irq2strs[maxlist];
static char *ratestrs[maxlist];
static char *devstrs[maxlist];
static int devvals[maxlist];
static char *boolstrs[]={"off", "on"};
static int boolvals[]={0,1};
static char *ampstrs[]={"25", "33", "50", "66", "normal", "150", "200", "300", "400", "600", "800"};
static int ampvals[]={25, 33, 50, 66, 100, 150, 200, 300, 400, 600, 800};
static char *panstrs[]={"inverted", "-75", "inverted headphones", "-25", "mono", "25", "headphones", "75", "normal"};
static char *revchostrs[]={"suppress", "-75", "-suppress half", "-25", "normal", "25", "medium", "75", "full"};
static int percvals[]={-100, -75, -50, -25, 0, 25, 50, 75, 100};

enum
{
  seldev, selport, selport2, seldma, seldma2, selirq, selirq2, selrate, sel16bit, selstereo, selintrpl, selamp, selpan, selrev, selcho, selsrnd, nitems
};

static selectitem items[nitems]=
{
  {" device:", 1, 0, devstrs, devvals, 0, 0, 0, 0},
  {"   port:", 0, 0, portstrs, 0, 0, -1, 0xFFFF, 0},
  {"  port2:", 0, 0, port2strs, 0, 0, -1, 0xFFFF, 0},
  {"    dma:", 0, 0, dmastrs, 0, 0, -1, 7, 0},
  {"   dma2:", 0, 0, dma2strs, 0, 0, -1, 7, 0},
  {"    irq:", 0, 0, irqstrs, 0, 0, -1, 15, 0},
  {"   irq2:", 0, 0, irq2strs, 0, 0, -1, 15, 0},
  {"   rate:", 0, 0, ratestrs, 0, 44100, 11025, 48000, 0},
  {" 16 bit:", 1, 2, boolstrs, boolvals, 0, 0, 0, 0},
  {" stereo:", 1, 2, boolstrs, boolvals, 0, 0, 0, 0},
  {"intrplt:", 1, 2, boolstrs, boolvals, 0, 0, 0, 0},
  {"amplify:", 2, 9, ampstrs, ampvals, 100, 0, 800, 0},
  {"panning:", 2, 9, panstrs, percvals, 100, -100, 100, 0},
  {" reverb:", 2, 9, revchostrs, percvals, 0, -100, 100, 0},
  {" chorus:", 2, 9, revchostrs, percvals, 0, -100, 100, 0},
  {"   srnd:", 2, 2, boolstrs, boolvals, 0, 0, 0, 0}
};

int imsSetup(imssetupresultstruct &res, imssetupdevicepropstruct *devprops, int ndevs)
{
  volatile unsigned long &biosclock=*(volatile unsigned long *)0x46C;

  int i,j;
  vgamode(3);
  inp(0x3da);
  outp(0x3c0, 0x10);
  outp(0x3c0, (inp(0x3c1)&~8));
  outp(0x3c0, 0x20);
  setcurshape(0x2000);

  for (i=0; i<ndevs; i++)
  {
    devstrs[i]=devprops[i].name;
    devvals[i]=i;
  }
  for (i=0; i<maxlist; i++)
  {
    portstrs[i]=portstrbuf[i];
    port2strs[i]=port2strbuf[i];
    dmastrs[i]=dmastrbuf[i];
    dma2strs[i]=dma2strbuf[i];
    irqstrs[i]=irqstrbuf[i];
    irq2strs[i]=irq2strbuf[i];
    ratestrs[i]=ratestrbuf[i];
  }

  items[seldev].npredefs=ndevs;
  items[seldev].value=(res.device<0)?0:(res.device>=ndevs)?(ndevs-1):res.device;
  items[selrate].value=(res.rate<11025)?11025:(res.rate>48000)?48000:res.rate;
  items[selstereo].value=res.stereo?1:0;
  items[sel16bit].value=res.bit16?1:0;
  items[selintrpl].value=res.intrplt?1:0;
  items[selamp].value=(res.amplify<0)?0:(res.amplify>800)?800:res.amplify;
  items[selpan].value=(res.panning<-100)?-100:(res.panning>100)?100:res.panning;
  items[selrev].value=(res.reverb<-100)?-100:(res.reverb>100)?100:res.reverb;
  items[selcho].value=(res.chorus<-100)?-100:(res.chorus>100)?100:res.chorus;
  items[selsrnd].value=res.surround?1:0;

  int endkey=-1;
  unsigned long endtime=0;
  int forcekey=-1;
  int cursel=0;
  while (1)
  {
    imssetupdevicepropstruct &prop=devprops[items[seldev].value];
    items[selport].value=prop.port;
    items[selport2].value=prop.port2;
    items[seldma].value=prop.dma;
    items[seldma2].value=prop.dma2;
    items[selirq].value=prop.irq;
    items[selirq2].value=prop.irq2;
    items[sel16bit].type=prop.bit16?1:0;
    items[selstereo].type=(prop.stereo==1)?1:0;
    items[selintrpl].type=prop.mixer?1:0;
    items[selamp].type=prop.amppan?2:0;
    items[selamp].npredefs=(prop.amppan==2)?7:11;
    items[selpan].type=prop.amppan?2:0;
    items[selrev].type=prop.rev?2:0;
    items[selcho].type=prop.cho?2:0;
    items[selsrnd].type=prop.srnd?1:0;
    for (j=0; j<prop.nports; j++)
    {
      ltoa(prop.ports[j], portstrs[j], 16);
      strupr(portstrs[j]);
    }
    items[selport].type=prop.nports?3:0;
    items[selport].npredefs=prop.nports;
    items[selport].values=prop.ports;
    for (j=0; j<prop.nports2; j++)
    {
      ltoa(prop.ports2[j], port2strs[j], 16);
      strupr(port2strs[j]);
    }
    items[selport2].type=prop.nports2?3:0;
    items[selport2].npredefs=prop.nports2;
    items[selport2].values=prop.ports2;
    for (j=0; j<prop.ndmas; j++)
      ltoa(prop.dmas[j], dmastrs[j], 10);
    items[seldma].type=prop.ndmas?2:0;
    items[seldma].npredefs=prop.ndmas;
    items[seldma].values=prop.dmas;
    for (j=0; j<prop.ndmas2; j++)
      ltoa(prop.dmas2[j], dma2strs[j], 10);
    items[seldma2].type=prop.ndmas2?2:0;
    items[seldma2].npredefs=prop.ndmas2;
    items[seldma2].values=prop.dmas2;
    for (j=0; j<prop.nirqs; j++)
      ltoa(prop.irqs[j], irqstrs[j], 10);
    items[selirq].type=prop.nirqs?2:0;
    items[selirq].npredefs=prop.nirqs;
    items[selirq].values=prop.irqs;
    for (j=0; j<prop.nirqs2; j++)
      ltoa(prop.irqs2[j], irq2strs[j], 10);
    items[selirq2].type=prop.nirqs2?2:0;
    items[selirq2].npredefs=prop.nirqs2;
    items[selirq2].values=prop.irqs2;
    for (j=0; j<prop.nrates; j++)
      ltoa(prop.rates[j], ratestrs[j], 10);
    items[selrate].type=prop.nrates?2:0;
    items[selrate].npredefs=prop.nrates;
    items[selrate].values=prop.rates;

    selectitem &citm=items[cursel];
    clearscreen();
    drawstring( 0,  0, (endkey==1)?0xA0:(endkey==0)?0xC0:0x30, "  Indoor Music System v0.6       SETUP     (c) '94-97  Niklas Beisert / pascal  ", 80);
    drawstring(24,  0, 0x17, "  2*return:start, 2*esc:quit, /:select (\x1B), \x1B/\x1A:select (\x1A)", 80);
    drawstring(24,  2, 0x1F, "2", 1);
    drawstring(24,  4, 0x1F, "return", 6);
    drawstring(24, 18, 0x1F, "2", 1);
    drawstring(24, 20, 0x1F, "esc", 3);
    drawstring(24, 30, 0x1F, "", 1);
    drawstring(24, 32, 0x1F, "", 1);
    drawstring(24, 46, 0x1F, "\x1B", 1);
    drawstring(24, 48, 0x1F, "\x1A", 1);
    if ((citm.type==2)||(citm.type==3))
    {
      drawstring(24, 56, 0x17, ", digit:enter", 13);
      drawstring(24, 58, 0x1F, "digit", 5);
    }

    drawstring(20,  0, 0x07, "컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴", 80);
    for (j=1; j<20; j++)
      drawstring( j, 48, 0x07, "�", 1);
    drawstring(20, 48, 0x07, "�", 1);

    if ((citm.type>=1)&&(citm.type<=3))
    {
      for (j=0; j<citm.npredefs; j++)
      {
        int sel=(citm.values[j]==citm.value);
        if (sel)
          drawstring(j+2, 50, 0x0F, "\x1A", 1);
        drawstring(j+2, 52, sel?0x0F:0x07, citm.strings[j], 27);
      }
    }
    for (i=0; i<nitems; i++)
    {
      selectitem &itm=items[i];
      if (i==cursel)
        drawstring(i+2,  1, 0x0F, "\x1A", 1);
      drawstring(i+2,  3, (i==cursel)?0x0F:itm.type?0x07:0x08, itm.name, 8);
      char str[50];
      *str=0;
      for (j=0; j<itm.npredefs; j++)
        if (itm.value==itm.values[j])
          break;
      if (j!=itm.npredefs)
        strcpy(str, itm.strings[j]);
      else
      {
        if (itm.value<0)
        {
          *str='-';
          ltoa(-itm.value, str+1, (itm.type==3)?0x10:10);
        }
        else
          ltoa(itm.value, str, (itm.type==3)?0x10:10);
        strupr(str);
      }
      drawstring(i+2,  12, (i==cursel)?0x8F:0x0F, itm.type?str:"", 35);
    }

    putscreen();

    int key;
    while (!ekbhit()&&(forcekey==-1))
      if ((endkey!=-1)&&((biosclock-endtime)>36))
        forcekey=0;
    if (forcekey!=-1)
    {
      key=forcekey;
      forcekey=-1;
    }
    else
    {
      key=egetch();
      if ((key&0xFF)==0xE0)
        key&=0xFF00;
      if (key&0xFF)
        key&=0x00FF;
    }
    if ((endkey==1)&&(key!=13))
      endkey=-1;
    if ((endkey==0)&&(key!=27))
      endkey=-1;
    if (key==13)
      if (endkey==1)
        break;
      else
      {
        endkey=1;
        endtime=biosclock;
      }
    if (key==27)
      if (endkey==0)
        break;
      else
      {
        endkey=0;
        endtime=biosclock;
      }

    switch (key)
    {
    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8':
    case '9': case '0': case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case '-': case '+': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      if (citm.type<2)
        break;
      if ((citm.type==2)&&(((key>='a')&&(key<='f'))||((key>='A')&&(key<='F'))))
        break;
      forcekey=edititem(cursel+2, 12, 35, citm, key);
      break;
    case 0x4B00:
      for (j=0; j<citm.npredefs; j++)
        if (citm.values[j]>=citm.value)
          break;
      j=j?(j-1):0;
      citm.value=citm.values[j];
      break;
    case 0x4D00:
      for (j=0; j<citm.npredefs; j++)
        if (citm.values[j]>citm.value)
          break;
      if (j==citm.npredefs)
        j--;
      citm.value=citm.values[j];
      break;
    case 0x4800:
      for (j=cursel-1; j>=0; j--)
        if (items[j].type)
        {
          cursel=j;
          break;
        }
      break;
    case 0x5000:
      for (j=cursel+1; j<nitems; j++)
        if (items[j].type)
        {
          cursel=j;
          break;
        }
      break;
    }
    prop.port=items[selport].value;
    prop.port2=items[selport2].value;
    prop.dma=items[seldma].value;
    prop.dma2=items[seldma2].value;
    prop.irq=items[selirq].value;
    prop.irq2=items[selirq2].value;
  }
  while (ekbhit())
    egetch();

  imssetupdevicepropstruct &rprop=devprops[items[seldev].value];
  res.device=items[seldev].value;
  res.stereo=(rprop.stereo==2)?1:devprops[items[seldev].value].stereo?items[selstereo].value:0;
  res.bit16=devprops[items[seldev].value].bit16?items[selstereo].value:0;
  res.intrplt=devprops[items[seldev].value].mixer?items[selintrpl].value:0;
  res.amplify=items[selamp].value;
  res.rate=items[selrate].value;
  res.panning=items[selpan].value;
  res.reverb=items[selrev].value;
  res.chorus=items[selcho].value;
  res.surround=items[selsrnd].value;
  vgamode(3);
  return endkey;
}
