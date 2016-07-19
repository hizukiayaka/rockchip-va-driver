#ifndef _V4L2_PLUGIN_RK_VENC_H_
#define _V4L2_PLUGIN_RK_VENC_H_

#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>

#include "rk_vepu_interface.h"

#define COMMENT(x)
#define ABS(x)          ((x) < (0) ? -(x) : (x))
#define MAX(a, b)       ((a) > (b) ?  (a) : (b))
#define MIN(a, b)       ((a) < (b) ?  (a) : (b))
#define SIGN(a)         ((a) < (0) ? (-1) : (1))
#define CLIP3(v, min, max)  ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))
#define MB_COUNT(x)	(((x) + 15) >> 4)

#define ALIGN(x, a)      (((x) + (a) - 1) & ~((a) - 1))

#define container_of(ptr, type, member) ({      \
		const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
		(type *)( (char *)__mptr - offsetof(type,member) );})

/*   copy from kernel driver    */

#define ROCKCHIP_RET_PARAMS_SIZE     488

/**
 * struct rk3288_vp8e_reg_params - low level encoding parameters
 * TODO: Create abstract structures for more generic controls or just
 *       remove unused fields.
 */
struct rk3288_vp8e_reg_params {
	uint32_t unused_00[5];
	uint32_t hdr_len;
	uint32_t unused_18[8];
	uint32_t enc_ctrl;
	uint32_t unused_3c;
	uint32_t enc_ctrl0;
	uint32_t enc_ctrl1;
	uint32_t enc_ctrl2;
	uint32_t enc_ctrl3;
	uint32_t enc_ctrl5;
	uint32_t enc_ctrl4;
	uint32_t str_hdr_rem_msb;
	uint32_t str_hdr_rem_lsb;
	uint32_t unused_60;
	uint32_t mad_ctrl;
	uint32_t unused_68;
	uint32_t qp_val[8];
	uint32_t bool_enc;
	uint32_t vp8_ctrl0;
	uint32_t rlc_ctrl;
	uint32_t mb_ctrl;
	uint32_t unused_9c[14];
	uint32_t rgb_yuv_coeff[2];
	uint32_t rgb_mask_msb;
	uint32_t intra_area_ctrl;
	uint32_t cir_intra_ctrl;
	uint32_t unused_e8[2];
	uint32_t first_roi_area;
	uint32_t second_roi_area;
	uint32_t mvc_ctrl;
	uint32_t unused_fc;
	uint32_t intra_penalty[7];
	uint32_t unused_11c;
	uint32_t seg_qp[24];
	uint32_t dmv_4p_1p_penalty[32];
	uint32_t dmv_qpel_penalty[32];
	uint32_t vp8_ctrl1;
	uint32_t bit_cost_golden;
	uint32_t loop_flt_delta[2];
};

/**
 * struct rk3288_h264e_reg_params - low level encoding parameters
 * TODO: Create abstract structures for more generic controls or just
 *       remove unused fields.
 */
struct rk3288_h264e_reg_params {
	uint32_t frame_coding_type;
	int32_t pic_init_qp;
	int32_t slice_alpha_offset;
	int32_t slice_beta_offset;
	int32_t chroma_qp_index_offset;
	int32_t filter_disable;
	uint16_t idr_pic_id;
	int32_t pps_id;
	int32_t frame_num;
	int32_t slice_size_mb_rows;
	int32_t h264_inter4x4_disabled;
	int32_t enable_cabac;
	int32_t transform8x8_mode;
	int32_t cabac_init_idc;

	/* rate control relevant */
	int32_t qp;
	int32_t mad_qp_delta;
	int32_t mad_threshold;
	int32_t qp_min;
	int32_t qp_max;
	int32_t cp_distance_mbs;
	int32_t cp_target[10];
	int32_t target_error[7];
	int32_t delta_qp[7];
};

/**
 * struct rockchip_reg_params - low level encoding parameters
 */
struct rockchip_reg_params {
	/* Mode-specific data. */
	union {
		struct rk3288_h264e_reg_params rk3288_h264e;
		struct rk3288_vp8e_reg_params rk3288_vp8e;
	};
};

struct rk_venc {
	struct rk_venc_ops *ops;
};


struct rk_venc_ops {
	int (*init)(struct rk_venc *enc, struct rk_vepu_init_param *enc_parms);
	int (*before_encode)(struct rk_venc *enc);
	int (*after_encode)(struct rk_venc *enc, uint32_t outputStreamSize);
	void (*deinit)(struct rk_venc *enc);
	int (*updatepriv)(struct rk_venc *enc, void *config, uint32_t cfglen);
	void (*updateparameter)(struct rk_venc *enc, struct rk_vepu_runtime_param *param);
	void (*get_payloads)(struct rk_venc *enc, size_t *num, uint32_t **ids,
			     void ***payloads, uint32_t **payload_sizes);
	void (*setparameter)(struct rk_venc *enc, enum parm_id id, void *parm);
};

static inline void rk_venc_free_ctx(struct rk_venc *enc)
{
	free(enc);
}

#endif
