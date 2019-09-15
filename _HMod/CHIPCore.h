/******************************************************************************
 * CHIPCore.h: Hexagonal Image Processing Framework CHIP Core
 ******************************************************************************
 * v1.0 - 30.06.2017
 *
 * Copyright (c) 2017 Tobias Schlosser
 *  (tobias.schlosser@informatik.tu-chemnitz.de)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/


#ifndef CHIPCORE_H
#define CHIPCORE_H


#include <stdint.h>


#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#ifndef u8
	#define u8  uint8_t
#endif
#ifndef u32
	#define u32 uint32_t
#endif

#ifndef bool
	// #define bool  _Bool
	#define bool  u32 // Bugfix Memory Layout
#endif
#ifndef false
	#define false 0
#endif
#ifndef true
	#define true  1
#endif


#define SIZEOF_ARRAY(array) (sizeof(array) / sizeof(array[0]))


typedef struct { float        x; float        y; } fPoint2d;
typedef struct { int          x; int          y; } iPoint2d;
typedef struct { unsigned int x; unsigned int y; } uPoint2d;

typedef struct { float x; float y; float z; } fPoint3d;

typedef struct { unsigned int value; unsigned int digits; } Hexint;

typedef struct { u8* p; unsigned int x; unsigned int y; } pArray2d;

typedef struct { u8** p; unsigned int size; } Hexarray;


float*   pc_reals;
iPoint2d pc_reals_min;
iPoint2d pc_reals_max;

float*   pc_spatials;
iPoint2d pc_spatials_min;
iPoint2d pc_spatials_max;

unsigned int** pc_nearest;

unsigned int** pc_adds;


void pArray2d_init(pArray2d* array, unsigned int x, unsigned int y);
void pArray2d_free(pArray2d* array);


Hexint       Hexint_init(int value, bool base7);
fPoint2d     getReal(Hexint self);
fPoint2d     getPolar(Hexint self);
unsigned int getInt(Hexint self);
Hexint       neg(Hexint self);
Hexint       add(Hexint self, Hexint object);
Hexint       mul_int(Hexint self, int object);
Hexint       getNearest(float x, float y);
fPoint3d     getHer(Hexint self);
fPoint2d     getSpatial(Hexint self);


void Hexarray_init(Hexarray* hexarray, unsigned int order);
void Hexarray_free(Hexarray* hexarray);


float sinc(float x);
float kernel(float x, float y, unsigned int technique);

void Hexsamp_sq2hex(pArray2d array, Hexarray* hexarray,
 unsigned int order, float scale, unsigned int technique);

void Hexsamp_hex2sq(Hexarray hexarray, pArray2d* array,
 float radius, float scale, unsigned int technique);


#endif

