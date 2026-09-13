/* San Angeles Observation - shared DirectX 11 and DirectX 12 scene.
 * Geometry and camera tracks derived from Jetro Lauha's 2004-2005 demo.c.
 * Copyright (c) 2004-2005 Jetro Lauha. See license-BSD.txt.
 */
#ifndef SAN_ANGELES_SCENE_H
#define SAN_ANGELES_SCENE_H

#include "shapes.h"
#include "cams.h"

#define SA_PI 3.14159265358979323846f
#define SA_RUN_LENGTH (20 * CAMTRACK_LEN)
#define SA_VERTEX_LIMIT 65536

/* x87 helpers keep the executable independent of the C math runtime. */
#pragma warning(push)
#pragma warning(disable:4035) /* x87 leaves the return value in ST(0). */
static double saSinD(double x) { __asm fld x __asm fsin }
static double saCosD(double x) { __asm fld x __asm fcos }
static float saSin(float x) { __asm fld x __asm fsin }
static float saCos(float x) { __asm fld x __asm fcos }
static float saSqrt(float x) { __asm fld x __asm fsqrt }
static int saInt(float x)
{
    int result;
    __asm fld x
    __asm fistp result
    return result;
}
static double saAbs(double x) { __asm fld x __asm fabs }
static double saPow(double x, double exponent)
{
    static const union { unsigned __int64 bits; double value; } infinity = { 0x7ff0000000000000ui64 };
    if (exponent == 0) return 1;
    if (x == 0) return exponent > 0 ? 0 : infinity.value;
    if (x == infinity.value) return exponent > 0 ? infinity.value : 0;
    __asm {
        fld exponent
        fld x
        fyl2x
        fld st(0)
        frndint
        fsub st(1), st(0)
        fxch st(1)
        f2xm1
        fld1
        faddp st(1), st(0)
        fscale
        fstp st(1)
    }
}
#pragma warning(pop)

typedef struct { float x, y, z; } SA_VECTOR;
typedef struct { SA_VECTOR position, normal; unsigned int color; } SA_VERTEX;
typedef struct { unsigned int first, count; } SA_MESH;
typedef struct {
    float eye[4], right[4], up[4], forward[4];
    float translateScale[4], rotateMirrorLight[4];
} SA_CONSTANTS;

#ifdef SA_EXTERNAL_VERTICES
/* DX12 generates directly into its mapped upload buffer. */
static SA_VERTEX *saVertices;
#else
static SA_VERTEX saVertices[SA_VERTEX_LIMIT];
#endif
static SA_MESH saMeshes[SUPERSHAPE_COUNT + 1];
static unsigned int saVertexCount, saSeed;

static unsigned int saRandom(void)
{
    saSeed = saSeed * 0x343fdu + 0x269ec3u;
    return saSeed >> 16;
}

static void saSubtract(SA_VECTOR *result,const SA_VECTOR *a,const SA_VECTOR *b)
{
    result->x=a->x-b->x; result->y=a->y-b->y; result->z=a->z-b->z;
}
static void saCross(SA_VECTOR *result,const SA_VECTOR *a,const SA_VECTOR *b)
{
    result->x=a->y*b->z-a->z*b->y;
    result->y=a->z*b->x-a->x*b->z;
    result->z=a->x*b->y-a->y*b->x;
}
static void saNormalize(SA_VECTOR *a)
{
    float length = saSqrt(a->x*a->x + a->y*a->y + a->z*a->z);
    if (length > 0) { a->x/=length; a->y/=length; a->z/=length; }
}
static float saRadius(float t, const float *p)
{
    // The large negative powers need double intermediates, as in demo.c.
    // Narrowing either inner power to float can erase entire shape features.
    return (float)saPow(saPow(saAbs(saCosD(p[0]*t/4)) / p[1], p[4]) +
                        saPow(saAbs(saSinD(p[0]*t/4)) / p[2], p[5]), 1/p[3]);
}
static void saPoint(SA_VECTOR *result,float r1,float r2,float t,float p)
{
    result->x=(float)(saCosD(t)*saCosD(p)/r1/r2);
    result->y=(float)(saSinD(t)*saCosD(p)/r1/r2);
    result->z=(float)(saSinD(p)/r2);
}
static void saVertex(const SA_VECTOR *p,const SA_VECTOR *n,unsigned int color)
{
    SA_VERTEX *v = &saVertices[saVertexCount++];
    v->position = *p; v->normal = *n; v->color = color;
}

