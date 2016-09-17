struct mixqpostprocregstruct
{
  void (*Process)(long *buffer, int len, int rate, int stereo);
  void (*Init)(int rate, int stereo);
  void (*Close)();
  mixqpostprocregstruct *next;
};

void mixqRegisterPostProc(mixqpostprocregstruct *);

#ifdef CPDOS

struct mixqpostprocaddregstruct
{
  int (*ProcessKey)(unsigned short key);
  mixqpostprocaddregstruct *next;
};

#endif
