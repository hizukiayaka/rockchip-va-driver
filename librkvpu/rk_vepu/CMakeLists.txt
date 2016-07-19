cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

ADD_LIBRARY (rkvepu STATIC
rk_vepu.c rk_vepu_debug.c
vp8_enc/encasiccontroller.c vp8_enc/encasiccontroller_v2.c
vp8_enc/rk_vp8encapi.c vp8_enc/vp8codeframe.c vp8_enc/vp8encapi.c
vp8_enc/vp8entropy.c vp8_enc/vp8header.c vp8_enc/vp8init.c
vp8_enc/vp8macroblocktools.c vp8_enc/vp8picparameterset.c
vp8_enc/vp8picturebuffer.c vp8_enc/vp8putbits.c
vp8_enc/vp8ratecontrol.c vp8_enc/vpu_mem.c
h264e/h264e.c h264e/h264e_mad.c
h264e/h264e_rate_control.c
)

TARGET_INCLUDE_DIRECTORIES(rkvepu PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}/vp8_enc"
"${CMAKE_CURRENT_SOURCE_DIR}/h264e"
)
