cmake_minimum_required(VERSION 3.20)
project(poc_workqueue_ray LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)

# ---- zfp (built locally by CMake) ----
set(ZFP_WITH_OPENMP OFF CACHE BOOL "" FORCE)
set(ZFP_WITH_CUDA OFF CACHE BOOL "" FORCE)
set(ZFP_WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZFP_WITH_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  zfp
  GIT_REPOSITORY https://github.com/LLNL/zfp.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(zfp)

# ---- CCTools Work Queue local build artifacts ----
set(CCTOOLS_ROOT "${CMAKE_SOURCE_DIR}/../cctools" CACHE PATH "Path to local cctools source/build")
set(WQ_INCLUDE_DIR "${CCTOOLS_ROOT}/work_queue/src")
set(DT_INCLUDE_DIR "${CCTOOLS_ROOT}/dttools/src")
set(WQ_LIB "${CCTOOLS_ROOT}/work_queue/src/libwork_queue.a")
set(DT_LIB "${CCTOOLS_ROOT}/dttools/src/libdttools.a")

if(NOT EXISTS "${WQ_LIB}")
  message(FATAL_ERROR "Missing ${WQ_LIB}. Build cctools first (make -C ../cctools -j work_queue)")
endif()
if(NOT EXISTS "${DT_LIB}")
  message(FATAL_ERROR "Missing ${DT_LIB}. Build cctools first (make -C ../cctools -j work_queue)")
endif()

# ---- compress task (shared for WQ and Ray) ----
add_executable(compress_task src/compress_task.cpp)
target_link_libraries(compress_task PRIVATE zfp)

# ---- Work Queue manager ----
add_executable(workqueue_manager src/workqueue_manager.cpp)
target_include_directories(workqueue_manager PRIVATE ${WQ_INCLUDE_DIR} ${DT_INCLUDE_DIR})
target_link_libraries(workqueue_manager PRIVATE ${WQ_LIB} ${DT_LIB} m dl pthread z)
