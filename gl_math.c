
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

#include "gl_common.h"

/*
 * Fast Square Root for 32 bit Floating Point
 * Based on Quake III implementation (id Software)
 * Simplified Newton-Raphson with cast manipulation
 */
GL_INTERNAL(float)
__glRsqrt(float x)
{
  float __half_in = 0.5f * x;
  /* The union avoids strict-aliasing
    warnings given by GCC */
  union {
    float __xflt;
    int __xint;
  } __glSqrtContext;
  __glSqrtContext.__xflt = x;
  /* Computing the initial guess */
  __glSqrtContext.__xint = 0x5f3759df -
    (__glSqrtContext.__xint >> 1);
  /* Newton step, repeating increases accuracy */
  return __glSqrtContext.__xflt *
    (1.5f - __glSqrtContext.__xflt *
            __glSqrtContext.__xflt * __half_in);
}

GL_INTERNAL(void)
__glNormalize(glVector3f *__vec)
{
#ifdef GL_FAST_MATH
  float __rl = __glRsqrt(
#else
  float __rl = 1.0f / sqrtf(
#endif
    __vec->x * __vec->x +
    __vec->y * __vec->y +
    __vec->z * __vec->z);
  __vec->x *= __rl;
  __vec->y *= __rl;
  __vec->z *= __rl;
}

/* Similar purpuse and usage from gluLookAt() */
GL_INTERNAL(void)
__glWorldViewMatrix(glCamera *__cam, glMatrix *__wvm)
{
  glVector3f n, u, v;
  __glMathAssign(n, __cam->dir)
  __glMathCrossProduct(u, __cam->dir, __cam->up)
  __glMathCrossProduct(v, n, u);
  /* TODO: Check if it is necessary to normalize
   the vectors before constructing the matrix */
  __wvm->m[0][0] = u.x;
  __wvm->m[0][1] = u.y;
  __wvm->m[0][2] = u.z;
  __wvm->m[0][3] = -__glMathDotProduct(u, __cam->eye);
  __wvm->m[1][0] = v.x;
  __wvm->m[1][1] = v.y;
  __wvm->m[1][2] = v.z;
  __wvm->m[1][3] = -__glMathDotProduct(v, __cam->eye);
  __wvm->m[2][0] = n.x;
  __wvm->m[2][1] = n.y;
  __wvm->m[2][2] = n.z;
  __wvm->m[2][3] = -__glMathDotProduct(n, __cam->eye);
}

#define _GL_CVERT obj->polys[i].verts[j]
GL_INTERNAL(void)
__glRenderPipeline(glContext *ctx, glPolygonBuffer *obj, glMatrix *mw)
{
  unsigned int i, j; float __rp;
  glVector3f edge1, edge2, delta;
  for (i = 0; i < obj->n; ++i) {
  /* *********************************
   * Model-World-View transformation
   * *********************************/
    if (!mw) {
      for (j = 0; j < 3; ++j) {
        /* Not specified model-world transformation */
        __glMathProductVar(_GL_CVERT.view, ctx->worldview, _GL_CVERT.model)
    } }
    else {
      for (j = 0; j < 3; ++j) {
        __glMathProductPtr(_GL_CVERT.world, mw, _GL_CVERT.model)
        __glMathProductVar(_GL_CVERT.view, ctx->worldview, _GL_CVERT.world)
    } }
  /* *********************************
   * Normal vector (View space)
   * *********************************/
      __glMathSubtract(edge1, obj->polys[i].verts[1].view, obj->polys[i].verts[0].view)
      __glMathSubtract(edge2, obj->polys[i].verts[2].view, obj->polys[i].verts[0].view)
      __glMathCrossProduct(obj->polys[i].normal, edge1, edge2)
    /* inline vector normalization */
    #ifdef GL_FAST_MATH
      __rp = __glRsqrt(
    #else
      __rp = 1.0f / sqrtf(
    #endif
    #define _GL_NORMAL obj->polys[i].normal
      _GL_NORMAL.x * _GL_NORMAL.x +
      _GL_NORMAL.y * _GL_NORMAL.y +
      _GL_NORMAL.z * _GL_NORMAL.z);
      _GL_NORMAL.x *= __rp;
      _GL_NORMAL.y *= __rp;
      _GL_NORMAL.z *= __rp;
    #undef _GL_NORMAL
  /* *********************************
   * Backface culling
   * *********************************/
    #ifdef GL_FAST_MATH
      __rp = __glRsqrt(
    #else
      __rp = 1.0f / sqrtf(
    #endif
    #define _GL_DELTA obj->polys[i].verts[0].view
      _GL_DELTA.x * _GL_DELTA.x +
      _GL_DELTA.y * _GL_DELTA.y +
      _GL_DELTA.z * _GL_DELTA.z);
    /* Consider the viewpoint at (0,0,0), therefore the vector
     * below represents the direction we are seeing the polygon */
      delta.x = __rp * _GL_DELTA.x;
      delta.y = __rp * _GL_DELTA.y;
      delta.z = __rp * _GL_DELTA.z;
    #undef _GL_DELTA
    /* Computing the cosine of the angle between normal and delta vectors
     * If backfacing, the computed cosine will be negative */
      __rp = __glMathDotProduct(delta, obj->polys[i].normal);
      obj->polys[i].backfacing = (__rp < 0.0f) ? (1) : (0);
      if (obj->polys[i].backfacing)
        continue;
  /* *********************************
   * View-Screen transformation
   * *********************************/
    for (j = 0; j < 3; ++j) {
    #define _GL_FRUST ctx->frustum
      /* Trick 1: using the "projection plane distance"
       * instead of the near plane distance */
      __rp = _GL_FRUST->plane[GL_PLANE_PROJECTION] / _GL_CVERT.view.z;
      /* Trick 2: translating to the center of the screen.
       * This is possibile by using Trick 1 division */
      _GL_CVERT.screen.x = _GL_CVERT.view.x * __rp + _GL_FRUST->center.x;
      _GL_CVERT.screen.y = _GL_CVERT.view.y * __rp + _GL_FRUST->center.y;
      _GL_CVERT.screen.z = _GL_CVERT.view.z;
    }
    #undef _GL_FRUST
  }
}
#undef _GL_CVERT
