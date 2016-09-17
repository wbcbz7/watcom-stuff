//                    USM Play C/C++ Interface file
//
//                  By Freddy Vetele (FreddyV/Useless)


#ifndef __USMPLAY_H
#define __USMPLAY_H

#define TimerSpeed 1193180
#define MAX_PLCHANNELS 32
#define LM_File 0
#define LM_Memory 1

// USM file Header structure (See USM.INC for more info about USM file format)

typedef struct {
  char           Id[4];        // USM Header Ident 'USMH'
  unsigned short Version;      // USM File Version
  char           Name[32];     // Music Name
  unsigned short Flags;
  unsigned short Tracker;
  unsigned short Channels;
  unsigned short Patterns;
  unsigned short Samples;
  unsigned short Instruments;
  unsigned short SongLength;
  unsigned short Restart;
  unsigned char  Tempo;
  unsigned char  BPM;
} USM;  

#ifdef __cplusplus
extern "C"
{
#endif

// Output device informations

extern char *DEV_Name;
extern unsigned short DEV_BasePort;
extern unsigned char DEV_IRQ;
extern unsigned char DEV_DMA;

// Error number

extern int Error_Number;

// Music Synchro

extern unsigned int   SynchroCount;
extern unsigned char  SynchroVal;
extern unsigned char  MusicEnd;
extern unsigned short Order;
extern unsigned short Pattern;
extern unsigned short Row;

extern unsigned char ChInstrument[MAX_PLCHANNELS];
extern unsigned char ChNote[MAX_PLCHANNELS];
extern unsigned char ChCommand[MAX_PLCHANNELS];


// *****************
// * Files Loaders *
// *****************

USM *MOD_Load(unsigned int loadmode, unsigned int panning,char *filename);
  #pragma aux MOD_Load "*" parm [ebx eax esi] value [esi] modify [eax ebx ecx edx esi edi]

USM *STM_Load(unsigned int loadmode, unsigned int panning,char *filename);
  #pragma aux STM_Load "*" parm [ebx eax esi] value [esi] modify [eax ebx ecx edx esi edi]

USM *S3M_Load(unsigned int loadmode, unsigned int panning,char *filename);
  #pragma aux S3M_Load "*" parm [ebx eax esi] value [esi] modify [eax ebx ecx edx esi edi]

USM *XM_Load(unsigned int loadmode, unsigned int panning,char *filename);
  #pragma aux XM_Load "*" parm [ebx eax esi] value [esi] modify [eax ebx ecx edx esi edi]

USM *IT_Load(unsigned int loadmode, unsigned int panning,char *filename);
  #pragma aux IT_Load "*" parm [ebx eax esi] value [esi] modify [eax ebx ecx edx esi edi]

USM *USM_Load(unsigned int loadmode, unsigned int panning,char *filename);
  #pragma aux USM_Load "*" parm [ebx eax esi] value [esi] modify [eax ebx ecx edx esi edi]


// *********************
// * USS.ASM Functions *
// *********************

void USS_Setup(void);
  #pragma aux USS_Setup "*" modify [eax ebx ecx edx esi edi]

void USS_AutoSetup(void);
  #pragma aux USS_AutoSetup "*"  modify [eax ebx ecx edx esi edi]

void USS_SetAmpli(int Ampli);
  #pragma aux USS_SetAmpli "*" parm [eax] modify [eax ebx ecx edx esi edi]


// *************************
// * USMPLAY.ASM Functions *
// *************************

void USMP_LoadInternalModule(USM *music);
  #pragma aux USMP_LoadInternalModule "*" parm [esi] modify [eax ebx ecx edx esi edi]

void USMP_FreeInternalModule(USM *music);
  #pragma aux USMP_FreeInternalModule "*" parm [esi] modify [eax ebx ecx edx esi edi]


void USMP_FreeModule(USM *music);
  #pragma aux USMP_FreeModule "*" parm [esi] modify [eax ebx ecx edx esi edi]


void USMP_StartPlay(USM *music);
  #pragma aux USMP_StartPlay "*" parm [esi] modify [eax ebx ecx edx esi edi]

void USMP_StopPlay(void);
  #pragma aux USMP_StopPlay "*" modify [eax ebx ecx edx esi edi]


void USMP_PrevOrder(void);
  #pragma aux USMP_PrevOrder "*" modify [eax]

void USMP_NextOrder(void);
  #pragma aux USMP_NextOrder "*" modify [eax esi]

void USMP_SetOrder(unsigned int order);
  #pragma aux USMP_SetOrder "*" parm [eax] modify [eax esi]

void USMP_SetPosition(unsigned int order,unsigned int row);
  #pragma aux USMP_SetPosition "*" parm [eax ebx] modify [eax ebx esi]


// **************************
// * HARDWARE.ASM Functions *
// **************************

void HardwareInit(unsigned long psp);
  #pragma aux HardwareInit parm [eax] modify [eax ebx ecx edx esi edi]

// ***********************
// * TIMER.ASM Functions *
// ***********************

int Timer_Start(void *function,int speed);
  #pragma aux Timer_Start parm [eax ebx] value [eax] modify [eax ebx ecx edx esi edi]

int Timer_Stop(void *function);
  #pragma aux Timer_Stop parm [eax] value [eax] modify [eax ebx ecx edx esi edi]


// **********************
// * ERROR.ASM Function *
// **********************

void Display_Error(int Error_Number);
  #pragma aux Display_Error parm [eax] modify [eax ebx ecx edx esi edi]

#ifdef __cplusplus
};
#endif

#endif