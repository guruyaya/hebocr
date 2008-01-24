
/***************************************************************************
 *            hocr.c
 *
 *  Fri Aug 12 20:13:33 2005
 *  Copyright  2005-2008  Yaacov Zamir
 *  <kzamir@walla.co.il>
 ****************************************************************************/

/*  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "hocr.h"
#include "ho_gtk.h"

gchar *image_in_filename = NULL;
gchar *text_out_filename = NULL;
gchar *data_out_filename = NULL;
gchar *image_out_path = NULL;
gboolean version = FALSE;
gboolean debug = FALSE;
gboolean verbose = FALSE;
gboolean no_gtk = FALSE;
gboolean dir_ltr = FALSE;
gboolean text_out_html = FALSE;
gboolean text_out_font_numbers = FALSE;

gchar *html_page_title = NULL;
gchar *html_page_author = NULL;
gchar *html_page_year = NULL;

gint threshold = 0;
gint adaptive_threshold = 0;
gint adaptive_threshold_type = 0;
gint scale_by = 0;
gint rotate_angle = 0;
gboolean auto_rotate = FALSE;
gboolean do_not_clean_image = FALSE;
gboolean remove_dots = FALSE;
gboolean remove_images = FALSE;
gboolean fix_broken_fonts = FALSE;
gboolean fix_ligated_fonts = FALSE;

gint paragraph_setup = 0;
gint slicing_threshold = 0;
gint slicing_width = 0;
gint font_spacing_code = 0;

gboolean show_grid = FALSE;
gboolean save_copy = FALSE;
gboolean save_bw = FALSE;
gboolean save_layout = FALSE;
gboolean save_fonts = FALSE;
gboolean debug_font_filter_list = FALSE;
gint debug_font_filter = 0;

gboolean only_image_proccesing = FALSE;
gboolean only_layout_analysis = FALSE;
gboolean only_save_fonts = FALSE;

gint spacing_code = 0;
gint lang_code = 0;
gint font_code = 0;

GError *error = NULL;

/* black and white text image */
ho_bitmap *m_page_text = NULL;

/* text layout */
ho_layout *l_page = NULL;

/* text for output */
gchar *text_out = NULL;

static gchar *copyright_message = "hocr - Hebrew OCR utility\n\
%s\n\
http://hocr.berlios.de\n\
Copyright (C) 2005-2008 Yaacov Zamir <kzamir@walla.co.il>\n\
\n\
This program is free software: you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation, either version 3 of the License, or\n\
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program.  If not, see <http://www.gnu.org/licenses/>.\n";

static gchar *font_filters_list = "hocr - Hebrew OCR utility\n\
%s\n\
Font filters list\n\
(add 100 to use filter on font holes, e.g. 118 is egdes in the font holes)\n\
 1.  main font.\n\
 2.  second part.\n\
 3.  holes in font.\n\
 4.  horizontal bars in font.\n\
 5.  vertical bars in font.\n\
 6.  45 deg. diagonals in font.\n\
 7.  135 deg. diagonals in font.\n\
 8.  thined font.\n\
 9.  crosses in font.\n\
 10. ends in font.\n\
 11. top font edges.\n\
 12. big top font edges.\n\
 13. bottom font edges.\n\
 14. big bottom font edges.\n\
 15. left font edges.\n\
 16. big left font edges.\n\
 17. right font edges.\n\
 18. big right font edges.\n\
 19. top font notches.\n\
 20. bottom font notches.\n\
 21. left font notches.\n\
 22. right font notches.\n\
";

static gchar *html_page_header =
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\
\"http://www.w3.org/TR/html4/strict.dtd\">\n\
<html>\n\
<head>\n\
<title></title>\n\
</head>\n\
<body dir=\"rtl\">\n\
\n\
<div class=\"ocr_header\" id=\"title\">\n\
  %s\n\
</div>\n\
<div class=\"ocr_header\" id=\"author\">\n\
  %s\n\
</div>\n\
<div class=\"ocr_header\" id=\"year\">\n\
  %s\n\
</div>\n\
<br/>\n\
<div class=\"ocr_page\" id=\"page_1\">\n\
  <div class=\"ocr_column\" id=\"column_1\">\n";

static gchar *html_page_footer = "  </div>\n</div>\n</body>\n</html>\n";

static gchar *html_debug_header =
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\
\"http://www.w3.org/TR/html4/strict.dtd\">\n\
<html>\n\
<head>\n\
<title></title>\n\
</head>\n\
<body>\n\
<pre>\n";

static gchar *html_debug_footer = "</pre>\n</body>\n</html>\n";

