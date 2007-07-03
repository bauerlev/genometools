/*
  Copyright (c) 2007 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#include "libgtcore/env.h"
#include "types.h"
#include "encseq-def.h"

/* obsolete code */

typedef struct
{
  uint32_t prefixlength;
  Seqpos totallength,
         countspecialmaxprefixlen0;
} CountCompletespecials;

static int lengthofspecialranges(/*@unused@*/ void *info,
                                 const PairSeqpos *pair,/*@unused@*/ Env *env)
{
  uint32_t len = (uint32_t) (pair->uint1 - pair->uint0);
  CountCompletespecials *csp = (CountCompletespecials *) info;

  if (pair->uint0 == 0)
  {
    if (pair->uint1 == csp->totallength)
    {
      csp->countspecialmaxprefixlen0 += (len+1);
    } else
    {
      csp->countspecialmaxprefixlen0 += len;
    }
  } else
  {
    if (pair->uint1 == csp->totallength)
    {
      csp->countspecialmaxprefixlen0 += len;
    } else
    {
      if (len >= (uint32_t) 2)
      {
        csp->countspecialmaxprefixlen0 += (len - 1);
      }
    }
  }
  return 0;
}

Seqpos determinefullspecials(const Encodedsequence *encseq,
                             Seqpos totallength,
                             uint32_t prefixlength,
                             Env *env)
{
  CountCompletespecials csp;

  csp.countspecialmaxprefixlen0 = 0;
  csp.prefixlength = prefixlength;
  csp.totallength = totallength;
  (void) overallspecialranges(encseq,lengthofspecialranges,&csp,env);
  return csp.countspecialmaxprefixlen0;
}
