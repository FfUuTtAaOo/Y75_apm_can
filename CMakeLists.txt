cmake_minimum_required(VERSION 3.22)

# ==========================================
# 交叉编译工具链（ARM GCC）
# 必须在 project() 之前设置，否则 CMake 会优先探测 MSVC / 本机编译器
# （Windows 上 CMake 默认生成 Visual Studio 工程，无法编译本工程）。
# 修改编译器路径时，只需改下面 TOOLCHAIN_BIN，或在命令行传
# -DTOOLCHAIN_BIN=xxx 覆盖；也可直接 -DCMAKE_C_COMPILER=xxx 显式指定。
# ==========================================
set(TOOLCHAIN_BIN
    "D:/ST/workspace/compiler/arm-gnu-toolchain-15.2.rel1-mingw-w64-x86_64-arm-none-eabi/bin"
    CACHE STRING "ARM GNU toolchain bin directory")
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "${TOOLCHAIN_BIN}/arm-none-eabi-gcc.exe")
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_BIN}/arm-none-eabi-gcc.exe")
endif()
if(NOT CMAKE_OBJCOPY)
    set(CMAKE_OBJCOPY "${TOOLCHAIN_BIN}/arm-none-eabi-objcopy.exe")
endif()
if(NOT CMAKE_SIZE)
    set(CMAKE_SIZE "${TOOLCHAIN_BIN}/arm-none-eabi-size.exe")
endif()

# 声明为裸机交叉编译（Generic 平台）：必须放在 project() 之前。
# 不声明的话，Windows 宿主会让 CMake 走 Windows 平台规则——即使加了
# CMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY，链接 exe 时仍会传
# PE 专用选项 --major-image-version/--minor-image-version，arm-none-eabi-ld
# 不认识而报 "unrecognized option"。
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m3)

# 嵌入式交叉编译：CMake 编译器自检只编译静态库，不尝试链接 .exe
# （arm-none-eabi-ld 不支持 Windows 的 --major-image-version 等链接选项，
#   默认自检会因此失败，报 CMAKE_C_COMPILER 测试错误）。
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ==========================================
# 配置区域：选择芯片型号 (APM32F1 或 APM32F4)
# ==========================================
set(MCU_FAMILY "APM32F1") # 这里改成 "APM32F1" 或 "APM32F4"
set(MCU_MODEL "APM32F103CBT6")

# 项目名称, 修改成项目编号
project(Y75_apm_can C ASM)

# ==========================================
# 生成器/编译器兼容性检查
# Windows 上不带 -G 时 CMake 默认选 Visual Studio 生成器（强制 MSVC），
# 无法编译 ARM 交叉工程，且会忽略上面设置的 CMAKE_C_COMPILER。
# 注意：CMAKE_GENERATOR 无法在 CMakeLists 中 set() 覆盖，只能靠命令行
# -G 或 IDE 设置。此处检测到 MSVC 时给出明确提示，避免出现一屏 C1189。
# ==========================================
if(MSVC)
    message(FATAL_ERROR
        "检测到 MSVC (Visual Studio 生成器)。本工程必须使用 ARM GCC 交叉编译器。\n"
        "请任选一种方式重新配置（先删除 build 目录）:\n"
        "  1) 命令行: cmake -S . -B build -G \"MinGW Makefiles\"\n"
        "  2) 一键脚本: .\\build.ps1\n"
        "  3) VS Code CMake Tools: 设置 cmake.generator 为 \"MinGW Makefiles\"")
endif()

# 目标产物不追加 .exe 后缀：MinGW 生成器在 Windows 宿主上默认会给
# add_executable() 的目标加 .exe，链接时 CMake 会传 PE 专用选项
# (--major-image-version 等)，arm-none-eabi-ld 不认识而失败。
# 显式清空后缀，让目标保持为 Y77.elf。
set(CMAKE_EXECUTABLE_SUFFIX "")
set(CMAKE_EXECUTABLE_SUFFIX_C "")
set(CMAKE_EXECUTABLE_SUFFIX_ASM "")

# 设置 C 标准
set(CMAKE_C_STANDARD 11)

