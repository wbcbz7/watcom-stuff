#include "mcp.h"

int mcpNChan;

void (*mcpIdle)();

int (*mcpLoadSamples)(sampleinfo* si, int n);
int (*mcpOpenPlayer)(int, void (*p)());
void (*mcpClosePlayer)();
void (*mcpSet)(int ch, int opt, int val);
int (*mcpGet)(int ch, int opt);
void (*mcpGetRealVolume)(int ch, int &l, int &r);
void (*mcpGetRealMasterVolume)(int &l, int &r);
void (*mcpGetMasterSample)(short *s, int len, int rate, int opt);
int (*mcpGetChanSample)(int ch, short *s, int len, int rate, int opt);
int (*mcpMixChanSamples)(int *ch, int n, short *s, int len, int rate, int opt);

int mcpMixMaxRate;
int mcpMixProcRate;
int mcpMixOpt;
int mcpMixBufSize;
int mcpMixMax;
int mcpMixPoll;