static GOptionEntry file_entries[] = {
  {"images-out-path", 'O', 0, G_OPTION_ARG_FILENAME, &image_out_path,
    "use PATH for output images", "PATH"},
  {"data-out", 'u', 0, G_OPTION_ARG_FILENAME, &data_out_filename,
    "use FILE as output data file name", "FILE"},
  {"html-out", 'h', 0, G_OPTION_ARG_NONE, &text_out_html,
    "output text in html format", NULL},
  {"page-title", '1', 0, G_OPTION_ARG_FILENAME, &html_page_title,
    "html page title", "NAME"},
  {"page-author", '2', 0, G_OPTION_ARG_FILENAME, &html_page_author,
    "html page author", "NAME"},
  {"page-year", '3', 0, G_OPTION_ARG_FILENAME, &html_page_year,
    "html date of publication", "NAME"},
  {NULL}
};

static GOptionEntry image_entries[] = {
  {"thresholding-type", 'T', 0, G_OPTION_ARG_INT,
      &adaptive_threshold_type,
      "thresholding type, 0 normal, 1 none, 2 fine",
    "NUM"},
  {"threshold", 't', 0, G_OPTION_ARG_INT, &threshold,
      "use NUM as threshold value, 1..100",
    "NUM"},
  {"adaptive-threshold", 'a', 0, G_OPTION_ARG_INT, &adaptive_threshold,
      "use NUM as adaptive threshold value, 1..100",
    "NUM"},
  {"scale", 's', 0, G_OPTION_ARG_INT, &scale_by,
      "scale input image by SCALE 1..9, 0 auto",
    "SCALE"},
  {"rotate", 'q', 0, G_OPTION_ARG_INT, &rotate_angle,
      "rotate image clockwise in deg.",
    "DEG"},
  {"auto-rotate", 'Q', 0, G_OPTION_ARG_NONE, &auto_rotate,
      "auto rotate image",
    NULL},
  {"do-not-clean", 'n', 0, G_OPTION_ARG_NONE, &do_not_clean_image,
      "do not try to remove artefacts from image",
    NULL},
  {"remove-halfton", 'r', 0, G_OPTION_ARG_NONE, &remove_dots,
      "remove halfton dots from input image",
    NULL},
  {"remove-images", 'R', 0, G_OPTION_ARG_NONE, &remove_images,
      "remove images from input image",
    NULL},
  {"fix-broken-fonts", 'e', 0, G_OPTION_ARG_NONE, &fix_broken_fonts,
      "try to link broken fonts",
    NULL},
  {"fix-ligated-fonts", 'E', 0, G_OPTION_ARG_NONE, &fix_ligated_fonts,
      "try to break ligated fonts",
    NULL},
  {NULL}
};

static GOptionEntry segmentation_entries[] = {
  {"colums setup", 'c', 0, G_OPTION_ARG_INT, &paragraph_setup,
      "colums setup: 1.. #colums, 0 auto, 255 free",
    "NUM"},
  {"slicing", 'x', 0, G_OPTION_ARG_INT, &slicing_threshold,
      "use NUM as font slicing threshold, 1..250",
    "NUM"},
  {"slicing-width", 'X', 0, G_OPTION_ARG_INT, &slicing_width,
      "use NUM as font slicing width, 50..250",
    "NUM"},
  {"font-spacing", 'w', 0, G_OPTION_ARG_INT, &font_spacing_code,
      "font spacing: tight ..-1, 0, 1.. spaced",
    "NUM"},
  {NULL}
};

static GOptionEntry debug_entries[] = {
  {"draw-grid", 'g', 0, G_OPTION_ARG_NONE, &show_grid,
    "draw grid on output images", NULL},
  {"save-copy", 'C', 0, G_OPTION_ARG_NONE, &save_copy,
    "save a compy of original image", NULL},
  {"save-bw", 'b', 0, G_OPTION_ARG_NONE, &save_bw,
    "save proccesd bw image", NULL},
  {"save-bw-exit", 'B', 0, G_OPTION_ARG_NONE, &only_image_proccesing,
    "save proccesd bw image and exit", NULL},
  {"save-layout", 'l', 0, G_OPTION_ARG_NONE, &save_layout,
    "save layout image", NULL},
  {"save-layout-exit", 'L', 0, G_OPTION_ARG_NONE, &only_layout_analysis,
    "save layout image and exit", NULL},
  {"save-fonts", 'f', 0, G_OPTION_ARG_NONE, &save_fonts,
    "save fonts", NULL},
  {"save-fonts-exit", 'F', 0, G_OPTION_ARG_NONE, &only_save_fonts,
    "save fonts images and exit", NULL},
  {"debug", 'd', 0, G_OPTION_ARG_NONE, &verbose,
    "print debuging information while running", NULL},
  {"debug-extra", 'D', 0, G_OPTION_ARG_NONE, &debug,
    "print extra debuging information", NULL},
  {"font-filter", 'y', 0, G_OPTION_ARG_INT, &debug_font_filter,
    "debug a font filter, use filter NUM, 1..16", "NUM"},
  {"font-filter-list", 'Y', 0, G_OPTION_ARG_NONE, &debug_font_filter_list,
    "print a list of debug a font filters", NULL},
  {"font-num-out", 'j', 0, G_OPTION_ARG_NONE, &text_out_font_numbers,
    "print font numbers in output text", NULL},
  {NULL}
};