# ==========================================
# 根据芯片家族设置编译参数
# -mthumb: 指令集
# -mfpu=fpv4-sp-d16: 使用硬件浮点计算
# -mfloat-abi=hard: 函数传参时直接用 FPU 寄存器
# 指定链接脚本 (你需要把对应的 .ld 文件放入 bsp/Linker)
# 丝印第10位决定: 4/6->LD, 8/B->MD, C/D->HD, E/F/G->XD/XL
# ==========================================
if(MCU_FAMILY STREQUAL "APM32F1")
    # APM32F1 (Cortex-M3)
    message(STATUS "Building for APM32F1 (Cortex-M3)")
    set(CPU_PARAMS "-mcpu=cortex-m3" "-mthumb")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/Libraries/Device/Geehy/APM32F10x/Source/gcc/apm32f103xb_flash.ld")
    
elseif(MCU_FAMILY STREQUAL "APM32F4")
    # APM32F4 (Cortex-M4F, 带FPU)
    message(STATUS "Building for APM32F4 (Cortex-M4F)")
    set(CPU_PARAMS "-mcpu=cortex-m4" "-mthumb" "-mfpu=fpv4-sp-d16" "-mfloat-abi=hard")
    set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/Libraries/Device/Geehy/APM32F40x/Source/gcc/apm32F407xg_flash.ld")
else()
    message(FATAL_ERROR "请设置正确的 MCU_FAMILY (APM32F1 或 APM32F4)")
endif()

# ==========================================
# 头文件路径
# TODO: 添加自己的头文件目录
# ==========================================
include_directories(
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/Board/inc
    ${CMAKE_SOURCE_DIR}/Libraries/APM32F10x_StdPeriphDriver/inc
    ${CMAKE_SOURCE_DIR}/Libraries/CMSIS/Include
    # ${CMAKE_SOURCE_DIR}/Libraries/CMSIS/Core/include
    ${CMAKE_SOURCE_DIR}/Libraries/Device/Geehy/APM32F10x/Include
)

# ==========================================
# 源文件
# Geehy Source中包含system_apm32f10x.c
# TODO: 添加自己的源文件
# ==========================================
file(GLOB_RECURSE SOURCES
    "src/*.c"
    "Board/src/*.c"
    "${CMAKE_SOURCE_DIR}/Libraries/APM32F10x_StdPeriphDriver/src/*.c"
)

# 启动文件 (汇编文件 .s)
# 注意：根据你的具体芯片容量选择 (LD/MD/HD/CL)
# 初始化堆栈指针 (SP)、把数据段 (Data) 从 Flash 搬到 RAM、把 BSS 段清零、设置中断向量表，最后跳转到 main()
if(MCU_FAMILY STREQUAL "APM32F1")
    set(STARTUP_ASM "${CMAKE_SOURCE_DIR}/Libraries/Device/Geehy/APM32F10x/Source/gcc/startup_apm32f103xb.S")
else()
    set(STARTUP_ASM "${CMAKE_SOURCE_DIR}/Libraries/Device/Geehy/APM32F40x/Source/gcc/startup_apm32f40x.S")
endif()

# ==========================================
# 生成可执行文件
# ==========================================
add_executable(${PROJECT_NAME}.elf ${SOURCES} ${STARTUP_ASM})

# 编译选项
# -Wall: 开启所有警告
# -fdata-sections, -ffunction-sections: 把每个函数和变量都放在独立的段
# -Wl,--gc-sections: 把没用到的函数和变量都删掉, 配合上面使用
target_compile_options(${PROJECT_NAME}.elf PRIVATE
    ${CPU_PARAMS}
    -Wall
    -fdata-sections
    -ffunction-sections
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp>
)

# 链接选项
target_link_options(${PROJECT_NAME}.elf PRIVATE
    ${CPU_PARAMS}
    -T${LINKER_SCRIPT}
    -specs=nano.specs
    -specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--no-warn-rwx-segments
    -u _printf_float
)

# 宏定义
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    APM32F103xB
    USE_STDPERIPH_DRIVER
    APM32F103_MINI
)
# ==========================================
# 生成 .bin 和 .hex
# ==========================================
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Building ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)