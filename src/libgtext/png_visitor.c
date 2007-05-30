/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#include <assert.h>
#include <cairo.h>
#include <gtcore.h>
#include <libgtext/genome_visitor_rep.h>
#include <libgtext/graphics.h>
#include <libgtext/png_visitor.h>

#define TRACK_HEIGHT            50
#define SPACE                   .05
#define ROOM                    .9
#define TEXT_POSITION           20
#define FEATURE_POSITION        30
#define EXON_HEIGHT             10
#define EXON_ARROW_WIDTH        8

struct PNGVisitor {
  const GenomeVisitor parent_instance;
  int width,
      height,
      global_track_number;
  unsigned int number_of_tracks;
  unsigned long from,
                to;
  Graphics *graphics;
  char *png_filename;
  Range drawed_sequence_range,
        last_range;
  bool drawed_sequence_range_is_defined,
       last_range_is_defined;
};

typedef struct {
  PNGVisitor *pngv;
  bool children_overlap;
  int local_track_number;
} Show_children_info;

#define png_visitor_cast(GV)\
        genome_visitor_cast(png_visitor_class(), GV)

static void png_visitor_free(GenomeVisitor *gv, Env *env)
{
  PNGVisitor *pngv = png_visitor_cast(gv);
  assert(pngv->png_filename);
  assert(pngv->width); /* the width has to be positive */
  assert(pngv->height); /* the height has to be positive */
  (void) cairo_surface_write_to_png(pngv->graphics->surf, pngv->png_filename);
  cairo_surface_destroy(pngv->graphics->surf); /* reference counted */
  /* we check this after writing the png to simplify debugging */
  assert(pngv->global_track_number <= pngv->number_of_tracks);
  graphics_delete(pngv->graphics, env);
}

static void draw_exon_box(PNGVisitor *pngv, GenomeFeature *gf, double width,
                          int track_number)
{
  Range feature_range = genome_node_get_range((GenomeNode*) gf);
  Strand feature_strand = genome_feature_get_strand(gf);
  double x, y, height;

  x =  pngv->width * SPACE + pngv->width * ROOM *
       ((double) (feature_range.start - pngv->drawed_sequence_range.start + 1) /
        (double) range_length(pngv->drawed_sequence_range));
  y = track_number * TRACK_HEIGHT + FEATURE_POSITION;
  height = EXON_HEIGHT;

  cairo_set_source_rgb(pngv->graphics->cr, 0, 0, 1);
  switch (feature_strand) {
    case STRAND_FORWARD:
      cairo_move_to(pngv->graphics->cr, x, y);
      if (width - EXON_ARROW_WIDTH > 0)
        cairo_rel_line_to(pngv->graphics->cr, width - EXON_ARROW_WIDTH, 0);
      cairo_line_to(pngv->graphics->cr, x + width, y + height / 2);
      if (width - EXON_ARROW_WIDTH > 0) {
        cairo_line_to(pngv->graphics->cr, x + width - EXON_ARROW_WIDTH,
                      y + height);
      }
      cairo_line_to(pngv->graphics->cr, x, y + height);
      cairo_close_path(pngv->graphics->cr);
      break;
    case STRAND_REVERSE:
      cairo_move_to(pngv->graphics->cr, x + width, y);
      if (width - EXON_ARROW_WIDTH > 0)
        cairo_rel_line_to(pngv->graphics->cr, -(width - EXON_ARROW_WIDTH), 0);
      cairo_line_to(pngv->graphics->cr, x, y + height / 2);
      cairo_line_to(pngv->graphics->cr, x + MIN(width, EXON_ARROW_WIDTH),
                    y + height);
      if (width - EXON_ARROW_WIDTH > 0)
        cairo_line_to(pngv->graphics->cr, x + width, y + height);
      cairo_close_path(pngv->graphics->cr);
      break;
    case STRAND_BOTH:
    case STRAND_UNKNOWN:
      cairo_rectangle(pngv->graphics->cr, x, y, width, height);
   }

   cairo_fill_preserve(pngv->graphics->cr);
   cairo_set_source_rgb(pngv->graphics->cr, 0, 0, 0);
   cairo_stroke(pngv->graphics->cr);
}