static void saCreateScene(void)
{
    unsigned int shape;
    int x, y, a;
    static const unsigned char corners[6] = { 0, 1, 3, 1, 2, 3 };
    saSeed = 15;
    saVertexCount = 0;
    for (shape = 0; shape < SUPERSHAPE_COUNT; ++shape) {
        const float *params = sSuperShapeParams[shape];
        int longitudeCount = saInt(params[12]), resolution = saInt(params[13]);
        int begin = resolution/4, end = resolution/2;
        float baseColor[3];
        saMeshes[shape].first = saVertexCount;
        for (a = 0; a < 3; ++a) baseColor[a] = (float)((saRandom()%155) + 100);
        for (x = 0; x < longitudeCount; ++x) for (y = begin; y < end; ++y) {
            float longitude[2],latitude[2],radius[4];
            for (a = 0; a < 2; ++a) {
                longitude[a] = -SA_PI + (x+a)*2*SA_PI/longitudeCount;
                latitude[a] = -SA_PI/2 + (y+a)*2*SA_PI/resolution;
                radius[a] = saRadius(longitude[a],params);
                radius[a+2] = saRadius(latitude[a],params+6);
            }
            if (radius[0] != 0 && radius[1] != 0 && radius[2] != 0 && radius[3] != 0) {
                SA_VECTOR p[4], normal,edge1,edge2;
                unsigned int color = 0xff000000u;
                for (a = 0; a < 4; ++a) {
                    int u = ((a+1)>>1)&1, v = a>>1;
                    saPoint(&p[a],radius[u],radius[v+2],longitude[u],latitude[v]);
                }
                if (y == begin+1) p[0].z = p[1].z = 0;
                saSubtract(&edge1,&p[1],&p[0]); saSubtract(&edge2,&p[3],&p[0]);
                saCross(&normal,&edge1,&edge2); saNormalize(&normal);
                for (a = 0; a < 3; ++a) {
                    int channel = saInt((p[0].z+0.5f)*baseColor[a]);
                    if (channel > 255) channel = 255;
                    if (channel < 0) channel = 0;
                    color |= (unsigned int)channel << (a*8);
                }
                for (a = 0; a < 6; ++a) saVertex(&p[corners[a]],&normal,color);
            }
        }
        saMeshes[shape].count = saVertexCount - saMeshes[shape].first;
    }
    saMeshes[SUPERSHAPE_COUNT].first = saVertexCount;
    for (y = -15; y < 15; ++y) for (x = -15; x < 15; ++x) {
        unsigned int gray = (saRandom()&0x5f) + 81;
        for (a = 0; a < 6; ++a) {
            int xm = x + ((0x1c >> a)&1), ym = y + ((0x31 >> a)&1);
            float offset = (float)(saCosD(xm*2)*saSinD(ym*4)*0.75f);
            SA_VECTOR p = { xm*4 + offset, ym*4 + offset, 0 }, n = { 0,0,1 };
            saVertex(&p,&n,0xff000000u | gray*0x010101u);
        }
    }
    saMeshes[SUPERSHAPE_COUNT].count = saVertexCount - saMeshes[SUPERSHAPE_COUNT].first;
}

/* Resolve from absolute time, so skipped frames can cross several cuts safely. */
static void saCamera(unsigned int tick, SA_CONSTANTS *c)
{
    unsigned int track = 0, start = 0, length;
    float values[5], fraction, fade;
    SA_VECTOR eye, target, forward, right, up;
    CAMTRACK *cam;
    int a;
    while (track+1 < sizeof(sCamTracks)/sizeof(sCamTracks[0]) &&
           tick >= start + sCamTracks[track].len*CAMTRACK_LEN) {
        start += sCamTracks[track].len*CAMTRACK_LEN;
        ++track;
    }
    cam = &sCamTracks[track];
    length = cam->len*CAMTRACK_LEN;
    fraction = (float)(tick-start)/length;
    for (a = 0; a < 5; ++a) values[a] = (cam->src[a]+cam->dest[a]*fraction)*0.01f;
    eye.x = target.x = values[0]; eye.y = target.y = values[1]; eye.z = target.z = values[2];
    if (cam->dist) {
        eye.x -= saCos(values[3])*cam->dist*0.1f;
        eye.y -= saSin(values[3])*cam->dist*0.1f;
        eye.z -= values[4];
    } else {
        target.x += saCos(values[3]); target.y += saSin(values[3]); target.z += values[4];
    }
    saSubtract(&forward,&target,&eye); saNormalize(&forward);
    /* cross(forward, {0,0,1}) without passing another vector by value. */
    right.x = forward.y; right.y = -forward.x; right.z = 0;
    saNormalize(&right); saCross(&up,&right,&forward);
    c->eye[0] = eye.x; c->eye[1] = eye.y; c->eye[2] = eye.z;
    c->right[0] = right.x; c->right[1] = right.y; c->right[2] = right.z;
    c->up[0] = up.x; c->up[1] = up.y; c->up[2] = up.z;
    c->forward[0] = forward.x; c->forward[1] = forward.y; c->forward[2] = forward.z;
    fade = (float)(tick-start);
    if (start+length-tick < tick-start) fade = (float)(start+length-tick);
    c->eye[3] = fade < 1024 ? fade/1024 : 1;
}

#endif
