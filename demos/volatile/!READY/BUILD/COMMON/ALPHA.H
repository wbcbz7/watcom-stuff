#ifndef ALPHA_H
#define ALPHA_H

void initalpha();
void donealpha();
void alphablend(unsigned short *dst, unsigned short *src, unsigned int a);
void alphafade (unsigned short *dst, unsigned short color, unsigned int a);

#endif