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

#include <string.h>
#include "core/assert.h"
#include "core/cstr.h"
#include "core/hashmap.h"
#include "core/ma.h"
#include "core/parseutils.h"
#include "core/splitter.h"
#include "core/undef.h"
#include "core/unused_api.h"
#include "core/warning.h"
#include "extended/compare.h"
#include "extended/genome_node.h"
#include "extended/gtf_parser.h"

#define GENE_ID_ATTRIBUTE         "gene_id"
#define GENE_NAME_ATTRIBUTE       "gene_name"
#define TRANSCRIPT_ID_ATTRIBUTE   "transcript_id"
#define TRANSCRIPT_NAME_ATTRIBUTE "transcript_name"

struct GTF_parser {
  GtHashmap *sequence_region_to_range, /* map from sequence regions to ranges */
          *gene_id_hash, /* map from gene_id to transcript_id hash */
          *seqid_to_str_mapping,
          *source_to_str_mapping,
          *gene_id_to_name_mapping,
          *transcript_id_to_name_mapping;
  GT_TypeChecker *type_checker;
};

typedef struct {
  GtQueue *genome_nodes;
  GtArray *mRNAs;
  GtHashmap *gene_id_to_name_mapping,
          *transcript_id_to_name_mapping;
} ConstructionInfo;

typedef enum {
  GTF_CDS,
  GTF_exon,
  GTF_stop_codon
} GTF_feature_type;

static const char *GTF_feature_type_strings[] = { "CDS",
                                                  "exon",
                                                  "stop_codon" };

static int GTF_feature_type_get(GTF_feature_type *type, char *feature_string)
{
  void *result;

  gt_assert(type && feature_string);

  result = bsearch(&feature_string,
                   GTF_feature_type_strings,
                   sizeof (GTF_feature_type_strings) /
                   sizeof (GTF_feature_type_strings[0]),
                   sizeof (char*),
                   compare);

  if (result) {
    *type = (GTF_feature_type)
            ((char**) result - (char**) GTF_feature_type_strings);
    return 0;
  }
  /* else type not found */
  return -1;
}

GTF_parser* gtf_parser_new(GT_TypeChecker *type_checker)
{
  GTF_parser *parser = gt_malloc(sizeof (GTF_parser));
  parser->sequence_region_to_range = gt_hashmap_new(HASH_STRING,
                                                 gt_free_func, gt_free_func);
  parser->gene_id_hash = gt_hashmap_new(HASH_STRING, gt_free_func,
                                     (GtFree) gt_hashmap_delete);
  parser->seqid_to_str_mapping = gt_hashmap_new(HASH_STRING, NULL,
                                             (GtFree) gt_str_delete);
  parser->source_to_str_mapping = gt_hashmap_new(HASH_STRING, NULL,
                                              (GtFree) gt_str_delete);
  parser->gene_id_to_name_mapping = gt_hashmap_new(HASH_STRING, gt_free_func,
                                                gt_free_func);
  parser->transcript_id_to_name_mapping = gt_hashmap_new(HASH_STRING,
                                                         gt_free_func,
                                                         gt_free_func);
  parser->type_checker = type_checker;
  return parser;
}

static int construct_sequence_regions(void *key, void *value, void *data,
                                      GT_UNUSED GtError *err)
{
  GtStr *seqid;
  GtRange range;
  GtGenomeNode *gn;
  GtQueue *genome_nodes = (GtQueue*) data;
  gt_error_check(err);
  gt_assert(key && value && data);
  seqid = gt_str_new_cstr(key);
  range = *(GtRange*) value;
  gn = gt_region_node_new(seqid, range.start, range.end);
  gt_queue_add(genome_nodes, gn);
  gt_str_delete(seqid);
  return 0;
}

