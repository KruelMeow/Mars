cmake_minimum_required(VERSION 3.4.1)
project(PROTOCOL)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -DWEBRTC_POSIX -fvisibility=hidden  -Wno-error=strict-aliasing -DUSING_XLOG_WEAK_FUNC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ")
set (MODULE_ROOT_PATH  ${CMAKE_CURRENT_LIST_DIR})
set (LOCAL_MODULE  "protocol")

set (LOCAL_SRC_FILES
    ${MODULE_ROOT_PATH}/ezxml.c
    ${MODULE_ROOT_PATH}/ezxml_pretty.c
    ${MODULE_ROOT_PATH}/tcp_xml.c
	)

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
                       atomic
                       )