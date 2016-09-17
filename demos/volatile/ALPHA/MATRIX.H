#ifndef MATRIX_CPP
#define MATRIX_CPP

typedef float mat4f[16];
typedef float mat3f[9];

typedef struct {
    float x, y, z, w;
} vec4f;

typedef struct {
    float x, y;
} vec2f;

typedef struct vec3f {
    float x, y, z;
} vec3f;

typedef struct vec2 {
    int x, y;
} vec2;

void matidentity(mat4f *v);
void matscale(mat4f *v, vec4f *f);
void mattranslate(mat4f *v, vec4f *f);
void mateuler(mat4f *v, int rx, int ry, int rz);
vec4f vecmatmul(vec4f *f, mat4f *v);
vec4f vecmatmulf(vec4f *f, mat4f *v);
void matmul(mat4f *f, mat4f *v);
inline float Q_rsqrt( float number );
void vecnormalize(vec4f *v);
vec4f vecsub(vec4f *v, vec4f *f);
void vecmove(float ax, float ay, float az, vec4f *v);
void vecmul(float ax, float ay, float az, vec4f *v);
vec4f veccross(vec4f *v, vec4f *f);
float vecdot(vec4f *v, vec4f *f);
void rotate3d(int ax, int ay, int az, vec4f *v);

#endif