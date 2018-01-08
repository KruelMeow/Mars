cmake_minimum_required(VERSION 3.4.1)
project(XLOG)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}      -fvisibility=hidden  -Wno-error=strict-aliasing -DUSING_XLOG_WEAK_FUNC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ")
set (MODULE_ROOT_PATH  ${CMAKE_CURRENT_LIST_DIR})
set (LOCAL_MODULE  "xlog")

set (LOCAL_SRC_FILES
   ${MODULE_ROOT_PATH}/../comm/xlogger/xloggerbase.c
   ${MODULE_ROOT_PATH}/crypt/log_crypt.cc
   ${MODULE_ROOT_PATH}/crypt/micro-ecc-master/uECC.c
   ${MODULE_ROOT_PATH}/jni/ConsoleLog.cc
   ${MODULE_ROOT_PATH}/src/appender.cc
   ${MODULE_ROOT_PATH}/src/formater.cc
   ${MODULE_ROOT_PATH}/src/log_buffer.cc
	)

#${MODULE_ROOT_PATH}/jni/com_tencent_mars_xlog_Xlog.cpp


set (LOCAL_C_INCLUDES
	${MODULE_ROOT_PATH}/
	${MODULE_ROOT_PATH}/../
	${MODULE_ROOT_PATH}/../../
	${MODULE_ROOT_PATH}/../../../
	)


include_directories(${LOCAL_C_INCLUDES})


add_library(${LOCAL_MODULE}
            STATIC
            ${LOCAL_SRC_FILES}
            )

target_link_libraries(${LOCAL_MODULE}
                       log
                       z
                       comm
                       cpufeatures
                       )


