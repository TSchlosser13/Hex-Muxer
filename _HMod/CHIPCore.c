/******************************************************************************
 * CHIPCore.c: Hexagonal Image Processing Framework CHIP Core
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

#include <math.h>

// #include "xil_printf.h"

#include "CHIPCore.h"


float*   pc_reals     = NULL;
iPoint2d pc_reals_min = { .x = 0, .y = 0 };
iPoint2d pc_reals_max = { .x = 0, .y = 0 };

float*   pc_spatials     = NULL;
iPoint2d pc_spatials_min = { .x = 0, .y = 0 };
iPoint2d pc_spatials_max = { .x = 0, .y = 0 };

unsigned int** pc_nearest = NULL;

unsigned int** pc_adds = NULL;


void pArray2d_init(pArray2d* array, unsigned int x, unsigned int y) {
	array->x = x;
	array->y = y;

	array->p = (u8*)calloc(3 * x * y, sizeof(u8));
}

void pArray2d_free(pArray2d* array) {
	free(array->p);
}


Hexint Hexint_init(int value, bool base7) {
	Hexint self;

	if(value > 0) {
		self.digits = 0;

		if(base7) {
			self.value = value;

			while(value) {
				value /= 10;

				self.digits++;
			}
		} else {
			unsigned int po10       = 1;
			unsigned int self_value = 0;

			while(value) {
				const unsigned int digit = value % 7;

				if(digit < 0) {
					printf("\n > Error - Hexint_init: digit=%i < 0\n", digit);
					// xil_printf("\n > Error - Hexint_init: digit=%i < 0\n", digit);
				}

				self_value += po10 * digit;
				po10       *= 10;
				value      /=  7;

				self.digits++;
			}

			self.value = self_value;
		}
	} else if(!value) {
		self.value  = 0;
		self.digits = 1;
	} else {
		printf("\n > Error - Hexint_init: value=%i < 0\n", value);
		// xil_printf("\n > Error - Hexint_init: value=%i < 0\n", value);
	}

	return self;
}

fPoint2d getReal(Hexint self) {
	const float  sqrt3 = sqrt(3); // TODO?
	fPoint2d     p     = { .x = 0.0f, .y = 0.0f };
	fPoint2d     pc    = { .x = 1.0f, .y = 0.0f };
	unsigned int digit;
	unsigned int value = self.value;

	for(int i = self.digits - 1; i > -1; i--) {
		digit  = value % 10;
		value /= 10;

		if(i < self.digits - 1) {
			const float pcx = pc.x;

			pc.x =     2 * pc.x - sqrt3 * pc.y;
			pc.y = sqrt3 *  pcx +     2 * pc.y;
		}

		// TODO: "+=" und "-="?
		if(digit == 1) {
			p.x += pc.x;
			p.y += pc.y;
		} else if(digit == 2) {
			p.x  = p.x + pc.x / 2 - (sqrt3 * pc.y) / 2;
			p.y  = p.y + (sqrt3 * pc.x) / 2 + pc.y / 2;
		} else if(digit == 3) {
			p.x  = p.x - pc.x / 2 - (sqrt3 * pc.y) / 2;
			p.y  = p.y + (sqrt3 * pc.x) / 2 - pc.y / 2;
		} else if(digit == 4) {
			p.x -= pc.x;
			p.y -= pc.y;
		} else if(digit == 5) {
			p.x  = p.x - pc.x / 2 + (sqrt3 * pc.y) / 2;
			p.y  = p.y - (sqrt3 * pc.x) / 2 - pc.y / 2;
		} else if(digit == 6) {
			p.x  = p.x + pc.x / 2 + (sqrt3 * pc.y) / 2;
			p.y  = p.y - (sqrt3 * pc.x) / 2 + pc.y / 2;
		}
	}

	return p;
}

fPoint2d getPolar(Hexint self) {
	fPoint2d p;

	if(self.value) {
		p = getReal(self);

		const float px = p.x;

		p.x = sqrt(p.x * p.x + p.y * p.y);
		p.y = atan2(p.y, px);
	} else {
		p.x = p.y = 0.0f;
	}

	return p;
}

unsigned int getInt(Hexint self) {
	unsigned int po7   = 1;
	unsigned int sum   = 0;
	unsigned int value = self.value;

	for(int i = self.digits - 1; i > -1; i--) {
		sum   += po7 * (value % 10);
		po7   *=  7;
		value /= 10;
	}

	return sum;
}

Hexint neg(Hexint self) {
	unsigned int digit;
	unsigned int po10  = 1;
	unsigned int sum   = 0;
	unsigned int value = self.value;

	for(int i = self.digits - 1; i > -1; i--) {
		digit = value % 10;

		if(digit == 1) {
			sum += 4 * po10;
		} else if(digit == 2) {
			sum += 5 * po10;
		} else if(digit == 3) {
			sum += 6 * po10;
		} else if(digit == 4) {
			sum +=     po10;
		} else if(digit == 5) {
			sum += 2 * po10;
		} else if(digit == 6) {
			sum += 3 * po10;
		}

		po10  *= 10;
		value /= 10;
	}

	return Hexint_init(sum, 1);
}

Hexint add(Hexint self, Hexint object) {
	const unsigned int M[7][7] = { {  0,  1,  2,  3,  4,  5,  6 }, \
	                               {  1, 63, 15,  2,  0,  6, 64 }, \
	                               {  2, 15, 14, 26,  3,  0,  1 }, \
	                               {  3,  2, 26, 25, 31,  4,  0 }, \
	                               {  4,  0,  3, 31, 36, 42,  5 }, \
	                               {  5,  6,  0,  4, 42, 41, 53 }, \
	                               {  6, 64,  1,  0,  5, 53, 52 } };

	u8*          digits;
	unsigned int len_max;
	unsigned int po10         = 1;
	unsigned int sum          = 0;
	unsigned int self_value   = self.value;
	unsigned int object_value = object.value;


	if((int)(self.digits - object.digits) > 0) {
		len_max = self.digits   + 1;
	} else {
		len_max = object.digits + 1;
	}

	digits = (u8*)malloc(2 * len_max * sizeof(u8));


	for(int i = len_max - 1; i > -1; i--) {
		if(len_max - self.digits)
			digits[i] = self_value % 10;

		if(len_max - object.digits)
			digits[len_max + i] = object_value % 10;

		if(self_value)   self_value   /= 10;
		if(object_value) object_value /= 10;
	}

	for(unsigned int i = 0; i < len_max; i++) {
		const unsigned int j = len_max - i - 1;
			  unsigned int t = M[digits[j]][digits[len_max + j]];
		      unsigned int c = t / 10;

		sum  += po10 * (t % 10);
		po10 *= 10;

		for(unsigned int k = i + 1; k < len_max; k++) {
			const unsigned int l = len_max - k - 1;

			if(c) {
				t         = M[digits[l]][c];
				c         = t / 10;
				digits[l] = t % 10;
			}
		}
	}


	free(digits);

	return Hexint_init(sum, 1);
}

Hexint mul_int(Hexint self, int object) {
	if(object < 0) {
		self = neg(self);
	} else if(!object) {
		return Hexint_init(0, 1);
	} else if(object ==  1) {
		return self;
	}

	Hexint sum = self;

	// for(unsigned int i = 1; i < abs(object); i++)
		// sum = add(sum, self);


	object = abs(object);

	// po2
	if(!(object & (object - 1))) {
		for(unsigned int po2 = 1; po2 < object; po2 *= 2) {
			// case n : sum = n + n
			switch(sum.value) {
				// object =   2
				case      1 : sum = Hexint_init(     63, 1); break;
				case      2 : sum = Hexint_init(     14, 1); break;
				case      4 : sum = Hexint_init(     36, 1); break;
				case      5 : sum = Hexint_init(     41, 1); break;
				// object =   4
				case     63 : sum = Hexint_init(    645, 1); break;
				case     14 : sum = Hexint_init(    156, 1); break;
				case     36 : sum = Hexint_init(    312, 1); break;
				case     41 : sum = Hexint_init(    423, 1); break;
				// object =   8
				case    645 : sum = Hexint_init(    651, 1); break;
				case    156 : sum = Hexint_init(    162, 1); break;
				case    312 : sum = Hexint_init(    324, 1); break;
				case    423 : sum = Hexint_init(    435, 1); break;
				// object =  16
				case    651 : sum = Hexint_init(   5043, 1); break;
				case    162 : sum = Hexint_init(   6054, 1); break;
				case    324 : sum = Hexint_init(   2016, 1); break;
				case    435 : sum = Hexint_init(   3021, 1); break;
				// object =  32
				case   5043 : sum = Hexint_init(  41315, 1); break;
				case   6054 : sum = Hexint_init(  52426, 1); break;
				case   2016 : sum = Hexint_init(  14642, 1); break;
				case   3021 : sum = Hexint_init(  25153, 1); break;
				// object =  64
				case  41315 : sum = Hexint_init(  46511, 1); break;
				case  52426 : sum = Hexint_init(  51622, 1); break;
				case  14642 : sum = Hexint_init(  13244, 1); break;
				case  25153 : sum = Hexint_init(  24355, 1); break;
				// object = 128
				case  46511 : sum = Hexint_init( 430403, 1); break;
				case  51622 : sum = Hexint_init( 540504, 1); break;
				case  13244 : sum = Hexint_init( 160106, 1); break;
				case  24355 : sum = Hexint_init( 210201, 1); break;
				// object = 256
				case 430403 : sum = Hexint_init(3153625, 1); break;
				case 540504 : sum = Hexint_init(4264136, 1); break;
				case 160106 : sum = Hexint_init(6426352, 1); break;
				case 210201 : sum = Hexint_init(1531463, 1); break;
				// order > 7
				default     : sum = add(sum, sum);           break;
			}
		}
	// !po2
	} else {
		const unsigned int factors_size = (unsigned int)trunc(log2(object));
		      Hexint       hfactors[factors_size];
		      unsigned int ufactors[factors_size];
		      unsigned int po2 = 0;
		      unsigned int f   = 0;

		// po2
		for(po2 = 1; po2 <= object / 2; po2 *= 2, f++) {
			hfactors[f] = sum;
			ufactors[f] = po2;


			// case n : sum = n + n
			switch(sum.value) {
				// object =   2
				case      1 : sum = Hexint_init(     63, 1); break;
				case      2 : sum = Hexint_init(     14, 1); break;
				case      4 : sum = Hexint_init(     36, 1); break;
				case      5 : sum = Hexint_init(     41, 1); break;
				// object =   4
				case     63 : sum = Hexint_init(    645, 1); break;
				case     14 : sum = Hexint_init(    156, 1); break;
				case     36 : sum = Hexint_init(    312, 1); break;
				case     41 : sum = Hexint_init(    423, 1); break;
				// object =   8
				case    645 : sum = Hexint_init(    651, 1); break;
				case    156 : sum = Hexint_init(    162, 1); break;
				case    312 : sum = Hexint_init(    324, 1); break;
				case    423 : sum = Hexint_init(    435, 1); break;
				// object =  16
				case    651 : sum = Hexint_init(   5043, 1); break;
				case    162 : sum = Hexint_init(   6054, 1); break;
				case    324 : sum = Hexint_init(   2016, 1); break;
				case    435 : sum = Hexint_init(   3021, 1); break;
				// object =  32
				case   5043 : sum = Hexint_init(  41315, 1); break;
				case   6054 : sum = Hexint_init(  52426, 1); break;
				case   2016 : sum = Hexint_init(  14642, 1); break;
				case   3021 : sum = Hexint_init(  25153, 1); break;
				// object =  64
				case  41315 : sum = Hexint_init(  46511, 1); break;
				case  52426 : sum = Hexint_init(  51622, 1); break;
				case  14642 : sum = Hexint_init(  13244, 1); break;
				case  25153 : sum = Hexint_init(  24355, 1); break;
				// object = 128
				case  46511 : sum = Hexint_init( 430403, 1); break;
				case  51622 : sum = Hexint_init( 540504, 1); break;
				case  13244 : sum = Hexint_init( 160106, 1); break;
				case  24355 : sum = Hexint_init( 210201, 1); break;
				// object = 256
				case 430403 : sum = Hexint_init(3153625, 1); break;
				case 540504 : sum = Hexint_init(4264136, 1); break;
				case 160106 : sum = Hexint_init(6426352, 1); break;
				case 210201 : sum = Hexint_init(1531463, 1); break;
				// order > 7
				default     : sum = add(sum, sum);           break;
			}
		}

		// Rest
		while(f--) {
			const unsigned int rest = object - po2;

			if(rest < ufactors[f]) {
				continue;
			} else {
				sum = add(sum, hfactors[f]);

				if(rest == ufactors[f])
					break;

				po2 += ufactors[f];
			}
		}
	}


	return sum;
}

Hexint getNearest(float x, float y) {
	const float sqrt3 = sqrt(3); // TODO?
	const float r1    = x - y / sqrt3;
	const float r2    = 2 * y / sqrt3;
	Hexint      h     = Hexint_init(0, 1);
	Hexint      h1    = Hexint_init(1, 1);
	Hexint      h2    = Hexint_init(2, 1);

	if(r1 < 0) {
		h1 = Hexint_init(4, 1);

		if(r1 + floor(r1) > 0.5f) {
			// h = add(h, h1);
			h = Hexint_init(4, 1);
		}
	} else if(r1 - floor(r1) > 0.5f) {
		// h = add(h, h1);
		h = Hexint_init(1, 1);
	}

	if(r2 < 0) {
		h2 = Hexint_init(5, 1);

		if(r2 + floor(r2) > 0.5f) {
			// h = add(h, h2);
			if(r1 < 0) {
				h = Hexint_init(42, 1);
			} else {
				h = Hexint_init( 6, 1);
			}
		}
	} else if(r2 - floor(r2) > 0.5f) {
		// h = add(h, h2);
		if(r1 < 0) {
			h = Hexint_init( 3, 1);
		} else {
			h = Hexint_init(15, 1);
		}
	}

	h = add(h, mul_int(h1, abs(trunc(r1))));
	h = add(h, mul_int(h2, abs(trunc(r2))));

	return h;
}

fPoint3d getHer(Hexint self) {
	fPoint3d     p     = { .x = 0.0f, .y = 0.0f, .z =  0.0f };
	fPoint3d     pc    = { .x = 1.0f, .y = 0.0f, .z = -1.0f };
	unsigned int digit;
	unsigned int value = self.value;

	for(int i = self.digits - 1; i > -1; i--) {
		digit  = value % 10;
		value /= 10;

		if(i < self.digits - 1) {
			const fPoint2d pcxy = { .x = pc.x, .y = pc.y };

			pc.x = (  4 *   pc.x - 5 *   pc.y +     pc.z ) / 3;
			pc.y = (      pcxy.x + 4 *   pc.y - 5 * pc.z ) / 3;
			pc.z = ( -5 * pcxy.x +     pcxy.y + 4 * pc.z ) / 3;
		}

		if(digit == 1) {
			p.x += pc.x;
			p.y += pc.y;
			p.z += pc.z;
		} else if(digit == 2) {
			p.x -= pc.y;
			p.y -= pc.z;
			p.z -= pc.x;
		} else if(digit == 3) {
			p.x += pc.z;
			p.y += pc.x;
			p.z += pc.y;
		} else if(digit == 4) {
			p.x -= pc.x;
			p.y -= pc.y;
			p.z -= pc.z;
		} else if(digit == 5) {
			p.x += pc.y;
			p.y += pc.z;
			p.z += pc.x;
		} else if(digit == 6) {
			p.x -= pc.z;
			p.y -= pc.x;
			p.z -= pc.y;
		}
	}

	return p;
}

fPoint2d getSpatial(Hexint self) {
	      fPoint2d p2;
	const fPoint3d p3 = getHer(self);

	p2.x = (  p3.x +     p3.y - 2 * p3.z ) / 3;
	p2.y = ( -p3.x + 2 * p3.y -     p3.z ) / 3;

	return p2;
}


void Hexarray_init(Hexarray* hexarray, unsigned int order) {
	hexarray->size = pow(7, order);

	hexarray->p = (u8**)malloc(hexarray->size * sizeof(u8*));
	for(unsigned int i = 0; i < hexarray->size; i++) {
		hexarray->p[i] = (u8*)calloc(3, sizeof(u8));
	}
}

void Hexarray_free(Hexarray* hexarray) {
	for(unsigned int i = 0; i < hexarray->size; i++) {
		free(hexarray->p[i]);
	}
	free(hexarray->p);
}


float sinc(float x) {
	return x ? sin(M_PI * x) / (M_PI * x) : 1.0f;
}

float kernel(float x, float y, unsigned int technique) {
	fPoint2d f;

	// BL
	if(!technique) {
		const fPoint2d abs = { .x = fabs(x), .y = fabs(y) };

		if(abs.x >= 0 && abs.x < 1) {
			f.x = 1 - abs.x;
		} else {
			f.x = 0.0f;
		}
		if(abs.y >= 0 && abs.y < 1) {
			f.y = 1 - abs.y;
		} else {
			f.y = 0.0f;
		}
	// BC
	} else if(technique == 1) {
		const fPoint2d abs   = { .x = fabs(x),       .y = fabs(y) };
		const fPoint2d abs_2 = { .x = abs.x * abs.x, .y = abs.y * abs.y };

		if(abs.x >= 0 && abs.x < 1) {
			f.x = 2 * abs_2.x * abs.x - 3 * abs_2.x + 1;
		} else {
			f.x = 0.0f;
		}
		if(abs.y >= 0 && abs.y < 1) {
			f.y = 2 * abs_2.y * abs.y - 3 * abs_2.y + 1;
		} else {
			f.y = 0.0f;
		}
	// Lanczos
	} else if(technique == 2) {
		const fPoint2d abs = { .x = fabs(x), .y = fabs(y) };

		if(abs.x >= 0 && abs.x < 1) {
			f.x = sinc(abs.x) * sinc(abs.x / 2);
		} else {
			f.x = 0.0f;
		}
		if(abs.y >= 0 && abs.y < 1) {
			f.y = sinc(abs.y) * sinc(abs.y / 2);
		} else {
			f.y = 0.0f;
		}
	// B-Splines (B_3)
	} else {
		const fPoint2d abs   = { .x = 2 * fabs(x),   .y = 2 * fabs(y) };
		const fPoint2d abs_2 = { .x = abs.x * abs.x, .y = abs.y * abs.y };

		if(abs.x < 1) {
			f.x = (3 * abs_2.x * abs.x - 6 * abs_2.x + 4) / 6;
		} else if(abs.x < 2) {
			f.x = (-abs_2.x * abs.x + 6 * abs_2.x - 12 * abs.x + 8) / 6;
		} else {
			f.x = 0.0f;
		}
		if(abs.y < 1) {
			f.y = (3 * abs_2.y * abs.y - 6 * abs_2.y + 4) / 6;
		} else if(abs.y < 2) {
			f.y = (-abs_2.y * abs.y + 6 * abs_2.y - 12 * abs.y + 8) / 6;
		} else {
			f.y = 0.0f;
		}
	}

	return f.x * f.y;
}

void Hexsamp_sq2hex(pArray2d array, Hexarray* hexarray,
 unsigned int order, float scale, unsigned int technique) {
	const fPoint2d cart_a = { .x = array.x / 2.0f, .y = array.y / 2.0f };

	// Hexarray_init(hexarray, order);

	for(unsigned int i = 0; i < hexarray->size; i++) {
		const unsigned int row      = (unsigned int)roundf(cart_a.y - scale * pc_reals[2 * i + 1]);
		const unsigned int col      = (unsigned int)roundf(cart_a.x + scale * pc_reals[2 * i]);
		const float        row_hex  =                      cart_a.y - scale * pc_reals[2 * i + 1];
		const float        col_hex  =                      cart_a.x + scale * pc_reals[2 * i];
		      float        out[3]   = { 0.0f, 0.0f, 0.0f };
		      float        out_n    =   0.0f;

		// Patch Transformation: Reichweite (7×7 -> 3×3 = 49 -> 9)
		// for(int x = col - 3; x < col + 4; x++) {
		for(int x = col - 1; x < col + 2; x++) {
			// for(int y = row - 3; y < row + 4; y++) {
			for(int y = row - 1; y < row + 2; y++) {
				if(x >= 0 && x < array.x && y >= 0 && y < array.y) {
					const float        xh = fabs(col_hex - x);
					const float        yh = fabs(row_hex - y);
					      float        k  = kernel(xh, yh, technique);
					const unsigned int p  = 3 * (y * array.x + x);


					// Patch Transformation: Flächeninhalt
					if((xh > 0.5f && xh < 1.0f) || (yh > 0.5f && yh < 1.0f)) {
						float factor = 1.0f;

						if(xh > 0.5f)
							factor *= (1.5f - xh);

						if(yh > 0.5f)
							factor *= (1.5f - yh);

						k *= factor;
					}


					out[0] += k * array.p[p];
					out[1] += k * array.p[p + 1];
					out[2] += k * array.p[p + 2];
					out_n  += k;
				}
			}
		}

		// Patch Transformation: Normalisierung
		if(out_n > 0.0f) {
			hexarray->p[i][0] = (int)roundf(out[0] / out_n);
			hexarray->p[i][1] = (int)roundf(out[1] / out_n);
			hexarray->p[i][2] = (int)roundf(out[2] / out_n);
		} else {
			hexarray->p[i][0] = (int)roundf(out[0]);
			hexarray->p[i][1] = (int)roundf(out[1]);
			hexarray->p[i][2] = (int)roundf(out[2]);
		}
	}
}

void Hexsamp_hex2sq(Hexarray hexarray, pArray2d* array,
 float radius, float scale, unsigned int technique) {
	// array->x = (unsigned int)roundf((pc_reals_max.x - pc_reals_min.x) / scale) + 1;
	// array->y = (unsigned int)roundf((pc_reals_max.y - pc_reals_min.y) / scale) + 1;
	// pArray2d_init(array, array->x, array->y);


	fPoint2d cart_a = { .x = pc_reals_min.x, .y = pc_reals_min.y };
	const unsigned int i_max = radius > 1.0f ? 49 : 7; // TODO?

	for(unsigned int y = 0; y < array->y; y++) {
		for(unsigned int x = 0; x < array->x; x++) {
			float out[3] = { 0.0f, 0.0f, 0.0f };
			float out_n  =   0.0f;


			for(unsigned int i = 0; i < i_max; i++) {
				// const unsigned int hi = getInt(add(getNearest(cart_a.x, cart_a.y), Hexint_init(i, 0)));
				const unsigned int hi = pc_adds[pc_nearest[x][y]][i];
				// const unsigned int hi = pc_adds[x][y][i]; // schlechtere Lokalität

				if(hi < hexarray.size) {
					const fPoint2d cart_ha = { .x = pc_reals[2 * hi],     \
					                           .y = pc_reals[2 * hi + 1] };

					if(fabs(cart_a.x - cart_ha.x) <= radius &&
					   fabs(cart_a.y - cart_ha.y) <= radius) {
						const float k = kernel(cart_a.x - cart_ha.x, cart_a.y - cart_ha.y, technique);

						out[0] += k * hexarray.p[hi][0];
						out[1] += k * hexarray.p[hi][1];
						out[2] += k * hexarray.p[hi][2];
						out_n  += k;
					}
				}
			}


			const unsigned int p = 3 * ((array->y - y - 1) * array->x + x);

			// Patch Transformation: Normalisierung
			if(out_n > 0.0f) {
				out[0] = roundf(out[0] / out_n);
				out[1] = roundf(out[1] / out_n);
				out[2] = roundf(out[2] / out_n);
			} else {
				out[0] = roundf(out[0]);
				out[1] = roundf(out[1]);
				out[2] = roundf(out[2]);
			}

			if(out[0] < 255) {
				array->p[p]     = (int)out[0];
			} else {
				array->p[p]     = 255;
			}
			if(out[1] < 255) {
				array->p[p + 1] = (int)out[1];
			} else {
				array->p[p + 1] = 255;
			}
			if(out[2] < 255) {
				array->p[p + 2] = (int)out[2];
			} else {
				array->p[p + 2] = 255;
			}


			cart_a.x += scale;
		}

		cart_a.x  = pc_reals_min.x;
		cart_a.y += scale;
	}
}

