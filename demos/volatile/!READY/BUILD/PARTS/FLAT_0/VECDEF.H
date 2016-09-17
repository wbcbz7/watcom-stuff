#ifndef VECDEF_H
#define VECDEF_H

typedef struct {
    int a, b, col;
} line;
        
typedef struct {
    int a, b, c; // vertices index
    int col;     // default color
    vec4f n;    // normal
} face;

typedef struct {
    float u, v;
} uv;

#endif