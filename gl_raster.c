
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

// http://www.luki.webzdarma.cz/luki_engine_en.htm

#include "gl_common.h"

#if GL_COLOR_DEPTH == 8
  #define _GL_RAWPTR (unsigned char*)
#elif GL_COLOR_DEPTH == 16
  #define _GL_RAWPTR (short*)
#endif

/*
 * Triangle Texture Mapper source (adapted):
 * http://www.lysator.liu.se/~mikaelk/doc/perspectivetexture/
 */
#define S_DIZDX     0
#define S_DUIZDX    1
#define S_DVIZDX    2
#define S_DIZDY     3
#define S_DUIZDY    4
#define S_DVIZDY    5
#define S_XA        6
#define S_XB        7
#define S_UIZA      8
#define S_VIZA      9
#define S_IZA       10
#define S_DXDYA     11
#define S_DXDYB     12
#define S_DUIZDYA   13
#define S_DVIZDYA   14
#define S_DIZDYA    15

static float sp[16];

GL_INTERNAL (void)
__glRasterPolygon (glContext* ctx, glPolygon *p)
{

  /* Shift XY coordinate system (+0.5, +0.5) to
   * match the subpixeling technique */
  float x1 = p->verts[0].screen.x + 0.5f;
  float y1 = p->verts[0].screen.y + 0.5f;

  float x2 = p->verts[1].screen.x + 0.5f;
  float y2 = p->verts[1].screen.y + 0.5f;

  float x3 = p->verts[2].screen.x + 0.5f;
  float y3 = p->verts[2].screen.y + 0.5f;

  /* Calculate alternative 1/Z, U/Z and V/Z
   * values which will be interpolated */
  float iz1 = 1 / p->verts[0].screen.z;
  float iz2 = 1 / p->verts[1].screen.z;
  float iz3 = 1 / p->verts[2].screen.z;

  float uiz1 = p->verts[0].texture.x * iz1;
  float uiz2 = p->verts[1].texture.x * iz2;
  float uiz3 = p->verts[2].texture.x * iz3;

  float viz1 = p->verts[0].texture.y * iz1;
  float viz2 = p->verts[1].texture.y * iz2;
  float viz3 = p->verts[2].texture.y * iz3;

  float dy;

  /* Sort the vertices in ascending Y order */
#define _GL_SWAP(s, t) dy = s; s = t; t = dy;
  if (y1 > y2) {
    _GL_SWAP (x1, x2)
    _GL_SWAP (y1, y2)
    _GL_SWAP (iz1, iz2)
    _GL_SWAP (uiz1, uiz2)
    _GL_SWAP (viz1, viz2)
  }
  if (y1 > y3) {
    _GL_SWAP (x1, x3)
    _GL_SWAP (y1, y3)
    _GL_SWAP (iz1, iz3)
    _GL_SWAP (uiz1, uiz3)
    _GL_SWAP (viz1, viz3)
  }
  if (y2 > y3) {
    _GL_SWAP (x2, x3)
    _GL_SWAP (y2, y3)
    _GL_SWAP (iz2, iz3)
    _GL_SWAP (uiz2, uiz3)
    _GL_SWAP (viz2, viz3)
  }
#undef _GL_SWAP

  /* Integer casting the Y-Coords */
  int y1i = y1;
  int y2i = y2;
  int y3i = y3;

  /* Skip poly if it's too thin to cover any pixels at all */
  if (y1i == y2i && y1i == y3i)
    return;

  /* Calculate horizontal and vertical increments for UV axes (these
    calcs are certainly not optimal, although they're stable
    (handles any dy being 0) */

  dy = 1.0f / ( (x3 - x1) * (y2 - y1) - (x2 - x1) * (y3 - y1) );

  sp[S_DIZDX] = ( (iz3 - iz1) * (y2 - y1) - (iz2 - iz1) * (y3 - y1) ) * dy;
  sp[S_DIZDY] = ( (iz2 - iz1) * (x3 - x1) - (iz3 - iz1) * (x2 - x1) ) * dy;

  sp[S_DUIZDX] = ( (uiz3 - uiz1) * (y2 - y1) - (uiz2 - uiz1) * (y3 - y1) ) * dy;
  sp[S_DVIZDX] = ( (viz3 - viz1) * (y2 - y1) - (viz2 - viz1) * (y3 - y1) ) * dy;

  sp[S_DUIZDY] = ( (uiz2 - uiz1) * (x3 - x1) - (uiz3 - uiz1) * (x2 - x1) ) * dy;
  sp[S_DVIZDY] = ( (viz2 - viz1) * (x3 - x1) - (viz3 - viz1) * (x2 - x1) ) * dy;

  float dxdy1 = 0.0f;
  float dxdy2 = 0.0f;
  float dxdy3 = 0.0f;

  if (y2 > y1)
    dxdy1 = (x2 - x1) / (y2 - y1);
  if (y3 > y1)
    dxdy2 = (x3 - x1) / (y3 - y1);
  if (y3 > y2)
    dxdy3 = (x3 - x2) / (y3 - y2);

  int side = dxdy2 > dxdy1;

  if (y1 == y2)
    side = x1 > x2;
  if (y2 == y3)
    side = x3 > x2;

  if (!side) { /* Longer edge is on the left side */
    /* Calculate slopes along left edge */
    sp[S_DXDYA]   = dxdy2;
    sp[S_DIZDYA]  = dxdy2 * sp[S_DIZDX]  + sp[S_DIZDY];
    sp[S_DUIZDYA] = dxdy2 * sp[S_DUIZDX] + sp[S_DUIZDY];
    sp[S_DVIZDYA] = dxdy2 * sp[S_DVIZDX] + sp[S_DVIZDY];

    /* Perform subpixel pre-stepping along left edge */
    dy = 1 - (y1 - y1i);

    sp[S_XA]   = x1   + dy * sp[S_DXDYA];
    sp[S_IZA]  = iz1  + dy * sp[S_DIZDYA];
    sp[S_UIZA] = uiz1 + dy * sp[S_DUIZDYA];
    sp[S_VIZA] = viz1 + dy * sp[S_DVIZDYA];

    if (y1i < y2i) { /* Draw upper segment if possibly visible */
      /* Set right edge X-slope and perform subpixel pre-stepping */
      sp[S_XB] = x1 + dy * dxdy1;
      sp[S_DXDYB] = dxdy1;

      __glRasterSegment (ctx, p, y1i, y2i);
    }
    if (y2i < y3i) { /* Draw lower segment if possibly visible */
      /* Set right edge X-slope and perform subpixel pre-stepping */
      sp[S_XB] = x2 + (1 - (y2 - y2i) ) * dxdy3;
      sp[S_DXDYB] = dxdy3;

      __glRasterSegment (ctx, p, y2i, y3i);
    }
  } else { /* Longer edge is on the right side */
    dy = 1 - (y1 - y1i);

    sp[S_DXDYB] = dxdy2;
    sp[S_XB] = x1 + dy * sp[S_DXDYB];

    if (y1i < y2i) { /* Draw upper segment if possibly visible */
      /* Set slopes along left edge and perform subpixel pre-stepping */
      sp[S_DXDYA]   = dxdy1;
      sp[S_DIZDYA]  = dxdy1 * sp[S_DIZDX]  + sp[S_DIZDY];
      sp[S_DUIZDYA] = dxdy1 * sp[S_DUIZDX] + sp[S_DUIZDY];
      sp[S_DVIZDYA] = dxdy1 * sp[S_DVIZDX] + sp[S_DVIZDY];

      sp[S_XA]   = x1   + dy * sp[S_DXDYA];
      sp[S_IZA]  = iz1  + dy * sp[S_DIZDYA];
      sp[S_UIZA] = uiz1 + dy * sp[S_DUIZDYA];
      sp[S_VIZA] = viz1 + dy * sp[S_DVIZDYA];

      __glRasterSegment (ctx, p, y1i, y2i);
    }
    if (y2i < y3i) { /* Draw lower segment if possibly visible */
      /* Set slopes along left edge and perform subpixel pre-stepping */
      sp[S_DXDYA]   = dxdy3;
      sp[S_DIZDYA]  = dxdy3 * sp[S_DIZDX]  + sp[S_DIZDY];
      sp[S_DUIZDYA] = dxdy3 * sp[S_DUIZDX] + sp[S_DUIZDY];
      sp[S_DVIZDYA] = dxdy3 * sp[S_DVIZDX] + sp[S_DVIZDY];

      dy = 1 - (y2 - y2i);

      sp[S_XA]   = x2   + dy * sp[S_DXDYA];
      sp[S_IZA]  = iz2  + dy * sp[S_DIZDYA];
      sp[S_UIZA] = uiz2 + dy * sp[S_DUIZDYA];
      sp[S_VIZA] = viz2 + dy * sp[S_DVIZDYA];

      __glRasterSegment (ctx, p, y2i, y3i);
    }
  }
}

