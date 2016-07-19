#include "h264e_common.h"
#include "h264e_mad.h"

#define ASSERT(x)

void h264_mad_init(struct v4l2_plugin_h264_mad_table *mad, uint32_t mbPerFrame)
{
    int32_t i;

    mad->mbPerFrame = mbPerFrame;

    mad->threshold = 256 * 6;

    for (i = 0; i < MAD_TABLE_LEN; i++) {
        mad->th[i] = 0;
        mad->count[i] = 0;
    }
    mad->pos = 0;
    mad->len = 0;
}

/*------------------------------------------------------------------------------
  update_tables()
------------------------------------------------------------------------------*/
static void update_tables(struct v4l2_plugin_h264_mad_table *p, int32_t th, int32_t count)
{
    const int32_t clen = 3;
    int32_t tmp = p->pos;

    p->th[tmp]    = th;
    p->count[tmp] = count;

    if (++p->pos >= clen) {
        p->pos = 0;
    }
    if (p->len < clen) {
        p->len++;
    }
}
/*------------------------------------------------------------------------------
  lin_sx()  calculate value of Sx for n points.
------------------------------------------------------------------------------*/
static int32_t lin_sx(int32_t *x, int32_t n)
{
    int32_t sum = 0;

    while (n--) {
        sum += x[n];
        if (sum < 0) {
            return I32_MAX;
        }
    }
    return sum;
}

/*------------------------------------------------------------------------------
  lin_sxy()  calculate value of Sxy for n points.
------------------------------------------------------------------------------*/
static int32_t lin_sxy(int32_t *qp, int32_t *r, int32_t n)
{
    int32_t sum = 0;

    while (n--) {
        sum += qp[n] * r[n];
        if (sum < 0) {
            return I32_MAX;
        }
    }
    return sum;
}

/*------------------------------------------------------------------------------
  lin_nsxx()  calculate value of n * Sxx for n points.
------------------------------------------------------------------------------*/
static int32_t lin_nsxx(int32_t *qp, int32_t n)
{
    int32_t sum = 0, d = n;

    while (n--) {
        sum += d * qp[n] * qp[n];
        if (sum < 0) {
            return I32_MAX;
        }
    }
    return sum;
}

/*------------------------------------------------------------------------------
    update_model()  Update model parameter by Linear Regression.
------------------------------------------------------------------------------*/
static void update_model(struct v4l2_plugin_h264_mad_table *p)
{
    int32_t *count = p->count, *th = p->th, n = p->len;
    int32_t sx = lin_sx(p->count, n);
    int32_t sy = lin_sx(p->th, n);
    int32_t a1 = 0, a2 = 0;

    /*int32_t i;
    for (i = 0; i < n; i++) {
        printf("model: cnt %i  th %i\n", count[i], th[i]);
    }*/

    ASSERT(sx >= 0);
    ASSERT(sy >= 0);

    if (n > 1)
    {
        a1 = lin_sxy(count, th, n);
        a1 = a1 < (I32_MAX / n) ? a1 * n : I32_MAX;

        if (sy) {
            a1 -= sx < (I32_MAX / sy) ? sx * sy : I32_MAX;
        }

        a2 = (lin_nsxx(count, n) - (sx * sx));
        if (a2) {
            a1 = DIV(a1 * DSCY, a2);
        } else {
            a1 = 0;
        }

        /* Value of a1 shouldn't be excessive */
        a1 = MAX(a1, 0);
        a1 = MIN(a1, 1024*DSCY);

        if (n)
            a2 = DIV(sy, n) - DIV(a1 * sx, n*DSCY);
        else
            a2 = 0;
    }

    p->a1 = a1;
    p->a2 = a2;

    /*printf("model: a2:%9d  a1:%8d\n", p->a2, p->a1);*/
}

/*------------------------------------------------------------------------------

    Update MAD threshold based on previous frame count of macroblocks with MAD
    under threshold. Trying to adjust threshold so that madCount == targetCount.

------------------------------------------------------------------------------*/
void H264MadThreshold(struct v4l2_plugin_h264_mad_table *mad, uint32_t madCount)
{
    /* Target to improve quality for 30% of macroblocks */
    int32_t targetCount = 30*mad->mbPerFrame/100;
    int32_t threshold = mad->threshold;
    int32_t lowLimit, highLimit;

    update_tables(mad, mad->threshold, madCount);
    update_model(mad);

    /* Calculate new threshold for next frame using either linear regression
     * model or adjustment based on current setting */
    if (mad->a1)
        threshold = mad->a1 * targetCount / DSCY + mad->a2;
    else if (madCount < targetCount)
        threshold = MAX(mad->threshold * 5/4, mad->threshold + 256);
    else
        threshold = MIN(mad->threshold * 3/4, mad->threshold - 256);

    /* For small count, ensure that we increase the threshold minimum 1 step */
    if (madCount < targetCount / 2)
        threshold = MAX(threshold, mad->threshold + 256);

    /* If previous frame had zero count, ensure that we increase threshold */
    if (!madCount)
        threshold = MAX(threshold, mad->threshold + 256*4);

    /* Limit how much the threshold can change between two frames */
    lowLimit = mad->threshold/2;
    highLimit = MAX(mad->threshold*2, 256*4);
    mad->threshold = MIN(highLimit, MAX(lowLimit, threshold));

    /* threshold_div256 has 6-bits range [0,63] */
    mad->threshold = ((mad->threshold+128)/256)*256;
    mad->threshold = MAX(0, MIN(63*256, mad->threshold));
}

