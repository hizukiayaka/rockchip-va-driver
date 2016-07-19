#ifndef _V4L2_PLUGIN_RK_H264E_H_
#define _V4L2_PLUGIN_RK_H264E_H_

#include <stdbool.h>
#include <stdint.h>

#include "rk_vepu_interface.h"
#include "h264e_common.h"
#include "h264e_rate_control.h"
#include "h264e_mad.h"
#include "rk_venc.h"

#define H264E_NUM_CTRLS	1
struct rk_h264_encoder {
	struct rk_venc *parent;

	struct v4l2_plugin_h264_sps sps;
	struct v4l2_plugin_h264_pps pps;
	struct v4l2_plugin_h264_slice_param slice;
	struct v4l2_plugin_h264_rate_control rc;
	struct v4l2_plugin_h264_mad_table mad;
	struct rockchip_reg_params params;
	struct v4l2_plugin_h264_feedback feedback;

	struct h264_rate_control rate_control;

	int width;
	int height;

	int sliceSizeMbRows;
	int h264Inter4x4Disabled;

	int frmInGop;

	uint32_t rk_ctrl_ids[H264E_NUM_CTRLS];
	void *rk_payloads[H264E_NUM_CTRLS];
	uint32_t rk_payload_sizes[H264E_NUM_CTRLS];
};

struct rk_venc* rk_h264_encoder_alloc_ctx(void);
void rk_h264_encoder_free_ctx(struct rk_h264_encoder *enc);

#endif