static GOptionEntry entries[] = {
  {"image-in", 'i', 0, G_OPTION_ARG_FILENAME, &image_in_filename,
    "use FILE as input image file name", "FILE"},
  {"text-out", 'o', 0, G_OPTION_ARG_FILENAME, &text_out_filename,
    "use FILE as output text file name", "FILE"},
  /* this option is not useful fot the moment {"ltr", 'z', 0,
   * G_OPTION_ARG_NONE, &dir_ltr, "left to right text", NULL}, */
  {"no-gtk", 'N', 0, G_OPTION_ARG_NONE, &no_gtk,
    "do not use gtk for file input and output", NULL},
  {"version", 'v', 0, G_OPTION_ARG_NONE, &version,
    "print version information and exit", NULL},
  {NULL}
};

int
hocr_printerr (const char *msg)
{
  g_printerr ("%s: %s\n\
Try `--help' for more information.\n", g_get_prgname (), msg);
  return TRUE;
}

int
hocr_cmd_parser (int *argc, char **argv[])
{
  GOptionContext *context;
  GOptionGroup *group;

  /* get user args */
  context = g_option_context_new ("- Hebrew OCR utility");

  group =
    g_option_group_new ("file", "File options", "Show file options", NULL,
    NULL);
  g_option_group_add_entries (group, file_entries);
  g_option_context_add_group (context, group);

  group =
    g_option_group_new ("image-proccesing", "Image proccesing options",
    "Show image proccesing options", NULL, NULL);
  g_option_group_add_entries (group, image_entries);
  g_option_context_add_group (context, group);

  group =
    g_option_group_new ("segmentation", "Segmentation options",
    "Show segmentation options", NULL, NULL);
  g_option_group_add_entries (group, segmentation_entries);
  g_option_context_add_group (context, group);

  group =
    g_option_group_new ("debug", "Debug options", "Show debug options",
    NULL, NULL);
  g_option_group_add_entries (group, debug_entries);
  g_option_context_add_group (context, group);

  g_option_context_add_main_entries (context, entries, PACKAGE_NAME);
  g_option_context_parse (context, argc, argv, &error);

  /* free allocated memory */
  g_option_context_free (context);

  if (error)
  {
    hocr_printerr ("unrecognized option");
    exit (1);
  }

  if (version)
  {
    g_printf (copyright_message, BUILD);
    exit (0);
  }

  if (debug_font_filter_list)
  {
    g_printf (font_filters_list, BUILD);
    exit (0);
  }

  if (!no_gtk && !image_in_filename)
  {
    hocr_printerr ("no input image file name (use -n for pipeing)");
    exit (1);
  }

  if (debug || verbose)
    g_print ("%s - Hebrew OCR utility\n", PACKAGE_STRING);

  /* sanity check */
  if (adaptive_threshold_type > 2 || adaptive_threshold_type < 0)
  {
    hocr_printerr ("unknown thresholding type using normal settings");
    adaptive_threshold_type = 0;
  }

  if (scale_by > 9 || scale_by < 0)
  {
    hocr_printerr ("unknown scaling factor using auto settings");
    scale_by = 0;
  }

  if (paragraph_setup > 255 || paragraph_setup < 0)
  {
    hocr_printerr ("unknown paragraph setup using auto settings");
    paragraph_setup = 0;
  }

  if (threshold > 100 || threshold < 0)
  {
    hocr_printerr ("unknown thresholding value using auto settings");
    threshold = 0;
  }

  if (adaptive_threshold > 100 || adaptive_threshold < 0)
  {
    hocr_printerr ("unknown thresholding value using auto settings");
    adaptive_threshold = 0;
  }

  if (font_spacing_code > 3 || font_spacing_code < -3)
  {
    hocr_printerr ("unknown font_spacing value using auto settings");
    font_spacing_code = 0;
  }

  return FALSE;
}

