/* -*- Mode: C; tab-width: 3; indent-tabs-mode: nil; c-basic-offset: 3 -*- */

/*
 * GImageView
 * Copyright (C) 2001 Takuro Ashie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __FreeBSD__
#include <sys/param.h>
#endif

#ifdef HAVE_OS2_H
#include <os2.h>
#include <sys/types.h>
#endif

#include <netinet/in.h>
#include <gtk/gtk.h>

#include "gimv_image.h"
#include "xcf.h"
#include "gimv_plugin.h"

#ifndef ABS
#   define ABS(x) (((x)>=0)?(x):(-(x)))
#endif
#ifndef MIN
#   define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#   define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define MAX_LAYERS 256
#define MAX_CHANNELS 256

/*
 * These two values are defined in GIMP, and they are very
 * IMPORTANT. Do NOT change them.
 */
#define TILE_WIDTH 64
#define TILE_HEIGHT 64

typedef enum
{
   PROP_END = 0,
   PROP_COLORMAP = 1,
   PROP_ACTIVE_LAYER = 2,
   PROP_ACTIVE_CHANNEL = 3,
   PROP_SELECTION = 4,
   PROP_FLOATING_SELECTION = 5,
   PROP_OPACITY = 6,
   PROP_MODE = 7,
   PROP_VISIBLE = 8,
   PROP_LINKED = 9,
   PROP_PRESERVE_TRANSPARENCY = 10,
   PROP_APPLY_MASK = 11,
   PROP_EDIT_MASK = 12,
   PROP_SHOW_MASK = 13,
   PROP_SHOW_MASKED = 14,
   PROP_OFFSETS = 15,
   PROP_COLOR = 16,
   PROP_COMPRESSION = 17,
   PROP_GUIDES = 18
} PropType;

typedef enum
{
   COMPRESS_NONE = 0,
   COMPRESS_RLE = 1,
   COMPRESS_ZLIB = 2,
   COMPRESS_FRACTAL = 3
} CompressionType;

typedef enum
{
   XCF_RGB_IMAGE     = 0,
   XCF_GRAY_IMAGE    = 1,
   XCF_INDEXED_IMAGE = 2
} GImageType;

typedef enum
{
   TILE_RGB = 0,
   TILE_GRAY = 1,
   TILE_INDEXED = 2,
   TILE_MASK = 3,
   TILE_CHANNEL = 4
} HierarchyType;

typedef struct
{
   guint32 width;
   guint32 height;
   guint32 bpp;
   HierarchyType type;
	
   /* level */
   gint curlevel;
   guint32 level_width;
   guint32 level_height;
	
   /* tile */
   gint curtile;
	
   /* data */
   guchar *data;
} XcfHierarchy;

typedef struct
{
   guint32 width;
   guint32 height;
   guint32 type;
   guint32 opacity;
   guint32 visible;
   guint32 linked;
   guint32 preserve_trans;
   guint32 apply_mask;
   guint32 edit_mask;
   guint32 show_mask;
   gint32 offset_x;
   gint32 offset_y;
   guint32 mode;
} XcfLayer;

typedef struct
{
   guint32 width;
   guint32 height;
   guint32 opacity;
   guint32 visible;
   guint32 show_mask;
   guint8 color[3];
} XcfChannel;

typedef struct
{
   gint version;
   guint32 width;
   guint32 height;
   guint32 type;
   guint8 compression;
	
   gint curlayer;
   gint curchannel;
	
   gint pass;
   guchar *pixels;

   /* indexed */
   guint32 num_cols;
   guchar cmap[768];
	
   /* channel */
   guint8 color[3];
	
} XcfImage;


guint    xcf_read_int32              (GimvIO          *gio,
                                      guint32         *data,
                                      gint             count);
guint    xcf_read_int8               (GimvIO          *gio,
                                      guint8          *data,
                                      gint             count);
guint    xcf_read_string             (GimvIO          *gio,
                                      gchar           *data);
gboolean xcf_load_image              (GimvImageLoader *loader,
                                      XcfImage        *image);
gboolean xcf_load_layer              (GimvImageLoader *loader,
                                      XcfImage        *image);
gboolean xcf_load_channel            (GimvImageLoader *loader,
                                      XcfImage        *image);
