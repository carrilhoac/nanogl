
/*
 *  Graphics Library (GL) using Software Rendering (SR)
 *  Copyright (C) Andre Caceres Carrilho, 2010-2017
 *
 *  This code is a minimalistic version of OpenGL aimed
 *  at CPU-based perspective projection and rasterization
 *  of textured triangles. The code is written only for
 *  single-threaded usage without SIMD or SSE instructions
 *  and does not follow Khronos Group standards
 */

#ifndef __gl_common__
#define __gl_common__

#include "gl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GL_COLOR_DEPTH 16

#define GL_FAST_MATH

#define GL_NULL 0
#define GL_CREATED 1
#define GL_READY 2

#define GL_PLANE_NEAR 0
#define GL_PLANE_FAR 1
#define GL_PLANE_PROJECTION 2

GL_INTERNAL(float) __glRsqrt(float);
GL_INTERNAL(void) __glNormalize(glVector3f*);
GL_INTERNAL(void) __glWorldViewMatrix(glCamera*, glMatrix*);
GL_INTERNAL(void) __glRenderPipeline(glContext *,
                            glPolygonBuffer *, glMatrix *);
GL_INTERNAL(void) __glRasterPolygon(glContext* ctx, glPolygon *p);
GL_INTERNAL(void) __glRasterSegment(glContext *ctx, glPolygon *p,
                                    int y1, int y2);
GL_INTERNAL(short) __glGetPixelBilinear(glTexture* img,
                                   float dx, float dy);

#define __glMathAssign(a, b) \
  a.x = b.x; a.y = b.y; a.z = b.z;
#define __glMathSubtract(c, a, b) \
  c.x = a.x - b.x; c.y = a.y - b.y; c.z = a.z - b.z;
#define __glMathDotProduct(a, b) \
  a.x * b.x + a.y * b.y + a.z * b.z
#define __glMathCrossProduct(c, a, b) \
  c.x = a.y * b.z - a.z * b.y; \
  c.y = a.z * b.x - a.x * b.z; \
  c.z = a.x * b.y - a.y * b.x;

#define __glMathProductPtr(c, a, b) \
  c.x = a->m[0][0] * b.x + a->m[0][1] * b.y + a->m[0][2] * b.z + a->m[0][3]; \
  c.y = a->m[1][0] * b.x + a->m[1][1] * b.y + a->m[1][2] * b.z + a->m[1][3]; \
  c.z = a->m[2][0] * b.x + a->m[2][1] * b.y + a->m[2][2] * b.z + a->m[2][3];

#define __glMathProductVar(c, a, b) \
  c.x = a.m[0][0] * b.x + a.m[0][1] * b.y + a.m[0][2] * b.z + a.m[0][3]; \
  c.y = a.m[1][0] * b.x + a.m[1][1] * b.y + a.m[1][2] * b.z + a.m[1][3]; \
  c.z = a.m[2][0] * b.x + a.m[2][1] * b.y + a.m[2][2] * b.z + a.m[2][3];

#ifdef __cplusplus
}
#endif /* !__cplusplus */
#endif /* !__gl_common__ */

