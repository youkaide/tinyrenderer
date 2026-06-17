# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.20)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("tinyrenderer")

# 指定 C++ 标准 (推荐 C++14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 直接将 0/src 下的代码编译成可执行文件
add_executable(TinyRenderer
    "0/src/main.cpp"
    "0/src/tgaimage.cpp"
    "0/src/tgaimage.h"
    "0/src/model.h"
    "0/src/model.cpp"
    "0/src/geometry.h" 
    "0/src/our_gl.h"
    "0/src/our_gl.cpp" 
    )
