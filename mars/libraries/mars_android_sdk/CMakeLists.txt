cmake_minimum_required(VERSION 3.4.1)
project(MARSSTN)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}      -fvisibility=hidden  -Wno-error=strict-aliasing  ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ")
#CMAKE_MODULE_LINKER_FLAGS
#CMAKE_SHARED_LINKER_FLAGS
#CMAKE_STATIC_LINKER_FLAGS

set (MODULE_ROOT_PATH  ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_MODULE_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS}  -Wl,--gc-sections")


set (LOCAL_MODULE  "marsstn")

set (LOCAL_SRC_FILES
   ${MODULE_ROOT_PATH}/jni/shortlink_packer.cc
   ${MODULE_ROOT_PATH}/jni/longlink_packer.cc
   ${MODULE_ROOT_PATH}/jni/JNI_OnLoad.cc
   ${MODULE_ROOT_PATH}/jni/import.cc
   ${MODULE_ROOT_PATH}/jni/com_tencent_mars_sdt_SdtLogic_C2Java.cc
   ${MODULE_ROOT_PATH}/jni/com_tencent_mars_sdt_SdtLogic_Java2C.cc
   ${MODULE_ROOT_PATH}/jni/com_tencent_mars_stn_StnLogic_C2Java.cc
   ${MODULE_ROOT_PATH}/jni/com_tencent_mars_stn_StnLogic_Java2C.cc
   ${MODULE_ROOT_PATH}/jni/com_tencent_mars_BaseEvent.cc
	)

set (LOCAL_C_INCLUDES
	${MODULE_ROOT_PATH}/
	${MODULE_ROOT_PATH}/jni/
	${MODULE_ROOT_PATH}/../
	${MODULE_ROOT_PATH}/../../
	${MODULE_ROOT_PATH}/../../../
	)


include_directories(${LOCAL_C_INCLUDES})

add_library(${LOCAL_MODULE}
            SHARED
            ${LOCAL_SRC_FILES}
            )

target_link_libraries(${LOCAL_MODULE}
                       log
                       comm
                       appcomm
                       baseevent
                       atomic
                       sdt
                       stn
                       xlog
                        )


add_library("marsxlog"
            SHARED
            ${MODULE_ROOT_PATH}/jni/JNI_OnLoad.cc
            ${MODULE_ROOT_PATH}/jni/import.cc
            )

target_link_libraries("marsxlog"
                       comm
                       xlog
                       z
                        )
