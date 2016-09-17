#include <memory.h>

// matrices defined by rows, e.g for mat4f:
// [ 0 1 2 3 ]
// [ 4 5 6 7 ]
// [ 8 9 a b ]
// [ c d e f ]

extern signed int sintab[65536],  costab[65536];
extern float      sintabf[65536], costabf[65536];

#include "matrix.h"

void matidentity(mat4f *v) {
    *v[0]  = *v[5]  = *v[10] = *v[15] = 1;
    *v[1]  = *v[2]  = *v[3]  = 0;
    *v[4]  = *v[6]  = *v[7]  = 0;
    *v[8]  = *v[9]  = *v[11] = 0;
    *v[12] = *v[13] = *v[14] = 0;
}

void matscale(mat4f *v, vec4f *f) {
    *v[0]  = f->x;
    *v[5]  = f->y;
    *v[10] = f->z;
    *v[15] = 1;
    *v[1]  = *v[2]  = *v[3]  = 0;
    *v[4]  = *v[6]  = *v[7]  = 0;
    *v[8]  = *v[9]  = *v[11] = 0;
    *v[12] = *v[13] = *v[14] = 0;
}

void mattranslate(mat4f *v, vec4f *f) {
    *v[3]  = f->x;
    *v[7]  = f->y;
    *v[11] = f->z;
    *v[0]  = *v[5]  = *v[10] = *v[15] = 1;
    *v[1]  = *v[2]  = 0;
    *v[4]  = *v[6]  = 0;
    *v[8]  = *v[9]  = 0;
    *v[12] = *v[13] = *v[14] = 0;
}

void mateuler(mat4f *v, int rx, int ry, int rz) {
    float a = costabf[rx & 0xFFFF];
    float b = sintabf[rx & 0xFFFF];
    float c = costabf[ry & 0xFFFF];
    float d = sintabf[ry & 0xFFFF];
    float e = costabf[rz & 0xFFFF];
    float f = sintabf[rz & 0xFFFF];
    
    float ad = a * d, bd = b * d;
    
    *v[0]  =  c * e;
    *v[1]  = -c * f;
    *v[2]  = -d;
    *v[4]  = -bd * e + a * f;
    *v[5]  =  bd * f + a * e;
    *v[6]  = -b * c;
    *v[8]  =  ad * e + b * f;
    *v[9]  = -ad * f + b * e;
    *v[10] = a * c;
    
    *v[3]  = *v[7] = *v[11] = *v[12] = *v[13] = *v[14] = 0;
    *v[15] = 1;
}

vec4f vecmatmul(vec4f *f, mat4f *v) {
    vec4f s; 
    s.x = (*v[0]  * f->x) + (*v[1]  * f->y) + (*v[2]  * f->z) + (*v[3]  * f->w);
    s.y = (*v[4]  * f->x) + (*v[5]  * f->y) + (*v[6]  * f->z) + (*v[7]  * f->w);
    s.z = (*v[8]  * f->x) + (*v[9]  * f->y) + (*v[10] * f->z) + (*v[11] * f->w);
    s.w = (*v[12] * f->x) + (*v[13] * f->y) + (*v[14] * f->z) + (*v[15] * f->w);
    
    return s;
}

// fast vec4f * mat4f multiply. only 9 muls!
// assuming f->w = 1
vec4f vecmatmulf(vec4f *f, mat4f *v) {
    vec4f s; 
    s.x = (*v[0]  * f->x) + (*v[1]  * f->y) + (*v[2]  * f->z) + *v[3];
    s.y = (*v[4]  * f->x) + (*v[5]  * f->y) + (*v[6]  * f->z) + *v[7];
    s.z = (*v[8]  * f->x) + (*v[9]  * f->y) + (*v[10] * f->z) + *v[11];
    s.w = 1;
    
    return s;
}

