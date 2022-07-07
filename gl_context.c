
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

GL_EXPORT(glContext*)
glInit(void)
{
  glContext* ctx = (glContext*) malloc(sizeof(glContext));
  if (!ctx)
    return GL_NULL;
  ctx->camera = GL_NULL;
  ctx->frustum = (glFrustum*) malloc(sizeof(glFrustum));
  if (!ctx->frustum)
    return 0;
  ctx->frame_buf = GL_NULL;
  ctx->depth_buf = GL_NULL;
  ctx->state = GL_NULL;
  return ctx;
}

GL_EXPORT(void)
glExit(glContext *context)
{
  if (context) {
    if (context->depth_buf) {
      if (context->depth_buf->depth)
        free(context->depth_buf->depth);
      free(context->depth_buf);
    }
    free(context);
    context = GL_NULL;
  }
}

GL_EXPORT(void)
glLookAt(glContext *context, glCamera *camera)
{
  if (!context || !camera)
    return;
  if (context->state < GL_CREATED)
    return;
  context->camera = camera;
  context->state = GL_READY;
  __glWorldViewMatrix(context->camera, &context->worldview);
}

GL_EXPORT(void)
glClear(glContext *context)
{
  unsigned int i;
  if (!context)
    return;
  if (context->state < GL_CREATED)
    return;
  /* Clearing the depth buffer */
  for (i = 0; i < context->depth_buf->n; ++i) {
    context->depth_buf->depth[i] = context->frustum->plane[GL_PLANE_FAR];
  }
  /* Clearing the frame buffer */
#if defined ALLEGRO_H
/*! FIXME */
 // clear_bitmap(context->frame_buf);
  clear_to_color(context->frame_buf, makecol(60, 60, 60));
#elif defined _SDL_H
  SDL_RenderClear(renderer);
#endif
}

GL_EXPORT(void)
glRender(glContext *context, glPolygonBuffer *object, glMatrix *modelworld)
{
  unsigned int i;
  if (!context || !object)
    return;
  if (context->state < GL_READY)
    return;
  __glRenderPipeline(context, object, modelworld);
  for (i = 0; i < object->n; ++i) {
    if (!object->polys[i].backfacing) {
      __glRasterPolygon(context, &object->polys[i]);
    }
  }
}

GL_EXPORT(int)
glPerspective(glContext *context, glVector2f *viewport_size,
  float z_near, float z_far, float fov)
{
  /* Avoiding segfault */
  if (!viewport_size || !context)
    return -1;
  if (viewport_size->x < 320 || viewport_size->y < 240)
    return -1;
  /* *********************************
   * Resetting the frustum
   * *********************************/
  glFrustum* ft = context->frustum;
  /* Setting the clipping planes */
  ft->plane[GL_PLANE_NEAR] = z_near;
  ft->plane[GL_PLANE_FAR] = z_far;
  /* Center of the screen coordinates
   Used later on perspective projection */
  ft->center.x = viewport_size->x * 0.5f;
  ft->center.y = viewport_size->y * 0.5f;
  /* Distance to the projection plane distance */
  ft->plane[GL_PLANE_PROJECTION] =
    ft->center.x / tan(fov * 0.00872664625997f);

//printf("%f\n", ft->plane[GL_PLANE_PROJECTION]);
  /* Detailed description of computation above can be seen in:
    http://permadi.com/1996/05/ray-casting-tutorial-5/ */
  /** TODO: Check the field of view interval
    [30,120] maybe? look/search online before */
  ft->fov = fov;
  /* *********************************
   * Resetting the Frame Buffer
   * *********************************/
#if defined ALLEGRO_H
  if (context->frame_buf) {
    destroy_bitmap(context->frame_buf);
    context->frame_buf = 0;
  }
  context->frame_buf = create_bitmap(
    viewport_size->x, viewport_size->y);
  if (!context->frame_buf)
    return -1;
#endif
  /* *********************************
   * Resetting the Depth Buffer
   * *********************************/
  glDepthBuffer *db = context->depth_buf;
  if (db) {
    if (db->depth)
      free(db->depth);
    free(db);
  }
  db = (glDepthBuffer*) malloc(sizeof(glDepthBuffer));
  if (!db)
    return -1;
  db->w = (unsigned int) viewport_size->x;
  db->h = (unsigned int) viewport_size->y;
  db->n = db->w * db->h; /* pixel count */
  db->depth = (float*) malloc(db->n * sizeof(float));
  context->depth_buf = db;
  context->state = GL_CREATED;
  return 0;
}
