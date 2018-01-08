cmake_minimum_required(VERSION 3.4.1)
project(COMM)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}    -fvisibility=hidden  -Wno-error=strict-aliasing -DUSING_XLOG_WEAK_FUNC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ")
set (MODULE_ROOT_PATH  ${CMAKE_CURRENT_LIST_DIR}/..)
set (LOCAL_MODULE  "comm")


set(can_use_assembler TRUE)
enable_language(ASM)

#add_definitions(-v -DANDROID -DNDEBUG -DOC_ARM_ASM) #-v 展示详细日志
add_definitions(-DANDROID -DNDEBUG -DOC_ARM_ASM)



if (${ANDROID_ABI} STREQUAL "armeabi-v7a")

      set (LOCAL_SRC_ASM2
        ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/jump_arm_aapcs_elf_gas.S
        ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/make_arm_aapcs_elf_gas.S
      )

elseif (${ANDROID_ABI} STREQUAL "arm64-v8a")
        set (LOCAL_SRC_ASM2
          ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/jump_arm64_aapcs_elf_gas.S
          ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/make_arm64_aapcs_elf_gas.S
        )

elseif (${ANDROID_ABI} STREQUAL "x86")

        set (LOCAL_SRC_ASM2
          ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/jump_i386_sysv_elf_gas.S
          ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/make_i386_sysv_elf_gas.S
        )

elseif (${ANDROID_ABI} STREQUAL "x86_64")

       set (LOCAL_SRC_ASM2
           ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/jump_x86_64_sysv_elf_gas.S
           ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/make_x86_64_sysv_elf_gas.S
       )

endif()


 if(CMAKE_SYSTEM_PROCESSOR STREQUAL armv7-a)
   set (LOCAL_SRC_ASM
          ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/jump_arm_aapcs_elf_gas.S
          ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/make_arm_aapcs_elf_gas.S
        )
 elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL i686)
    set (LOCAL_SRC_ASM
             ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/jump_i386_sysv_elf_gas.S
             ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/make_i386_sysv_elf_gas.S
           )
 elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL mips)

       set (LOCAL_SRC_ASM
              ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/jump_i386_sysv_elf_gas.S
              ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/make_i386_sysv_elf_gas.S
            )

 endif()