static int construct_mRNAs(GT_UNUSED void *key, void *value, void *data,
                           GtError *err)
{
  ConstructionInfo *cinfo = (ConstructionInfo*) data;
  GtArray *gt_genome_node_array = (GtArray*) value,
        *mRNAs = (GtArray*) cinfo->mRNAs;
  GtGenomeNode *mRNA_node, *first_node, *gn;
  const char *tname;
  GtStrand mRNA_strand;
  GtRange mRNA_range;
  GtStr *mRNA_seqid;
  unsigned long i;
  int had_err = 0;

  gt_error_check(err);
  gt_assert(key && value && data);
   /* at least one node in array */
  gt_assert(gt_array_size(gt_genome_node_array));

  /* determine the range and the strand of the mRNA */
  first_node = *(GtGenomeNode**) gt_array_get(gt_genome_node_array, 0);
  mRNA_range = gt_genome_node_get_range(first_node);
  mRNA_strand = gt_feature_node_get_strand((GtFeatureNode*) first_node);
  mRNA_seqid = gt_genome_node_get_seqid(first_node);
  for (i = 1; i < gt_array_size(gt_genome_node_array); i++) {
    gn = *(GtGenomeNode**) gt_array_get(gt_genome_node_array, i);
    mRNA_range = gt_range_join(mRNA_range, gt_genome_node_get_range(gn));
    /* XXX: an error check is necessary here, otherwise gt_strand_join() can
       cause a failed assertion */
    mRNA_strand = gt_strand_join(mRNA_strand,
                          gt_feature_node_get_strand((GtFeatureNode*) gn));
    if (gt_str_cmp(mRNA_seqid, gt_genome_node_get_seqid(gn))) {
      gt_error_set(err, "The features on lines %u and %u refer to different "
                "genomic sequences (``seqname''), although they have the same "
                "gene IDs (``gene_id'') which must be globally unique",
                gt_genome_node_get_line_number(first_node),
                gt_genome_node_get_line_number(gn));
      had_err = -1;
      break;
    }
  }

  if (!had_err) {
    mRNA_node = gt_feature_node_new(mRNA_seqid, gft_mRNA, mRNA_range.start,
                                      mRNA_range.end, mRNA_strand);

    if ((tname = gt_hashmap_get(cinfo->transcript_id_to_name_mapping,
                              (const char*) key))) {
      gt_feature_node_add_attribute((GtFeatureNode*) mRNA_node, "Name",
                                      tname);
    }

    /* register children */
    for (i = 0; i < gt_array_size(gt_genome_node_array); i++) {
      gn = *(GtGenomeNode**) gt_array_get(gt_genome_node_array, i);
      gt_feature_node_add_child((GtFeatureNode*) mRNA_node,
                                (GtFeatureNode*) gn);
    }

    /* store the mRNA */
    gt_array_add(mRNAs, mRNA_node);
  }

  return had_err;
}

static int construct_genes(GT_UNUSED void *key, void *value, void *data,
                           GtError *err)
{
  GtHashmap *transcript_id_hash = (GtHashmap*) value;
  ConstructionInfo *cinfo = (ConstructionInfo*) data;
  GtQueue *genome_nodes = cinfo->genome_nodes;
  const char *gname;
  GtArray *mRNAs = gt_array_new(sizeof (GtGenomeNode*));
  GtGenomeNode *gene_node, *gn;
  GtStrand gene_strand;
  GtRange gene_range;
  GtStr *gene_seqid;
  unsigned long i;
  int had_err = 0;

  gt_error_check(err);
  gt_assert(key && value && data);
  cinfo->mRNAs = mRNAs;
  had_err = gt_hashmap_foreach(transcript_id_hash, construct_mRNAs, cinfo, err);
  if (!had_err) {
    gt_assert(gt_array_size(mRNAs)); /* at least one mRNA constructed */

    /* determine the range and the strand of the gene */
    gn = *(GtGenomeNode**) gt_array_get(mRNAs, 0);
    gene_range = gt_genome_node_get_range(gn);
    gene_strand = gt_feature_node_get_strand((GtFeatureNode*) gn);
    gene_seqid = gt_genome_node_get_seqid(gn);
    for (i = 1; i < gt_array_size(mRNAs); i++) {
      gn = *(GtGenomeNode**) gt_array_get(mRNAs, i);
      gene_range = gt_range_join(gene_range, gt_genome_node_get_range(gn));
      gene_strand = gt_strand_join(gene_strand,
                          gt_feature_node_get_strand((GtFeatureNode*) gn));
      gt_assert(gt_str_cmp(gene_seqid, gt_genome_node_get_seqid(gn)) == 0);
    }

    gene_node = gt_feature_node_new(gene_seqid, gft_gene, gene_range.start,
                                      gene_range.end, gene_strand);

    if ((gname = gt_hashmap_get(cinfo->gene_id_to_name_mapping,
                              (const char*) key))) {
      gt_feature_node_add_attribute((GtFeatureNode*) gene_node, "Name",
                                      gname);
    }

    /* register children */
    for (i = 0; i < gt_array_size(mRNAs); i++) {
      gn = *(GtGenomeNode**) gt_array_get(mRNAs, i);
      gt_feature_node_add_child((GtFeatureNode*) gene_node,
                                (GtFeatureNode*) gn);
    }

    /* store the gene */
    gt_queue_add(genome_nodes, gene_node);

    /* free */
    gt_array_delete(mRNAs);
  }

  return had_err;
}

