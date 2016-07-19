#ifndef H264_RATE_CONTROL_H
#define H264_RATE_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "rk_venc_rate_control.h"

enum
{ H264RC_OVERFLOW = -1 };

#define RC_CBR_HRD  0   /* 1 = Constant bit rate model. Must use filler
                         * data to conform */

#define CTRL_LEVELS          7  /* DO NOT CHANGE THIS */
#define CHECK_POINTS_MAX    10  /* DO NOT CHANGE THIS */
#define RC_TABLE_LENGTH     10  /* DO NOT CHANGE THIS */
#define RK_CLIP3(min, max, v)  ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

#define LINEAR_MODEL_STATISTIC_COUNT    15
/* y = bx + a */
struct linear_model {
    int32_t n;                  /* elements count */
    int32_t i;                  /* elements index for store */

    int64_t b;                  /* coefficient */
    int64_t a;

    int64_t qp[LINEAR_MODEL_STATISTIC_COUNT];   /* x */
    int64_t r[LINEAR_MODEL_STATISTIC_COUNT];
    int64_t y[LINEAR_MODEL_STATISTIC_COUNT];    /* y = qp*qp*r */

    int32_t qp_last;            /* qp value in last calculate */
};

typedef struct
{
    int32_t wordError[CTRL_LEVELS]; /* Check point error bit */
    int32_t qpChange[CTRL_LEVELS];  /* Check point qp difference */
    int32_t wordCntTarget[CHECK_POINTS_MAX];    /* Required bit count */
    int32_t wordCntPrev[CHECK_POINTS_MAX];  /* Real bit count */
    int32_t checkPointDistance;
    int32_t checkPoints;
} h264QpCtrl_s;

/* Virtual buffer */
typedef struct
{
    int32_t bufferSize;          /* size of the virtual buffer */
    int32_t bitRate;             /* input bit rate per second */
    int32_t bitPerPic;           /* average number of bits per picture */
    int32_t picTimeInc;          /* timeInc since last coded picture */
    int32_t timeScale;           /* input frame rate numerator */
    int32_t unitsInTic;          /* input frame rate denominator */
    int32_t virtualBitCnt;       /* virtual (channel) bit count */
    int32_t realBitCnt;          /* real bit count */
    int32_t bufferOccupancy;     /* number of bits in the buffer */
    int32_t skipFrameTarget;     /* how many frames should be skipped in a row */
    int32_t skippedFrames;       /* how many frames have been skipped in a row */
    int32_t nonZeroTarget;
    int32_t bucketFullness;      /* Leaky Bucket fullness */
    /* new rate control */
    int32_t gopRem;
    int32_t windowRem;
} h264VirtualBuffer_s;

struct v4l2_plugin_h264_rate_control
{
    bool picRc;
    bool mbRc;             /* Mb header qp can vary, check point rc */
    bool picSkip;          /* Frame Skip enable */
    bool hrd;              /* HRD restrictions followed or not */
    uint32_t fillerIdx;
    int32_t mbPerPic;            /* Number of macroblock per picture */
    int32_t mbRows;              /* MB rows in picture */
    int32_t coeffCntMax;         /* Number of coeff per picture */
    int32_t nonZeroCnt;
    int32_t srcPrm;              /* Source parameter */
    int32_t qpSum;               /* Qp sum counter */
    uint32_t sliceTypeCur;
    uint32_t sliceTypePrev;
    bool frameCoded;            /* Pic coded information */
    int32_t fixedQp;             /* Pic header qp when fixed */
    int32_t qpHdr;               /* Pic header qp of current voded picture */
    int32_t qp;
    int32_t qpMin;               /* Pic header minimum qp, user set */
    int32_t qpMax;               /* Pic header maximum qp, user set */
    int32_t qpHdrPrev;           /* Pic header qp of previous coded picture */
    int32_t qpLastCoded;         /* Quantization parameter of last coded mb */
    int32_t qpTarget;            /* Target quantrization parameter */
    uint32_t estTimeInc;
    int32_t outRateNum;
    int32_t outRateDenom;
    int32_t gDelaySum;
    int32_t gInitialDelay;
    int32_t gInitialDoffs;
    h264QpCtrl_s qpCtrl;
    h264VirtualBuffer_s virtualBuffer;
    //sei_s sei;
    int32_t gBufferMin, gBufferMax;
   /* new rate control */
    linReg_s linReg;       /* Data for R-Q model */
    linReg_s overhead;
    linReg_s rError;       /* Rate prediction error (bits) */
    int32_t targetPicSize;
    int32_t sad;
    int32_t frameBitCnt;
   /* for gop rate control */
    int32_t gopQpSum;
    int32_t gopQpDiv;
    uint32_t frameCnt;
    int32_t gopLen;
    bool roiRc; 
    int32_t roiQpHdr;
    int32_t roiQpDelta;
    int32_t roiStart;
    int32_t roiLength;
    int32_t intraQpDelta;
    uint32_t fixedIntraQp;
    int32_t mbQpAdjustment;     /* QP delta for MAD macroblock QP adjustment */

	//add for rk30 new ratecontrol
    linReg_s intra;        /* Data for intra frames */
    linReg_s intraError;   /* Prediction error for intra frames */
    linReg_s gop;          /* Data for GOP */

    struct linear_model intra_frames;
    struct linear_model inter_frames;
	
    int32_t gopBitCnt;          /* Current GOP bit count so far */
    int32_t gopAvgBitCnt;       /* Previous GOP average bit count */
	
    int32_t windowLen;          /* Bitrate window which tries to match target */
    int32_t intraInterval;      /* Distance between two previous I-frames */
    int32_t intraIntervalCtr;
};

struct h264_rate_control {
	struct linear_model lin_mod;
	uint32_t acc_interframe_qp;
	uint32_t acc_interframe_cnt;
	int32_t qp;
	uint32_t bits_rate;
	uint32_t frmrate_num;
	uint32_t frmrate_denom;
	uint32_t actual_bits;
	uint32_t prev_bits;
	uint32_t gop_len;
	int32_t qp_prev;
	uint32_t bits[3]; // bits count used for pid control
    bool mb_rc_en;
    int32_t bits_per_non_zero_coef;
    int32_t non_zero_cnt;
    h264QpCtrl_s qpCtrl;
};

void h264e_before_rate_control(struct h264_rate_control *rc);

void h264e_after_rate_control(struct h264_rate_control *rc,
uint32_t coded_bytes, uint32_t non_zero_cnt, uint32_t qp_sum);

#endif /* H264_RATE_CONTROL_H */
