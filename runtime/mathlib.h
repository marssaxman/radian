// Copyright 2013 Mars Saxman
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the
// use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software in a
// product, an acknowledgment in the product documentation would be appreciated
// but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.

#ifndef radian_mathlib_h
#define radian_mathlib_h

#include "closures.h"

extern struct closure math_sin;
extern struct closure math_cos;
extern struct closure math_tan;
extern struct closure math_asin;
extern struct closure math_acos;
extern struct closure math_atan;
extern struct closure math_atan2;
extern struct closure math_sinh;
extern struct closure math_cosh;
extern struct closure math_tanh;
extern struct closure math_asinh;
extern struct closure math_acosh;
extern struct closure math_atanh;
extern struct closure to_float;
extern struct closure floor_float;
extern struct closure ceiling_float;
extern struct closure truncate_float;

#endif