set (LOCAL_SRC_FILES
    ${MODULE_ROOT_PATH}/boost/libs/atomic/src/lockpool.cpp
    ${LOCAL_SRC_ASM2}
    ${MODULE_ROOT_PATH}/boost/libs/context/src/execution_context.cpp
    ${MODULE_ROOT_PATH}/boost/libs/context/src/posix/stack_traits.cpp
    ${MODULE_ROOT_PATH}/boost/libs/coroutine/src/detail/coroutine_context.cpp
    ${MODULE_ROOT_PATH}/boost/libs/coroutine/src/exceptions.cpp
    ${MODULE_ROOT_PATH}/boost/libs/coroutine/src/posix/stack_traits.cpp
    ${MODULE_ROOT_PATH}/boost/libs/date_time/src/gregorian/date_generators.cpp
    ${MODULE_ROOT_PATH}/boost/libs/date_time/src/gregorian/greg_month.cpp
    ${MODULE_ROOT_PATH}/boost/libs/date_time/src/gregorian/greg_weekday.cpp
    ${MODULE_ROOT_PATH}/boost/libs/date_time/src/gregorian/gregorian_types.cpp
    ${MODULE_ROOT_PATH}/boost/libs/date_time/src/posix_time/posix_time_types.cpp
    ${MODULE_ROOT_PATH}/boost/libs/filesystem/src/codecvt_error_category.cpp
    ${MODULE_ROOT_PATH}/boost/libs/filesystem/src/operations.cpp
    ${MODULE_ROOT_PATH}/boost/libs/filesystem/src/path.cpp
    ${MODULE_ROOT_PATH}/boost/libs/filesystem/src/path_traits.cpp
    ${MODULE_ROOT_PATH}/boost/libs/filesystem/src/portability.cpp
    ${MODULE_ROOT_PATH}/boost/libs/filesystem/src/unique_path.cpp
    ${MODULE_ROOT_PATH}/boost/libs/filesystem/src/utf8_codecvt_facet.cpp
    ${MODULE_ROOT_PATH}/boost/libs/filesystem/src/windows_file_codecvt.cpp
    ${MODULE_ROOT_PATH}/boost/libs/iostreams/src/file_descriptor.cpp
    ${MODULE_ROOT_PATH}/boost/libs/iostreams/src/mapped_file.cpp
    ${MODULE_ROOT_PATH}/boost/libs/smart_ptr/src/sp_collector.cpp
    ${MODULE_ROOT_PATH}/boost/libs/smart_ptr/src/sp_debug_hooks.cpp
    ${MODULE_ROOT_PATH}/boost/libs/system/src/error_code.cpp
    ${MODULE_ROOT_PATH}/boost/libs/thread/src/future.cpp
    ${MODULE_ROOT_PATH}/boost/libs/thread/src/pthread/once.cpp
    ${MODULE_ROOT_PATH}/boost/libs/thread/src/pthread/once_atomic.cpp
    ${MODULE_ROOT_PATH}/boost/libs/thread/src/tss_null.cpp
    ${MODULE_ROOT_PATH}/comm/adler32.c
    ${MODULE_ROOT_PATH}/comm/alarm.cc
    ${MODULE_ROOT_PATH}/comm/android/callstack.cc
    ${MODULE_ROOT_PATH}/comm/android/dumpcrash_stack.cc
    ${MODULE_ROOT_PATH}/comm/android/getprocessname.c
    ${MODULE_ROOT_PATH}/comm/android/wakeuplock.cc
    ${MODULE_ROOT_PATH}/comm/anr.cc
    ${MODULE_ROOT_PATH}/comm/assert/__assert.c
    ${MODULE_ROOT_PATH}/comm/autobuffer.cc
    ${MODULE_ROOT_PATH}/comm/basepacker.cc
    ${MODULE_ROOT_PATH}/comm/boost_exception.cc
    ${MODULE_ROOT_PATH}/comm/comm_frequency_limit.cc
    ${MODULE_ROOT_PATH}/comm/corepattern/coreservice_base.cc
    ${MODULE_ROOT_PATH}/comm/coroutine/coro_socket.cc
    ${MODULE_ROOT_PATH}/comm/coroutine/dns.cc
    ${MODULE_ROOT_PATH}/comm/crypt/ibase64.cc
    ${MODULE_ROOT_PATH}/comm/crypt/pkcs7_padding.c
    ${MODULE_ROOT_PATH}/comm/debugger/test_spy_sample.cc
    ${MODULE_ROOT_PATH}/comm/debugger/testspy.cc
    ${MODULE_ROOT_PATH}/comm/dns/dns.cc
    ${MODULE_ROOT_PATH}/comm/http.cc
    ${MODULE_ROOT_PATH}/comm/jni/build.conf
    ${MODULE_ROOT_PATH}/comm/jni/ifaddrs.c
    ${MODULE_ROOT_PATH}/comm/jni/platform_comm.cc
    ${MODULE_ROOT_PATH}/comm/jni/util/comm_function.cc
    ${MODULE_ROOT_PATH}/comm/jni/util/scope_jenv.cc
    ${MODULE_ROOT_PATH}/comm/jni/util/scoped_jstring.cc
    ${MODULE_ROOT_PATH}/comm/jni/util/var_cache.cc
    ${MODULE_ROOT_PATH}/comm/jni/xlogger_threadinfo.cc
    ${MODULE_ROOT_PATH}/comm/md5.c
    ${MODULE_ROOT_PATH}/comm/memdbg.cc
    ${MODULE_ROOT_PATH}/comm/messagequeue/message_queue.cc
    ${MODULE_ROOT_PATH}/comm/mmap_util.cc
    ${MODULE_ROOT_PATH}/comm/network/getdnssvraddrs.cc
    ${MODULE_ROOT_PATH}/comm/network/getgateway.c
    ${MODULE_ROOT_PATH}/comm/network/getifaddrs.cc
    ${MODULE_ROOT_PATH}/comm/network/netinfo_util.cc
    ${MODULE_ROOT_PATH}/comm/ptrbuffer.cc
    ${MODULE_ROOT_PATH}/comm/singleton.cc
    ${MODULE_ROOT_PATH}/comm/socket/block_socket.cc
    ${MODULE_ROOT_PATH}/comm/socket/complexconnect.cc
    ${MODULE_ROOT_PATH}/comm/socket/getsocktcpinfo.cc
    ${MODULE_ROOT_PATH}/comm/socket/local_ipstack.cc
    ${MODULE_ROOT_PATH}/comm/socket/nat64_prefix_util.cc
    ${MODULE_ROOT_PATH}/comm/socket/socket_address.cc
    ${MODULE_ROOT_PATH}/comm/socket/tcpclient.cc
    ${MODULE_ROOT_PATH}/comm/socket/tcpclient_fsm.cc
    ${MODULE_ROOT_PATH}/comm/socket/tcpserver.cc
    ${MODULE_ROOT_PATH}/comm/socket/tcpserver_fsm.cc
    ${MODULE_ROOT_PATH}/comm/socket/udpclient.cc
    ${MODULE_ROOT_PATH}/comm/socket/udpserver.cc
    ${MODULE_ROOT_PATH}/comm/socket/unix_socket.cc
    ${MODULE_ROOT_PATH}/comm/strutil.cc
    ${MODULE_ROOT_PATH}/comm/tickcount.cc
    ${MODULE_ROOT_PATH}/comm/time_utils.c
    ${MODULE_ROOT_PATH}/comm/tinyxml2.cc
    ${MODULE_ROOT_PATH}/comm/unix/socket/socketbreaker.cc
    ${MODULE_ROOT_PATH}/comm/unix/socket/socketpoll.cc
    ${MODULE_ROOT_PATH}/comm/unix/socket/socketselect.cc
    ${MODULE_ROOT_PATH}/comm/xlogger/loginfo_extract.c
    ${MODULE_ROOT_PATH}/comm/xlogger/xloggerbase.c
    )

set (LOCAL_C_INCLUDES
   ${MODULE_ROOT_PATH}
   ${MODULE_ROOT_PATH}/../
   ${MODULE_ROOT_PATH}/../../
   ${MODULE_ROOT_PATH}/comm
   ${MODULE_ROOT_PATH}/comm/xlogger
   ${MODULE_ROOT_PATH}/comm/socket
   ${MODULE_ROOT_PATH}/comm/unix
   ${MODULE_ROOT_PATH}/comm/network
   ${MODULE_ROOT_PATH}/boost/libs/
   ${MODULE_ROOT_PATH}/comm/messagequeue
   ${MODULE_ROOT_PATH}/comm/jni
   ${MODULE_ROOT_PATH}/boost/libs
   ${MODULE_ROOT_PATH}/boost/
   ${MODULE_ROOT_PATH}/boost/libs/context/src/asm/
	)


include_directories(${LOCAL_C_INCLUDES})


add_library(${LOCAL_MODULE}
            STATIC
            ${LOCAL_SRC_FILES}
            )

target_link_libraries(${LOCAL_MODULE}
                       log
                       atomic
                       cpufeatures
                       android
                       )
target_include_directories(${LOCAL_MODULE} PRIVATE
                           ${ANDROID_NDK}/sources/android/native_app_glue)