/* FIXME: this function use globals */
ho_bitmap *
hocr_load_input_bitmap ()
{
  ho_pixbuf *pix = NULL;
  ho_bitmap *m_bw = NULL;

  ho_bitmap *m_bw_temp = NULL;
  ho_bitmap *m_bw_temp2 = NULL;

  /* read image from file */
  if (no_gtk)
    pix = ho_pixbuf_pnm_load (image_in_filename);
  else
    pix = ho_gtk_pixbuf_load (image_in_filename);

  if (!pix)
  {
    hocr_printerr ("can't read input image\n");
    exit (1);
  }

  if (debug || verbose)
    g_print (" input image is %d by %d pixels, with %d color channels\n",
      pix->width, pix->height, pix->n_channels);

  /* if copy image */
  if (save_copy)
  {
    gchar *filename;

    /* create file name */
    if (no_gtk)
      filename = g_strdup_printf ("%s-image-copy.pgm", image_out_path);
    else
      filename = g_strdup_printf ("%s-image-copy.png", image_out_path);

    /* save to file system */
    if (filename)
    {
      if (no_gtk)
        ho_pixbuf_pnm_save (pix, filename);
      else
        ho_gtk_pixbuf_save (pix, filename);

      g_free (filename);
    }
  }

  m_bw =
    ho_pixbuf_to_bitmap_wrapper (pix, scale_by, adaptive_threshold_type,
    threshold, adaptive_threshold);

  if (!m_bw)
  {
    hocr_printerr ("can't convert to black and white\n");
    exit (1);
  }

  /* prompt user scale */
  if (debug && scale_by)
    if (scale_by == 1)
      g_print (" user no scaling.\n", scale_by);
    else
      g_print (" user scale by %d.\n", scale_by);

  /* do we need to auto scale ? */
  if (!scale_by)
  {
    /* get fonts size for autoscale */
    if (ho_dimentions_font_width_height_nikud (m_bw, 6, 200, 6, 200))
    {
      hocr_printerr ("can't create object map\n");
      exit (1);
    }

    /* if fonts are too small and user wants auto scale, re-scale image */
    if (m_bw->font_height < 10)
      scale_by = 4;
    else if (m_bw->font_height < 25)
      scale_by = 2;
    else
      scale_by = 1;

    if (debug)
      if (scale_by > 1)
        g_print (" auto scale by %d.\n", scale_by);
      else
        g_print (" no auto scale neaded.\n");

    if (scale_by > 1)
    {
      /* re-create bitmap */
      ho_bitmap_free (m_bw);
      m_bw =
        ho_pixbuf_to_bitmap_wrapper (pix, scale_by,
        adaptive_threshold_type, threshold, adaptive_threshold);

      if (!m_bw)
      {
        hocr_printerr
          ("can't convert to black and white after auto scaleing \n");
        exit (1);
      }

    }
  }

  /* remove very small and very large things */
  if (!do_not_clean_image)
  {
    m_bw_temp =
      ho_bitmap_filter_by_size (m_bw, 3, 3 * m_bw->height / 4, 3,
      3 * m_bw->width / 4);
    if (m_bw_temp)
    {
      ho_bitmap_free (m_bw);
      m_bw = m_bw_temp;
      m_bw_temp = NULL;
    }
  }

  /* remove halfton dots this also imply that input image is b/w */
  if (remove_dots)
  {
    if (debug)
      g_print (" remove halfton dots.\n");

    m_bw_temp = ho_bitmap_filter_remove_dots (m_bw, 4, 4);
    if (m_bw_temp)
    {
      ho_bitmap_free (m_bw);
      m_bw = m_bw_temp;
      m_bw_temp = NULL;
    }
  }

  /* fix_broken_fonts */
  if (fix_broken_fonts)
  {
    if (debug)
      g_print (" try to fix broken fonts.\n");

    m_bw_temp = ho_bitmap_dilation (m_bw);
    if (m_bw_temp)
    {
      ho_bitmap_free (m_bw);
      m_bw = m_bw_temp;
      m_bw_temp = NULL;
    }
  }

  /* fix_ligated_fonts */
  if (fix_ligated_fonts)
  {
    if (debug)
      g_print (" try to fix ligated fonts.\n");

    m_bw_temp = ho_bitmap_opening (m_bw);
    if (m_bw_temp)
    {
      ho_bitmap_free (m_bw);
      m_bw = m_bw_temp;
      m_bw_temp = NULL;
    }
  }

  /* remove too big and too small objects */
  if (remove_images)
  {
    if (debug)
      g_print (" remove images.\n");

    /* adaptive threshold braks big objects use adaptive_threshold_type = 1
     * (none) */
    m_bw_temp = ho_pixbuf_to_bitmap_wrapper (pix, scale_by,
      1, threshold, adaptive_threshold);
    if (!m_bw_temp)
    {
      hocr_printerr ("can't convert to black and white for image removal \n");
      exit (1);
    }

    /* get fonts size for autoscale */
    if (ho_dimentions_font_width_height_nikud (m_bw, 6, 200, 6, 200))
    {
      hocr_printerr ("can't create object map\n");
      exit (1);
    }

    /* remove big objects */
    m_bw_temp2 = ho_bitmap_filter_by_size (m_bw_temp,
      m_bw->font_height * 4,
      m_bw_temp->height, m_bw->font_width * 9, m_bw_temp->width);

    if (!m_bw_temp2)
    {
      hocr_printerr ("can't convert to black and white for image removal \n");
      exit (1);
    }

    ho_bitmap_andnot (m_bw, m_bw_temp2);
    ho_bitmap_free (m_bw_temp2);

    /* remove small objects */
    m_bw_temp2 = ho_bitmap_filter_by_size (m_bw_temp,
      1, m_bw->font_height / 5, 1, m_bw->font_width / 8);
    ho_bitmap_free (m_bw_temp);

    if (!m_bw_temp2)
    {
      hocr_printerr ("can't convert to black and white for image removal \n");
      exit (1);
    }

    ho_bitmap_andnot (m_bw, m_bw_temp2);
    ho_bitmap_free (m_bw_temp2);
  }

  /* free input pixbuf */
  ho_pixbuf_free (pix);

  /* rotate image */
  if (rotate_angle)
  {
    m_bw_temp = ho_bitmap_rotate (m_bw, rotate_angle);
    ho_bitmap_free (m_bw);

    if (!m_bw_temp)
    {
      hocr_printerr ("can't rotate image \n");
      exit (1);
    }

    m_bw = m_bw_temp;
  }

  /* auto rotate image */
  if (auto_rotate)
  {
    int angle;

    /* get fonts size for auto angle */
    if (ho_dimentions_font_width_height_nikud (m_bw, 6, 200, 6, 200))
    {
      hocr_printerr ("can't create object map\n");
      exit (1);
    }

    angle = ho_dimentions_get_lines_angle (m_bw);

    if (debug || verbose)
      g_print (" auto rotate: angle %d.\n", angle);

    if (angle)
    {
      m_bw_temp = ho_bitmap_rotate (m_bw, angle);
      ho_bitmap_free (m_bw);

      if (!m_bw_temp)
      {
        hocr_printerr ("can't auto rotate image \n");
        exit (1);
      }

      m_bw = m_bw_temp;
    }
  }

  return m_bw;
}

