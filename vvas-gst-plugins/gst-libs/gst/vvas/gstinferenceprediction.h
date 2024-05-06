/*
 * GStreamer
 * Copyright (C) 2018-2020 RidgeRun <support@ridgerun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/*
 * This is the modified version of RidgeRun code
 * (https://github.com/RidgeRun/gst-inference) to support Xilinx VVAS product
 * specific use cases
 */

#ifndef __GST_INFERENCE_PREDICTION__
#define __GST_INFERENCE_PREDICTION__

#include <vvas_core/vvas_infer_prediction.h>
#include <gst/vvas/gstinferenceclassification.h>
#include <gst/vvas/gstvvasinpinfer.h>
#include <gst/video/video.h>
#include <vvas/vvasmeta.h>
#include <math.h>
#include <stdbool.h>

G_BEGIN_DECLS
typedef struct _BoundingBox BoundingBox;
typedef struct _GstInferencePrediction GstInferencePrediction;

/**
 * @struct _BoundingBox
 * @brief Size and coordinates of a prediction region
 */
struct _BoundingBox
{
  /** horizontal coordinate of the upper left position of the bounding box in pixels */
  gint x;
  /** vertical coordinate of the upper left position of the bounding box in pixels */
  gint y;
  /** width of the bounding box in pixels */
  guint width;
  /** height of the bounding box in pixels */
  guint height;
  /** Color of the box */
  VvasColorMetadata box_color;
};

/**
 * @struct _GstInferencePrediction
 * @brief Abstraction that represents a prediction
 */
struct _GstInferencePrediction
{
  /** private base object */
  GstMiniObject base;
  /** mutex */
  GMutex mutex;

  /** buffer contains cropped images for cascade usecase */
  GstBuffer *sub_buffer;
  
	/** for future extension */
  void *reserved_1;
  void *reserved_2;
  void *reserved_3;
  void *reserved_4;
  void *reserved_5;
 
  /** Vvas Infer prediction */
  VvasInferPrediction prediction;
};

/**
 * gst_inference_prediction_new:
 *
 * Creates a new GstInferencePrediction. Values can be later assigned
 * manually, however these assignments should be done with the
 * GST_INFERENCE_PREDICTION_LOCK held. See
 * gst_inference_prediction_new_full for a thread safe version.
 *
 * Returns: A newly allocated and initialized GstInferencePrediction.
 */
GstInferencePrediction *gst_inference_prediction_new (void);

/**
 * gst_inference_prediction_new_full:
 * @bbox: The bounding box of this prediction.
 *
 * Creates a new GstInferencePrediction and initializes its internal
 * values.
 *
 * Returns: A newly allocated and initialized GstInferencePrediction.
 */
GstInferencePrediction *gst_inference_prediction_new_full (VvasBoundingBox *
    bbox);

/**
 * gst_inference_prediction_reset:
 * @self: the prediction to reset
 *
 * Clears a prediction, effectively removing al children and resetting
 * all members.
 */
void gst_inference_prediction_reset (GstInferencePrediction * self);

/**
 * gst_inference_prediction_copy:
 * @self: the prediction to copy
 *
 * Copies a prediction into a newly allocated one. This is a deep
 * copy, meaning that all children and classifications are copied as
 * well. No references are shared.
 *
 * Returns: a newly allocated copy of the original prediction
 */
GstInferencePrediction *gst_inference_prediction_copy (const
    GstInferencePrediction * self);

/**
 * gst_inference_prediction_ref:
 * @self: the prediction to ref
 *
 * Increase the reference counter of the prediction.
 *
 * Returns: the same prediction, for convenience purposes.
 */
GstInferencePrediction *gst_inference_prediction_ref (GstInferencePrediction *
    self);

/**
 * gst_inference_prediction_unref:
 * @self: the prediction to unref
 *
 * Decreases the reference counter of the prediction. When the
 * reference counter hits zero, the prediction is freed.
 */
void gst_inference_prediction_unref (GstInferencePrediction * self);

