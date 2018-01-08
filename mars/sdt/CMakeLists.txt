cmake_minimum_required(VERSION 3.4.1)
project(SDT)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}      -fvisibility=hidden  -Wno-error=strict-aliasing -DUSING_XLOG_WEAK_FUNC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ")
set (MODULE_ROOT_PATH  ${CMAKE_CURRENT_LIST_DIR})
set (LOCAL_MODULE  "sdt")

set (LOCAL_SRC_FILES
   ${MODULE_ROOT_PATH}/src/activecheck/basechecker.cc
   ${MODULE_ROOT_PATH}/src/activecheck/dnschecker.cc
   ${MODULE_ROOT_PATH}/src/activecheck/httpchecker.cc
   ${MODULE_ROOT_PATH}/src/activecheck/pingchecker.cc
   ${MODULE_ROOT_PATH}/src/activecheck/tcpchecker.cc
   ${MODULE_ROOT_PATH}/src/checkimpl/dnsquery.cc
   ${MODULE_ROOT_PATH}/src/checkimpl/httpquery.cc
   ${MODULE_ROOT_PATH}/src/checkimpl/pingquery.cc
   ${MODULE_ROOT_PATH}/src/checkimpl/tcpquery.cc
   ${MODULE_ROOT_PATH}/src/sdt_core.cc
   ${MODULE_ROOT_PATH}/src/tools/netchecker_trafficmonitor.cc
   ${MODULE_ROOT_PATH}/jni/com_tencent_mars_sdt_SdtLogic_C2Java.cc
   ${MODULE_ROOT_PATH}/sdt_logic.cc
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
                       comm
                       appcomm
                       baseevent
                       comm
                       atomic
                       )