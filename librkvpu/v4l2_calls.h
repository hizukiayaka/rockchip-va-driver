#ifndef _V4L2_CALLS_H_
#define _V4L2_CALLS_H_

#include "rk_vepu_debug.h"
#include "rk_vepu_interface.h"

void *rk_v4l2_init(int fd);
void rk_v4l2_close(void *dev_ops_priv);
int 
rk_v4l2_ioctl(void *dev_ops_priv, int fd,
unsigned long int cmd, void *arg);

#endif