static int show_children(GenomeNode *gn, void *data, Env *env)
{
  GenomeFeature *gf = (GenomeFeature*) gn;
  Range feature_range = genome_node_get_range(gn);
  Show_children_info *info = (Show_children_info*) data,
                     local_info;
  char buf[BUFSIZ];
  double x_left, x_right, width, text_y;

  env_error_check(env);
  assert(info->pngv->drawed_sequence_range_is_defined);
  assert(info->local_track_number < info->pngv->number_of_tracks);

  text_y = info->local_track_number * TRACK_HEIGHT + TEXT_POSITION;
  x_left = info->pngv->width * SPACE + info->pngv->width * ROOM *
           ((double) (feature_range.start -
                      info->pngv->drawed_sequence_range.start + 1) /
            (double) range_length(info->pngv->drawed_sequence_range));
  x_right = info->pngv->width * SPACE + info->pngv->width * ROOM *
            ((double) (feature_range.end -
                       info->pngv->drawed_sequence_range.start + 1) /
             (double) range_length(info->pngv->drawed_sequence_range));
  width = info->pngv->width * ROOM *
          ((double) range_length(feature_range) /
           (double) range_length(info->pngv->drawed_sequence_range));

  cairo_set_source_rgb(info->pngv->graphics->cr, 0, 0, 0);

  cairo_move_to(info->pngv->graphics->cr, x_left, text_y);
  (void) snprintf(buf, BUFSIZ, "%lu", feature_range.start);
  cairo_show_text(info->pngv->graphics->cr, buf);

  cairo_move_to(info->pngv->graphics->cr, x_left + width / 2, text_y);
  cairo_show_text(info->pngv->graphics->cr,
                  genome_feature_type_get_cstr(genome_feature_get_type(gf)));

  cairo_move_to(info->pngv->graphics->cr, x_right, text_y);
  (void) snprintf(buf, BUFSIZ, "%lu", feature_range.end);
  cairo_show_text(info->pngv->graphics->cr, buf);

  draw_exon_box(info->pngv, gf, width, info->local_track_number);

  if (info->children_overlap)
    info->local_track_number++;

  if (genome_node_has_children(gn)) {
    local_info.pngv = info->pngv;
    local_info.children_overlap =
      !genome_node_direct_children_do_not_overlap(gn, env);
    local_info.local_track_number = info->local_track_number;
    if (!info->children_overlap)
      local_info.local_track_number++;

    genome_node_traverse_direct_children(gn, &local_info, show_children, env);

    if (!local_info.children_overlap)
      local_info.local_track_number++;
    assert(local_info.local_track_number >= info->local_track_number);

    info->local_track_number = local_info.local_track_number;

  if (!info->children_overlap)
    info->local_track_number--;
  }
  return 0;
}

static int png_visitor_genome_feature(GenomeVisitor *gv, GenomeFeature *gf,
                                         Env *env)
{
  PNGVisitor *pngv = png_visitor_cast(gv);
  Show_children_info info;
  Range feature_range;
  char buf[BUFSIZ];
  double x_left, x_right, width, text_y;

  assert(pngv->drawed_sequence_range_is_defined);

  feature_range = genome_node_get_range((GenomeNode*) gf);

  /* reset track number if necessary */
  if (pngv->last_range_is_defined &&
      !range_overlap(pngv->last_range, feature_range)) {
    pngv->global_track_number = 1;
    pngv->last_range_is_defined = false;
  }

  assert(pngv->global_track_number < pngv->number_of_tracks);

  /* calculate coordinates */
  x_left = pngv->width * SPACE +
           pngv->width * ROOM *
           ((double) (feature_range.start -
                      pngv->drawed_sequence_range.start + 1)/
            (double) range_length(pngv->drawed_sequence_range));
  x_right = pngv->width * SPACE +
            pngv->width * ROOM *
            ((double) (feature_range.end -
                       pngv->drawed_sequence_range.start + 1)/
             (double) range_length(pngv->drawed_sequence_range)),
  width = pngv->width * ROOM *
          ((double) range_length(feature_range) /
           (double) range_length(pngv->drawed_sequence_range));
  text_y = pngv->global_track_number * TRACK_HEIGHT + TEXT_POSITION;

  cairo_set_source_rgb(pngv->graphics->cr, 0, 0, 0);
  /* show <start --- feature_type --- end> */
  cairo_move_to(pngv->graphics->cr, x_left, text_y);
  (void) snprintf(buf, BUFSIZ, "%lu", genome_node_get_start((GenomeNode*) gf));
  cairo_show_text(pngv->graphics->cr, buf);

  cairo_move_to(pngv->graphics->cr, x_left + width / 2, text_y);
  cairo_show_text(pngv->graphics->cr,
                  genome_feature_type_get_cstr(genome_feature_get_type(gf)));

  cairo_move_to(pngv->graphics->cr, x_right, text_y);
  (void) snprintf(buf, BUFSIZ, "%lu", genome_node_get_end((GenomeNode*) gf));
  cairo_show_text(pngv->graphics->cr, buf);
  /* draw feature line */
  cairo_move_to(pngv->graphics->cr, x_left,
                pngv->global_track_number * TRACK_HEIGHT + FEATURE_POSITION);
  cairo_rel_line_to(pngv->graphics->cr, width, 0);
  cairo_stroke(pngv->graphics->cr);

  pngv->global_track_number++;

  if (genome_node_has_children((GenomeNode*) gf)) {
    info.pngv = pngv;
    info.children_overlap =
      !genome_node_direct_children_do_not_overlap((GenomeNode*) gf, env);
    info.local_track_number = pngv->global_track_number;
    genome_node_traverse_direct_children((GenomeNode*) gf, &info,
                                         show_children, env);
    if (!info.children_overlap)
      info.local_track_number++;
    assert(info.local_track_number >= pngv->global_track_number);
    pngv->global_track_number = info.local_track_number;
  }

  /* store range for later use */
  if (pngv->last_range_is_defined) {
    assert(feature_range.start >= pngv->last_range.start);
    if (feature_range.end > pngv->last_range.end)
      pngv->last_range.end = feature_range.end;
  }
  else {
    pngv->last_range = feature_range;
  }
  pngv->last_range_is_defined = true;

  return 0;
}

