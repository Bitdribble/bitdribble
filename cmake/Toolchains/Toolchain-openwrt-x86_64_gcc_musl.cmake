#
# The openwrt toolchain is under .../openwrt/staging_dir/toolchain-XXX,
# with compiler under .../openwrt/staging_dir/toolchain-XXX/bin
#
# For cross compilation, set the following:
# export STAGING_DIR=.../openwrt/staging_dir
# export TOOLCHAIN_DIR=$STAGING_DIR/toolchain-x86_64_gcc-7.3.0_musl
# export TARGET_DIR=$STAGING_DIR/target-x86_64_musl
# export PATH=$TOOLCHAIN_DIR/bin:$PATH

set(CMAKE_C_COMPILER x86_64-openwrt-linux-musl-gcc)
set(CMAKE_CXX_COMPILER x86_64-openwrt-linux-musl-g++)

include_directories("$ENV{TOOLCHAIN_DIR}/include" "$ENV{TARGET_DIR}/usr/include")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L$ENV{TARGET_DIR}/usr/lib")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L$ENV{TARGET_DIR}/usr/lib")

set(OPENWRT 1)
