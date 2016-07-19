#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>

#include "h264e.h"
#include "h264e_rate_control.h"
#include "rk_vepu_debug.h"
#include "va/va.h"
#include "va/va_enc_h264.h"

static void h264e_init_sps(struct v4l2_plugin_h264_sps *sps) {

	sps->profile_idc = 66;

	/* fixed values limited by hardware */
	sps->constraint_set0_flag = 0;
	sps->constraint_set1_flag = 0;
	sps->constraint_set2_flag = 0;
	sps->constraint_set3_flag = 1;

	sps->level_idc = H264ENC_LEVEL_3_1;
	sps->seq_parameter_set_id = 0;

	/* fixed values limited by hardware */
	sps->chroma_format_idc = 1;
	sps->bit_depth_luma_minus8 = 0;
	sps->bit_depth_chroma_minus8 = 0;
	sps->qpprime_y_zero_transform_bypass_flag = 0;
	sps->log2_max_frame_num_minus4 = 0;
	sps->pic_order_cnt_type = 2;

	sps->max_num_ref_frames = 1;
	sps->gaps_in_frame_num_value_allowed_flag = 0;
	sps->pic_width_in_mbs = 1280 / 16;
	sps->pic_height_in_map_units = 720 / 16;
	sps->frame_mbs_only_flag = 1;

	sps->direct_8x8_inference_flag = 0;
	sps->frame_cropping_flag = 0;
}

static void h264e_init_pps(struct v4l2_plugin_h264_pps *pps)
{
	pps->pic_parameter_set_id = 0;
	pps->seq_parameter_set_id = 0;

	pps->entropy_coding_mode_flag = 0;

	/* fixed value limited by hardware */
	pps->pic_order_present_flag = 0;

	pps->num_ref_idx_l0_default_active_minus1 = 0;
	pps->num_ref_idx_l1_default_active_minus1 = 0;

	/* fixed value limited by hardware */
	pps->weighted_pred_flag = 0;
	pps->weighted_bipred_idc = 0;

	pps->pic_init_qp_minus26 = 0;
	pps->pic_init_qs_minus26 = 0;
	pps->chroma_qp_index_offset = 2;
	pps->deblocking_filter_control_present_flag = 1;
	pps->constrained_intra_pred_flag = 0;

	/* fixed value limited by hardware */
	pps->redundant_pic_cnt_present_flag = 0;

	pps->transform_8x8_mode_flag = 0;
}

static void h264e_init_slice(struct v4l2_plugin_h264_slice_param *slice)
{
	slice->slice_type = ISLICE;
	slice->pic_parameter_set_id = 0;
	slice->frame_num = 0;
	slice->idr_pic_id = -1;
	slice->cabac_init_idc = 0;
	slice->disable_deblocking_filter_idc = 0;
	slice->slice_alpha_c0_offset_div2 = 0;
	slice->slice_beta_offset_div2 = 0;
}

static void h264e_init_rc(struct v4l2_plugin_h264_rate_control *rc,
	struct rk_venc *ictx)
{
	struct rk_h264_encoder *ctx = (struct rk_h264_encoder *)ictx;
	rc->picRc = 1;
	rc->mbRc = 1;
	rc->picSkip = 0;
	/* -8..7, MAD based MB QP adjustment, 0 = disabled */
	rc->mbQpAdjustment = 0;
	rc->outRateNum = 30;
	rc->outRateDenom = 1;
	rc->virtualBuffer.bitRate = 1000000;
	rc->virtualBuffer.realBitCnt = 0;
	rc->virtualBuffer.timeScale = rc->outRateNum;
	rc->virtualBuffer.virtualBitCnt = 0;
	rc->virtualBuffer.bucketFullness = 0;
	rc->virtualBuffer.picTimeInc = 0;
	rc->gopLen = 150;
	rc->qpHdr = 26;
	rc->qpMin = 10;
	rc->qpMax = 51;
	rc->mbPerPic = MB_PER_PIC(ctx);
	rc->intraQpDelta = -3;
	rc->fixedIntraQp = 0;
	rc->mbRows = ctx->sps.pic_height_in_map_units;

