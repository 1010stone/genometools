#include "genometools.h"

static GtArray* create_example_features(void)
{
  GtArray *features;
  GtGenomeNode *gene, *exon, *intron; /* features */
  GtStr *seqid; /* holds the sequence id the features refer to */

  /* construct the example features */
  features = gt_array_new(sizeof (GtGenomeNode*));
  seqid = gt_str_new_cstr("chromosome_21");

  /* construct a gene on the forward strand with two exons */
  gene = gt_genome_feature_new(seqid, "gene", 100, 900, GT_STRAND_FORWARD);
  exon = gt_genome_feature_new(seqid, "exon", 100, 200, GT_STRAND_FORWARD);
  gt_genome_node_add_child(gene, exon);
  intron = gt_genome_feature_new(seqid, "intron", 201, 799, GT_STRAND_FORWARD);
  gt_genome_node_add_child(gene, intron);
  exon = gt_genome_feature_new(seqid, "exon", 800, 900, GT_STRAND_FORWARD);
  gt_genome_node_add_child(gene, exon);

  /* store forward gene in feature array */
  gt_array_add(features, gene);

  /* construct a single-exon gene on the reverse strand
     (within the intron of the forward strand gene) */
  gene = gt_genome_feature_new(seqid, "gene", 400, 600, GT_STRAND_REVERSE);
  exon = gt_genome_feature_new(seqid, "exon", 400, 600, GT_STRAND_REVERSE);
  gt_genome_node_add_child(gene, exon);

  /* store reverse gene in feature array */
  gt_array_add(features, gene);

  /* free */
  gt_str_delete(seqid);

  return features;
}

static void handle_error(GtError *err)
{
  fprintf(stderr, "error writing canvas %s\n", gt_error_get(err));
  exit(EXIT_FAILURE);
}

static void draw_example_features(GtArray *features, const char *style_file,
                                  const char *output_file)
{
  GtRange range = { 1, 1000 }; /* the genomic range to draw */
  GtStyle *style;
  GT_Diagram *diagram;
  GtCanvas *canvas;
  GtError *err = gt_error_new();

  /* create style */
  if (!(style = gt_style_new(false, err)))
    handle_error(err);

  /* load style file */
  if (gt_style_load_file(style, style_file, err))
    handle_error(err);

  /* create diagram */
  diagram = gt_diagram_new_from_array(features, &range, style);

  /* create canvas */
  canvas = gt_canvas_cairo_file_new(style, GT_GRAPHICS_PNG, 600, NULL);

  /* sketch diagram on canvas */
  gt_diagram_sketch(diagram, canvas);

  /* write canvas to file */
  if (gt_canvas_cairo_file_to_file((GtCanvasCairoFile*) canvas, output_file,
                                   err)) {
    handle_error(err);
  }

  /* free */
  gt_canvas_delete(canvas);
  gt_diagram_delete(diagram);
  gt_style_delete(style);
  gt_error_delete(err);
}

static void delete_example_features(GtArray *features)
{
  unsigned long i;
  for (i = 0; i < gt_array_size(features); i++)
    gt_genome_node_rec_delete(*(GtGenomeNode**) gt_array_get(features, i));
  gt_array_delete(features);
}

int main(int argc, char *argv[])
{
  GtArray *features; /* stores the created example features */

  if (argc != 3) {
    fprintf(stderr, "Usage: %s style_file output_file\n", argv[0]);
    return EXIT_FAILURE;
  }

  features = create_example_features();

  draw_example_features(features, argv[1], argv[2]);

  delete_example_features(features);

  return EXIT_SUCCESS;
}
