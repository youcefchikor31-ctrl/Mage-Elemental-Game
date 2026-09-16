cmake_minimum_required(VERSION 3.22.1)
project(MageElements CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ANDROID_STL_TYPE c++_static)

set(PLATFORM "Android" CACHE STRING "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# دمج مكتبة Raylib مباشرة في المشروع
add_subdirectory(raylib)

# بناء اللعبة كمكتبة مشتركة مخصصة لنظام NativeActivity في أندرويد
add_library(main SHARED main.cpp)

target_include_directories(main PRIVATE raylib/src)

# استخدام whole-archive لضمان تصدير دوال تشغيل أندرويد بالكامل دون حذف
target_link_libraries(main PRIVATE
    -Wl,--whole-archive raylib -Wl,--no-whole-archive
    android
    log
    OpenSLES
    GLESv2
    EGL
    m
)
