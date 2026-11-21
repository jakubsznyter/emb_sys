#!/usr/bin/env bash
# Run in an AMD 2026.1 Vitis environment. Rebuilds ARM software only.
set -euo pipefail
assignment_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
python3 "$assignment_root/scripts/setup_paths.py"
vitis_root=${XILINX_VITIS:?Source the Vitis 2026.1 settings64.sh first.}
domain_dir="$assignment_root/platform/export/platform/sw/standalone_ps7_cortexa9_0"
cmake_bin="$vitis_root/tps/lnx64/cmake-4.0.1/bin/cmake"
export PATH="$vitis_root/gnu/aarch32/lin/gcc-arm-none-eabi/bin:$vitis_root/bin:$PATH"
"$cmake_bin" --fresh -S "$assignment_root/hello_world/src" -B "$assignment_root/hello_world/build" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$vitis_root/bin/ninja" \
    -DCMAKE_TOOLCHAIN_FILE="$domain_dir/cortexa9_toolchain.cmake" \
    -DCMAKE_MODULE_PATH="$domain_dir" \
    -DCMAKE_INCLUDE_PATH="$domain_dir/include" \
    -DCMAKE_LIBRARY_PATH="$domain_dir/lib" \
    -DCMAKE_SPECS_FILE="$domain_dir/Xilinx.spec" \
    -DCMAKE_BUILD_TYPE=Debug
"$cmake_bin" --build "$assignment_root/hello_world/build" --parallel 2
