/*
  Copyright (c) 2005-2011 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2005-2008 Center for Bioinformatics, University of Hamburg

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

#ifndef FILE_API_H
#define FILE_API_H

#include "core/error_api.h"

/* This class defines (generic) files in __GenomeTools__. A generic file is is a
   file which either uncompressed or compressed (with gzip or bzip2).
   A <NULL>-pointer as generic file implies stdout. */
typedef struct GtFile GtFile;

/* Create a new GtFile object and open the underlying file handle with given
   <mode>. Returns NULL and sets <err> accordingly, if the file <path> could not
   be opened. The compression mode is determined by the ending of <path> (gzip
   compression if it ends with '.gz', bzip2 compression if it ends with '.bz2',
   and uncompressed otherwise). */
GtFile* gt_file_new(const char *path, const char *mode, GtError *err);

/* Close the underlying file handle and destroy the <file> object. */
void    gt_file_delete(GtFile *file);

/* Write <\0>-terminated string <cstr> to <file>. Similar to <fputs(3)>, but
   terminates on error. */
void    gt_file_xfputs(const char *cstr, GtFile *file);

#endif
