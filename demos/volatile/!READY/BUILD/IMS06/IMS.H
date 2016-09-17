
struct imsinitstruct
{
  int usequiet;
  int usersetup;
  int rate;
  int stereo;
  int bit16;
  int interpolate;
  int amplify;
  int panning;
  int reverb;
  int chorus;
  int surround;
  int bufsize;
  int pollmin;
};

void imsFillDefaults(imsinitstruct &);
int imsInit(imsinitstruct &);
void imsClose();
