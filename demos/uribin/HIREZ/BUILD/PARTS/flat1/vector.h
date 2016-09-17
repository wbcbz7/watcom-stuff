typedef struct {
    float x, y, z, col;
} vertex;

typedef struct {
    int a, b, col;
} line;

typedef struct {
    int a, b, c; // vertices index
    int col;     // default color
    vertex n;    // normal
} face;

typedef struct { int x, y; } vertex2d;

// general routines
void vecrotate (int ax, int ay, int az, vertex *v) {
    // hehehe, this code is fully ported from my old freebasic demoz ;)
    int i;
    float sinx = sintabf[ax], cosx = costabf[ax];
    float siny = sintabf[ay], cosy = costabf[ay];
    float sinz = sintabf[az], cosz = costabf[az];
    float bx, by, bz, px, py, pz;  // temp var storage

        
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

void vecnormalize(vertex *v) {
	float l = 1.0 / sqrt(v->x*v->x + v->y*v->y + v->z*v->z);

	v->x *= l;
	v->y *= l;
	v->z *= l;
}

vertex vecsub (vertex *v, vertex *f) {
    vertex d;
    
    d.x = f->x - v->x;
    d.y = f->y - v->y;
    d.z = f->z - v->z;
    
    return d;
}

void vecmove (float ax, float ay, float az, vertex *v) {
    v->x += ax;
    v->y += ay;
    v->z += az;
}

void vecmul (float ax, float ay, float az, vertex *v) {
    
    v->x *= ax;
    v->y *= ay;
    v->z *= az;
}

vertex veccross (vertex *v, vertex *f) {
    vertex d;
    
    d.x = (v->y * f->z) - (v->z * f->y);
    d.y = (v->z * f->x) - (v->x * f->z);
    d.z = (v->x * f->y) - (v->y * f->x);
    
    return d;
}

float vecdot (vertex *v, vertex *f) {
    return ((v->x * f->x) + (v->y * f->y) + (v->z * f->z));
}

void vecproject2d (vertex *v, vertex2d *f) {
    float t;
    
    if (v->z < 0) {
        t = DIST / (v->z + ee);
        f->x = (v->x * t) + (X_SIZE >> 1);
        f->y = (v->y * t) + (Y_SIZE >> 1);
    } 
}

void vecproject2dp (vertex *v, vertex2d *f) {
    
    if (v->z < 0) {
        f->x = v->x + (X_SIZE >> 1);
        f->y = v->y + (Y_SIZE >> 1);
    } 
}
