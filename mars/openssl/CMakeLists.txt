cmake_minimum_required(VERSION 3.4.1)
project(OPENSSL)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}      -fvisibility=hidden  -Wno-error=strict-aliasing -DUSING_XLOG_WEAK_FUNC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ")
set (MODULE_ROOT_PATH  ${CMAKE_CURRENT_LIST_DIR}/export)
set (LOCAL_MODULE  "openssl")

add_library(openssl_crypto STATIC IMPORTED )
set_target_properties(openssl_crypto PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/openssl_lib_android/libcrypto.a)



set (LOCAL_SRC_FILES
    ${MODULE_ROOT_PATH}/md5_digest.c
    ${MODULE_ROOT_PATH}/rsa_private_decrypt.c
    ${MODULE_ROOT_PATH}/ecdh_crypt.c
    ${MODULE_ROOT_PATH}/aes_crypt.c
    ${MODULE_ROOT_PATH}/ecdsa_verify.c
    ${MODULE_ROOT_PATH}/crypto/rsa_pss_sha256.cpp
    ${MODULE_ROOT_PATH}/crypto/openssl_multi_thread_support.cpp
    ${MODULE_ROOT_PATH}/crypto/gen_rsa_key_pair.cpp
    ${MODULE_ROOT_PATH}/crypto/test_rsa_pss_sha256.cpp
    ${MODULE_ROOT_PATH}/crypto/ecdh_util.cpp
    ${MODULE_ROOT_PATH}/crypto/pay_openssl_crypto_util.cpp
    ${MODULE_ROOT_PATH}/crypto/rsa_crypt.cpp
	)

set (LOCAL_C_INCLUDES
	${MODULE_ROOT_PATH}/
	${CMAKE_CURRENT_LIST_DIR}/export_include
	${CMAKE_CURRENT_LIST_DIR}/include
	${MODULE_ROOT_PATH}/../
	${MODULE_ROOT_PATH}/../../
	${MODULE_ROOT_PATH}/../../../
	${MODULE_ROOT_PATH}/../../comm
	${MODULE_ROOT_PATH}/crypto
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
                       openssl_crypto
                       )