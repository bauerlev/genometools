/*
  Copyright (c) 2006-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2008 Center for Bioinformatics, University of Hamburg

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

#ifndef GENOME_VISITOR_REP_H
#define GENOME_VISITOR_REP_H

#include <stdio.h>
#include "libgtext/genome_visitor.h"

/* the ``genome visitor'' interface */
struct GenomeVisitorClass {
  size_t size;
  void (*free)(GenomeVisitor*);
  int  (*comment)(GenomeVisitor*, Comment*, Error*);
  int  (*genome_feature)(GenomeVisitor*, GenomeFeature*, Error*);
  int  (*sequence_region)(GenomeVisitor*, SequenceRegion*, Error*);
  int  (*sequence_node)(GenomeVisitor*, SequenceNode*, Error*);
};

struct GenomeVisitor {
  const GenomeVisitorClass *c_class;
};

GenomeVisitor* genome_visitor_create(const GenomeVisitorClass*);
void*          genome_visitor_cast(const GenomeVisitorClass*, GenomeVisitor*);

#endif
