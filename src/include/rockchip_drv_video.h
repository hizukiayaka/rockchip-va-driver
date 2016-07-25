/*
 * Copyright (c) 2016 Rockchip Electronics Co., Ltd.
 *
 * Based on libva's dummy_drv_video.
 *
 * Copyright (c) 2007 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _ROCKCHIP_DRV_VIDEO_H_
#define _ROCKCHIP_DRV_VIDEO_H_

#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <va/va.h>
#include <va/va_backend.h>
#include <va/va_enc_h264.h>

#include "config.h"
#include "object_heap.h"
#include "rockchip_image.h"
#include "rockchip_surface.h"
#include "rockchip_picture.h"
#include "rockchip_encoder.h"
#include "v4l2_utils.h"

#define ASSERT              assert
#define EXPORT              __attribute__ ((visibility("default")))
#define ARRAY_SIZE(x)       (sizeof(x)/sizeof(x[0]))
#define TIME_TO_MS(tv)      (tv.tv_sec * 1000 + tv.tv_usec / 1000)
#define DURATION(tv1, tv2)  (TIME_TO_MS(tv2) - TIME_TO_MS(tv1))

#define INIT_DRIVER_DATA    struct rockchip_driver_data * const driver_data = (struct rockchip_driver_data *) ctx->pDriverData;

#define CONFIG(id)  ((object_config_p) object_heap_lookup( &driver_data->config_heap, id ))
#define CONTEXT(id) ((object_context_p) object_heap_lookup( &driver_data->context_heap, id ))
#define SURFACE(id) ((object_surface_p) object_heap_lookup( &driver_data->surface_heap, id ))
#define IMAGE(id)   ((object_image_p) object_heap_lookup( &driver_data->image_heap, id ))
#define BUFFER(id)  ((object_buffer_p) object_heap_lookup( &driver_data->buffer_heap, id ))

#define CONFIG_ID_OFFSET        0x01000000
#define CONTEXT_ID_OFFSET       0x02000000
#define SURFACE_ID_OFFSET       0x04000000
#define IMAGE_ID_OFFSET         0x08000000
#define BUFFER_ID_OFFSET        0x10000000

#define ROCKCHIP_MAX_PROFILES               11
#define ROCKCHIP_MAX_ENTRYPOINTS            5
#define ROCKCHIP_MAX_CONFIG_ATTRIBUTES      10
#define ROCKCHIP_MAX_IMAGE_FORMATS          10
#define ROCKCHIP_MAX_SUBPIC_FORMATS         4
#define ROCKCHIP_MAX_DISPLAY_ATTRIBUTES     4
#define ROCKCHIP_STR_VENDOR                 "Rockchip Driver 1.0"

struct rockchip_driver_data {
    struct object_heap  config_heap;
    struct object_heap  context_heap;
    struct object_heap  surface_heap;
    struct object_heap  image_heap;
    struct object_heap  buffer_heap;
};

typedef struct object_config {
    struct object_base  base;
    VAProfile           profile;
    VAEntrypoint        entrypoint;
    VAConfigAttrib      attrib_list[ROCKCHIP_MAX_CONFIG_ATTRIBUTES];
    int                 attrib_count;
} object_config_t, *object_config_p;

typedef struct {
    int             stream_bytes;
    int             frames;
    struct timeval  tm;

    int             fps;
    int             bitrate;
    int             intra_ratio;
} encode_statistics_t, *encode_statistics_p;

typedef struct object_context {
    struct object_base  base;
    VAContextID         context_id;
    VAConfigID          config_id;
    VASurfaceID         current_render_target;
    int                 picture_width;
    int                 picture_height;
    int                 num_render_targets;
    int                 flags;
    VASurfaceID        *render_targets;
    int                 streaming;

    enc_context_p       enc_ctx;
    encode_statistics_t statistics;

    union {
        encode_params_h264_t h264_params;
    };

    struct v4l2_ext_control ctrl[5];

} object_context_t, *object_context_p;

#endif /* _ROCKCHIP_DRV_VIDEO_H_ */