	ctx->rate_control.frmrate_num = rc->outRateNum;
	ctx->rate_control.frmrate_denom = rc->outRateDenom;
	ctx->rate_control.bits_rate = 1000000;
	ctx->rate_control.gop_len = rc->gopLen;
}

static int h264e_init(struct rk_venc *ictx,
	struct rk_vepu_init_param *param)
{
	struct rk_h264_encoder *ctx = (struct rk_h264_encoder *)ictx;

	h264e_init_sps(&ctx->sps);
	h264e_init_pps(&ctx->pps);
	h264e_init_slice(&ctx->slice);

	ctx->width = param->width;
	ctx->height = param->height;
	ctx->sliceSizeMbRows = 0;
	ctx->frmInGop = 0;
	
	ctx->sps.pic_width_in_mbs = (ctx->width + 15) / 16;
	ctx->sps.pic_height_in_map_units = (ctx->height + 15) / 16;

	h264e_init_rc(&ctx->rc, ictx);

	if (ctx->sps.level_idc >= H264ENC_LEVEL_3_1)
		ctx->h264Inter4x4Disabled = 1;
	else
		ctx->h264Inter4x4Disabled = 0;

	ctx->pps.pic_init_qp_minus26 = ctx->rc.qpHdr - 26;
	if (ctx->pps.transform_8x8_mode_flag == 2 ||
		((ctx->pps.transform_8x8_mode_flag == 1) &&
		 (MB_PER_PIC(ctx) >= 3600))) {
		ctx->pps.transform_8x8_mode_flag = 1;
	}

	ctx->rk_ctrl_ids[0] = V4L2_CID_PRIVATE_ROCKCHIP_REG_PARAMS;

	return 0;
}

static void h264e_deinit(struct rk_venc *ictx)
{

}

static int h264e_begin_picture(struct rk_venc *ictx)
{
	struct rk_h264_encoder *ctx = (struct rk_h264_encoder *)ictx;
	int i;
	int sliceType = PSLICE;
	int timeInc = 1;
	struct rk3288_h264e_reg_params *params = &ctx->params.rk3288_h264e;

	params->frame_coding_type = FRAME_CODING_TYPE_INTER;

	if (ctx->frmInGop == 0) {
		h264_mad_init(&ctx->mad, MB_PER_PIC(ctx));
		ctx->slice.idr_pic_id++;
		sliceType = ISLICE;
		params->frame_coding_type = FRAME_CODING_TYPE_INTRA;
		timeInc = 0;
	}

	h264e_before_rate_control(&ctx->rate_control);

	params->pic_init_qp = ctx->pps.pic_init_qp_minus26 + 26;
	params->transform8x8_mode = ctx->pps.transform_8x8_mode_flag;
	params->enable_cabac = ctx->pps.entropy_coding_mode_flag;
	params->chroma_qp_index_offset = ctx->pps.chroma_qp_index_offset;
	params->pps_id = ctx->pps.pic_parameter_set_id;

	params->filter_disable = ctx->slice.disable_deblocking_filter_idc;
	params->slice_alpha_offset = ctx->slice.slice_alpha_c0_offset_div2 * 2;
	params->slice_beta_offset = ctx->slice.slice_beta_offset_div2 * 2;
	params->idr_pic_id = ctx->slice.idr_pic_id;
	params->frame_num = ctx->slice.frame_num;
	params->slice_size_mb_rows = ctx->sliceSizeMbRows;
	params->cabac_init_idc = ctx->slice.cabac_init_idc;

	params->qp = ctx->rc.qpMin;//ctx->rate_control.qp;
	params->mad_qp_delta = ctx->rc.mbQpAdjustment;
	params->mad_threshold =ctx->mad.threshold / 256;
	params->qp_min = ctx->rc.qpMin;
	params->qp_max = ctx->rc.qpMax;
	params->cp_distance_mbs = ctx->rc.qpCtrl.checkPointDistance;

	for (i = 0; i < ctx->rc.qpCtrl.checkPoints; i++) {
		params->cp_target[i] = ctx->rate_control.qpCtrl.wordCntTarget[i];//ctx->rc.qpCtrl.wordCntTarget[i];
	}

	for (i = 0; i < 7; i++) {
		//params->targetError[i] = ctx->rc.qpCtrl.wordError[i];
		//params->deltaQp[i] = ctx->rc.qpCtrl.qpChange[i];
		params->target_error[i] = ctx->rate_control.qpCtrl.wordError[i];
		params->delta_qp[i] = ctx->rate_control.qpCtrl.qpChange[i];
	}

	params->h264_inter4x4_disabled = ctx->h264Inter4x4Disabled;

	ctx->rk_payloads[0] = &ctx->params;
	ctx->rk_payload_sizes[0] = sizeof(ctx->params);

	return 0;
}

