/*
** autogenerated content - DO NOT EDIT
*/
/*
  Copyright (C) 2007 Thomas Jahns <Thomas.Jahns@gmx.net>

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <time.h>
#include <sys/time.h>

#include "core/assert_api.h"
#include "core/bitpackstring.h"
#include "core/error.h"
#include "core/ensure.h"
#include "core/log.h"
#include "core/ma.h"
#include "core/yarandom.h"

enum {
/*   MAX_RND_NUMS = 10, */
  MAX_RND_NUMS = 100000,
};

static inline int
icmp(uint64_t a, uint64_t b)
{
  if (a > b)
    return 1;
  else if (a < b)
    return -1;
  else /* if (a == b) */
    return 0;
}

/**
 * \brief bit count reference
 * @param v count the number of bits set in v
 */
static inline int
genBitCount(uint64_t v)
{
  unsigned c; /* c accumulates the total bits set in v */
  for (c = 0; v; c++)
    v &= v - 1; /* clear the least significant bit set */
  return c;
}

#define freeResourcesAndReturn(retval) \
  do {                                 \
    gt_free(numBitsList);              \
    gt_free(randSrc);                  \
    gt_free(randCmp);                  \
    gt_free(bitStore);                 \
    gt_free(bitStoreCopy);             \
    return retval;                     \
  } while (0)