gboolean xcf_load_layer_mask         (GimvImageLoader *loader,
                                      XcfImage        *image,
                                      XcfHierarchy    *hierarchy);
gboolean xcf_load_hierarchy	       (GimvImageLoader *loader,
                                      XcfImage        *image,
                                      XcfHierarchy    *hierarchy);
gboolean xcf_load_level		          (GimvImageLoader *loader,
                                      XcfImage        *image,
                                      XcfHierarchy    *hierarchy);
gboolean xcf_load_tile		          (GimvImageLoader *loader,
                                      XcfImage        *image,
                                      XcfHierarchy    *hierarchy);
gboolean xcf_load_image_properties   (GimvImageLoader *loader,
                                      XcfImage        *image);
gboolean xcf_load_layer_properties   (GimvImageLoader *loader,
                                      XcfLayer        *layer);
gboolean xcf_load_channel_properties (GimvImageLoader *loader,
                                      XcfChannel      *channel);
void     xcf_put_pixel_element	    (XcfImage        *image,
                                      guchar          *line,
                                      gint             x,
                                      gint             off,
                                      gint             ic);


static GimvImageLoaderPlugin gimv_xcf_loader[] =
{
   {
      if_version:    GIMV_IMAGE_LOADER_IF_VERSION,
      id:            "XCF",
      priority_hint: GIMV_IMAGE_LOADER_PRIORITY_CAN_CANCEL,
      check_type:    NULL,
      get_info:      NULL,
      loader:        xcf_load,
   }
};

static const gchar *xcf_extensions[] =
{
   "xcf",
};

