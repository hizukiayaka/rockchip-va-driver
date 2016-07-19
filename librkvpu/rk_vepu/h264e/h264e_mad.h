#ifndef H264_MAD_H
#define H264_MAD_H

#include <stdint.h>
//#include "enccommon.h"

#define DSCY                      32 /* n * 32 */
#define I32_MAX           2147483647 /* 2 ^ 31 - 1 */
#define MAD_TABLE_LEN              5

struct v4l2_plugin_h264_mad_table {
    int32_t  a1;               /* model parameter, y = a1*x + a2 */
    int32_t  a2;               /* model parameter */
    int32_t  th[MAD_TABLE_LEN];     /* mad threshold */
    int32_t  count[MAD_TABLE_LEN];  /* number of macroblocks under threshold */
    int32_t  pos;              /* current position */
    int32_t  len;              /* current lenght */
    int32_t  threshold;        /* current frame threshold */
    int32_t  mbPerFrame;       /* number of macroblocks per frame */
};

/*------------------------------------------------------------------------------
    Function prototypes
------------------------------------------------------------------------------*/

void h264_mad_init(struct v4l2_plugin_h264_mad_table *mad, uint32_t mbPerFrame);

void H264MadThreshold(struct v4l2_plugin_h264_mad_table *madTable, uint32_t madCount);

#endif