int gtf_parser_parse(GTF_parser *parser, GtQueue *genome_nodes,
                     GtStr *filenamestr, FILE *fpin, unsigned int be_tolerant,
                     GtError *err)
{
  GtStr *seqid_str, *source_str, *line_buffer;
  char *line;
  size_t line_length;
  unsigned long i, line_number = 0;
  GtGenomeNode *gn;
  GtRange range, *rangeptr;
  Phase phase_value;
  GtStrand gt_strand_value;
  GtSplitter *splitter, *attribute_splitter;
  float score_value;
  char *seqname,
       *source,
       *feature,
       *start,
       *end,
       *score,
       *strand,
       *frame,
       *attributes,
       *token,
       *gene_id,
       *gene_name = NULL,
       *transcript_id,
       *transcript_name = NULL,
       **tokens;
  GtHashmap *transcript_id_hash; /* map from transcript id to array of genome
                                    nodes */
  GtArray *gt_genome_node_array;
  ConstructionInfo cinfo;
  GTF_feature_type gtf_feature_type;
  bool gff_type_is_valid = false;
  const char *type = NULL;
  const char *filename;
  bool score_is_defined;
  int had_err = 0;

  gt_assert(parser && genome_nodes && fpin);
  gt_error_check(err);

  filename = gt_str_get(filenamestr);

  /* alloc */
  line_buffer = gt_str_new();
  splitter = gt_splitter_new(),
  attribute_splitter = gt_splitter_new();

#define HANDLE_ERROR                                                \
        if (had_err) {                                              \
          if (be_tolerant) {                                        \
            fprintf(stderr, "skipping line: %s\n", gt_error_get(err)); \
            gt_error_unset(err);                                       \
            gt_str_reset(line_buffer);                                 \
            had_err = 0;                                            \
            continue;                                               \
          }                                                         \
          else {                                                    \
            had_err = -1;                                           \
            break;                                                  \
          }                                                         \
        }

  while (gt_str_read_next_line(line_buffer, fpin) != EOF) {
    line = gt_str_get(line_buffer);
    line_length = gt_str_length(line_buffer);
    line_number++;
    had_err = 0;

    if (line_length == 0)
      warning("skipping blank line %lu in file \"%s\"", line_number, filename);
    else if (line[0] == '#') {
      /* storing comment */
      gn = gt_comment_node_new(line+1);
      gt_genome_node_set_origin(gn, filenamestr, line_number);
      gt_queue_add(genome_nodes, gn);
    }
    else {
      /* process tab delimited GTF line */
      gt_splitter_reset(splitter);
      gt_splitter_split(splitter, line, line_length, '\t');
      if (gt_splitter_size(splitter) != 9UL) {
        gt_error_set(err, "line %lu in file \"%s\" contains %lu tab (\\t) "
                  "separated fields instead of 9", line_number, filename,
                  gt_splitter_size(splitter));
        had_err = -1;
        break;
      }
      tokens = gt_splitter_get_tokens(splitter);
      seqname    = tokens[0];
      source     = tokens[1];
      feature    = tokens[2];
      start      = tokens[3];
      end        = tokens[4];
      score      = tokens[5];
      strand     = tokens[6];
      frame      = tokens[7];
      attributes = tokens[8];

      /* parse feature */
      if (GTF_feature_type_get(&gtf_feature_type, feature) == -1) {
        /* we skip unknown features */
        fprintf(stderr, "skipping line %lu in file \"%s\": unknown feature: "
                        "\"%s\"\n", line_number, filename, feature);
        gt_str_reset(line_buffer);
        continue;
      }

      /* translate into GFF3 feature type */
      switch (gtf_feature_type) {
        case GTF_CDS:
        case GTF_stop_codon:
          gff_type_is_valid = gt_type_checker_is_valid(parser->type_checker,
                                                       gft_CDS);
          type = gft_CDS;
          break;
        case GTF_exon:
          gff_type_is_valid = gt_type_checker_is_valid(parser->type_checker,
                                                       gft_exon);
          type = gft_exon;
      }
      gt_assert(gff_type_is_valid);

      /* parse the range */
      had_err = gt_parse_range(&range, start, end, line_number, filename, err);
      HANDLE_ERROR;

      /* process seqname (we have to do it here because we need the range) */
      if ((rangeptr = gt_hashmap_get(parser->sequence_region_to_range,
                                     seqname))) {
        /* sequence region is already defined -> update range */
        *rangeptr = gt_range_join(range, *rangeptr);
      }
      else {
        /* sequence region is not already defined -> define it */
        rangeptr = gt_malloc(sizeof (GtRange));
        *rangeptr = range;
        gt_hashmap_add(parser->sequence_region_to_range, gt_cstr_dup(seqname),
                    rangeptr);
      }

      /* parse the score */
      had_err = gt_parse_score(&score_is_defined, &score_value, score,
                               line_number, filename, err);
      HANDLE_ERROR;

      /* parse the strand */
      had_err = gt_parse_strand(&gt_strand_value, strand, line_number, filename,
                               err);
      HANDLE_ERROR;

      /* parse the frame */
      had_err = gt_parse_phase(&phase_value, frame, line_number, filename, err);
      HANDLE_ERROR;

      /* parse the attributes */
      gt_splitter_reset(attribute_splitter);
      gene_id = NULL;
      transcript_id = NULL;
      gt_splitter_split(attribute_splitter, attributes, strlen(attributes),
                        ';');
      for (i = 0; i < gt_splitter_size(attribute_splitter); i++) {
        token = gt_splitter_get_token(attribute_splitter, i);
        /* skip leading blanks */
        while (*token == ' ')
          token++;
        /* look for the two mandatory attributes */
        if (strncmp(token, GENE_ID_ATTRIBUTE, strlen(GENE_ID_ATTRIBUTE)) == 0) {
          if (strlen(token) + 2 < strlen(GENE_ID_ATTRIBUTE)) {
            gt_error_set(err, "missing value to attribute \"%s\" on line %lu "
                         "in file \"%s\"", GENE_ID_ATTRIBUTE, line_number,
                         filename);
            had_err = -1;
          }
          HANDLE_ERROR;
          gene_id = token + strlen(GENE_ID_ATTRIBUTE) + 1;
        }
        else if (strncmp(token, TRANSCRIPT_ID_ATTRIBUTE,
                         strlen(TRANSCRIPT_ID_ATTRIBUTE)) == 0) {
          if (strlen(token) + 2 < strlen(TRANSCRIPT_ID_ATTRIBUTE)) {
            gt_error_set(err, "missing value to attribute \"%s\" on line %lu "
                         "in file \"%s\"", TRANSCRIPT_ID_ATTRIBUTE, line_number,
                         filename);
            had_err = -1;
          }
          HANDLE_ERROR;
          transcript_id = token + strlen(TRANSCRIPT_ID_ATTRIBUTE) + 1;
        }
        else if (strncmp(token, GENE_NAME_ATTRIBUTE,
                         strlen(GENE_NAME_ATTRIBUTE)) == 0) {
          if (strlen(token) + 2 < strlen(GENE_NAME_ATTRIBUTE)) {
            gt_error_set(err, "missing value to attribute \"%s\" on line %lu "
                         "in file \"%s\"", GENE_NAME_ATTRIBUTE, line_number,
                         filename);
            had_err = -1;
          }
          HANDLE_ERROR;
          gene_name = token + strlen(GENE_NAME_ATTRIBUTE) + 1;
          /* for output we want to strip quotes */
          if (*gene_name == '"')
            gene_name++;
          if (gene_name[strlen(gene_name)-1] == '"')
            gene_name[strlen(gene_name)-1] = '\0';
        }
        else if (strncmp(token, TRANSCRIPT_NAME_ATTRIBUTE,
                         strlen(TRANSCRIPT_NAME_ATTRIBUTE)) == 0) {
          if (strlen(token) + 2 < strlen(TRANSCRIPT_NAME_ATTRIBUTE)) {
            gt_error_set(err, "missing value to attribute \"%s\" on line %lu "
                         "in file \"%s\"", TRANSCRIPT_NAME_ATTRIBUTE,
                         line_number, filename);
            had_err = -1;
          }
          HANDLE_ERROR;
          transcript_name = token + strlen(TRANSCRIPT_NAME_ATTRIBUTE) + 1;
          /* for output we want to strip quotes */
          if (*transcript_name == '"')
            transcript_name++;
          if (transcript_name[strlen(transcript_name)-1] == '"')
            transcript_name[strlen(transcript_name)-1] = '\0';
        }
      }

      /* check for the mandatory attributes */
      if (!gene_id) {
        gt_error_set(err, "missing attribute \"%s\" on line %lu in file \"%s\"",
                  GENE_ID_ATTRIBUTE, line_number, filename);
        had_err = -1;
      }
      HANDLE_ERROR;
      if (!transcript_id) {
        gt_error_set(err, "missing attribute \"%s\" on line %lu in file \"%s\"",
                  TRANSCRIPT_ID_ATTRIBUTE, line_number, filename);
        had_err = -1;
      }
      HANDLE_ERROR;

      /* process the mandatory attributes */
      if (!(transcript_id_hash = gt_hashmap_get(parser->gene_id_hash,
                                             gene_id))) {
        transcript_id_hash = gt_hashmap_new(HASH_STRING, gt_free_func,
                                         (GtFree) gt_array_delete);
        gt_hashmap_add(parser->gene_id_hash, gt_cstr_dup(gene_id),
                    transcript_id_hash);
      }
      gt_assert(transcript_id_hash);

      if (!(gt_genome_node_array = gt_hashmap_get(transcript_id_hash,
                                            transcript_id))) {
        gt_genome_node_array = gt_array_new(sizeof (GtGenomeNode*));
        gt_hashmap_add(transcript_id_hash, gt_cstr_dup(transcript_id),
                    gt_genome_node_array);
      }
      gt_assert(gt_genome_node_array);

      /* save optional gene_name and transcript_name attributes */
      if (transcript_name
            && !gt_hashmap_get(parser->transcript_id_to_name_mapping,
                             transcript_id)) {
        gt_hashmap_add(parser->transcript_id_to_name_mapping,
                    gt_cstr_dup(transcript_id),
                    gt_cstr_dup(transcript_name));
      }
      if (gene_name && !gt_hashmap_get(parser->gene_id_to_name_mapping,
                                    gene_id)) {
        gt_hashmap_add(parser->gene_id_to_name_mapping,
                    gt_cstr_dup(gene_id),
                    gt_cstr_dup(gene_name));
      }

      /* get seqid */
      seqid_str = gt_hashmap_get(parser->seqid_to_str_mapping, seqname);
      if (!seqid_str) {
        seqid_str = gt_str_new_cstr(seqname);
        gt_hashmap_add(parser->seqid_to_str_mapping, gt_str_get(seqid_str),
                    seqid_str);
      }
      gt_assert(seqid_str);

      /* construct the new feature */
      gn = gt_feature_node_new(seqid_str, type, range.start, range.end,
                                 gt_strand_value);
      gt_genome_node_set_origin(gn, filenamestr, line_number);

      /* set source */
      source_str = gt_hashmap_get(parser->source_to_str_mapping, source);
      if (!source_str) {
        source_str = gt_str_new_cstr(source);
        gt_hashmap_add(parser->source_to_str_mapping, gt_str_get(source_str),
                    source_str);
      }
      gt_assert(source_str);
      gt_feature_node_set_source((GtFeatureNode*) gn, source_str);

      if (score_is_defined)
        gt_feature_node_set_score((GtFeatureNode*) gn, score_value);
      if (phase_value != GT_PHASE_UNDEFINED)
        gt_feature_node_set_phase(gn, phase_value);
      gt_array_add(gt_genome_node_array, gn);
    }

    gt_str_reset(line_buffer);
  }

  /* process all comments features */
  if (!had_err) {
    had_err = gt_hashmap_foreach(parser->sequence_region_to_range,
                              construct_sequence_regions, genome_nodes, NULL);
    gt_assert(!had_err); /* construct_sequence_regions() is sane */
  }

  /* process all genome_features */
  cinfo.genome_nodes = genome_nodes;
  cinfo.gene_id_to_name_mapping = parser->gene_id_to_name_mapping;
  cinfo.transcript_id_to_name_mapping = parser->transcript_id_to_name_mapping;
  if (!had_err) {
    had_err = gt_hashmap_foreach(parser->gene_id_hash, construct_genes,
                              &cinfo, err);
  }

  /* free */
  gt_splitter_delete(splitter);
  gt_splitter_delete(attribute_splitter);
  gt_str_delete(line_buffer);

  return had_err;
}

void gtf_parser_delete(GTF_parser *parser)
{
  if (!parser) return;
  gt_hashmap_delete(parser->sequence_region_to_range);
  gt_hashmap_delete(parser->gene_id_hash);
  gt_hashmap_delete(parser->seqid_to_str_mapping);
  gt_hashmap_delete(parser->source_to_str_mapping);
  gt_hashmap_delete(parser->transcript_id_to_name_mapping);
  gt_hashmap_delete(parser->gene_id_to_name_mapping);
  gt_free(parser);
}
