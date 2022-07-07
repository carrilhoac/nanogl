
/*
**  Graphics Library (GL) using Software Rendering (SR)
**  Copyright (C) Andre Caceres Carrilho, 2010-2017
**
**  This code is a minimalistic version of OpenGL aimed
**  at CPU-based perspective projection and rasterization
**  of textured triangles. The code is written only for
**  single-threaded usage without SIMD or SSE instructions
**  and does not follow Khronos Group standards
*/

#ifndef __gl_h__
#define __gl_h__

#include <allegro.h>
#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! TODO: Fix this DLL Export */
#define GL_EXPORT(x) x
#define GL_INTERNAL(x) x

#if defined ALLEGRO_H
  #define glTexture BITMAP
#elif defined _SDL_H
  #define glTexture SDL_Texture
#else
  #error Dependency missing
#endif

typedef uint8_t glBool;
typedef int32_t glInt;
typedef uint32_t glSize;

typedef struct {
  float x, y;
} glVector2f;

typedef struct {
  float x, y, z;
} glVector3f;

typedef struct {
  float m[3][4];
} glMatrix;

typedef struct {
  glVector3f model;
  glVector3f world;
  glVector3f view;
  glVector3f screen;
  glVector2f texture;
} glVertex;

typedef struct {
  glBool backfacing;
  glVertex verts[3];
  glVector3f normal;
  glTexture *texptr;
} glPolygon;

typedef struct {
  glPolygon *polys;
  glSize n;
} glPolygonBuffer;

typedef struct {
  float *depth;
  glSize w, h, n;
} glDepthBuffer;

typedef struct {
  glVector2f center;
  float plane[3];
  float fov;
} glFrustum;

typedef struct {
  glVector3f eye;
  glVector3f dir;
  glVector3f up;
} glCamera;

typedef struct {
  glInt state;
  glMatrix worldview;
  glCamera *camera;
  glFrustum *frustum;
  glTexture *frame_buf;
  glDepthBuffer *depth_buf;
} glContext;

GL_EXPORT(glContext*) glInit(void);
GL_EXPORT(void) glExit(glContext *context);
GL_EXPORT(void) glClear(glContext *context);
GL_EXPORT(void) glLookAt(glContext *context, glCamera *camera);
GL_EXPORT(void) glRender(glContext *context, glPolygonBuffer *object, glMatrix *modelworld);
GL_EXPORT(glInt) glPerspective(glContext *context,
  glVector2f *viewport_size, float z_near, float z_far, float fov);

#ifdef __cplusplus
}
#endif /* !__cplusplus */
#endif /* !__gl_h__ */
