
cmake_minimum_required(VERSION 2.8)

#--------------------------------------------------------------------------------------#
#项目logic name
project(ezsh)

#--------------------------------------------------------------------------------------#
#设置一些通用变量值，指定所有目录放于一个目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_CONFIGURATION_HEADER_DIR  "${CMAKE_BINARY_DIR}/include")


#--------------------------------------------------------------------------------------#
#处理构建类型
set(BUILD_TYPE_DOCSTRING "Build types are: Debug, Release, MinSizeRel, RelWithDebInfo.")
if(NOT CMAKE_CONFIGURATION_TYPES)
    if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING ${BUILD_TYPE_DOCSTRING} FORCE)
    set(HAVE_CMAKE_BUILD_TYPE true)
    endif()
else()
    if(CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE CACHE STRING FORCE)
    endif()
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(GCC true)
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CLANG true)
endif()

#--------------------------------------------------------------------------------------#
#平台相关的编译选项
set(BoostRoot $ENV{BoostLibPrefix})
add_definitions(-DGMacroSourceRoot=${CMAKE_SOURCE_DIR})
if(GCC)
    set(ComplierRoot $ENV{HOME}/self/gcc49)
    add_definitions(-DEasyGCC)
    set(CFlags  "-MP -MMD -pipe -fPIC -pipe -fdiagnostics-color=auto")
    set(CDebugFlags     "-g3 -O0 -DDEBUG ")
    set(CReleaseFlags   "-g1 -O2 -DNDEBUG -flto -fuse-linker-plugin -ffat-lto-objects")
    set(CWarningFlags   "-Wall -Wextra -Wpedantic -Wformat=2 -Winit-self -Wno-long-long")
    set(CWarningFlags   "${CWarningFlags} -Wunused-value -Wunused-parameter -Wreturn-type -Wswitch")
    set(CppFlags        "${CFlags} -std=c++11 -ftemplate-backtrace-limit=1")
    set(CppReleaseFlags "${CReleaseFlags}")
    set(CppWarningFlags "${CWarningFlags} -Wdisabled-optimization -Wold-style-cast -Wdelete-non-virtual-dtor")
    set(CppWarningFlags "${CppWarningFlags} -Wnon-virtual-dtor -Wuninitialized -Wunreachable-code")
    set(ExeLinkFlags    "-Wl,-rpath,${ComplierRoot}/lib64 -static-libstdc++ -static-libgcc -flto")
    set(ExeLinkFlags    "${ExeLinkFlags} -fuse-linker-plugin -ffat-lto-objects -fwhole-program")
elseif(CLANG)
    set(ComplierRoot $ENV{HOME}/self/clang)
    add_definitions(-DBOOST_ASIO_HAS_STD_CHRONO)
    set(CFlags          "-MP -MMD -pipe -fPIC -pipe")
    set(CDebugFlags     "-g3 -O0 -DDEBUG")
    set(CReleaseFlags   "-g1 -O2 -DNDEBUG")
    set(CWarningFlags   "-Wall -Wextra -Wpedantic -Wformat=2 -Winit-self -Wno-long-long ")
    set(CWarningFlags   "${CWarningFlags} -Wunused-value -Wunused-parameter -Wreturn-type -Wswitch")
    set(CppFlags        "${CFlags} -std=c++14")
    set(CppReleaseFlags "${CDebugFlags}")
    set(CppWarningFlags "-Wall -Wextra -Wstring-conversion -Wloop-analysis -Wbool-conversion -Wloop-analysis")
    set(ExeLinkFlags    "-Wl,-rpath,${ComplierRoot}/lib64 -static-libstdc++ -static-libgcc")
else(MSVC)
    include_directories(SYSTEM ${BoostRoot})
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0501 -D_SCL_SECURE_NO_WARNINGS)
    link_directories("${BoostRoot}/lib32-msvc-12.0" ${CMAKE_BINARY_DIR})
endif()
include_directories(SYSTEM
    ${BoostRoot}/include
    ${CMAKE_SOURCE_DIR}/libs/png
    ${CMAKE_SOURCE_DIR}/libs/zlib
    ${CMAKE_SOURCE_DIR}/libs/jpeg
    ${CMAKE_SOURCE_DIR}/libs/sqlite
    ${CMAKE_SOURCE_DIR}/libs/webp/src
    ${CMAKE_SOURCE_DIR}/libs/utf8
    ${CMAKE_CONFIGURATION_HEADER_DIR}
)
include_directories(${CMAKE_SOURCE_DIR})
link_directories(${BoostRoot}/lib ${CMAKE_BINARY_DIR})

set(CMAKE_C_FLAGS_DEBUG       "${CMAKE_C_FLAGS_DEBUG}     ${CWarningFlags}   ${CFlags}   ${CDebugFlags}")
set(CMAKE_C_FLAGS_RELEASE     "${CMAKE_C_FLAGS_RELEASE}   ${CWarningFlags}   ${CFlags}   ${CReleaseFlags}")
set(CMAKE_CXX_FLAGS_DEBUG     "${CMAKE_CXX_FLAGS_DEBUG}   ${CppWarningFlags} ${CppFlags} ${CppDebugFlags}")
set(CMAKE_CXX_FLAGS_RELEASE   "${CMAKE_CXX_FLAGS_RELEASE} ${CppWarningFlags} ${CppFlags} ${CppReleaseFlags}")
set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}  ${ExeLinkFlags} ")

add_subdirectory(libs/zlib)
add_subdirectory(libs/png)
add_subdirectory(libs/jpeg)
add_subdirectory(libs/webp)
add_subdirectory(libs/sqlite)


#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#===============================================================================================
set(ezshSrc
    output.cpp parser.cpp
    cmdTextpp.cpp cmdContext.cpp cmdScript.cpp cmdStart.cpp cmdUnicode.cpp
    cmdList.cpp cmdRemove.cpp cmdSQLite.cpp cmdWebp.cpp cmdForeach.cpp
    ezsh.cpp option.cpp context.cpp fileset.cpp glob.cpp script.cpp
)

add_executable(ezsh ${ezshSrc})
set(BoostLibs
    boost_regex boost_program_options boost_filesystem
    boost_system
)
set(ezshLibs sqlite webp)
if(MSVC)
    set(dsub "-vc120-mt-gd-1_57")
    set(rsub "-vc120-mt-1_57")
    target_link_libraries(ezsh ${ezshLibs})
    foreach(file ${BoostLibs})
        target_link_libraries(ezsh debug lib${file}${dsub} optimized lib${file}${rsub})
    endforeach()
else()
    target_link_libraries(ezsh ${ezshLibs} ${BoostLibs} pthread dl)
endif()

