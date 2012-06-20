/*
  Copyright (c) 2012 Joachim Bonnet <joachim.bonnet@studium.uni-hamburg.de>
  Copyright (c) 2012 Dirk Willrodt <willrodt@zbh.uni-hamburg.de>

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

#ifndef ENCDESC_H
#define ENCDESC_H

#include "core/error_api.h"
#include "core/str_array_api.h"
#include "core/timer_api.h"
#include "extended/string_iter.h"

/* Class <GtEncdesc> stores fasta header in a compressed form. This can save a
   lot of disk or ram space for repetitive headers, for example in multiple
   fasta with short reads. */
typedef struct GtEncdesc GtEncdesc;

/* Class <GtEncdescEncoder> can be used to encode fasta header. */
typedef struct GtEncdescEncoder GtEncdescEncoder;

/* Returns a new <GtEncdescEncoder> object */
GtEncdescEncoder* gt_encdesc_encoder_new(void);

/* Sets <timer> as the timer in <ee> */
void              gt_encdesc_encoder_set_timer(GtEncdescEncoder *ee,
                                               GtTimer *timer);

/* Returns a pointer to the <GtTimer> in <ee>, might be NULL */
GtTimer*          gt_encdesc_encoder_get_timer(GtEncdescEncoder *ee);

/* Set the sampling method of <ee> to either __none__, __page__wise or
   __regular__ sampling. Sampling increases encoded size and decreases time for
   random access. */
void              gt_encdesc_encoder_set_sampling_none(GtEncdescEncoder *ee);
void              gt_encdesc_encoder_set_sampling_page(GtEncdescEncoder *ee);
void              gt_encdesc_encoder_set_sampling_regular(GtEncdescEncoder *ee);

/* Returns true if __page__wise/__regular__ sampling is set in <ee>. */
bool              gt_encdesc_encoder_sampling_is_page(GtEncdescEncoder *ee);
bool              gt_encdesc_encoder_sampling_is_regular(GtEncdescEncoder *ee);

/* Sets the sampling rate */
void              gt_encdesc_encoder_set_sampling_rate(GtEncdescEncoder *ee,
                                                   unsigned long sampling_rate);

/* Returns the samplingrate, undefined if sampling is set to no sampling */
unsigned long     gt_encdesc_encoder_get_sampling_rate(GtEncdescEncoder *ee);

/* Uses the settings in <ee> to encode the strings provided by <str_iter> and
   writes them to a file with prefix <name>. */
int               gt_encdesc_encoder_encode(GtEncdescEncoder *ee,
                                            GtStringIter *str_iter,
                                            const char *name,
                                            GtError *err);

/* loads a <GtEncdesc> from file with prefix <name> */
GtEncdesc*        gt_encdesc_load(const char *name,
                                  GtError *err);

/* returns the number of encoded headers in <encdesc> */
unsigned long     gt_encdesc_num_of_descriptions(GtEncdesc *encdesc);

/* Decodes description with number <num> and writes it to <desc>, the <GtStr>
   will be reset bofore writing to it.
   Returns 1 on success, 0 on EOF and -1 on error */
int               gt_encdesc_decode(GtEncdesc *encdesc,
                                    unsigned long num,
                                    GtStr *desc,
                                    GtError *err);

void              gt_encdesc_delete(GtEncdesc *encdesc);

void              gt_encdesc_encoder_delete(GtEncdescEncoder *ee);

int gt_encdesc_unit_test(GtError *err);

#endif