/******************************************************************************
 * Hex-MuxerHMod.c: CHIPCore-Wrapper für Hex-Muxer
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "CHIPCore.h"

#include "Hex-MuxerHMod.h"


uPoint2d size_out;

pArray2d array;
Hexarray hexarray;
pArray2d array_hex;


// Vorberechnungen

void HexMuxerHMod_init(u32 width_d, u32 height_d,
 u32 order, float scale, float radius) {
	const unsigned int size  = pow(7, order);
	const unsigned int size7 = size * 7;

	Hexint   hi;
	fPoint2d pr;
	fPoint2d ps;


	puts("[1/4] Coordinates:");

	pc_reals    = (float*)malloc(2 * size7 * sizeof(float));
	pc_spatials = (float*)malloc(2 * size  * sizeof(float));

	for(unsigned int i = 0; i < size7; i++) {
		if(!(i % 1000))
			putchar('.');


		hi = Hexint_init(i, 0);

		pr = getReal(hi);
		ps = getSpatial(hi);

		pc_reals[2 * i]     = pr.x;
		pc_reals[2 * i + 1] = pr.y;

		if(i < size) {
			if(pr.x < pc_reals_min.x) {
				pc_reals_min.x = (int)roundf(pr.x);
			} else if(pr.x > pc_reals_max.x) {
				pc_reals_max.x = (int)roundf(pr.x);
			}
			if(pr.y < pc_reals_min.y) {
				pc_reals_min.y = (int)roundf(pr.y);
			} else if(pr.y > pc_reals_max.y) {
				pc_reals_max.y = (int)roundf(pr.y);
			}

			if(ps.x < pc_spatials_min.x) {
				pc_spatials_min.x = (int)ps.x;
			} else if(ps.x > pc_spatials_max.x) {
				pc_spatials_max.x = (int)ps.x;
			}
			if(ps.y < pc_spatials_min.y) {
				pc_spatials_min.y = (int)ps.y;
			} else if(ps.y > pc_spatials_max.y) {
				pc_spatials_max.y = (int)ps.y;
			}
		}
	}

	for(unsigned int i = 0; i < size; i++) {
		if(!(i % 1000))
			putchar('.');


		ps = getSpatial(Hexint_init(i, 0));

		if(ps.y > 1.0f) {
			ps.x -= roundf((ps.y - 1) / 2);
		} else if(ps.y < 0.0f) {
			ps.x -= roundf(ps.y / 2);
		}

		ps.x -= pc_spatials_min.x;
		ps.y  = pc_spatials_max.y - ps.y;

		pc_spatials[2 * i]     = ps.x;
		pc_spatials[2 * i + 1] = ps.y;
	}

	puts("\nOK\n");


	size_out.x = (unsigned int)roundf((pc_reals_max.x - pc_reals_min.x) / scale) + 1;
	size_out.y = (unsigned int)roundf((pc_reals_max.y - pc_reals_min.y) / scale) + 1;

	puts("[2/4] Relationships:");

	pc_nearest = (unsigned int**)malloc(size_out.x * sizeof(unsigned int*));

	for(unsigned int i = 0; i < size_out.x; i++) {
		putchar('.');


		pc_nearest[i] = (unsigned int*)malloc(size_out.y * sizeof(unsigned int));

		for(unsigned int j = 0; j < size_out.y; j++) {
			pc_nearest[i][j] = getInt(Hexint_init(getNearest(pc_reals_min.x + i * scale, \
				pc_reals_min.y + j * scale).value, 1));
		}
	}

	puts("\nOK\n");


	const unsigned int i_max = radius > 1.0f ? 49 : 7; // TODO?

	puts("[3/4] Additions:");

	pc_adds = (unsigned int**)malloc(size7 * sizeof(unsigned int*));

	for(unsigned int i = 0; i < size7; i++) {
		if(!(i % 1000))
			putchar('.');


		const Hexint base = Hexint_init(i, 0);

		pc_adds[i] = (unsigned int*)malloc(i_max * sizeof(unsigned int));

		for(unsigned int j = 0; j < i_max; j++)
			pc_adds[i][j] = getInt(add(base, Hexint_init(j, 0)));
	}

	puts("\nOK\n");


	puts("[4/4] Hex. FBs:");

	pArray2d_init(&array, width_d, height_d);
	Hexarray_init(&hexarray, order);
	pArray2d_init(&array_hex, size_out.x, size_out.y);

	puts("OK\n\n");
}

void HexMuxerHMod_free() {
	free(pc_reals);

	free(pc_spatials);

	for(unsigned int i = 0; i < SIZEOF_ARRAY(pc_nearest); i++) {
		free(pc_nearest[i]);
	}
	free(pc_nearest);

	for(unsigned int i = 0; i < SIZEOF_ARRAY(pc_adds);    i++) {
		free(pc_adds[i]);
	}
	free(pc_adds);


	pArray2d_free(&array);
	Hexarray_free(&hexarray);
	pArray2d_free(&array_hex);
}


void HexMuxerHMod(u8* srcFrame, u8* destFrame,
 u32 width, u32 height, u32 width_d, u32 height_d,
 u32 order, float scale, float radius, u32 mode_i, u32 mode_d) {
	// pArray2d_init(&array, width, height);

	int p     = 0;
	int pd    = 0;
	int slice = 3 * width_d * sizeof(srcFrame[0]);

	for(unsigned int h = 0; h < array.y; h++) {
		memcpy(array.p + pd, srcFrame + p, slice);

		p  += 3 * width;
		pd += 3 * width_d;
	}


	Hexsamp_sq2hex(array, &hexarray, order, 1 / scale, mode_i);


	// pArray2d_free(&array);


	if(!mode_d) {
		const int width_base  = (int)roundf(((int)width_d  - (pc_spatials_max.x - pc_spatials_min.x)) / 2);
		const int height_base = (int)roundf(((int)height_d - (pc_spatials_max.y - pc_spatials_min.y)) / 2);

		for(unsigned int i = 0; i < hexarray.size; i++) {
			const int w = width_base  + (int)pc_spatials[2 * i];
			const int h = height_base + (int)pc_spatials[2 * i + 1];

			if(w >= 0 && h >= 0 && w < width && h < height) {
				p = 3 * (h * width + w);

				destFrame[p]     = hexarray.p[i][0]; // Y
				destFrame[p + 1] = hexarray.p[i][1]; // Cb
				destFrame[p + 2] = hexarray.p[i][2]; // Cr
			}
		}
	} else {
		Hexsamp_hex2sq(hexarray, &array_hex, radius, scale, mode_i);


		const int width_base  = (int)roundf(((int)width_d  - array_hex.x) / 2);
		const int height_base = (int)roundf(((int)height_d - array_hex.y) / 2);

		if(width > array_hex.x) {
			p     = 3 * (  height_base * width       + width_base );
			pd    = 0;
			slice = 3 * array_hex.x * sizeof(array_hex.p[0]);
		} else {
			p     = 0;
			pd    = 3 * ( -height_base * array_hex.x - width_base );
			slice = 3 * width       * sizeof(array_hex.p[0]);
		}

		for(unsigned int h = 0; h < array_hex.y; h++) {
			memcpy(destFrame + p, array_hex.p + pd, slice);

			p  += 3 * width;
			pd += 3 * array_hex.x;
		}


		// pArray2d_free(&array);
	}


	// Hexarray_free(&hexarray);
}

