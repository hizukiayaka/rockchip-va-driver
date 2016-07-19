#include <memory.h>
#include <assert.h>

#include "h264e.h"
#include "h264e_common.h"
#include "h264e_rate_control.h"
#include "rk_vepu_debug.h"

#define INITIAL_BUFFER_FULLNESS   60    /* Decoder Buffer in procents */
#define MIN_PIC_SIZE              50    /* Procents from picPerPic */

#define DSCY                      32 /* n * 32 */
#define I32_MAX           2147483647 /* 2 ^ 31 - 1 */
#define QP_DELTA          2
#define QP_DELTA_LIMIT    10
#define INTRA_QP_DELTA    (0)
#define WORD_CNT_MAX      65535

/*------------------------------------------------------------------------------
  Local structures
------------------------------------------------------------------------------*/
/* q_step values scaled up by 4 and evenly rounded */
int32_t q_step[52] = {
	3, 3, 3, 4, 4, 5, 5, 6, 7, 7,
	8, 9, 10, 11, 13, 14, 16, 18, 20, 23,
	25, 28, 32, 36, 40, 45, 51, 57, 64, 72,
	80, 90, 101, 114, 128, 144, 160, 180, 203, 228,
	256, 288, 320, 360, 405, 456, 513, 577, 640, 720,
	810, 896
};

/*
 * according to linear formula 'R * Q * Q = b * Q + a'
 * now give the target R, calculate a Qp value using
 * approximation
 */
static int32_t calculate_qp_using_linear_model(struct linear_model *model, int64_t r)
{
	int32_t qp = model->qp_last;
	int64_t estimate_r = 0;
	int64_t diff = 0;
	int64_t diff_min = I32_MAX;
	int64_t qp_best = qp;
	int32_t tmp;
	int64_t qs;

	if (model->b == 0 && model->a == 0) {
		return qp_best;
	}

	do {
		qs = q_step[qp];
		estimate_r = DIV(model->b, qs) + DIV(model->a, qs * qs);
		diff = estimate_r - r;

		VPU_PLG_INF("qp %d, diff %lld\n", qp, diff);

		if (ABS(diff) < diff_min) {
			diff_min = ABS(diff);
			qp_best = qp;
			if (diff > 0) {
				qp++;
			} else {
				qp--;
			}
		} else {
			break;
		}
	} while (qp <= 51 && qp >=0);

	model->qp_last = qp_best;

	tmp = qp_best - model->qp_last;
	if (tmp > QP_DELTA) {
		qp_best = model->qp_last + QP_DELTA;
		/* When QP is totally wrong, allow faster QP increase */
		if (tmp > QP_DELTA_LIMIT)
			qp_best = model->qp_last + QP_DELTA * 2;
	} else if (tmp < -QP_DELTA) {
		qp_best = model->qp_last - QP_DELTA;
	}

	return qp_best;
}

static void store_linear_x_y(struct linear_model *model, int32_t r, int32_t qp)
{
	int64_t qs = q_step[qp];
	int i;

	for (i = 0; i < model->n; i ++) {
		if (model->qp[i] == qs) {
			model->r[i] += r;
			model->r[i] /= 2;
			return;
		}
	}

	model->qp_last = qp;

	model->qp[model->i] = qs;
	model->r[model->i] = r;
	model->y[model->i] = r * qs * qs;

	model->n++;
	model->n = MIN(model->n, LINEAR_MODEL_STATISTIC_COUNT);

	model->i++;
	model->i %= LINEAR_MODEL_STATISTIC_COUNT;
}

/*
 * This function want to calculate coefficient 'b' 'a' using ordinary
 * least square.
 * y = b * x + a
 * b_n = accumulate(x * y) - n * (average(x) * average(y))
 * a_n = accumulate(x * x) * accumulate(y) - accumulate(x) * accumulate(x * y)
 * denom = accumulate(x * x) - n * (square(average(x))
 * b = b_n / denom
 * a = a_n / denom
 */
