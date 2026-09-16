set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# 工具链前缀 (如果你没有把工具链加入环境变量，这里需要指定自己安装arm-gcc的绝对路径)
set(TOOLCHAIN_PREFIX "D:/ST/workspace/compiler/arm-gnu-toolchain-15.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-")

# 指定编译器
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc.exe)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++.exe)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc.exe)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy.exe)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size.exe)

# 告诉CMake不需要编译成Windows可执行文件，不需要链接编译成静态库.a
# 必须设置，否则 CMake 会尝试链接主机程序而报错
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)