static GimvMimeTypeEntry xcf_mime_types[] =
{
   {
      mime_type:      "image/x-xcf",
      description:    "GIMP Image Format",
      extensions:     xcf_extensions,
      extensions_len: sizeof (xcf_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-xcf-gimp",
      description:    "GIMP Image Format",
      extensions:     xcf_extensions,
      extensions_len: sizeof (xcf_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
   {
      mime_type:      "image/x-compressed-xcf",
      description:    "Compressed GIMP Image Format",
      extensions:     xcf_extensions,
      extensions_len: sizeof (xcf_extensions) / sizeof (gchar *),
      icon:           NULL,
   },
};

GIMV_PLUGIN_GET_IMPL(gimv_xcf_loader, GIMV_PLUGIN_IMAGE_LOADER)
GIMV_PLUGIN_GET_MIME_TYPE(xcf_mime_types)

GimvPluginInfo gimv_plugin_info =
{
   if_version:    GIMV_PLUGIN_IF_VERSION,
   name:          N_("XCF Image Loader"),
   version:       "0.1.1",
   author:        N_("Takuro Ashie"),
   description:   NULL,
   get_implement: gimv_plugin_get_impl,
   get_mime_type: gimv_plugin_get_mime_type,
   get_prefs_ui:  NULL,
};


gboolean
xcf_get_header (GimvIO *gio, xcf_info *info)
{
   guchar buf[10];
   gint version;
   guint32 type;
   guint bytes;

   /* signature */
   gimv_io_read (gio, buf, 9, &bytes);
   if (((gint) bytes < 1) || strncmp (buf, "gimp xcf ", 9) != 0) return FALSE;
	
   /* version */
   gimv_io_read (gio, buf, 5, &bytes);
   if (((gint) bytes < 1) || buf[4] != '\0') return FALSE;
   if (!strncmp (buf, "file", 4)) {
      version = 0;
   } else if (buf[0] == 'v') {
      version = atoi (&buf[1]);
   } else {
      return FALSE;
   }
   if (version < 0 || version > 1) {
      return FALSE;
   }
	
   /* width, height, type */
   if (!xcf_read_int32 (gio, &info->width, 1) ||
       !xcf_read_int32 (gio, &info->height, 1) ||
       !xcf_read_int32 (gio, &type, 1))
   {
      return FALSE;
   }
	
   switch (type) {
   case XCF_RGB_IMAGE:
      info->ncolors = 24;
      info->alpha = 0;
      break;
   case XCF_GRAY_IMAGE:
      info->ncolors = 8;
      info->alpha = 0;
      break;
   case XCF_INDEXED_IMAGE:
      info->ncolors = 8;
      info->alpha = 0;
      break;
   default: /* unknown image type */
      return FALSE;
   }

   return TRUE;
}


GimvImage *
xcf_load (GimvImageLoader *loader, gpointer data)
{
   GimvIO *gio;
   GimvImage *image = NULL;
   XcfImage xcf_image;
   gboolean success;

   g_return_val_if_fail (loader, NULL);

   gio = gimv_image_loader_get_gio (loader);
   if (!gio) return NULL;

   xcf_image.compression = COMPRESS_NONE;
   xcf_image.pixels = NULL;

   /* load the image */
   success = xcf_load_image (loader, &xcf_image);

   if (success) {
      image = gimv_image_create_from_data (xcf_image.pixels,
                                           xcf_image.width, xcf_image.height,
                                           FALSE);
   }

   return image;
}


gboolean
xcf_load_image (GimvImageLoader *loader, XcfImage *image)
{
   GimvIO *gio;
   guchar buf[10];
   guint32 layer_offset[MAX_LAYERS];
   guint32 channel_offset[MAX_CHANNELS];
   gint i, num_layers, num_channels;
   gint num_bytes = 3;
   guint32 offset, bytes;
   glong pos;
   gboolean success;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   /* signature */
   gimv_io_read (gio, buf, 9, &bytes);
   if (((gint) bytes < 9) || memcmp (buf, "gimp xcf ", 9) != 0)
      return FALSE;

   /* version */
   gimv_io_read (gio, buf, 5, &bytes);
   if ((gint) bytes < 5 || buf[4] != '\0') return FALSE;

   if (!memcmp (buf, "file", 4))
      image->version = 0;
   else if (buf[0] == 'v')
      image->version = atoi (&buf[1]);
   else
      return FALSE;

   if (image->version < 0 || image->version > 1) return FALSE;

   /* width, height, type */
   if (!xcf_read_int32 (gio, &image->width, 1)
       || !xcf_read_int32 (gio, &image->height, 1)
       || !xcf_read_int32 (gio, &image->type, 1))
   {
      return FALSE;
   }

   switch (image->type) {
   case XCF_RGB_IMAGE:
   case XCF_GRAY_IMAGE:
   case XCF_INDEXED_IMAGE:
      break;
   default: /* unknown image type */
      return FALSE;
   }

   success = xcf_load_image_properties (loader, image);
   if (!success) return FALSE;

#ifdef XCF_DEBUG
   g_print ("version=%i width=%i height=%i type=%i compression=%i\n",
            image->version,
            image->width,
            image->height,
            image->type,
            image->compression);
#endif

   if (!gimv_image_loader_progress_update (loader))
      return FALSE;

   image->pixels = g_malloc (sizeof (guchar) * image->width * image->height * num_bytes);

   /* load layers */
   num_layers = 0;
   while (TRUE) {
      if (!xcf_read_int32 (gio, &offset, 1)) goto ERROR;
      if (offset == 0) break;
      if (num_layers < MAX_LAYERS)
         layer_offset[num_layers++] = offset;
   }

   gimv_io_tell (gio, &pos);
   image->curlayer = 0;
   image->pass = 0;
   for (i = num_layers - 1; i >= 0; i--) {
      gimv_io_seek (gio, layer_offset[i], SEEK_SET);
      if (!xcf_load_layer (loader, image)) goto ERROR;
      image->curlayer++;
   }

   gimv_io_seek (gio, pos, SEEK_SET);

   /* load channels */
   num_channels = 0;
   while (TRUE) {
      if (!xcf_read_int32 (gio, &offset, 1)) goto ERROR;
      if (offset == 0) break;
      if (num_channels < MAX_CHANNELS)
         channel_offset[num_channels++] = offset;
   }

   image->curchannel = 0;
   for (i = 0; i < num_channels; i++) {
      gimv_io_seek (gio, channel_offset[i], SEEK_SET);
      if (!xcf_load_channel (loader, image)) goto ERROR;
      image->curchannel++;
   }
	
   /* That's all. */
   return TRUE;

ERROR:
   g_free (image->pixels);
   image->pixels = NULL;
   return FALSE;
}


gboolean
xcf_load_layer (GimvImageLoader *loader, XcfImage *image)
{
   GimvIO *gio;
   XcfHierarchy hierarchy;
   XcfLayer layer;
   guint32 offset;
   gulong pos, len;
   guint i, left, right, width;
   guchar *pix;
#ifdef XCF_DEBUG
   guchar name[256];
#endif

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   layer.offset_x = 0;
   layer.offset_y = 0;
   layer.opacity = 255;
   layer.visible = 1;
   layer.linked = 0;
   layer.preserve_trans = 0;
   layer.apply_mask = 0;
   layer.edit_mask = 0;
   layer.show_mask = 0;
   layer.mode = GIMV_IMAGE_NORMAL_MODE;

   /* layer width, height, type */
   if (!xcf_read_int32(gio, &layer.width, 1)
       || !xcf_read_int32(gio, &layer.height, 1)
       || !xcf_read_int32(gio, &layer.type, 1))
   {
      return FALSE;
   }
	
#ifdef XCF_DEBUG
   xcf_read_string (gio, name);
#else
   xcf_read_string (gio, NULL);
#endif

   if (!xcf_load_layer_properties (loader, &layer)) return FALSE;

#ifdef XCF_DEBUG
   g_print ("Loading layer %i: %s\n", image->curlayer, name);
   g_print ("    width=%i height=%i type=%i offset_x=%i offset_y=%i\n",
            layer.width,
            layer.height,
            layer.type,
            layer.offset_x,
            layer.offset_y);
#endif

   if (!layer.visible) return TRUE;
	
   /* hierarchy */
   len = sizeof (guchar) * layer.width * layer.height * 4;
   hierarchy.data = g_malloc (len);
   memset (hierarchy.data, 255, len);

   if (!xcf_read_int32 (gio, &offset, 1)) {
      g_free (hierarchy.data);
      return FALSE;
   }
   gimv_io_tell (gio, &pos);
   gimv_io_seek (gio, offset, SEEK_SET);

   hierarchy.type = image->type;
   if (!xcf_load_hierarchy (loader, image, &hierarchy)) {
      g_free (hierarchy.data);
      return FALSE;
   }
   gimv_io_seek (gio, pos, SEEK_SET);
	
   /* layer mask */
   if (!xcf_read_int32 (gio, &offset, 1)) {
      g_free (hierarchy.data);
      return FALSE;
   }
   if (offset > 0) {
      gimv_io_tell (gio, &pos);
      gimv_io_seek (gio, offset, SEEK_SET);

      if (!xcf_load_layer_mask (loader, image, &hierarchy)) {
         g_free (hierarchy.data);
         return FALSE;
      }
      gimv_io_seek (gio, pos, SEEK_SET);
   }

   /* do layer opacity */
   if (layer.opacity < 255) {
      len = layer.width * layer.height;
      for (i = 0, pix = &hierarchy.data[3]; i < len; i++, pix += 4) {
         *pix = (*pix) * layer.opacity / 255;
      }
   }

   /* put pixels */
   left = MAX (layer.offset_x, 0);
   right = MIN (layer.offset_x + layer.width, image->width);
   width = right - left;
   for (i = MAX (layer.offset_y, 0);
        i < MIN (layer.offset_y + layer.height, image->height);
        i++)
   {
      guchar *rgbbuffer = image->pixels + (image->width * i + left) * 3;

      gimv_image_add_layer (&hierarchy.data[((i - layer.offset_y) * layer.width
                                             + (left - layer.offset_x)) << 2],
                            width, left, 4, image->pass, layer.mode, rgbbuffer);
   }
	
   image->pass++;
	
   g_free(hierarchy.data);

   return TRUE;
}


gboolean
xcf_load_channel (GimvImageLoader *loader, XcfImage *image)
{
   GimvIO *gio;
   XcfHierarchy hierarchy;
   XcfChannel channel;
   guint32 offset;
   guint i;
   gulong pos, len;
   guchar *pix;
#ifdef XCF_DEBUG
   guchar name[256];
#endif

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   channel.opacity = 255;
   channel.visible = 1;

   /* channel width, height */
   if (!xcf_read_int32 (gio, &channel.width, 1)
       || !xcf_read_int32 (gio, &channel.height, 1))
   {
      return FALSE;
   }

#ifdef XCF_DEBUG
   xcf_read_string (gio, name);
   g_print ("Loading channel %i: %s\n", image->curchannel, name);
   g_print ("    width=%i height=%i\n",
            channel.width,
            channel.height);
#else
   xcf_read_string (gio, NULL);
#endif

   if (!xcf_load_channel_properties (loader, &channel)) return FALSE;

   if (!channel.visible) return TRUE;

   /* hierarchy */
   if (!xcf_read_int32 (gio, &offset, 1)) return FALSE;
   gimv_io_tell (gio, &pos);
   gimv_io_seek (gio, offset, SEEK_SET);

   hierarchy.type = TILE_CHANNEL;
   image->color[0] = channel.color[0];
   image->color[1] = channel.color[1];
   image->color[2] = channel.color[2];
   len = channel.width * channel.height;
   hierarchy.data = g_malloc (sizeof (guchar) * len * 4);
   if (!xcf_load_hierarchy (loader, image, &hierarchy)) {
      g_free (hierarchy.data);
      return FALSE;
   }
   gimv_io_seek (gio, pos, SEEK_SET);

   /* do opacity */
   if (channel.opacity < 255) {
      for (i = 0, pix = &hierarchy.data[3]; i < len; i++, pix += 4) {
         *pix = (*pix) * channel.opacity / 255;
      }
   }

   /* put pixels */
   for (i = 0; i < channel.height; i++) {
      guchar *rgbbuffer = image->pixels + (image->width * i) * 3;

      gimv_image_add_layer (&hierarchy.data[i * channel.width * 4],
                            channel.width, 0, 4,
                            image->pass, GIMV_IMAGE_NORMAL_MODE, rgbbuffer);
   }

   image->pass++;

   g_free (hierarchy.data);
   return TRUE;
}


gboolean
xcf_load_layer_mask (GimvImageLoader *loader, XcfImage *image, XcfHierarchy *hierarchy)
{
   GimvIO *gio;
   guint32 width, height;
   XcfChannel channel;
   guint32 offset;
   glong pos;
#ifdef XCF_DEBUG
   guchar name[256];
#endif

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   if (!xcf_read_int32 (gio, &width, 1)
       || !xcf_read_int32 (gio, &height, 1))
   {
      return FALSE;
   }

#ifdef XCF_DEBUG
   if (!xcf_read_string (gio, name)) return FALSE;
   g_print ("    Loading layer mask %s: width=%i height=%i\n",
            name, width, height);
#else
   if (!xcf_read_string(gio, NULL)) return FALSE;
#endif

   if (!xcf_load_channel_properties (loader, &channel)) return FALSE;
   if (!xcf_read_int32 (gio, &offset, 1)) return FALSE;
   gimv_io_tell (gio, &pos);
   gimv_io_seek (gio, offset, SEEK_SET);
   hierarchy->type = TILE_MASK;
   if (!xcf_load_hierarchy (loader, image, hierarchy)) return FALSE;
   gimv_io_seek (gio, pos, SEEK_SET);

   return TRUE;
}


gboolean
xcf_load_hierarchy (GimvImageLoader *loader, XcfImage *image, XcfHierarchy *hierarchy)
{
   GimvIO *gio;
   guint32 offset;
   glong pos;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   /* hierarchy width, height, bpp(BYTEs Per Pixel) */
   if (!xcf_read_int32 (gio, &hierarchy->width, 1)
       || !xcf_read_int32 (gio, &hierarchy->height, 1)
       || !xcf_read_int32 (gio, &hierarchy->bpp, 1))
   {
      return FALSE;
   }

#ifdef XCF_DEBUG
   g_print("	Hierarchy: width=%i height=%i bpp=%i\n",
           hierarchy->width, hierarchy->height, hierarchy->bpp);
#endif

   hierarchy->curlevel = 0;
   while (TRUE) {
      if (!xcf_read_int32 (gio, &offset, 1)) return FALSE;
      if (offset == 0) break;
      gimv_io_tell (gio, &pos);
      gimv_io_seek (gio, offset, SEEK_SET);
      if (!xcf_load_level (loader, image, hierarchy)) return FALSE;
      gimv_io_seek (gio, pos, SEEK_SET);
      hierarchy->curlevel++;
   }

   return TRUE;
}


gboolean
xcf_load_level (GimvImageLoader *loader, XcfImage *image, XcfHierarchy *hierarchy)
{
   GimvIO *gio;
   guint32 offset;
   glong pos;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   /* level width, height */
   if (!xcf_read_int32 (gio, &hierarchy->level_width, 1)
       || !xcf_read_int32 (gio, &hierarchy->level_height, 1))
   {
      return FALSE;
   }

#ifdef XCF_DEBUG
   g_print ("        Level: width=%i height=%i\n",
            hierarchy->level_width, hierarchy->level_height);
#endif

   hierarchy->curtile = 0;
   while (TRUE) {
      if (!xcf_read_int32 (gio, &offset, 1)) return FALSE;
      if (offset == 0) break;
      gimv_io_tell (gio, &pos);
      gimv_io_seek (gio, offset, SEEK_SET);
      if (!xcf_load_tile (loader, image, hierarchy)) return FALSE;
      gimv_io_seek (gio, pos, SEEK_SET);
      hierarchy->curtile++;

      if (!gimv_image_loader_progress_update (loader))
         return FALSE;
   }

   return TRUE;
}


gboolean
xcf_load_tile (GimvImageLoader *loader, XcfImage *image, XcfHierarchy *hierarchy)
{
   GimvIO *gio;
   guint32 current, size, bpp, tile_height, tile_width;
   guint i, j;
   gint n, c, t, off, ntile_rows, ntile_columns;
   gint curtile_row, curtile_column;
   gint lastrow_height, lastcolumn_width;
   gint rowi, columni;
   guchar *data;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   ntile_rows = (hierarchy->level_height + TILE_HEIGHT - 1) / TILE_HEIGHT;
   ntile_columns = (hierarchy->level_width + TILE_WIDTH - 1) / TILE_WIDTH;
   curtile_row = hierarchy->curtile / ntile_columns;
   curtile_column = hierarchy->curtile % ntile_columns;

   lastcolumn_width = hierarchy->level_width - ((ntile_columns - 1) * TILE_WIDTH);
   lastrow_height = hierarchy->level_height - ((ntile_rows - 1) * TILE_HEIGHT);

   tile_width = TILE_WIDTH;
   tile_height = TILE_HEIGHT;

   if (curtile_column == ntile_columns - 1) {
      tile_width = lastcolumn_width;
   }
   if (curtile_row == ntile_rows - 1) {
      tile_height = lastrow_height;
   }

   size = tile_width * tile_height;

#ifdef XCF_DEBUG
   g_print ("        Tile %i: row=%i column=%i width=%i height=%i\n",
            hierarchy->curtile,
            curtile_row, curtile_column,
            tile_width, tile_height);
#endif

   bpp = hierarchy->bpp;

   switch (image->compression) {
   case COMPRESS_NONE:
      for (i = 0; i < bpp; i++) {
         switch (hierarchy->type) {
         case TILE_RGB:
            off = i;
            break;
         case TILE_GRAY:
            if (i == 0) off = -1;
            else off = 3;
            break;
         case TILE_INDEXED:
            if (i == 0) off = -2;
            else off = 3;
            break;
         case TILE_MASK:
            off = 4;
            break;
         case TILE_CHANNEL:
            off = 5;
            break;
         default:
            off = -3;
            break;
         }

         current = 0;
         while (current < size) {
            c = gimv_io_getc (gio, NULL);
            if (c == EOF) return FALSE;
            rowi = current / tile_width;
            columni = current % tile_width;
            data = &hierarchy->data[((rowi+curtile_row * TILE_HEIGHT)
                                     * hierarchy->level_width
                                     + curtile_column * TILE_WIDTH) * 4];
            xcf_put_pixel_element (image, data, columni, off, c);
            current++;
         }
      }
      break;
   case COMPRESS_RLE:
      for (i = 0; i < bpp; i++) {
         switch (hierarchy->type) {
         case TILE_RGB:
            off = i;
            break;
         case TILE_GRAY:
            if (i == 0) off = -1;
            else off = 3;
            break;
         case TILE_INDEXED:
            if (i == 0) off = -2;
            else off = 3;
            break;
         case TILE_MASK:
            off = 4;
            break;
         case TILE_CHANNEL:
            off = 5;
            break;
         default:
            off = -3;
            break;
         }

         current = 0;
         while (current < size) {
            n = gimv_io_getc (gio, NULL);
            if (n == EOF) return FALSE;
            if (n >= 128) {
               if (n == 128) {
                  n = gimv_io_getc (gio, NULL);
                  if (n == EOF) return FALSE;
                  t = gimv_io_getc (gio, NULL);
                  if (t == EOF) return FALSE;
                  n = (n << 8) | t;
               } else {
                  n = 256 - n;
               }
               for (j = 0; (gint) j < n && current < size; j++) {
                  c = gimv_io_getc (gio, NULL);
                  if (c == EOF) return FALSE;
                  rowi = current / tile_width;
                  columni = current % tile_width;
                  data = &hierarchy->data[((rowi + curtile_row * TILE_HEIGHT)
                                           * hierarchy->level_width
                                           + curtile_column * TILE_WIDTH) * 4];
                  xcf_put_pixel_element (image, data, columni, off, c);
                  current++;
               }
            } else {
               n++;
               if (n == 128) {
                  n = gimv_io_getc (gio, NULL);
                  if (n == EOF) return FALSE;
                  t = gimv_io_getc (gio, NULL);
                  if (t == EOF) return FALSE;
                  n = (n << 8) | t;
               }
               c = gimv_io_getc (gio, NULL);
               if (c == EOF) return FALSE;
               for (j = 0; (gint) j < n && current < size; j++) {
                  rowi = current / tile_width;
                  columni = current % tile_width;
                  data = &hierarchy->data[((rowi + curtile_row * TILE_HEIGHT)
                                           * hierarchy->level_width
                                           + curtile_column * TILE_WIDTH) * 4];
                  xcf_put_pixel_element (image, data, columni, off, c);
                  current++;
               }
            }
         }
      }
      break;

   case COMPRESS_ZLIB:
   case COMPRESS_FRACTAL:
   default:
      return FALSE;
   }

   return TRUE;
}


gboolean
xcf_load_image_properties (GimvImageLoader *loader, XcfImage *image)
{
   GimvIO *gio;
   PropType prop_type;
   guint32 prop_size;
   guint i, j;
   guint8 compression;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   while (TRUE) {
      if (!xcf_read_int32(gio, &prop_type, 1)
          || !xcf_read_int32(gio, &prop_size, 1))
         return FALSE;

      switch (prop_type) {
      case PROP_END:
         return TRUE;
      case PROP_COLORMAP:
         if (!xcf_read_int32 (gio, &image->num_cols, 1)) return FALSE;
         if (image->version == 0) {
            gimv_io_seek (gio, image->num_cols, SEEK_SET);
            for (i = 0, j = 0; i < image->num_cols; i++) {
               image->cmap[j++] = i;
               image->cmap[j++] = i;
               image->cmap[j++] = i;
            }
         } else {
            if (!xcf_read_int8 (gio, image->cmap, image->num_cols * 3))
               return FALSE;
         }
         break;
      case PROP_COMPRESSION:
         if (!xcf_read_int8 (gio, &compression, 1)) return FALSE;
         if ((compression != COMPRESS_NONE)
             && (compression != COMPRESS_RLE)
             && (compression != COMPRESS_ZLIB)
             && (compression != COMPRESS_FRACTAL))
         {
            return FALSE;
         }
         image->compression = compression;
         break;
      case PROP_GUIDES:
      default:
         gimv_io_seek (gio, prop_size, SEEK_CUR);
         break;
      }
   }
}


gboolean
xcf_load_layer_properties (GimvImageLoader *loader, XcfLayer *layer)
{
   GimvIO *gio;
   PropType prop_type;
   guint32 prop_size, dummy;
  
   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   while (TRUE) {
      if (!xcf_read_int32 (gio, &prop_type, 1)
          || !xcf_read_int32 (gio, &prop_size, 1))
      {
         return FALSE;
      }

      switch (prop_type) {
      case PROP_END:
         return TRUE;
      case PROP_ACTIVE_LAYER:
         break;
      case PROP_FLOATING_SELECTION:
         if (!xcf_read_int32 (gio, &dummy, 1)) return FALSE;
         break;
      case PROP_OPACITY:
         if (!xcf_read_int32 (gio, &layer->opacity, 1)) return FALSE;
         break;
      case PROP_VISIBLE:
         if (!xcf_read_int32 (gio, &layer->visible, 1)) return FALSE;
         break;
      case PROP_LINKED:
         if (!xcf_read_int32 (gio, &layer->linked, 1)) return FALSE;
         break;
      case PROP_PRESERVE_TRANSPARENCY:
         if (!xcf_read_int32 (gio, &layer->preserve_trans, 1)) return FALSE;
         break;
      case PROP_APPLY_MASK:
         if (!xcf_read_int32 (gio, &layer->apply_mask, 1)) return FALSE;
         break;
      case PROP_EDIT_MASK:
         if (!xcf_read_int32 (gio, &layer->edit_mask, 1)) return FALSE;
         break;
      case PROP_SHOW_MASK:
         if (!xcf_read_int32 (gio, &layer->show_mask, 1)) return FALSE;
         break;
      case PROP_OFFSETS:
         if (!xcf_read_int32 (gio, &layer->offset_x, 1)) return FALSE;
         if (!xcf_read_int32 (gio, &layer->offset_y, 1)) return FALSE;
         break;
      case PROP_MODE:
         if (!xcf_read_int32 (gio, &layer->mode, 1)) return FALSE;
         break;
      default:
         gimv_io_seek (gio, prop_size, SEEK_CUR);
         break;
      }
   }
}


gboolean
xcf_load_channel_properties (GimvImageLoader *loader, XcfChannel *channel)
{
   GimvIO *gio;
   PropType prop_type;
   guint32 prop_size;

   gio = gimv_image_loader_get_gio (loader);
   g_return_val_if_fail (gio, FALSE);

   while (TRUE) {
      if (!xcf_read_int32 (gio, &prop_type, 1)
          || !xcf_read_int32 (gio, &prop_size, 1))
         return FALSE;

      switch (prop_type) {
      case PROP_END:
         return TRUE;
      case PROP_ACTIVE_CHANNEL:
      case PROP_SELECTION:
         break;
      case PROP_OPACITY:
         if (!xcf_read_int32 (gio, &channel->opacity, 1)) return FALSE;
         break;
      case PROP_VISIBLE:
         if (!xcf_read_int32 (gio, &channel->visible, 1)) return FALSE;
         break;
      case PROP_SHOW_MASKED:
         if (!xcf_read_int32 (gio, &channel->show_mask, 1)) return FALSE;
         break;
      case PROP_COLOR:
         if (!xcf_read_int8 (gio, channel->color, 3)) return FALSE;
         break;
      default:
         gimv_io_seek (gio, prop_size, SEEK_CUR);
         break;
      }
   }
}


void
xcf_put_pixel_element (XcfImage *image, guchar *line, gint x, gint off, gint ic)
{
   gint i, t = x<<2;
   guchar c = (guchar) ic;
	
   switch (off) {
   case -1: /* gray */
      line[t++] = c;
      line[t++] = c;
      line[t]   = c;
      break;
   case -2: /* indexed */
      line[t++] = image->cmap[ic * 3];
      line[t++] = image->cmap[ic * 3 + 1];
      line[t]   = image->cmap[ic * 3 + 2];
      break;
   case -3: /* do nothing */
      break;
   case 4: /* mask */
      for (i = 0; i < 4; i++, t++)
         line[t + 1] = line[t] * c / 255;
      break;
   case 5: /* channel */
      line[t++] = image->color[0];
      line[t++] = image->color[1];
      line[t++] = image->color[2];
      line[t]   = 255 - c;
   default:
      line[t + off] = c;
      break;
   }
}


guint
xcf_read_int32 (GimvIO *gio, guint32 *data, gint count)
{
   guint total;

   total = count;
   if (count > 0) {
      xcf_read_int8 (gio, (guint8 *) data, count * 4);

      while (count--) {
         *data = ntohl (*data);
         data++;
      }
   }

   return total * 4;
}


guint
xcf_read_int8 (GimvIO *gio, guint8 *data, gint count)
{
   guint total;
   int bytes;

   total = count;
   while (count > 0) {
      gimv_io_read (gio, data, count, &bytes);
      if (bytes <= 0) /* something bad happened */
         break;
      count -= bytes;
      data += bytes;
   }

   return total;
}


guint
xcf_read_string (GimvIO *gio, gchar *data)
{
   guint32 tmp;
   guint total;

   total = xcf_read_int32 (gio, &tmp, 1);
   if (data == NULL) {
      gimv_io_seek (gio, tmp, SEEK_CUR);
      total += tmp;
   } else {
      if (tmp > 0) {
         total += xcf_read_int8 (gio, (guint8 *) data, tmp);
      }
      data[tmp] = '\0';
   }

   return total;
}
