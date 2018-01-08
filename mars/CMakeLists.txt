cmake_minimum_required(VERSION 3.4.1)
project(mars)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  " )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fopenmp" )

set (MODULE_MARS_PATH  ${CMAKE_SOURCE_DIR})
include(AndroidNdkModules)
android_ndk_import_module_cpufeatures()
message (${MODULE_MARS_PATH})

add_subdirectory(${MODULE_MARS_PATH}/openssl)
add_subdirectory(${MODULE_MARS_PATH}/comm)
add_subdirectory(${MODULE_MARS_PATH}/baseevent)
add_subdirectory(${MODULE_MARS_PATH}/log)
add_subdirectory(${MODULE_MARS_PATH}/app)
add_subdirectory(${MODULE_MARS_PATH}/sdt)
add_subdirectory(${MODULE_MARS_PATH}/stn)
add_subdirectory(${MODULE_MARS_PATH}/libraries/mars_android_sdk)



