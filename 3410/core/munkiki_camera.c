/* Munkiki's Castles: the camera of the 3D view (the tail of d.m), kept
   apart from the rasterizer so that each fits its bank on the Game Boy.
   The rotation and the moves it works out are read by munkiki_3d.c
   (munkiki_3d_int.h). */
#include <string.h>

#include "munkiki_int.h"
#include "munkiki_3d_int.h"

/* A quarter turn is 64 of these. */
#define ANGLE_TURN 256


/* sin of 0 to 90 degrees in 64 steps, 1.14. */
static const int16_t sine_table[65] = {
    0, 402, 804, 1205, 1606, 2006, 2404, 2801, 3196, 3590, 3981, 4370, 4756, 5139, 5520, 5897,
    6270, 6639, 7005, 7366, 7723, 8076, 8423, 8765, 9102, 9434, 9760, 10080, 10394, 10702, 11003, 11297,
    11585, 11866, 12140, 12406, 12665, 12916, 13160, 13395, 13623, 13842, 14053, 14256, 14449, 14635, 14811, 14978,
    15137, 15286, 15426, 15557, 15679, 15791, 15893, 15986, 16069, 16143, 16207, 16261, 16305, 16340, 16364, 16379,
    16384
};


static int16_t sine(uint8_t angle)
{
    angle &= ANGLE_TURN - 1;
    if (angle < 64)
        return sine_table[angle];
    if (angle < 128)
        return sine_table[128 - angle];
    if (angle < 192)
        return (int16_t)-sine_table[angle - 128];
    return (int16_t)-sine_table[256 - angle];
}


static int16_t cosine(uint8_t angle)
{
    return sine((uint8_t)(angle + 64));
}


/* sin and cos of the look-around angles, -45 to 45 degrees by 5, 1.14. */
static const int16_t sine5[10] = { 0, 1428, 2845, 4240, 5604, 6924, 8192, 9397, 10531, 11585 };
static const int16_t cosine5[10] = { 16384, 16322, 16135, 15826, 15396, 14849, 14189, 13420, 12551, 11585 };


/* The pitch of 20 degrees. */
#define PITCH_SIN 5603
#define PITCH_COS 15396


int16_t mk3d_rot[9];
int32_t mk3d_at_x, mk3d_at_y, mk3d_at_z;
int32_t mk3d_move_x, mk3d_move_y, mk3d_move_z;
uint8_t mk3d_rot_serial;
#define rot mk3d_rot
#define at_x mk3d_at_x
#define at_y mk3d_at_y
#define at_z mk3d_at_z
#define move_x mk3d_move_x
#define move_y mk3d_move_y
#define move_z mk3d_move_z
/* Where the eye is in the world, in sixteenths of a cell, for the order. */
static int16_t eye_x, eye_y, eye_z;
int16_t mk3d_eye_x, mk3d_eye_y, mk3d_eye_z;


static void matrix_mul(int16_t *out, const int16_t *a, const int16_t *b)
{
    uint8_t i, j, k;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            int32_t s = 0;

            for (k = 0; k < 3; k++)
                s += (int32_t)a[i * 3 + k] * b[k * 3 + j];
            out[i * 3 + j] = (int16_t)(s >> FIX);
        }
    }
}


static void matrix_apply(int32_t *out, const int16_t *m, int32_t x, int32_t y, int32_t z)
{
    out[0] = (m[0] * x + m[1] * y + m[2] * z) >> FIX;
    out[1] = (m[3] * x + m[4] * y + m[5] * z) >> FIX;
    out[2] = (m[6] * x + m[7] * y + m[8] * z) >> FIX;
}


/* glRotate about y by `angle` (binary), and about x by (s, c). */
static void rotation_y(int16_t *m, uint8_t angle)
{
    int16_t s = sine(angle), c = cosine(angle);

    m[0] = c;
    m[1] = 0;
    m[2] = s;
    m[3] = 0;
    m[4] = FIX_ONE;
    m[5] = 0;
    m[6] = (int16_t)-s;
    m[7] = 0;
    m[8] = c;
}