GL_INTERNAL (void)
__glRasterSegment (glContext *ctx, glPolygon *p, int y1, int y2)
{
  int x1, x2, xclip, yclip, zid;
  float z, u, v, dx;
  float iz, uiz, viz;

  while (y1 < y2) {
    x1 = sp[S_XA];
    x2 = sp[S_XB];

    dx = 1 - (sp[S_XA] - x1);

    iz  = sp[S_IZA]  + dx * sp[S_DIZDX];
    uiz = sp[S_UIZA] + dx * sp[S_DUIZDX];
    viz = sp[S_VIZA] + dx * sp[S_DVIZDX];

    /* Clipping the frame-buffer rect (Y-axis) */
    yclip = y1 >= 0 && y1 < ctx->frame_buf->h;

    /* Depth-buffer pixel index */
    zid = y1 * ctx->frame_buf->w + x1;

    while (x1++ < x2) {
      z = 1 / iz;
      u = uiz * z;
      v = viz * z;

      zid++;

      /* Clipping the frame-buffer rect (X-axis) */
      xclip = x1 >= 0 && x1 < ctx->frame_buf->w;

      if (xclip && yclip &&
          /* Clipping near and far planes (frustum) */
          z > ctx->frustum->plane[GL_PLANE_NEAR] &&
          z < ctx->frustum->plane[GL_PLANE_FAR]) {
        /* Z-Buffer sort (depth) */
        if (z < ctx->depth_buf->depth[zid]) {
          ctx->depth_buf->depth[zid] = z;
          /* Nearest Neighbour */
          (_GL_RAWPTR ctx->frame_buf->line[y1]) [x1] =
            (_GL_RAWPTR p->texptr->line[ (int) v]) [ (int) u];
        }
      }

      /* Step 1/Z, U/Z and V/Z horizontally */
      iz  += sp[S_DIZDX];
      uiz += sp[S_DUIZDX];
      viz += sp[S_DVIZDX];
    }
    /* Step along both edges */
    sp[S_XA] += sp[S_DXDYA];
    sp[S_XB] += sp[S_DXDYB];

    sp[S_IZA] += sp[S_DIZDYA];

    sp[S_UIZA] += sp[S_DUIZDYA];
    sp[S_VIZA] += sp[S_DVIZDYA];

    y1++;
  }
}
