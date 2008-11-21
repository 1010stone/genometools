#include "genometools.h"
#include "annotationsketch/custom_track_example.h"
#include "core/seq.h"
#include "core/bioseq.h"

static void handle_error(GtError *err)
{
  fprintf(stderr, "error: %s\n", gt_error_get(err));
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  const char *style_file, *png_file, *gff3_file, *seqid;
  GtStyle *style;
  GtBioseq *bioseq;
  GtFeatureIndex *feature_index;
  GtRange range;
  GtDiagram *diagram;
  GtLayout *layout;
  GtCanvas *canvas;
  GtCustomTrack *custom, *custom2;
  unsigned long height, windowsize;
  GtError *err = gt_error_new();

  if (argc != 9) {
    fprintf(stderr, "Usage: %s style_file PNG_file GFF3_file Seq_file seqid"
                    " start end windowsize\n",
                    argv[0]);
    return EXIT_FAILURE;
  }

  style_file = argv[1];
  png_file = argv[2];
  gff3_file = argv[3];

  /* create style */
  if (!(style = gt_style_new(err)))
    handle_error(err);

  /* load style file */
  if (gt_style_load_file(style, style_file, err))
    handle_error(err);

  /* create feature index */
  feature_index = gt_feature_index_memory_new();

  /* add GFF3 file to index */
  if (gt_feature_index_add_gff3file(feature_index, gff3_file, err))
    handle_error(err);

  /* create diagram for first sequence ID in feature index */
  seqid = argv[5];
  gt_feature_index_get_range_for_seqid(feature_index, &range, seqid);
  sscanf(argv[6], "%lu", &range.start);
  sscanf(argv[7], "%lu", &range.end);
  sscanf(argv[8], "%lu", &windowsize);

  diagram = gt_diagram_new(feature_index, seqid, &range, style, err);
  if (gt_error_is_set(err))
    handle_error(err);

  /* load sequence for GC plot */
  bioseq = gt_bioseq_new(argv[4], err);
  if (gt_error_is_set(err))
    handle_error(err);

  /* create custom track with GC plot for first sequence in file,
     window size 1000, 40px height and average line at 16.5% */
  custom = gt_custom_track_gc_content_new(gt_bioseq_get_seq(bioseq, 0),
                                          windowsize,
                                          70,
                                          0.165);
  gt_diagram_add_custom_track(diagram, custom);
  /* create example custom track */
  custom2 = gt_custom_track_example_new();
  gt_diagram_add_custom_track(diagram, custom2);

  /* create layout with given width, determine resulting image height */
  layout = gt_layout_new(diagram, 1000, style, err);
  if (gt_error_is_set(err))
    handle_error(err);
  height = gt_layout_get_height(layout);

  /* create PNG canvas */
  canvas = gt_canvas_cairo_file_new(style, GT_GRAPHICS_PNG, 1000, height, NULL);

  /* sketch layout on canvas */
  gt_layout_sketch(layout, canvas);

  /* write canvas to file */
  if (gt_canvas_cairo_file_to_file((GtCanvasCairoFile*) canvas, png_file, err))
    handle_error(err);

  /* free */
  gt_custom_track_delete(custom);
  gt_custom_track_delete(custom2);
  gt_bioseq_delete(bioseq);
  gt_canvas_delete(canvas);
  gt_layout_delete(layout);
  gt_diagram_delete(diagram);
  gt_feature_index_delete(feature_index);
  gt_style_delete(style);
  gt_error_delete(err);

  return EXIT_SUCCESS;
}