static int png_visitor_sequence_region(GenomeVisitor *gv, SequenceRegion *sr,
                                         Env *env)
{
  PNGVisitor *pngv = png_visitor_cast(gv);
  Range sr_range;
  char buf[BUFSIZ];

  env_error_check(env);
  assert(!pngv->drawed_sequence_range_is_defined);

  sr_range = genome_node_get_range((GenomeNode*) sr);
  pngv->drawed_sequence_range.start = MAX(sr_range.start, pngv->from);
  pngv->drawed_sequence_range.end = MIN(sr_range.end, pngv->to);
  pngv->drawed_sequence_range_is_defined = true;

  assert(!pngv->global_track_number);
  cairo_set_source_rgb(pngv->graphics->cr, 0, 0, 0);
  /* show <start --- sequence id --- end> */
  cairo_move_to(pngv->graphics->cr, pngv->width * SPACE,
                pngv->global_track_number * TRACK_HEIGHT + TEXT_POSITION);
  (void) snprintf(buf, BUFSIZ, "%lu", pngv->drawed_sequence_range.start);
  cairo_show_text(pngv->graphics->cr, buf);
  cairo_move_to(pngv->graphics->cr, pngv->width * (SPACE + ROOM / 2),
                pngv->global_track_number * TRACK_HEIGHT + TEXT_POSITION);
  cairo_show_text(pngv->graphics->cr,
                  str_get(genome_node_get_seqid((GenomeNode*) sr)));
  cairo_move_to(pngv->graphics->cr, pngv->width * (SPACE + ROOM),
                pngv->global_track_number * TRACK_HEIGHT + TEXT_POSITION);
  (void) snprintf(buf, BUFSIZ, "%lu", pngv->drawed_sequence_range.end);
  cairo_show_text(pngv->graphics->cr, buf);
  /* draw sequence line */
  cairo_move_to(pngv->graphics->cr, pngv->width * SPACE,
                pngv->global_track_number * TRACK_HEIGHT + FEATURE_POSITION);
  cairo_rel_line_to(pngv->graphics->cr, pngv->width * ROOM, 0);
  cairo_stroke(pngv->graphics->cr);

  pngv->global_track_number++;
  return 0;
}

const GenomeVisitorClass* png_visitor_class()
{
  static GenomeVisitorClass gvc = { sizeof (PNGVisitor),
                                    png_visitor_free,
                                    NULL,
                                    png_visitor_genome_feature,
                                    png_visitor_sequence_region,
                                    NULL };
  return &gvc;
}

GenomeVisitor* png_visitor_new(char *png_filename, int width,
                               unsigned int number_of_tracks,
                               unsigned long from, unsigned long to, Env *env)
{
  GenomeVisitor *gv;
  PNGVisitor *pngv;

  env_error_check(env);
  assert(png_filename && width != UNDEF_INT);

  gv = genome_visitor_create(png_visitor_class(), env);
  pngv = png_visitor_cast(gv);

  pngv->number_of_tracks = number_of_tracks;
  pngv->from = from;
  pngv->to = to;
  pngv->width = width ;
  pngv->height = pngv->number_of_tracks * TRACK_HEIGHT;
  pngv->global_track_number = 0;
  pngv->graphics = graphics_new(pngv->width, pngv->height, env);
  pngv->png_filename = png_filename;
  pngv->drawed_sequence_range_is_defined = false;
  pngv->last_range_is_defined = false;
  return gv;
}
