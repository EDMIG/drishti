/*!
  @file   arithmetic.h
  @author David Hirvonen (dhirvonen elucideye com)
  @brief  Declaration of optimized vector arithmetic.

  \copyright Copyright 2014-2016 Elucideye, Inc. All rights reserved.
  \license{This project is released under the 3 Clause BSD License.}

*/

#ifndef DRISHTI_CORE_ARITHMETIC_H
#define DRISHTI_CORE_ARITHMETIC_H

#include "core/drishti_core.h"

DRISHTI_CORE_BEGIN

template <typename T> T round(T x);

void add16sAnd16s(const int16_t *pa, const int16_t *pb, int16_t *pc, int n);
void add16sAnd32s(const int32_t *pa, const int16_t *pb, int32_t *pc, int n);
void add32f(const float *pa, const float *pb, float *pc, int n);
void convertFixedPoint(const float *pa, int16_t *pb, int n, int fraction);

DRISHTI_CORE_END

#endif
