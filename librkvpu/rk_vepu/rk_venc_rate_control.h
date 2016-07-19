#ifndef _V4L2_PLUGIN_RK_VENC_RATE_CONTROL_H_
#define _V4L2_PLUGIN_RK_VENC_RATE_CONTROL_H_

typedef struct {
  int32_t  a1;               /* model parameter */
  int32_t  a2;               /* model parameter */
  int32_t  qp_prev;          /* previous QP */
  int32_t  qs[15];           /* quantization step size */
  int32_t  bits[15];         /* Number of bits needed to code residual */
  int32_t  pos;              /* current position */
  int32_t  len;              /* current lenght */
  int32_t  zero_div;         /* a1 divisor is 0 */
} linReg_s;

#endif
