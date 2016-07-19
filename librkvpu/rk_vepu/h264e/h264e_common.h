#ifndef _V4L2_PLUGIN_RK_H264E_COMMON_H_
#define _V4L2_PLUGIN_RK_H264E_COMMON_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;

typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;

#define MB_PER_PIC(ctx)		\
	(ctx->sps.pic_width_in_mbs * ctx->sps.pic_height_in_map_units)

#define ABS(x)          ((x) < (0) ? -(x) : (x))
#define MAX(a, b)       ((a) > (b) ?  (a) : (b))
#define MIN(a, b)       ((a) < (b) ?  (a) : (b))
#define SIGN(a)         ((a) < (0) ? (-1) : (1))
#define DIV(a, b)       ((b) ? ((a) + (SIGN(a) * (b)) / 2) / (b) : (a))

/* frame coding type defined by hardware */
#define FRAME_CODING_TYPE_INTER		0
#define FRAME_CODING_TYPE_INTRA		1

enum H264ENC_LEVEL {
	H264ENC_LEVEL_1 = 10,
	H264ENC_LEVEL_1_b = 99,
	H264ENC_LEVEL_1_1 = 11,
	H264ENC_LEVEL_1_2 = 12,
	H264ENC_LEVEL_1_3 = 13,
	H264ENC_LEVEL_2 = 20,
	H264ENC_LEVEL_2_1 = 21,
	H264ENC_LEVEL_2_2 = 22,
	H264ENC_LEVEL_3 = 30,
	H264ENC_LEVEL_3_1 = 31,
	H264ENC_LEVEL_3_2 = 32,
	H264ENC_LEVEL_4_0 = 40,
	H264ENC_LEVEL_4_1 = 41
};

enum sliceType_e
{
	PSLICE = 0,
	ISLICE = 2,
	PSLICES = 5,
	ISLICES = 7
};

struct v4l2_plugin_h264_feedback {
	int32_t qpSum;
	int32_t cp[10];
	int32_t madCount;
	int32_t rlcCount;
};

struct v4l2_plugin_h264_sps {
	u8 profile_idc;
	u8 constraint_set0_flag :1;
	u8 constraint_set1_flag :1;
	u8 constraint_set2_flag :1;
	u8 constraint_set3_flag :1;
	/*u8 constraint_set4_flag :1;
	u8 constraint_set5_flag :1;*/
	u8 level_idc;
	u8 seq_parameter_set_id;
	u8 chroma_format_idc;
	/*u8 separate_colour_plane_flag :1;*/
	u8 bit_depth_luma_minus8;
	u8 bit_depth_chroma_minus8;
	u8 qpprime_y_zero_transform_bypass_flag :1;
	u16 log2_max_frame_num_minus4;
	u8 pic_order_cnt_type;
	/*u16 max_pic_order_cnt;
	u8 delta_pic_order_always_zero_flag :1;*/
	/*s32 offset_for_non_ref_pic;
	s32 offset_for_top_to_bottom_field;
	u8 num_ref_frames_in_pic_order_cnt_cycle;
	s32 offset_for_ref_frame[255];*/
	u8 max_num_ref_frames;
	u8 gaps_in_frame_num_value_allowed_flag :1;
	u16 pic_width_in_mbs;
	u16 pic_height_in_map_units;
	u8 frame_mbs_only_flag :1;
	/*u8 mb_adaptive_frame_field_flag :1;*/
	u8 direct_8x8_inference_flag :1;
	u8 frame_cropping_flag :1;
	u32 frame_crop_left_offset;
	u32 frame_crop_right_offset;
	u32 frame_crop_top_offset;
	u32 frame_crop_bottom_offset;
	u8 vui_parameters_present_flag :1;
};

struct v4l2_plugin_h264_pps {
	u8 pic_parameter_set_id;
	u8 seq_parameter_set_id;
	u8 entropy_coding_mode_flag :1;
	u8 pic_order_present_flag :1;
	u8 num_slice_groups_minus_1;
	u8 num_ref_idx_l0_default_active_minus1;
	u8 num_ref_idx_l1_default_active_minus1;
	u8 weighted_pred_flag :1;
	u8 weighted_bipred_idc;
	s8 pic_init_qp_minus26;
	s8 pic_init_qs_minus26;
	s8 chroma_qp_index_offset;
	u8 deblocking_filter_control_present_flag :1;
	u8 constrained_intra_pred_flag :1;
	u8 redundant_pic_cnt_present_flag :1;
	u8 transform_8x8_mode_flag :1;
};

struct v4l2_plugin_h264_slice_param {
	/*u16 first_mb_in_slice;*/
	u8 slice_type;
	u8 pic_parameter_set_id;
	/*u8 colour_plane_id;*/
	u16 frame_num;
	u16 idr_pic_id;
	/*u16 pic_order_cnt_lsb;
	s32 delta_pic_order_cnt_bottom;
	s32 delta_pic_order_cnt0;
	s32 delta_pic_order_cnt1;
	u8 redundant_pic_cnt;
	struct v4l2_h264_pred_weight_table pred_weight_table;*/
	u8 cabac_init_idc; 
	/*s8 slice_qp_delta;
	u8 sp_for_switch_flag :1;
	s8 slice_qs_delta;*/
	u8 disable_deblocking_filter_idc;
	s8 slice_alpha_c0_offset_div2;
	s8 slice_beta_offset_div2;
	/*u32 slice_group_change_cycle;
	u8 num_ref_idx_l0_active_minus1;
	u8 num_ref_idx_l1_active_minus1;*/
	/* Entries on each list are indices
	* into v4l2_h264_decode_param.dpb[]. */
	/*u8 ref_pic_list0[32];
	u8 ref_pic_list1[32];
	u8 flags;*/
};

#endif