static void rotation_x(int16_t *m, int16_t s, int16_t c)
{
    m[0] = FIX_ONE;
    m[1] = 0;
    m[2] = 0;
    m[3] = 0;
    m[4] = c;
    m[5] = (int16_t)-s;
    m[6] = 0;
    m[7] = s;
    m[8] = c;
}


/* The tail of d.m: the camera for this frame. */
static void camera(void)
{
    int16_t ry[9], rx[9], tmp[9];
    int32_t v[3];
    int8_t deg;
    uint8_t yaw = (uint8_t)((mk.facing * 64 + 32768L) >> 16);
    int32_t py = mk.py < 65536L ? 65536L : mk.py;

    /* Munkiki's centre, (x, y + 0.5, z) cells, in sixteenths of a unit. */
    at_x = (mk.px * 20) >> 12;
    at_y = ((py + 32768L) * 20) >> 12;
    at_z = (mk.pz * 20) >> 12;
    rotation_y(ry, yaw);
    if (mk.phase != 1) {
        /* Rx(pitch) Ry(yaw), written out: four products. */
        int16_t ys = sine(yaw), yc = cosine(yaw);

        rot[0] = yc;
        rot[1] = 0;
        rot[2] = ys;
        rot[3] = (int16_t)(((int32_t)PITCH_SIN * ys) >> FIX);
        rot[4] = PITCH_COS;
        rot[5] = (int16_t)(-((int32_t)PITCH_SIN * yc) >> FIX);
        rot[6] = (int16_t)(-((int32_t)PITCH_COS * ys) >> FIX);
        rot[7] = PITCH_SIN;
        rot[8] = (int16_t)(((int32_t)PITCH_COS * yc) >> FIX);
        move_x = 0;
        move_y = -8;
        move_z = -320;
    } else {
        /* Looking around: T(0, -0.5, -2) Rx(c0) Ry(c1) T(0, 0, c0 / 8) Ry(yaw). */
        int16_t s, c;

        deg = (int8_t)(mk.look_yaw >> 16);
        s = deg < 0 ? (int16_t)-sine5[-deg / 5] : sine5[deg / 5];
        c = cosine5[(deg < 0 ? -deg : deg) / 5];
        tmp[0] = c;
        tmp[1] = 0;
        tmp[2] = s;
        tmp[3] = 0;
        tmp[4] = FIX_ONE;
        tmp[5] = 0;
        tmp[6] = (int16_t)-s;
        tmp[7] = 0;
        tmp[8] = c;
        deg = (int8_t)(mk.look_pitch >> 16);
        s = deg < 0 ? (int16_t)-sine5[-deg / 5] : sine5[deg / 5];
        c = cosine5[(deg < 0 ? -deg : deg) / 5];
        rotation_x(rx, s, c);
        matrix_mul(rot, rx, tmp);   /* Rx(c0) Ry(c1) */
        matrix_apply(v, rot, 0, 0, ((int32_t)deg * 16) / 8);
        move_x = v[0];
        move_y = v[1] - 8;
        move_z = v[2] - 32;
        memcpy(tmp, rot, sizeof tmp);
        matrix_mul(rot, tmp, ry);
    }
    /* The eye is where eye = 0: at - rot^T move, in sixteenths of a cell. */
    {
        int32_t ex = -((int32_t)rot[0] * move_x + rot[3] * move_y + rot[6] * move_z) >> FIX;
        int32_t ey = -((int32_t)rot[1] * move_x + rot[4] * move_y + rot[7] * move_z) >> FIX;
        int32_t ez = -((int32_t)rot[2] * move_x + rot[5] * move_y + rot[8] * move_z) >> FIX;

        eye_x = (int16_t)(((int16_t)(at_x + ex) * 4) / 5);
        eye_y = (int16_t)(((int16_t)(at_y + ey) * 4) / 5);
        eye_z = (int16_t)(((int16_t)(at_z + ez) * 4) / 5);
    }
}



void mk3d_camera(int32_t bob) MK_FAR
{
    camera();
    mk3d_rot_serial++;
    at_y += bob;
    mk3d_eye_x = eye_x;
    mk3d_eye_y = eye_y;
    mk3d_eye_z = eye_z;
}
