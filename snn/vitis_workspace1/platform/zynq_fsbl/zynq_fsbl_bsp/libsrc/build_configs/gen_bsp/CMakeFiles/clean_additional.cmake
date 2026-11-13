# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/diskio.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/ff.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/ffconf.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/sleep.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/xilffs.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/xilffs_config.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/xilrsa.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/xiltimer.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/include/xtimer_config.h"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxilffs.a"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxilrsa.a"
  "/home/kuba/snn_env/vitis_workspace1/platform/zynq_fsbl/zynq_fsbl_bsp/lib/libxiltimer.a"
  )
endif()
