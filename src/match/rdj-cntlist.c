/*
  Copyright (c) 2010-2011 Giorgio Gonnella <gonnella@zbh.uni-hamburg.de>
  Copyright (c) 2010-2011 Center for Bioinformatics, University of Hamburg

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

#include <stdio.h>
#include <limits.h>
#include "core/fa.h"
#include "core/fileutils.h"
#include "core/xansi_api.h"
#include "match/rdj-cntlist.h"

#define GT_CNTLIST_BIT_HEADER 0

static inline void gt_cntlist_show_ascii(GtBitsequence *cntlist,
    unsigned long nofreads, FILE *file)
{
  unsigned long i;
  gt_assert(file != NULL);
  fprintf(file, "[n: %lu]\n", nofreads);
  for (i = 0; i < nofreads; i++)
    if (GT_ISIBITSET(cntlist, i))
      fprintf(file, "%lu\n", i);
}

static inline void gt_cntlist_show_bit(GT_UNUSED GtBitsequence *cntlist,
    unsigned long nofreads, FILE *file)
{
  gt_assert(file != NULL);
  (void)putc(GT_CNTLIST_BIT_HEADER, file);
  (void)putc((char)sizeof(unsigned long), file);
  (void)fwrite(&(nofreads), sizeof (unsigned long), (size_t)1, file);
  (void)fwrite(cntlist, sizeof (GtBitsequence), GT_NUMOFINTSFORBITS(nofreads),
      file);
}

int gt_cntlist_show(GtBitsequence *cntlist, unsigned long nofreads,
    const char *path, bool binary, GtError *err)
{
  FILE *file;
  gt_assert(cntlist != NULL);
  if (path == NULL)
    file = stdout;
  else
  {
    file = gt_fa_fopen(path, binary ? "wb" : "w", err);
    if (file == NULL)
      return -1;
  }
  gt_assert(file != NULL);
  (binary ? gt_cntlist_show_bit : gt_cntlist_show_ascii)
    (cntlist, nofreads, file);
  if (path != NULL)
    gt_fa_fclose(file);
  return 0;
}

int gt_cntlist_parse_bit(FILE *infp, GtBitsequence **cntlist,
    unsigned long *nofreads, GtError *err)
{
  int c;
  size_t n;

  gt_assert(infp != NULL && nofreads != NULL && cntlist != NULL);
  gt_error_check(err);
  c = gt_xfgetc(infp);
  if (c == EOF)
  {
    gt_error_set(err, "contained reads list: unexpected end of file");
    return -1;
  }
  else if (c != (char)sizeof(unsigned long))
  {
    gt_error_set(err, "contained reads list: %dbit version "
        "of GenomeTools required to use this list", c * CHAR_BIT);
    return -1;
  }
  n = fread(nofreads, sizeof (unsigned long), (size_t)1, infp);
  if (n != (size_t)1 || *nofreads == 0)
  {
    gt_error_set(err, "contained reads list: unrecognized format");
    return -1;
  }
  GT_INITBITTAB(*cntlist, *nofreads);
  n = fread(*cntlist, sizeof (GtBitsequence),
      GT_NUMOFINTSFORBITS(*nofreads), infp);
  if (n != GT_NUMOFINTSFORBITS(*nofreads))
  {
    gt_error_set(err, "contained reads file: unrecognized format");
    return -1;
  }
  return 0;
}

int gt_cntlist_parse_ascii(FILE *infp, GtBitsequence **cntlist,
    unsigned long *nofreads, GtError *err)
{
  int n;
  unsigned long seqnum;

  gt_assert(infp != NULL && nofreads != NULL && cntlist != NULL);
  /*@i1@*/ gt_error_check(err);
  n = fscanf(infp, "[n: %lu]\n", nofreads);
  if (n!=1 || *nofreads == 0)
  {
    gt_error_set(err, "contained reads file: unrecognized format");
    return -1;
  }
  GT_INITBITTAB(*cntlist, *nofreads);
  while (true)
  {
    n = fscanf(infp, "%lu\n", &seqnum);
    if (n == EOF) break;
    else if (n != 1)
    {
      gt_error_set(err, "contained reads file: unrecognized format");
      return -1;
    }
    GT_SETIBIT(*cntlist, seqnum);
  }
  return 0;
}

int gt_cntlist_parse(const char *filename, GtBitsequence **cntlist,
    unsigned long *nofreads, GtError *err)
{
  int c, retval = 0;
  FILE *infp;

  infp = gt_fa_fopen(filename, "rb", err);

  if (infp == NULL) return -1;

  c = gt_xfgetc(infp);
  switch (c)
  {
    case EOF:
      gt_error_set(err, "%s: unexpected end of file", filename);
      retval = 1;
      break;
    case GT_CNTLIST_BIT_HEADER:
      retval = gt_cntlist_parse_bit(infp, cntlist, nofreads, err);
      break;
    default:
      gt_xungetc(c, infp);
      retval = gt_cntlist_parse_ascii(infp, cntlist, nofreads, err);
      break;
  }
  gt_fa_fclose(infp);

  return retval;
}

unsigned long gt_cntlist_count(const GtBitsequence *cntlist,
    unsigned long nofreads)
{
  unsigned long i, counter = 0;

  for (i = 0; i < nofreads; i++)
    if ((bool)GT_ISIBITSET(cntlist, i))
      counter++;
  return counter;
}

unsigned long gt_cntlist_xload(const char *filename, GtBitsequence **cntlist,
    unsigned long expected_nofreads)
{
  int retval;
  unsigned long found_nofreads;
  GtError *err;

  if (!gt_file_exists(filename))
  {
    fprintf(stderr, "FATAL: error by loading contained reads list: "
        "file %s does not exist\n", filename);
    exit(EXIT_FAILURE);
  }

  err = gt_error_new();
  retval = gt_cntlist_parse(filename, cntlist, &found_nofreads, err);
  if (retval != 0)
  {
    fprintf(stderr, "FATAL: error by parsing contained reads list: %s\n",
        gt_error_get(err));
    exit(EXIT_FAILURE);
  }
  gt_error_delete(err);

  if (found_nofreads != expected_nofreads)
  {
    fprintf(stderr, "FATAL: error by parsing contained reads list: "
        "file specifies a wrong number of reads\nexpected %lu, found %lu\n",
        expected_nofreads, found_nofreads);
    exit(EXIT_FAILURE);
  }

  return gt_cntlist_count(*cntlist, found_nofreads);
}
