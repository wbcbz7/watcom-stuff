long imuldiv(long,long,long);
#pragma aux imuldiv parm [eax] [edx] [ebx] value [eax] = "imul edx" "idiv ebx"
unsigned long umuldiv(unsigned long,unsigned long,unsigned long);
#pragma aux umuldiv parm [eax] [edx] [ebx] value [eax] = "mul edx" "div ebx"
long imulshr16(long,long);
#pragma aux imulshr16 parm [eax] [edx] [ebx] value [eax] = "imul edx" "shrd eax,edx,16"
unsigned long umulshr16(unsigned long,unsigned long);
#pragma aux umulshr16 parm [eax] [edx] [ebx] value [eax] = "mul edx" "shrd eax,edx,16"
unsigned long umuldivrnd(unsigned long, unsigned long, unsigned long);
#pragma aux umuldivrnd parm [eax] [edx] [ecx] modify [ebx] = "mul edx" "mov ebx,ecx" "shr ebx,1" "add eax,ebx" "adc edx,0" "div ecx"
void memsetd(void *, long, int);
#pragma aux memsetd parm [edi] [eax] [ecx] = "rep stosd"
void memsetw(void *, int, int);
#pragma aux memsetw parm [edi] [eax] [ecx] = "rep stosw"
void memsetb(void *, int, int);
#pragma aux memsetb parm [edi] [eax] [ecx] = "rep stosb"
short _disableint();
#pragma aux _disableint value [ax] = "pushf" "pop ax" "cli"
void _restoreint(short);
#pragma aux _restoreint parm [ax] = "push ax" "popf"

