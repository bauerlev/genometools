/*
  Copyright (c) 2011-2012 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2011-2012 Center for Bioinformatics, University of Hamburg

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
#include "core/ma.h"
#include "esa-seqread.h"
/* no include for seqnumrelpos.h */

int gt_esa_bottomup_RAM_shulen(const unsigned long *bucketofsuffixes,
                        const unsigned long *lcptab_bucket,
                        unsigned long numberofsuffixes,
                        GtArrayGtBUItvinfo_shulen *stack,
                        GtBUstate_shulen *bustate,
                        /* no parameter snrp */
                        GtError *err)
{
  const unsigned long incrementstacksize = 32UL;
  unsigned long lcpvalue,
                previoussuffix,
                idx;
  GtBUItvinfo_shulen *lastinterval = NULL;
  bool haserr = false, firstedge, firstedgefromroot;

  if (bustate->previousbucketlastsuffix != ULONG_MAX)
  {
    previoussuffix = bustate->previousbucketlastsuffix;
    lcpvalue = (unsigned long) lcptab_bucket[0];
    firstedgefromroot = bustate->firstedgefromroot;
    idx = 0;
    gt_assert(stack->nextfreeGtBUItvinfo > 0);
    if (lcpvalue <= TOP_ESA_BOTTOMUP_shulen.lcp)
    {
      if (TOP_ESA_BOTTOMUP_shulen.lcp > 0 || !firstedgefromroot)
      {
        firstedge = false;
      } else
      {
        firstedge = true;
        firstedgefromroot = false;
      }
      if (processleafedge_shulen(firstedge,
                          TOP_ESA_BOTTOMUP_shulen.lcp,
                          &TOP_ESA_BOTTOMUP_shulen.info,
                          previoussuffix,
                          bustate,
                          err) != 0)
      {
        haserr = true;
      }
    }
    gt_assert(lastinterval == NULL);
    while (!haserr && lcpvalue < TOP_ESA_BOTTOMUP_shulen.lcp)
    {
      lastinterval = POP_ESA_BOTTOMUP_shulen;
      lastinterval->rb = idx + bustate->idxoffset;
      /* no call to processlcpinterval_shulen */
      if (lcpvalue <= TOP_ESA_BOTTOMUP_shulen.lcp)
      {
        if (TOP_ESA_BOTTOMUP_shulen.lcp > 0 || !firstedgefromroot)
        {
          firstedge = false;
        } else
        {
          firstedge = true;
          firstedgefromroot = false;
        }
        if (processbranchingedge_shulen(firstedge,
                   TOP_ESA_BOTTOMUP_shulen.lcp,
                   &TOP_ESA_BOTTOMUP_shulen.info,
                   lastinterval->lcp,
                   lastinterval->rb - lastinterval->lb + 1,
                   &lastinterval->info,
                   bustate,
                   err) != 0)
        {
          haserr = true;
        }
        lastinterval = NULL;
      }
    }
    if (!haserr && lcpvalue > TOP_ESA_BOTTOMUP_shulen.lcp)
    {
      if (lastinterval != NULL)
      {
        unsigned long lastintervallb = lastinterval->lb;
        unsigned long lastintervallcp = lastinterval->lcp,
              lastintervalrb = lastinterval->rb;
        PUSH_ESA_BOTTOMUP_shulen(lcpvalue,lastintervallb);
        if (processbranchingedge_shulen(true,
                       TOP_ESA_BOTTOMUP_shulen.lcp,
                       &TOP_ESA_BOTTOMUP_shulen.info,
                       lastintervallcp,
                       lastintervalrb - lastintervallb + 1,
                       NULL,
                       bustate,
                       err) != 0)
        {
          haserr = true;
        }
        lastinterval = NULL;
      } else
      {
        PUSH_ESA_BOTTOMUP_shulen(lcpvalue,idx + bustate->idxoffset);
        if (processleafedge_shulen(true,
                            TOP_ESA_BOTTOMUP_shulen.lcp,
                            &TOP_ESA_BOTTOMUP_shulen.info,
                            previoussuffix,
                            bustate,
                            err) != 0)
        {
          haserr = true;
        }
      }
    }
  } else
  {
    PUSH_ESA_BOTTOMUP_shulen(0,0);
    firstedgefromroot = true;
  }
  gt_assert(numberofsuffixes > 0);
  for (idx = 0; !haserr && idx < numberofsuffixes-1; idx++)
  {
    lcpvalue = (unsigned long) lcptab_bucket[idx+1];
    previoussuffix = bucketofsuffixes[idx];
    gt_assert(stack->nextfreeGtBUItvinfo > 0);
    if (lcpvalue <= TOP_ESA_BOTTOMUP_shulen.lcp)
    {
      if (TOP_ESA_BOTTOMUP_shulen.lcp > 0 || !firstedgefromroot)
      {
        firstedge = false;
      } else
      {
        firstedge = true;
        firstedgefromroot = false;
      }
      if (processleafedge_shulen(firstedge,
                          TOP_ESA_BOTTOMUP_shulen.lcp,
                          &TOP_ESA_BOTTOMUP_shulen.info,
                          previoussuffix,
                          bustate,
                          err) != 0)
      {
        haserr = true;
      }
    }
    gt_assert(lastinterval == NULL);
    while (!haserr && lcpvalue < TOP_ESA_BOTTOMUP_shulen.lcp)
    {
      lastinterval = POP_ESA_BOTTOMUP_shulen;
      lastinterval->rb = idx + bustate->idxoffset;
      /* no call to processlcpinterval_shulen */
      if (lcpvalue <= TOP_ESA_BOTTOMUP_shulen.lcp)
      {
        if (TOP_ESA_BOTTOMUP_shulen.lcp > 0 || !firstedgefromroot)
        {
          firstedge = false;
        } else
        {
          firstedge = true;
          firstedgefromroot = false;
        }
        if (processbranchingedge_shulen(firstedge,
                   TOP_ESA_BOTTOMUP_shulen.lcp,
                   &TOP_ESA_BOTTOMUP_shulen.info,
                   lastinterval->lcp,
                   lastinterval->rb - lastinterval->lb + 1,
                   &lastinterval->info,
                   bustate,
                   err) != 0)
        {
          haserr = true;
        }
        lastinterval = NULL;
      }
    }
    if (!haserr && lcpvalue > TOP_ESA_BOTTOMUP_shulen.lcp)
    {
      if (lastinterval != NULL)
      {
        unsigned long lastintervallb = lastinterval->lb;
        unsigned long lastintervallcp = lastinterval->lcp,
              lastintervalrb = lastinterval->rb;
        PUSH_ESA_BOTTOMUP_shulen(lcpvalue,lastintervallb);
        if (processbranchingedge_shulen(true,
                       TOP_ESA_BOTTOMUP_shulen.lcp,
                       &TOP_ESA_BOTTOMUP_shulen.info,
                       lastintervallcp,
                       lastintervalrb - lastintervallb + 1,
                       NULL,
                       bustate,
                       err) != 0)
        {
          haserr = true;
        }
        lastinterval = NULL;
      } else
      {
        PUSH_ESA_BOTTOMUP_shulen(lcpvalue,idx + bustate->idxoffset);
        if (processleafedge_shulen(true,
                            TOP_ESA_BOTTOMUP_shulen.lcp,
                            &TOP_ESA_BOTTOMUP_shulen.info,
                            previoussuffix,
                            bustate,
                            err) != 0)
        {
          haserr = true;
        }
      }
    }
  }
  if (!haserr)
  {
    bustate->previousbucketlastsuffix = bucketofsuffixes[numberofsuffixes-1];
    bustate->firstedgefromroot = firstedgefromroot;
  }
  return haserr ? -1 : 0;
}