/* FIXME: this functions use globals */

int
hocr_exit ()
{
  int block_index;
  int line_index;

  /* allert user */
  if (debug)
    g_print ("free memory.\n");

  /* free image matrixes */
  ho_bitmap_free (m_page_text);

  /* free page layout masks */
  ho_layout_free (l_page);

  /* free file names */
  if (image_in_filename)
    g_free (image_in_filename);
  if (image_out_path)
    g_free (image_out_path);
  if (text_out_filename)
    g_free (text_out_filename);
  if (data_out_filename)
    g_free (data_out_filename);

  /* free html page names */
  if (html_page_title)
    g_free (html_page_title);
  if (html_page_author)
    g_free (html_page_author);
  if (html_page_year)
    g_free (html_page_year);

  /* exit program */
  exit (0);

  return FALSE;
}

int
main (int argc, char *argv[])
{
  int number_of_fonts;
  ho_string *s_text_out = NULL;
  ho_string *s_data_out = NULL;

  /* start of argument analyzing section */

  hocr_cmd_parser (&argc, &argv);

  /* init gtk */
  if (!no_gtk)
    gtk_init (&argc, &argv);

  /* end of argument analyzing section */

  /* start of image proccesing section */
  if (debug || verbose)
    g_print ("start image proccesing.\n");

  /* load the image from input filename */
  m_page_text = hocr_load_input_bitmap ();

  if (debug || verbose)
    if (scale_by > 1)
      g_print
        (" procced image is %d by %d pixels (origianl scaled by %d), one color bit.\n",
        m_page_text->width, m_page_text->height, scale_by);
    else
      g_print (" procced image is %d by %d pixels, one color bit.\n",
        m_page_text->width, m_page_text->height);

  /* if only image proccesing or save b/w image */
  if (save_bw || only_image_proccesing)
  {
    gchar *filename;
    ho_pixbuf *pix_out;

    pix_out = ho_pixbuf_new_from_bitmap (m_page_text);

    if (show_grid)
      ho_pixbuf_draw_grid (pix_out, 120, 30, 0, 0, 0);

    /* create file name */
    if (no_gtk)
      filename = g_strdup_printf ("%s-image-bw.pgm", image_out_path);
    else
      filename = g_strdup_printf ("%s-image-bw.png", image_out_path);

    /* save to file system */
    if (filename)
    {
      if (no_gtk)
        ho_pixbuf_pnm_save (pix_out, filename);
      else
        ho_gtk_pixbuf_save (pix_out, filename);

      /* free locale memory */
      ho_pixbuf_free (pix_out);
      g_free (filename);
    }
  }

  if (debug || verbose)
    g_print ("end image proccesing.\n");

  /* if user want save the b/w picture image proccesing produced */
  if (only_image_proccesing)
    hocr_exit ();

  /* end of image proccesing section */
  /* remember: by now you have allocated and not freed: m_page_text */

  /* start of layout analyzing section */
  if (debug || verbose)
    g_print ("start image layout analysis.\n");

  if (!paragraph_setup)
  {
    int cols;

    cols = ho_dimentions_get_columns (m_page_text);
    if (cols > 0)
    {
      paragraph_setup = cols;
      if (debug)
        g_print ("  guessing %d columns.\n", cols);
    }
    else if (debug)
      g_print ("  guessing one column.\n");
  }

  l_page =
    ho_layout_new (m_page_text, font_spacing_code, paragraph_setup, dir_ltr);

  ho_layout_create_block_mask (l_page);

  if (debug || verbose)
    g_print ("  found %d blocks.\n", l_page->n_blocks);

  /* start counting fonts */
  number_of_fonts = 0;

  /* look for lines inside blocks */
  {
    int block_index;
    int line_index;
    int word_index;

    for (block_index = 0; block_index < l_page->n_blocks; block_index++)
    {
      if (debug || verbose)
        g_print ("  analyzing block %d.\n", block_index + 1);
      ho_layout_create_line_mask (l_page, block_index);

      if (debug || verbose)
        g_print ("    found %d lines.\n", l_page->n_lines[block_index]);

      if (debug)
        g_print
          ("    font height %d width %d, line spacing %d\n",
          l_page->m_blocks_text[block_index]->font_height,
          l_page->m_blocks_text[block_index]->font_width,
          l_page->m_blocks_text[block_index]->line_spacing);

      /* look for words inside line */
      for (line_index = 0; line_index < l_page->n_lines[block_index];
        line_index++)
      {
        if (debug || verbose)
          g_print ("      analyzing line %d.\n", line_index + 1);
        ho_layout_create_word_mask (l_page, block_index, line_index);

        if (debug)
          g_print ("        line avg fill %d common fill %d\n",
            l_page->m_lines_text[block_index][line_index]->
            avg_line_fill,
            l_page->m_lines_text[block_index][line_index]->com_line_fill);

        if (debug)
          g_print ("        found %d words. font spacing %d\n",
            l_page->n_words[block_index][line_index],
            l_page->m_lines_text[block_index][line_index]->font_spacing);

        /* look for fonts inside word */
        for (word_index = 0;
          word_index < l_page->n_words[block_index][line_index]; word_index++)
        {
          if (debug)
            g_print ("        analyzing word %d.\n", word_index + 1);
          ho_layout_create_font_mask (l_page, block_index, line_index,
            word_index, slicing_threshold, slicing_width);

          if (debug)
            g_print ("          found %d fonts.\n",
              l_page->n_fonts[block_index][line_index][word_index]);

          /* count fonts */
          number_of_fonts += l_page->
            n_fonts[block_index][line_index][word_index];
        }
      }
    }
  }

  if (debug || verbose)
    g_print ("end of image layout analysis.\n");

  /* if only layout analysis or save layout image */
  if (save_layout || only_layout_analysis)
  {
    gchar *filename = NULL;
    ho_pixbuf *pix_out = NULL;

    /* allocate */
    /* 
     * pix_out = ho_pixbuf_new_from_layout (layout, show_grid, text_bitmap,
     * block_frame_red, block_frame_green, block_frame_blue, block_frame_alfa,
     * block_frame_width, line_frame_red, line_frame_green, line_frame_blue,
     * line_frame_alfa, line_frame_width, word_frame_red, word_frame_green,
     * word_frame_blue, word_frame_alfa, word_frame_width, font_frame_red,
     * font_frame_green, font_frame_blue, font_frame_alfa, font_frame_width); */
    pix_out =
      ho_pixbuf_new_from_layout (l_page, show_grid, m_page_text, 0, 0, 255, 150,
      5, 255, 0, 0, 255, 5, 255, 240, 0, 180, 255, 0, 250, 0, 235, 255);

    /* create file name */
    if (no_gtk)
      filename = g_strdup_printf ("%s-image-layout.pgm", image_out_path);
    else
      filename = g_strdup_printf ("%s-image-layout.png", image_out_path);

    /* save to file system */
    if (filename)
    {
      if (no_gtk)
        ho_pixbuf_pnm_save (pix_out, filename);
      else
        ho_gtk_pixbuf_save (pix_out, filename);

      ho_pixbuf_free (pix_out);
      g_free (filename);
    }
  }

  /* if user only want layout image exit now */
  if (only_layout_analysis)
    hocr_exit ();

  /* start of word recognition section */
  if (debug || verbose)
    g_print ("start word recognition section.\n");

  if (debug || verbose)
    g_print ("  found %d fonts in page.\n", number_of_fonts);

  {
    int i;
    int font_number = 0;
    int block_index;
    int line_index;
    int word_index;
    int font_index;

    ho_pixbuf *pix_out = NULL;
    ho_bitmap *m_text = NULL;
    ho_bitmap *m_mask = NULL;
    ho_bitmap *m_font_mask = NULL;
    ho_bitmap *m_font_filter = NULL;
    ho_bitmap *m_font_test = NULL;
    ho_bitmap *m_edge = NULL;

    ho_objmap *o_fonts = NULL;

    gchar *filename = NULL;
    gsize length;
    gsize terminator_pos;
    GError *error = NULL;

    /* start the text out */
    s_text_out = ho_string_new ();
    if (!s_text_out)
    {
      hocr_printerr ("can't allocate text memory");
      hocr_exit ();
    }

    /* start the data out */
    if (data_out_filename)
    {
      s_data_out = ho_string_new ();
      if (!s_data_out)
      {
        hocr_printerr ("can't allocate data memory");
        hocr_exit ();
      }

      /* start of page */
      if (text_out_html && s_text_out)
      {
        if (html_page_title && html_page_author && html_page_year)
          text_out =
            g_strdup_printf (html_page_header, html_page_title,
            html_page_author, html_page_year);
        else if (html_page_title && html_page_author)
          text_out =
            g_strdup_printf (html_page_header, html_page_title,
            html_page_author, "");
        else if (html_page_title)
          text_out =
            g_strdup_printf (html_page_header, html_page_title, "", "");
        else
          text_out = g_strdup_printf (html_page_header, "", "", "");

        ho_string_cat (s_text_out, text_out);
        g_free (text_out);
      }

      /* init the first line of data file */
      text_out = g_strdup_printf (html_debug_header);
      ho_string_cat (s_data_out, text_out);
      g_free (text_out);

    }

    for (block_index = 0; block_index < l_page->n_blocks; block_index++)
    {
      /* start of paragraph */

      if (text_out_html && s_text_out)
      {
        text_out =
          g_strdup_printf
          ("    <div class=\"ocr_par\" id=\"par_%d\" title=\"bbox %d %d %d %d\">\n",
          block_index + 1, l_page->m_blocks_text[block_index]->x,
          l_page->m_blocks_text[block_index]->y,
          l_page->m_blocks_text[block_index]->x +
          l_page->m_blocks_text[block_index]->width,
          l_page->m_blocks_text[block_index]->y +
          l_page->m_blocks_text[block_index]->height);
        ho_string_cat (s_text_out, text_out);
        g_free (text_out);
      }

      for (line_index = 0; line_index < l_page->n_lines[block_index];
        line_index++)
      {
        /* start of line */

        if (text_out_font_numbers)
        {
          text_out = g_strdup_printf ("(%04d) ", font_number + 1);
          ho_string_cat (s_text_out, text_out);
          g_free (text_out);
        }

        for (word_index = 0;
          word_index < l_page->n_words[block_index][line_index]; word_index++)
        {
          for (font_index = 0;
            font_index <
            l_page->n_fonts[block_index][line_index][word_index]; font_index++)
          {
            /* count recognized fonts */
            font_number++;

            if (verbose || debug)
              g_print ("    recognizing font %d (%d).\n",
                font_number, number_of_fonts);

            if (debug)
              g_print
                ("    in - block:%d line:%d word:%d font:%d.\n",
                block_index + 1, line_index + 1,
                word_index + 1, font_index + 1);

            /* get the font */
            m_text =
              ho_layout_get_font_text (l_page, block_index,
              line_index, word_index, font_index);
            m_mask =
              ho_layout_get_font_line_mask (l_page, block_index,
              line_index, word_index, font_index);

            /* get font main sign */
            m_font_mask = ho_font_main_sign (m_text, m_mask);

            if (m_mask && m_font_mask)
            {
              /* get nikud */
              m_font_filter = ho_bitmap_clone (m_text);
              ho_bitmap_andnot (m_font_filter, m_font_mask);

              /* debug the font filter functions: */
              if (save_fonts && debug_font_filter)
              {
                if (debug_font_filter < 100)
                {
                  m_font_test =
                    ho_font_filter (m_font_mask, m_mask, debug_font_filter);
                }
                else
                {
                  m_font_test =
                    ho_font_holes_filter (m_font_mask,
                    m_mask, debug_font_filter - 100);
                }

                /* dilate points 9 (ends) 10 (crosses) */
                if (debug_font_filter == 9
                  || debug_font_filter == 10
                  || debug_font_filter == 109 || debug_font_filter == 110)
                {
                  ho_bitmap *m_temp = NULL;

                  m_temp = ho_bitmap_dilation (m_font_test);
                  ho_bitmap_free (m_font_test);
                  m_font_test = m_temp;
                }
              }
            }

            /* recognize the font and send it out */

            /* if user ask, dump font images to disk */
            if (save_fonts && m_mask && m_font_filter && m_font_mask)
            {
              if (no_gtk)
                filename =
                  g_strdup_printf ("%s-font-%d.pgm",
                  image_out_path, font_number);
              else
                filename =
                  g_strdup_printf ("%s-font-%d.png",
                  image_out_path, font_number);

              /* if user want to debug font filters, printout the filtered font 
               */
              if (m_font_test && debug_font_filter)
              {
                m_font_mask->x = 0;
                m_font_mask->y = 0;
                m_font_test->x = 0;
                m_font_test->y = 0;
                m_mask->x = 0;
                m_mask->y = 0;

                if (no_gtk)
                  ho_font_pnm_save (m_font_mask, m_font_test, m_mask, filename);
                else
                  ho_gtk_font_save (m_font_mask, m_font_test, m_mask, filename);
              }
              else
              {
                m_font_mask->x = 0;
                m_font_mask->y = 0;
                m_font_filter->x = 0;
                m_font_filter->y = 0;
                m_mask->x = 0;
                m_mask->y = 0;

                if (no_gtk)
                  ho_font_pnm_save (m_font_mask,
                    m_font_filter, m_mask, filename);
                else
                  ho_gtk_font_save (m_font_mask,
                    m_font_filter, m_mask, filename);
              }

              g_free (filename);
            }

            /* get data on font */
            if (m_font_mask && m_mask)
            {
              char *font;
              double array_in[HO_ARRAY_IN_SIZE];
              double array_out[HO_ARRAY_OUT_SIZE];

              /* insert font to text out */
              ho_recognize_create_array_in (m_font_mask, m_mask, array_in);
              ho_recognize_create_array_out (array_in, array_out);
              font = ho_recognize_array_out_to_font (array_out);
              /* font = ho_recognize_font (m_font_mask, m_mask); */
              ho_string_cat (s_text_out, font);

              /* if debug printout font data stream */
              if (debug)
              {
                int i;

                for (i = 0; i < HO_ARRAY_IN_SIZE; i++)
                {
                  if (!(i % 50))
                    g_print ("\n");
                  if (!(i % 10))
                    g_print ("\n");
                  else if (!(i % 5))
                    g_print (" ");

                  g_print ("%03.2f, ", array_in[i]);

                }

                g_print ("\n");
              }

              /* if user want to dump data to file */
              if (data_out_filename && m_mask && m_font_filter && m_font_mask)
              {
                int i;

                /* print out the number of the font */
                text_out = g_strdup_printf ("\nfont %d:\n ", font_number);
                ho_string_cat (s_data_out, text_out);
                g_free (text_out);

                /* print font image in html code */
                text_out =
                  g_strdup_printf
                  ("\n<img src=\"%s-font-%d.png\" alt=\"font image\">\n",
                  image_out_path, font_number);
                ho_string_cat (s_data_out, text_out);
                g_free (text_out);

                for (i = 0; i < HO_ARRAY_IN_SIZE; i++)
                {
                  if (!(i % 50))
                    ho_string_cat (s_data_out, "\n");
                  if (!(i % 10))
                    ho_string_cat (s_data_out, "\n");
                  else if (!(i % 5))
                    ho_string_cat (s_data_out, "  ");

                  text_out = g_strdup_printf ("%03.2f, ", array_in[i]);
                  ho_string_cat (s_data_out, text_out);

                  g_free (text_out);
                }

                text_out = g_strdup_printf ("\nfont guess is %s\n ", font);
                ho_string_cat (s_data_out, text_out);
              }

              /* if debug print out the font */
              if (debug)
                g_print ("font guess is %s\n", font);

            }

            /* free bitmaps and others */
            ho_bitmap_free (m_font_filter);
            ho_bitmap_free (m_font_test);
            ho_bitmap_free (m_font_mask);
            ho_bitmap_free (m_text);
            ho_bitmap_free (m_mask);

            m_text = m_mask = m_font_test = m_font_filter = m_font_mask = NULL;

            /* oh ... */

          }                     /* end of fonts loop */
          /* add a space after a word to text out */
          ho_string_cat (s_text_out, " ");

        }                       /* end of words loop */

        /* end of line */

        if (text_out_html && s_text_out)
        {
          ho_string_cat (s_text_out, "<br/>");
        }

        /* add a line-end after a line to text out */
        ho_string_cat (s_text_out, "\n");
      }

      /* end of paragraph */

      if (text_out_html && s_text_out)
      {
        ho_string_cat (s_text_out, "<br/>");
      }

      /* add a line-end after a block end */
      ho_string_cat (s_text_out, "\n");

      if (text_out_html && s_text_out)
      {
        text_out = g_strdup_printf ("    </div>\n");
        ho_string_cat (s_text_out, text_out);
        g_free (text_out);
      }

    }

  }

  /* end of page */
  if (text_out_html && s_text_out)
  {
    text_out = g_strdup_printf (html_page_footer);
    ho_string_cat (s_text_out, text_out);
    g_free (text_out);
  }

  /* close data file html fromat */
  if (s_data_out)
  {
    text_out = g_strdup_printf (html_debug_footer);
    ho_string_cat (s_data_out, text_out);
    g_free (text_out);
  }

  if (debug || verbose)
    g_print ("end word recognition section.\n");
  /* end of word recognition section */

  /* start user output section */

  if (debug || verbose)
    g_print ("start output section.\n");

  /* convert text from ho_string to (gchar *) */
  text_out = g_strdup_printf ("%s", s_text_out->string);
  ho_string_free (s_text_out);

  /* save text */
  if (text_out_filename && text_out)
  {
    /* if filename is '-' print to stdout */
    if (text_out_filename[0] == '-' && text_out_filename[1] == '\0')
      g_printf (text_out);
    else
      g_file_set_contents (text_out_filename, text_out, -1, &error);

    if (error)
    {
      hocr_printerr ("can't write text to file");
    }
  }

  g_free (text_out);
  text_out = NULL;

  /* convert data from ho_string to (gchar *) */
  if (s_data_out)
  {
    text_out = g_strdup_printf ("%s", s_data_out->string);
    ho_string_free (s_data_out);
  }

  /* save data */
  if (data_out_filename && text_out)
  {
    g_file_set_contents (data_out_filename, text_out, -1, &error);

    if (error)
    {
      hocr_printerr ("can't write data to file");
    }
  }

  if (text_out)
    g_free (text_out);

  if (debug || verbose)
    g_print ("end output section.\n");

  /* end of user putput section */

  /* start of memory cleanup section */

  hocr_exit ();

  return FALSE;
}