void matmul(mat4f *f, mat4f *v) {
    mat4f s;
    memcpy(&s, f, sizeof(mat4f));
    
    *f[0]  = (*v[0]  * s[0])  + (*v[1]  * s[4])  + (*v[2]  * s[8])  + (*v[3]  * s[12]);
    *f[1]  = (*v[0]  * s[1])  + (*v[1]  * s[5])  + (*v[2]  * s[9])  + (*v[3]  * s[13]);
    *f[2]  = (*v[0]  * s[2])  + (*v[1]  * s[6])  + (*v[2]  * s[10]) + (*v[3]  * s[14]);
    *f[3]  = (*v[0]  * s[3])  + (*v[1]  * s[7])  + (*v[2]  * s[11]) + (*v[3]  * s[15]);
    
    *f[4]  = (*v[4]  * s[0])  + (*v[5]  * s[4])  + (*v[6]  * s[8])  + (*v[7]  * s[12]);
    *f[5]  = (*v[4]  * s[1])  + (*v[5]  * s[5])  + (*v[6]  * s[9])  + (*v[7]  * s[13]);
    *f[6]  = (*v[4]  * s[2])  + (*v[5]  * s[6])  + (*v[6]  * s[10]) + (*v[7]  * s[14]);
    *f[7]  = (*v[4]  * s[3])  + (*v[5]  * s[7])  + (*v[6]  * s[11]) + (*v[7]  * s[15]);
    
    *f[8]  = (*v[8]  * s[0])  + (*v[9]  * s[4])  + (*v[10] * s[8])  + (*v[11] * s[12]);
    *f[9]  = (*v[8]  * s[1])  + (*v[9]  * s[5])  + (*v[10] * s[9])  + (*v[11] * s[13]);
    *f[10] = (*v[8]  * s[2])  + (*v[9]  * s[6])  + (*v[10] * s[10]) + (*v[11] * s[14]);
    *f[11] = (*v[8]  * s[3])  + (*v[9]  * s[7])  + (*v[10] * s[11]) + (*v[11] * s[15]);
    
    *f[12] = (*v[12] * s[0])  + (*v[13] * s[4])  + (*v[14] * s[8])  + (*v[15] * s[12]);
    *f[13] = (*v[12] * s[1])  + (*v[13] * s[5])  + (*v[14] * s[9])  + (*v[15] * s[13]);
    *f[14] = (*v[12] * s[2])  + (*v[13] * s[6])  + (*v[14] * s[10]) + (*v[15] * s[14]);
    *f[15] = (*v[12] * s[3])  + (*v[13] * s[7])  + (*v[14] * s[11]) + (*v[15] * s[15]);
}


// well known inverse sqrt from quake 3 source :)
inline float Q_rsqrt( float number ){
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;                       // evil floating point bit level hacking
	i  = 0x5f375a86 - ( i >> 1 );               // what the fuck? (more accurate constant also)
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

void vecnormalize(vec4f *v) {
    float l = Q_rsqrt(v->x*v->x + v->y*v->y + v->z*v->z);
    
	v->x *= l;
	v->y *= l;
	v->z *= l;
}

vec4f vecsub(vec4f *v, vec4f *f) {
    vec4f d;
    
    d.x = f->x - v->x;
    d.y = f->y - v->y;
    d.z = f->z - v->z;
    
    return d;
}

void vecmove(float ax, float ay, float az, vec4f *v) {
    v->x += ax;
    v->y += ay;
    v->z += az;
}

void vecmul(float ax, float ay, float az, vec4f *v) {
    v->x *= ax;
    v->y *= ay;
    v->z *= az;
}

vec4f veccross(vec4f *v, vec4f *f) {
    vec4f d;
    
    d.x = (v->y * f->z) - (v->z * f->y);
    d.y = (v->z * f->x) - (v->x * f->z);
    d.z = (v->x * f->y) - (v->y * f->x);
    
    return d;
}

float vecdot(vec4f *v, vec4f *f) {
    return ((v->x * f->x) + (v->y * f->y) + (v->z * f->z));
}

// LEGACY!

void rotate3d(int ax, int ay, int az, vec4f *v) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[ax], cosx = costabf[ax];
    float siny = sintabf[ay], cosy = costabf[ay];
    float sinz = sintabf[az], cosz = costabf[az];
    float bx, by, bz, px, py, pz;  // temp var storage

        //pt[i] = p[i];
        
        py = v->y;
        pz = v->z;
        v->y = (py * cosx - pz * sinx);
        v->z = (py * sinx + pz * cosx);
        
        px = v->x;
        pz = v->z;
        v->x = (px * cosy - pz * siny);
        v->z = (px * siny + pz * cosy);
        
        px = v->x;
        py = v->y;
        v->x = (px * cosz - py * sinz);
        v->y = (px * sinz + py * cosz);

} 

// reverse rotate
void rotate3dr(int ax, int ay, int az, vec4f *v) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[0xFFFF - ax], cosx = costabf[0xFFFF - ax];
    float siny = sintabf[0xFFFF - ay], cosy = costabf[0xFFFF - ay];
    float sinz = sintabf[0xFFFF - az], cosz = costabf[0xFFFF - az];
    float bx, by, bz, px, py, pz;  // temp var storage

        //pt[i] = p[i];
        
        px = v->x;
        py = v->y;
        v->x = (px * cosz - py * sinz);
        v->y = (px * sinz + py * cosz);
        
        px = v->x;
        pz = v->z;
        v->x = (px * cosy - pz * siny);
        v->z = (px * siny + pz * cosy);
        
        py = v->y;
        pz = v->z;
        v->y = (py * cosx - pz * sinx);
        v->z = (py * sinx + pz * cosx);


}