/**
 * gst_inference_prediction_to_string:
 * @self: the prediction to serialize
 *
 * Serializes the prediction along with it's classifications and
 * children into a JSON-like string. Free this string after usage
 * using g_free()
 *
 * Returns: a string representing the prediction.
 */
gchar *gst_inference_prediction_to_string (GstInferencePrediction * self);

/**
 * gst_inference_prediction_append:
 * @self: the parent prediction
 * @child: the prediction to append as a child
 *
 * Append a new prediction as part of the parent prediction
 * children. The parent takes ownership, use
 * gst_inference_prediction_ref() if you wish to keep a reference.
 */
void gst_inference_prediction_append (GstInferencePrediction * self,
    GstInferencePrediction * child);

/**
 * gst_inference_prediction_get_children:
 * @self: the parent prediction
 *
 * Gets a list of the immediate children of the current prediction. In
 * other words, the children of these childrens are not returned. The
 * references of these children are still owned by the parent.
 *
 * Returns: A linked list of the child predictions.
 */
GSList *gst_inference_prediction_get_children (GstInferencePrediction * self);

/**
 * gst_inference_prediction_append_classification:
 * @self: the parent prediction
 * @c: the classification to append to the prediction
 *
 * A new GstInferenceClassification to associate with this
 * prediction. The prediction takes ownership of the classification
 */
void gst_inference_prediction_append_classification (GstInferencePrediction *
    self, GstInferenceClassification * c);

/**
 * gst_inference_prediction_scale:
 * @self: the prediction to scale
 * @to: the resulting image size
 * @from: the original image size
 *
 * Modifies the BoundingBox associated with this prediction (and all
 * its children) to scale to the new image size. This is typically
 * used by the GstMeta subsystem automatically and not for public
 * usage.
 *
 * Returns: a newly allocated and scaled prediction.
 */
GstInferencePrediction *gst_inference_prediction_scale (GstInferencePrediction *
    self, GstVideoInfo * to, GstVideoInfo * from);

/**
 * gst_inference_prediction_scale_ip:
 * @self: the prediction to scale in place
 * @to: the resulting image size
 * @from: the original image size
 *
 * Modifies the BoundingBox associated with this prediction (and all
 * its children) to scale to the new image size. This is typically
 * used by the GstMeta subsystem automatically and not for public
 * usage.
 */
void gst_inference_prediction_scale_ip (GstInferencePrediction * self,
    GstVideoInfo * to, GstVideoInfo * from);

/**
 * gst_inference_prediction_find:
 * @self: the root prediction
 * @id: the prediction_id of the prediction to return
 *
 * Traverses the prediction tree looking for a child with the given
 * id.
 *
 * Returns: a reference to the prediction with id or NULL if not
 * found. Unref after usage.
 */
GstInferencePrediction *gst_inference_prediction_find (GstInferencePrediction *
    self, guint64 id);

/**
 * gst_inference_prediction_get_enabled:
 * @self: the root prediction
 *
 * Traverse the prediction three saving the predictions that are enabled.
 *
 * Returns: a GList of predictions that are enabled.
 */
GList *gst_inference_prediction_get_enabled (GstInferencePrediction * self);

/**
 * gst_inference_prediction_merge:
 * @src: the source prediction
 * @dst: the destination prediction
 *
 * Copies the extra information from src to dst.
 *
 * Returns: TRUE if new sub-predictions were added, FALSE otherwise.
 */
gboolean gst_inference_prediction_merge (GstInferencePrediction * src,
    GstInferencePrediction * dst);

/**
 * GST_INFERENCE_PREDICTION_LOCK:
 * @p: The GstInferencePrediction to lock
 *
 * Locks the prediction to avoid concurrent access from different
 * threads.
 */
#define GST_INFERENCE_PREDICTION_LOCK(p) g_mutex_lock (&((p)->mutex))

/**
 * GST_INFERENCE_PREDICTION_UNLOCK:
 * @p: The GstInferencePrediction to unlock
 *
 * Unlocks the prediction to yield the access to other threads.
 */
#define GST_INFERENCE_PREDICTION_UNLOCK(p) g_mutex_unlock (&((p)->mutex))

G_END_DECLS
#endif // __GST_INFERENCE_PREDICTION__
