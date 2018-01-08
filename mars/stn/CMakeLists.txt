cmake_minimum_required(VERSION 3.4.1)
project(STN)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}      -fvisibility=hidden  -Wno-error=strict-aliasing -DUSING_XLOG_WEAK_FUNC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ")
set (MODULE_ROOT_PATH  ${CMAKE_CURRENT_LIST_DIR})
set (LOCAL_MODULE  "stn")

set (LOCAL_SRC_FILES
   ${MODULE_ROOT_PATH}/src/anti_avalanche.cc
   ${MODULE_ROOT_PATH}/src/dynamic_timeout.cc
   ${MODULE_ROOT_PATH}/src/flow_limit.cc
   ${MODULE_ROOT_PATH}/src/frequency_limit.cc
   ${MODULE_ROOT_PATH}/src/longlink.cc
   ${MODULE_ROOT_PATH}/src/longlink_connect_monitor.cc
   ${MODULE_ROOT_PATH}/src/longlink_identify_checker.cc
   ${MODULE_ROOT_PATH}/src/longlink_speed_test.cc
   ${MODULE_ROOT_PATH}/src/longlink_task_manager.cc
   ${MODULE_ROOT_PATH}/src/net_channel_factory.cc
   ${MODULE_ROOT_PATH}/src/net_check_logic.cc
   ${MODULE_ROOT_PATH}/src/net_core.cc
   ${MODULE_ROOT_PATH}/src/net_source.cc
   ${MODULE_ROOT_PATH}/src/netsource_timercheck.cc
   ${MODULE_ROOT_PATH}/src/proxy_test.cc
   ${MODULE_ROOT_PATH}/src/shortlink.cc
   ${MODULE_ROOT_PATH}/src/shortlink_task_manager.cc
   ${MODULE_ROOT_PATH}/src/signalling_keeper.cc
   ${MODULE_ROOT_PATH}/src/simple_ipport_sort.cc
   ${MODULE_ROOT_PATH}/src/smart_heartbeat.cc
   ${MODULE_ROOT_PATH}/src/task_profile.cc
   ${MODULE_ROOT_PATH}/src/timing_sync.cc
   ${MODULE_ROOT_PATH}/src/zombie_task_manager.cc
   ${MODULE_ROOT_PATH}/stn.cc
   ${MODULE_ROOT_PATH}/stn_logic.cc
	)

  # ${MODULE_ROOT_PATH}/jni/com_tencent_mars_stn_StnLogic.cpp
  # ${MODULE_ROOT_PATH}/jni/com_tencent_mars_stn_StnLogic_Java2C.cc

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
                       sdt
                       xlog
                        )