static void calculate_linear_coefficient(struct linear_model *model)
{
	int i = 0;
	int n;
	int64_t acc_xy = 0;
	int64_t acc_x = 0;
	int64_t acc_y = 0;
	int64_t acc_sq_x = 0;

	int64_t b_num = 0;
	int64_t a_num = 0;
	int64_t denom = 0;

	int64_t *x = model->qp;
	int64_t *y = model->y;

	n = model->n;
	i = n;

	while (i--) {
		acc_xy += x[i] * y[i];
		acc_x += x[i];
		acc_y += y[i];
		acc_sq_x += x[i] * x[i];
	}

	b_num = n * acc_xy - acc_x * acc_y;
	denom = n * acc_sq_x - acc_x * acc_x;

	model->b = DIV(b_num, denom);

	a_num = acc_sq_x * acc_y - acc_x * acc_xy;

	model->a = DIV(a_num, denom);

	VPU_PLG_INF("a %lld, b %lld\n", model->a, model->b);
}

static void calculate_mb_model_using_linear_model(struct h264_rate_control *rc, int32_t non_zero_target)
{
	struct rk_h264_encoder *enc = container_of(rc, struct rk_h264_encoder, rate_control);
	const int32_t sscale = 256;
	h264QpCtrl_s *qc = &rc->qpCtrl;
	int32_t scaler;
	int32_t i;
	int32_t tmp;
	int32_t mb_per_pic = MB_COUNT(enc->width) * MB_COUNT(enc->height);
	int32_t chk_ptr_cnt = MIN(MB_COUNT(enc->height), CHECK_POINTS_MAX);
	int32_t chk_ptr_distance = mb_per_pic / (chk_ptr_cnt + 1);
	int32_t bits_per_pic = rc->bits_rate * rc->frmrate_denom / rc->frmrate_num;

	assert(non_zero_target < (0x7FFFFFFF / sscale));

	if(non_zero_target > 0) {
		/* scaler is non-zero coefficent count per macro-block plus 256 */
		scaler = DIV(non_zero_target * sscale, (int32_t)mb_per_pic);
	} else {
		return;
	}

	for(i = 0; i < chk_ptr_cnt; i++) {
		/* tmp is non-zero coefficient count target for i-th check point */
		tmp = (scaler * (chk_ptr_distance * (i + 1) + 1)) / sscale;
		tmp = MIN(WORD_CNT_MAX, tmp / 32 + 1);
		if (tmp < 0) tmp = WORD_CNT_MAX;    /* Detect overflow */
		qc->wordCntTarget[i] = tmp; /* div32 for regs */
	}

	/* calculate nz count for avg. bits per frame */
	/* tmp is target non-zero coefficient count for average size pic  */
	tmp = DIV(bits_per_pic * 256, rc->bits_per_non_zero_coef);

	/* ladder 'non-zero coefficent count target' - 'non-zero coefficient actual' of check point */
	qc->wordError[0] = -tmp * 3;
	qc->qpChange[0] = -3;
	qc->wordError[1] = -tmp * 2;
	qc->qpChange[1] = -2;
	qc->wordError[2] = -tmp * 1;
	qc->qpChange[2] = -1;
	qc->wordError[3] = tmp * 1;
	qc->qpChange[3] = 0;
	qc->wordError[4] = tmp * 2;
	qc->qpChange[4] = 1;
	qc->wordError[5] = tmp * 3;
	qc->qpChange[5] = 2;
	qc->wordError[6] = tmp * 4;
	qc->qpChange[6] = 3;

	for(i = 0; i < CTRL_LEVELS; i++) {
		tmp = qc->wordError[i];
		tmp = CLIP3(tmp/4, -32768, 32767);
		qc->wordError[i] = tmp;
	}
}

