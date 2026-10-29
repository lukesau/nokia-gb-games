/* Munkiki's Castles: what the rasterizer (munkiki_3d.c) and the camera
   (munkiki_camera.c) share, in different banks on the Game Boy. */
#ifndef CORE_MUNKIKI_3D_INT_H
#define CORE_MUNKIKI_3D_INT_H

#include <stdint.h>

/* Fixed point: 1.14 for the rotation, so that a product with a position
   in sixteenths stays in 32 bits. */
#define FIX 14
#define FIX_ONE (1L << FIX)

/* The camera: eye = rot · (p - at) + move, p and the results in
   sixteenths of a unit. */
extern int16_t mk3d_rot[9];
extern int32_t mk3d_at_x, mk3d_at_y, mk3d_at_z;
extern int32_t mk3d_move_x, mk3d_move_y, mk3d_move_z;
/* Counts the cameras set, for what the rasterizer keeps between meshes. */
extern uint8_t mk3d_rot_serial;

#endif