int
gt_bitPackStringInt64_unit_test(GtError *err)
{
  BitString bitStore = NULL;
  BitString bitStoreCopy = NULL;
  uint64_t *randSrc = NULL; /*< create random ints here for input as bit
                                *  store */
  uint64_t *randCmp = NULL; /*< used for random ints read back */
  unsigned *numBitsList = NULL;
  size_t i, numRnd;
  BitOffset offsetStart, offset;
  int had_err = 0;
  offset = offsetStart = random()%(sizeof (uint64_t) * CHAR_BIT);
  numRnd = random() % (MAX_RND_NUMS + 1);
  gt_log_log("offset=%lu, numRnd=%lu\n",
          (long unsigned)offsetStart, (long unsigned)numRnd);
  {
    BitOffset numBits = sizeof (uint64_t) * CHAR_BIT * numRnd + offsetStart;
    randSrc = gt_malloc(sizeof (uint64_t)*numRnd);
    bitStore = gt_malloc(bitElemsAllocSize(numBits) * sizeof (BitElem));
    bitStoreCopy = gt_calloc(bitElemsAllocSize(numBits), sizeof (BitElem));
    randCmp = gt_malloc(sizeof (uint64_t)*numRnd);
  }
  /* first test unsigned types */
  gt_log_log("gt_bsStoreUInt64/gt_bsGetUInt64: ");
  for (i = 0; i < numRnd; ++i)
  {
#if 64 > 32 && LONG_BIT < 64
    uint64_t v = randSrc[i] = (uint64_t)random() << 32 | random();
#else /* 64 > 32 && LONG_BIT < 64 */
    uint64_t v = randSrc[i] = random();
#endif /* 64 > 32 && LONG_BIT < 64 */
    int bits = gt_requiredUInt64Bits(v);
    gt_bsStoreUInt64(bitStore, offset, bits, v);
    offset += bits;
  }
  offset = offsetStart;
  for (i = 0; i < numRnd; ++i)
  {
    uint64_t v = randSrc[i];
    int bits = gt_requiredUInt64Bits(v);
    uint64_t r = gt_bsGetUInt64(bitStore, offset, bits);
    ensure(had_err, r == v);
    if (had_err)
    {
      gt_log_log("Expected %"PRIu64", got %"PRIu64", i = %lu\n",
              v, r, (unsigned long)i);
      freeResourcesAndReturn(had_err);
    }
    offset += bits;
  }
  gt_log_log("passed\n");
  if (numRnd > 0)
  {
    uint64_t v = randSrc[0], r = 0;
    unsigned numBits = gt_requiredUInt64Bits(v);
    BitOffset i = offsetStart + numBits;
    uint64_t mask = ~(uint64_t)0;
    if (numBits < 64)
      mask = ~(mask << numBits);
    gt_log_log("bsSetBit, gt_bsClearBit, bsToggleBit, gt_bsGetBit: ");
    while (v)
    {
      int lowBit = v & 1;
      v >>= 1;
      ensure(had_err, lowBit == (r = gt_bsGetBit(bitStore, --i)));
      if (had_err)
      {
        gt_log_log("Expected %d, got %d, i = %llu\n",
                lowBit, (int)r, (unsigned long long)i);
        freeResourcesAndReturn(had_err);
      }
    }
    i = offsetStart + numBits;
    gt_bsClear(bitStoreCopy, offsetStart, numBits, random()&1);
    v = randSrc[0];
    while (i)
    {
      int lowBit = v & 1;
      v >>= 1;
      if (lowBit)
        bsSetBit(bitStoreCopy, --i);
      else
        gt_bsClearBit(bitStoreCopy, --i);
    }
    v = randSrc[0];
    r = gt_bsGetUInt64(bitStoreCopy, offsetStart, numBits);
    ensure(had_err, r == v);
    if (had_err)
    {
      gt_log_log("Expected %"PRIu64", got %"PRIu64"\n", v, r);
      freeResourcesAndReturn(had_err);
    }
    for (i = 0; i < numBits; ++i)
      bsToggleBit(bitStoreCopy, offsetStart + i);
    r = gt_bsGetUInt64(bitStoreCopy, offsetStart, numBits);
    ensure(had_err, r == (v = (~v & mask)));
    if (had_err)
    {
      gt_log_log("Expected %"PRIu64", got %"PRIu64"\n", v, r);
      freeResourcesAndReturn(had_err);
    }
    gt_log_log("passed\n");
  }
  if (numRnd > 1)
  {
    gt_log_log("gt_bsCompare: ");
    {
      uint64_t v0 = randSrc[0];
      int bits0 = gt_requiredUInt64Bits(v0);
      uint64_t r0;
      offset = offsetStart;
      r0 = gt_bsGetUInt64(bitStore, offset, bits0);
      for (i = 1; i < numRnd; ++i)
      {
        uint64_t v1 = randSrc[i];
        int bits1 = gt_requiredUInt64Bits(v1);
        uint64_t r1 = gt_bsGetUInt64(bitStore, offset + bits0, bits1);
        int result = -2;   /*< -2 is not a return value of gt_bsCompare, thus
                            *   if it is displayed, there was an earlier
                            *   error. */
        ensure(had_err, r0 == v0 && r1 == v1);
        ensure(had_err, icmp(v0, v1) ==
               (result = gt_bsCompare(bitStore, offset, bits0,
                                   bitStore, offset + bits0, bits1)));
        if (had_err)
        {
          gt_log_log("Expected v0 %s v1, got v0 %s v1,\n for v0=%"
                  PRIu64" and v1=%"PRIu64",\n"
                  "i = %lu, bits0=%u, bits1=%u\n",
                  (v0 > v1?">":(v0 < v1?"<":"==")),
                  (result > 0?">":(result < 0?"<":"==")), v0, v1,
                  (unsigned long)i, bits0, bits1);
          freeResourcesAndReturn(had_err);
        }
        offset += bits0;
        bits0 = bits1;
        v0 = v1;
        r0 = r1;
      }
    }
    gt_log_log("passed\n");
  }
  gt_log_log("gt_bsStoreUniformUInt64Array/gt_bsGetUInt64: ");
  {
    unsigned numBits = random()%64 + 1;
    uint64_t mask = ~(uint64_t)0;
    if (numBits < 64)
      mask = ~(mask << numBits);
    offset = offsetStart;
    gt_bsStoreUniformUInt64Array(bitStore, offset, numBits, numRnd,
                                     randSrc);
    for (i = 0; i < numRnd; ++i)
    {
      uint64_t v = randSrc[i] & mask;
      uint64_t r = gt_bsGetUInt64(bitStore, offset, numBits);
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log("Expected %"PRIu64", got %"PRIu64",\n"
                "i = %lu, bits=%u\n", v, r, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    gt_log_log("passed\n");
    gt_log_log("gt_bsStoreUniformUInt64Array/"
               "gt_bsGetUniformUInt64Array: ");
    gt_bsGetUniformUInt64Array(bitStore, offset = offsetStart,
                               numBits, numRnd, randCmp);
    for (i = 0; i < numRnd; ++i)
    {
      uint64_t v = randSrc[i] & mask;
      uint64_t r = randCmp[i];
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log( "Expected %"PRIu64", got %"PRIu64",\n"
                " i = %lu, bits=%u\n",
                v, r, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
    }
    if (numRnd > 1)
    {
      uint64_t v = randSrc[0] & mask;
      uint64_t r;
      gt_bsGetUniformUInt64Array(bitStore, offsetStart,
                                 numBits, 1, &r);
      if (r != v)
      {
        gt_log_log("Expected %"PRIu64", got %"PRIu64","
                " one value extraction\n",
                v, r);
        freeResourcesAndReturn(had_err);
      }
    }
    gt_log_log(" passed\n");
  }
  /* int types */
  gt_log_log("gt_bsStoreInt64/gt_bsGetInt64: ");
  for (i = 0; i < numRnd; ++i)
  {
    int64_t v = (int64_t)randSrc[i];
    unsigned bits = gt_requiredInt64Bits(v);
    gt_bsStoreInt64(bitStore, offset, bits, v);
    offset += bits;
  }
  offset = offsetStart;
  for (i = 0; i < numRnd; ++i)
  {
    int64_t v = randSrc[i];
    unsigned bits = gt_requiredInt64Bits(v);
    int64_t r = gt_bsGetInt64(bitStore, offset, bits);
    ensure(had_err, r == v);
    if (had_err)
    {
      gt_log_log("Expected %"PRId64", got %"PRId64",\n"
                  "i = %lu, bits=%u\n",
                  v, r, (unsigned long)i, bits);
      freeResourcesAndReturn(had_err);
    }
    offset += bits;
  }
  gt_log_log("passed\n");
  gt_log_log("gt_bsStoreUniformInt64Array/gt_bsGetInt64: ");
  {
    unsigned numBits = random()%64 + 1;
    int64_t mask = ~(int64_t)0;
    if (numBits < 64)
      mask = ~(mask << numBits);
    offset = offsetStart;
    gt_bsStoreUniformInt64Array(bitStore, offset, numBits, numRnd,
                                (int64_t *)randSrc);
    for (i = 0; i < numRnd; ++i)
    {
      int64_t m = (int64_t)1 << (numBits - 1);
      int64_t v = (int64_t)((randSrc[i] & mask) ^ m) - m;
      int64_t r = gt_bsGetInt64(bitStore, offset, numBits);
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log("Expected %"PRId64", got %"PRId64",\n"
                    "i = %lu, numBits=%u\n",
                    v, r, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    gt_log_log("passed\n");
    gt_log_log("gt_bsStoreUniformInt64Array/"
               "gt_bsGetUniformInt64Array: ");
    gt_bsGetUniformInt64Array(bitStore, offset = offsetStart,
                              numBits, numRnd, (int64_t *)randCmp);
    for (i = 0; i < numRnd; ++i)
    {
      int64_t m = (int64_t)1 << (numBits - 1);
      int64_t v = (int64_t)((randSrc[i] & mask) ^ m) - m;
      int64_t r = randCmp[i];
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log("Expected %"PRId64", got %"PRId64", i = %lu\n",
                v, r, (unsigned long)i);
        freeResourcesAndReturn(had_err);
      }
    }
    if (numRnd > 0)
    {
      int64_t m = (int64_t)1 << (numBits - 1);
      int64_t v = (int64_t)((randSrc[0] & mask) ^ m) - m;
      int64_t r = 0;
      gt_bsGetUniformInt64Array(bitStore, offsetStart,
                                numBits, 1, &r);
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log("Expected %"PRId64", got %"PRId64
                ", one value extraction\n",
                v, r);
        freeResourcesAndReturn(had_err);
      }
    }
    gt_log_log("passed\n");
  }

  gt_log_log("gt_bsStoreNonUniformUInt64Array/gt_bsGetUInt64: ");
  {
    BitOffset bitsTotal = 0;
    numBitsList = gt_malloc(sizeof (unsigned) * numRnd);
    for (i = 0; i < numRnd; ++i)
      bitsTotal += (numBitsList[i] = random()%64 + 1);
    offset = offsetStart;
    gt_bsStoreNonUniformUInt64Array(bitStore, offset, numRnd, bitsTotal,
                                     numBitsList, randSrc);
    for (i = 0; i < numRnd; ++i)
    {
      unsigned numBits = numBitsList[i];
      uint64_t mask = (numBits < 64)?
        ~((~(uint64_t)0) << numBits):~(uint64_t)0;
      uint64_t v = randSrc[i] & mask;
      uint64_t r = gt_bsGetUInt64(bitStore, offset, numBits);
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log("Expected %"PRIu64", got %"PRIu64",\n"
                "i = %lu, bits=%u\n",
                v, r, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    gt_log_log("passed\n");
    gt_log_log("gt_bsStoreNonUniformUInt64Array/"
            "gt_bsGetNonUniformUInt64Array: ");
    gt_bsGetNonUniformUInt64Array(bitStore, offset = offsetStart,
                                   numRnd, bitsTotal, numBitsList, randCmp);
    for (i = 0; i < numRnd; ++i)
    {
      unsigned numBits = numBitsList[i];
      uint64_t mask = (numBits < 64)?
        ~((~(uint64_t)0) << numBits):~(uint64_t)0;
      uint64_t v = randSrc[i] & mask,
        r = randCmp[i];
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log( "Expected %"PRIu64", got %"PRIu64",\n"
                " i = %lu, bits=%u\n",
                v, r, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
    }
    if (numRnd > 1)
    {
      unsigned numBits = numBitsList[0];
      uint64_t mask = (numBits < 64)?
        ~((~(uint64_t)0) << numBits):~(uint64_t)0;
      uint64_t v = randSrc[0] & mask;
      uint64_t r;
      gt_bsGetNonUniformUInt64Array(bitStore, offsetStart, 1, numBits,
                                     numBitsList, &r);
      if (r != v)
      {
        gt_log_log("Expected %"PRIu64", got %"PRIu64", "
                " one value extraction\n",
                v, r);
        freeResourcesAndReturn(had_err);
      }
    }
    gt_log_log(" passed\n");
    gt_free(numBitsList);
    numBitsList = NULL;
  }
  gt_log_log("bsNonStoreUniformInt64Array/gt_bsGetInt64: ");
  {
    BitOffset bitsTotal = 0;
    numBitsList = gt_malloc(sizeof (unsigned) * numRnd);
    for (i = 0; i < numRnd; ++i)
      bitsTotal += (numBitsList[i] = random()%64 + 1);
    offset = offsetStart;
    gt_bsStoreNonUniformInt64Array(bitStore, offset, numRnd, bitsTotal,
                                     numBitsList, (int64_t *)randSrc);
    for (i = 0; i < numRnd; ++i)
    {
      unsigned numBits = numBitsList[i];
      int64_t mask = (numBits < 64)
        ? ~((~(int64_t)0) << numBits) : ~(int64_t)0;
      int64_t m = (int64_t)1 << (numBits - 1);
      int64_t v = (int64_t)((randSrc[i] & mask) ^ m) - m;
      int64_t r = gt_bsGetInt64(bitStore, offset, numBits);
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log("Expected %"PRId64", got %"PRId64",\n"
                    "i = %lu, numBits=%u\n",
                    v, r, (unsigned long)i, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    gt_log_log("passed\n");
    gt_log_log("gt_bsStoreNonUniformInt64Array/"
            "gt_bsGetNonUniformInt64Array: ");
    gt_bsGetNonUniformInt64Array(bitStore, offset = offsetStart, numRnd,
                                   bitsTotal, numBitsList,
                                   (int64_t *)randCmp);
    for (i = 0; i < numRnd; ++i)
    {
      unsigned numBits = numBitsList[i];
      int64_t mask = (numBits < 64)
        ? ~((~(int64_t)0) << numBits) : ~(int64_t)0;
      int64_t m = (int64_t)1 << (numBits - 1);
      int64_t v = (int64_t)((randSrc[i] & mask) ^ m) - m;
      int64_t r = randCmp[i];
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log("Expected %"PRId64", got %"PRId64", i = %lu\n",
                v, r, (unsigned long)i);
        freeResourcesAndReturn(had_err);
      }
    }
    if (numRnd > 0)
    {
      unsigned numBits = numBitsList[0];
      int64_t mask = (numBits < 64)
        ? ~((~(int64_t)0) << numBits) : ~(int64_t)0;
      int64_t m = (int64_t)1 << (numBits - 1);
      int64_t v = (int64_t)((randSrc[0] & mask) ^ m) - m;
      int64_t r = 0;
      gt_bsGetNonUniformInt64Array(bitStore, offsetStart,
                                     1, numBits, numBitsList, &r);
      ensure(had_err, r == v);
      if (had_err)
      {
        gt_log_log("Expected %"PRId64", got %"PRId64
                ", one value extraction\n",
                v, r);
        freeResourcesAndReturn(had_err);
      }
    }
    gt_log_log("passed\n");
    gt_free(numBitsList);
    numBitsList = NULL;
  }

  if (numRnd > 0)
  {
    gt_log_log("gt_bsCopy: ");
    {
      /* first decide how many of the values to use and at which to start */
      size_t numValueCopies, copyStart;
      BitOffset numCopyBits = 0, destOffset;
      unsigned numBits = random()%64 + 1;
      uint64_t mask = ~(uint64_t)0;
      if (numBits < 64)
        mask = ~(mask << numBits);
      if (random()&1)
      {
        numValueCopies = random()%(numRnd + 1);
        copyStart = random()%(numRnd - numValueCopies + 1);
      }
      else
      {
        copyStart = random() % numRnd;
        numValueCopies = random()%(numRnd - copyStart) + 1;
      }
      gt_assert(copyStart + numValueCopies <= numRnd);
      offset = offsetStart + (BitOffset)copyStart * numBits;
      gt_bsStoreUniformUInt64Array(bitStore, offset, numBits,
                                       numValueCopies, randSrc);
      destOffset = random()%(offsetStart + 64
                             * (BitOffset)(numRnd - numValueCopies) + 1);
      numCopyBits = (BitOffset)numBits * numValueCopies;
      /* the following gt_bsCopy should be equivalent to:
       * gt_bsStoreUniformUInt64Array(bitStoreCopy, destOffset,
       *                              numBits, numValueCopies, randSrc); */
      gt_bsCopy(bitStore, offset, bitStoreCopy, destOffset, numCopyBits);
      ensure(had_err,
             gt_bsCompare(bitStore, offset, numCopyBits,
                       bitStoreCopy, destOffset, numCopyBits) == 0);
      if (had_err)
      {
        gt_log_log("Expected equality on bitstrings\n"
                    "offset = %llu, destOffset = %llu,"
                    " numCopyBits=%llu\n",
                    (unsigned long long)offset,
                    (unsigned long long)destOffset,
                    (unsigned long long)numCopyBits);
        /* FIXME: implement bitstring output function */
        freeResourcesAndReturn(had_err);
      }
      gt_log_log("passed\n");
    }
  }
  if (numRnd > 0)
  {
    gt_log_log("gt_bsClear: ");
    {
      /* first decide how many of the values to use and at which to start */
      size_t numResetValues, resetStart;
      BitOffset numResetBits = 0;
      unsigned numBits = random()%64 + 1;
      int bitVal = random()&1;
      int64_t cmpVal = bitVal?-1:0;
      uint64_t mask = ~(uint64_t)0;
      if (numBits < 64)
        mask = ~(mask << numBits);
      if (random()&1)
      {
        numResetValues = random()%(numRnd + 1);
        resetStart = random()%(numRnd - numResetValues + 1);
      }
      else
      {
        resetStart = random() % numRnd;
        numResetValues = random()%(numRnd - resetStart) + 1;
      }
      gt_assert(resetStart + numResetValues <= numRnd);
      offset = offsetStart;
      gt_bsStoreUniformInt64Array(bitStore, offset, numBits, numRnd,
                                    (int64_t *)randSrc);
      numResetBits = (BitOffset)numBits * numResetValues;
      gt_bsClear(bitStore, offset + (BitOffset)resetStart * numBits,
              numResetBits, bitVal);
      {
        int64_t m = (int64_t)1 << (numBits - 1);
        for (i = 0; i < resetStart; ++i)
        {
          int64_t v = (int64_t)((randSrc[i] & mask) ^ m) - m;
          int64_t r = gt_bsGetInt64(bitStore, offset, numBits);
          ensure(had_err, r == v);
          if (had_err)
          {
            gt_log_log( "Expected %"PRId64", got %"PRId64",\n"
                     "i = %lu, numBits=%u\n",
                     v, r, (unsigned long)i, numBits);
            freeResourcesAndReturn(had_err);
          }
          offset += numBits;
        }
        for (; i < resetStart + numResetValues; ++i)
        {
          int64_t r = gt_bsGetInt64(bitStore, offset, numBits);
          ensure(had_err, r == cmpVal);
          if (had_err)
          {
            gt_log_log("Expected %"PRId64", got %"PRId64",\n"
                    "i = %lu, numBits=%u\n",
                    cmpVal, r, (unsigned long)i, numBits);
            freeResourcesAndReturn(had_err);
          }
          offset += numBits;
        }
        for (; i < numRnd; ++i)
        {
          int64_t v = (int64_t)((randSrc[i] & mask) ^ m) - m;
          int64_t r = gt_bsGetInt64(bitStore, offset, numBits);
          ensure(had_err, r == v);
          if (had_err)
          {
            gt_log_log("Expected %"PRId64", got %"PRId64",\n"
                    "i = %lu, numBits=%u\n",
                    v, r, (unsigned long)i, numBits);
            freeResourcesAndReturn(had_err);
          }
          offset += numBits;
        }
      }
    }
    gt_log_log("passed\n");
  }
  if (numRnd > 0)
  {
    gt_log_log("gt_bs1BitsCount: ");
    {
      /* first decide how many of the values to use and at which to start */
      size_t numCountValues, countStart;
      BitOffset numCountBits = 0, bitCountRef = 0, bitCountCmp;
      unsigned numBits = random()%64 + 1;
      uint64_t mask = ~(uint64_t)0;
      if (numBits < 64)
        mask = ~(mask << numBits);
      if (random()&1)
      {
        numCountValues = random()%(numRnd + 1);
        countStart = random()%(numRnd - numCountValues + 1);
      }
      else
      {
        countStart = random() % numRnd;
        numCountValues = random()%(numRnd - countStart) + 1;
      }
      gt_assert(countStart + numCountValues <= numRnd);
      offset = offsetStart;
      gt_bsStoreUniformUInt64Array(bitStore, offset, numBits, numRnd,
                                       randSrc);
      numCountBits = (BitOffset)numBits * numCountValues;
      bitCountCmp = gt_bs1BitsCount(bitStore,
                                 offset + (BitOffset)countStart * numBits,
                                 numCountBits);
      for (i = countStart; i < countStart + numCountValues; ++i)
      {
        uint64_t v = (uint64_t)randSrc[i] & mask;
        bitCountRef += genBitCount(v);
      }
      ensure(had_err, bitCountRef == bitCountCmp);
      if (had_err)
      {
        gt_log_log("Expected %llu, got %llu,\n"
                "numBits=%u\n", (unsigned long long)bitCountRef,
                (unsigned long long)bitCountCmp, numBits);
        freeResourcesAndReturn(had_err);
      }
      offset += numBits;
    }
    gt_log_log("passed\n");
  }
  freeResourcesAndReturn(had_err);
}