static void calculate_mb_model_using_adaptive_model(struct h264_rate_control *rc, int32_t non_zero_target)
{
	struct rk_h264_encoder *enc = container_of(rc, struct rk_h264_encoder, rate_control);
	const int32_t sscale = 256;
	h264QpCtrl_s *qc = &rc->qpCtrl;
	int32_t i;
	int32_t tmp;
	int32_t scaler;
	int32_t mb_per_pic = MB_COUNT(enc->width) * MB_COUNT(enc->height);
	int32_t chk_ptr_cnt = MIN(MB_COUNT(enc->height), CHECK_POINTS_MAX);
	int32_t chk_ptr_distance = mb_per_pic / (chk_ptr_cnt + 1);
	int32_t bits_per_pic = rc->bits_rate * rc->frmrate_denom / rc->frmrate_num;

	assert(non_zero_target < (0x7FFFFFFF / sscale));

	if((non_zero_target > 0) && (rc->non_zero_cnt > 0)) {
		scaler = DIV(non_zero_target * sscale, rc->non_zero_cnt);
	} else {
		return;
	}

	for(i = 0; i < chk_ptr_cnt; i++) {
		tmp = (int32_t) (qc->wordCntPrev[i] * scaler) / sscale;
		tmp = MIN(WORD_CNT_MAX, tmp / 32 + 1);
		if (tmp < 0) tmp = WORD_CNT_MAX;    /* Detect overflow */
		qc->wordCntTarget[i] = tmp; /* div32 for regs */
	}

	/* Qp change table */

	/* calculate nz count for avg. bits per frame */
	tmp = DIV(bits_per_pic * 256, (rc->bits_per_non_zero_coef * 3));

	qc->wordError[0] = -tmp * 3;
	qc->qpChange[0] = -3;
	qc->wordError[1] = -tmp * 2;
	qc->qpChange[1] = -2;
	qc->wordError[2] = -tmp * 1;
	qc->qpChange[2] = -1;
	qc->wordError[3] = tmp * 1;
	qc->qpChange[3] = 0;
	qc->wordError[4] = tmp * 2;
	qc->qpChange[4] = 1;
	qc->wordError[5] = tmp * 3;
	qc->qpChange[5] = 2;
	qc->wordError[6] = tmp * 4;
	qc->qpChange[6] = 3;

	for(i = 0; i < CTRL_LEVELS; i++) {
		tmp = qc->wordError[i];
		tmp = CLIP3(tmp/4, -32768, 32767);
		qc->wordError[i] = tmp;
	}
}

static void calculate_mb_model(struct h264_rate_control *rc, int32_t target_bits)
{
	struct rk_h264_encoder *enc = container_of(rc, struct rk_h264_encoder, rate_control);
	int32_t nonZeroTarget;
	int32_t mb_per_pic = MB_COUNT(enc->width) * MB_COUNT(enc->height);
	int32_t coeff_cnt_max = mb_per_pic * 24 * 16;

	VPU_PLG_INF("in\n");

	/* Disable Mb Rc for Intra Slices, because coeffTarget will be wrong */
	if(enc->frmInGop == 0 || rc->bits_per_non_zero_coef == 0) {
		return;
	}

	/* Required zero cnt */
	nonZeroTarget = DIV(target_bits * 256, rc->bits_per_non_zero_coef);
	nonZeroTarget = MIN(coeff_cnt_max, MAX(0, nonZeroTarget));

	nonZeroTarget = MIN(0x7FFFFFFFU / 1024U, (uint32_t)nonZeroTarget);

	/* Use linear model when previous frame can't be used for prediction */
	if (enc->frmInGop != 0 || (rc->non_zero_cnt == 0)) {
		calculate_mb_model_using_linear_model(rc, nonZeroTarget);
	} else {
		calculate_mb_model_using_adaptive_model(rc, nonZeroTarget);
	}
}