static int h264e_end_picture(struct rk_venc *ictx,
	 uint32_t outputStreamSize)
{
	struct rk_h264_encoder *ctx = (struct rk_h264_encoder *)ictx;
	static int idx = 0;
	struct v4l2_plugin_h264_feedback *feedback = &ctx->feedback;

	h264e_after_rate_control(&ctx->rate_control, outputStreamSize,
		feedback->rlcCount, feedback->qpSum);
	H264MadThreshold(&ctx->mad, feedback->madCount);

	ctx->frmInGop++;
	if (ctx->frmInGop >= ctx->rc.gopLen)
		ctx->frmInGop = 0;

	ctx->slice.frame_num++;
	ctx->slice.frame_num %=
		(1 << (ctx->sps.log2_max_frame_num_minus4 + 4));
	idx++;

	return 0;
}

static int h264e_update_priv(struct rk_venc *ictx, void *config, uint32_t cfglen)
{
	struct rk_h264_encoder *ctx = (struct rk_h264_encoder *)ictx;

	assert(ctx);
	assert(config);
	assert(cfglen == ROCKCHIP_RET_PARAMS_SIZE);

	memcpy(&ctx->feedback, config, sizeof(ctx->feedback));

	return 0;
}

static void h264e_update_parameter(struct rk_venc *ictx,
	struct rk_vepu_runtime_param *param)
{
	struct rk_h264_encoder *ctx = (struct rk_h264_encoder *)ictx;

	assert(ctx);
	assert(param);

	if (param->bitrate != 0) {
		ctx->rate_control.bits_rate = param->bitrate;
	}

	if (param->framerate_numer != 0 && param->framerate_denom != 0) {
		ctx->rc.outRateNum = param->framerate_numer;
		ctx->rc.outRateDenom = param->framerate_denom;
		ctx->rate_control.frmrate_num = param->framerate_numer;
		ctx->rate_control.frmrate_denom = param->framerate_denom;
	}

	if (param->keyframe_request) {
		VPU_PLG_INF("set key frame request\n");
		ctx->frmInGop = 0;
	}

	if (param->intra_period != 0)
		ctx->rc.gopLen = ctx->rate_control.gop_len = param->intra_period;

	if (param->initial_qp != 0)
		ctx->rc.qpHdr = param->initial_qp;

	if (param->min_qp != 0)
		ctx->rc.qpMin = param->min_qp;

	ctx->rc.picSkip = param->frame_skip;
}

static void h264e_get_payloads(struct rk_venc *ictx, size_t *num, uint32_t **ids,
	void ***payloads, uint32_t **payload_sizes)
{
	struct rk_h264_encoder *ctx = (struct rk_h264_encoder *)ictx;

	*num = H264E_NUM_CTRLS;
	*ids = ctx->rk_ctrl_ids;
	*payloads = ctx->rk_payloads;
	*payload_sizes = ctx->rk_payload_sizes;
}

static struct rk_venc_ops h264_enc_ops = {
	.init = h264e_init,
	.before_encode = h264e_begin_picture,
	.after_encode = h264e_end_picture,
	.deinit = h264e_deinit,
	.updatepriv = h264e_update_priv,
	.updateparameter = h264e_update_parameter,
	.get_payloads = h264e_get_payloads,
};

struct rk_venc* rk_h264_encoder_alloc_ctx(void)
{
	struct rk_venc* enc =
		(struct rk_venc*)calloc(1, sizeof(struct rk_h264_encoder));

	if (enc == NULL) {
		return NULL;
	}

	enc->ops = &h264_enc_ops;

	return enc;
}


