/*
 * Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* clang-format off */

#include "stdioInterf.h"
#include "fioMacros.h"

/* transfer intrinsic */

static int I8(next_index)(__INT_T *index, F90_Desc *s)
{
  __INT_T i;

  for (i = 0; i < F90_RANK_G(s); i++) {
    index[i]++;
    if (index[i] <= DIM_UBOUND_G(s, i))
      return 1; /* keep going */
    index[i] = F90_DIM_LBOUND_G(s, i);
  }
  return 0; /* finished */
}

void ENTFTN(TRANSFER, transfer)(void *rb,         /* result base */
                                void *sb,         /* source base */
                                __INT_T *rs,      /* size of result element */
                                __INT_T *ms,      /* size of mold */
                                F90_Desc *result, /* result descriptor */
                                F90_Desc *source, /* source descriptor */
                                void *rsd, void *msd)
{
  int result_scalar, source_scalar;
  __INT_T extent, size, rsize, ssize;
  __INT_T sindex[7];
  __INT_T k;
  __INT_T i;
  /*
   * when the source is an array, a loop is executed to transfer the
   * source to the result.  Each element is first copied to temp via
   * __fort_get_scalar(), but the 'scalar' can actually be a character
   * object.  Need to check the length of the scalar to ensure that
   * it fits in temp; if not, need to malloc a temp.
   */
  double temp[16]; /* sufficient for a character*128 source */
  char *ptemp;

  result_scalar = F90_TAG_G(result) != __DESC;
  source_scalar = F90_TAG_G(source) != __DESC;
  rsize = *rs;
  if (result_scalar && source_scalar) {
    if (rsize > *ms)
      rsize = *ms;
    __fort_bcopy(rb, sb, rsize);
    return;
  }
  if (!result_scalar) {
    extent = F90_DIM_EXTENT_G(result, 0);
    if (extent < 0)
      extent = 0;
    rsize *= extent;
  }

  /* we assume that result is replicated and contiguous */

  if (source_scalar) {

    /* have to store elements of result */

    ssize = *ms;
    while (ssize > 0 && rsize > 0) {

      /* we have rsize bytes left to copy. See if there are that
       * many left in source */

      size = rsize;
      if (size > ssize)
        size = ssize;
      __fort_bcopy(rb, sb, size);
      rb = (char *)rb + size;
      sb = (char *)sb + size;
      ssize -= size;
      rsize -= size;
    }
    return;
  }

  /* source is an array */

  k = F90_RANK_G(source);
  ssize = *ms;
  for (i = 0; i < k; ++i) {
    sindex[i] = F90_DIM_LBOUND_G(source, i);
    extent = F90_DIM_EXTENT_G(source, i);
    if (extent < 0)
      extent = 0;
    ssize *= extent;
  }
  ptemp = (char *)&temp;
  if (*ms > sizeof(temp)) {
    ptemp = __fort_malloc(*ms);
  }
  while (ssize > 0 && rsize > 0) {
    I8(__fort_get_scalar)(ptemp, sb, source, sindex);
    I8(next_index)(sindex, source);
    size = rsize;
    if (size > *ms)
      size = *ms;
    __fort_bcopy(rb, ptemp, size);
    rb = (char *)rb + size;
    ssize -= size;
    rsize -= size;
  }
  if (ptemp != (char *)&temp) {
    __fort_free(ptemp);
  }
}