static int32_t caluate_qp_by_bits_est(int32_t bits, int32_t pels)
{
    const int32_t qp_tbl[2][9] = {
                    {27, 44, 72, 119, 192, 314, 453, 653, 0x7FFFFFFF},
                    {49, 45, 41, 37, 33, 29, 25, 21, 17}};
    const int32_t upscale = 8000;
    int32_t i = -1;

    /* prevents overflow, QP would anyway be 17 with this high bitrate
       for all resolutions under and including 1920x1088 */
    if (bits > 1000000)
        return 17;

    /* Make room for multiplication */
    pels >>= 8;
    bits >>= 5;

    /* Adjust the bits value for the current resolution */
    bits *= pels + 250;
    assert(pels > 0);
    assert(bits > 0);
    bits /= 350 + (3 * pels) / 4;
    bits = DIV(bits * upscale, pels << 6);

    while (qp_tbl[0][++i] < bits);

    return qp_tbl[1][i];
}

void h264e_before_rate_control(struct h264_rate_control *rc)
{
	struct rk_h264_encoder *enc = container_of(rc, struct rk_h264_encoder, rate_control);
	int32_t bits_per_pic = rc->bits_rate * rc->frmrate_denom / rc->frmrate_num;
	uint32_t expected_bits = enc->frmInGop * bits_per_pic;//rc->prev_bits + bits_per_pic;

	int32_t bits_diff = expected_bits - rc->actual_bits;
	int32_t i;

	rc->prev_bits = rc->actual_bits;

	VPU_PLG_INF("bits diff %d, expected %u, actual %u\n", bits_diff, expected_bits, rc->actual_bits);

	for(i = 0; i < CHECK_POINTS_MAX; i++) {
		rc->qpCtrl.wordCntTarget[i] = 0;
	}

	if (enc->frmInGop == 0) {
		if (rc->acc_interframe_cnt != 0) {
			rc->qp = DIV(rc->acc_interframe_qp, rc->acc_interframe_cnt);
		} else {
			rc->qp = caluate_qp_by_bits_est(bits_per_pic, enc->width * enc->height);
		}

		rc->actual_bits = 0;
		rc->acc_interframe_cnt = 0;
		rc->acc_interframe_qp = 0;
	} else if (enc->frmInGop == 1 || enc->frmInGop == 2) {
		rc->qp = bits_diff > 0 ? rc->qp_prev - 3 : rc->qp_prev + 3;
	} else {
		int32_t req = bits_per_pic + bits_diff / (int32_t)MIN(3, rc->gop_len - enc->frmInGop);

		VPU_PLG_INF("req %d\n", req);

		if (req < 0) {
			rc->qp = rc->qp_prev + 5;
		} else {
			rc->qp = calculate_qp_using_linear_model(&rc->lin_mod, DIV(req * 256, bits_per_pic));
		}

		if (rc->mb_rc_en) {
			calculate_mb_model(rc, req);
		}
	}

	rc->qp = CLIP3(rc->qp, 0, 51);
	VPU_PLG_INF("get qp %u\n", rc->qp);
}

void h264e_after_rate_control(struct h264_rate_control *rc, uint32_t coded_bytes,
	uint32_t non_zero_cnt, uint32_t qp_sum)
{
	struct rk_h264_encoder *enc = container_of(rc, struct rk_h264_encoder, rate_control);
	uint32_t coded_bits = coded_bytes * 8;
	int32_t bits_per_pic = rc->bits_rate * rc->frmrate_denom / rc->frmrate_num;

	VPU_PLG_INF("get coded bits: %u\n", coded_bits);

	rc->actual_bits += coded_bits;
	rc->qp_prev = rc->qp;

	if (enc->frmInGop != 0) {
		store_linear_x_y(&rc->lin_mod, DIV(coded_bits * 256, bits_per_pic), rc->qp);
		calculate_linear_coefficient(&rc->lin_mod);

		rc->acc_interframe_qp += rc->qp;
		rc->acc_interframe_cnt++;

		rc->bits_per_non_zero_coef = DIV(coded_bits * 256, non_zero_cnt);
		rc->non_zero_cnt = non_zero_cnt;
	}
}
