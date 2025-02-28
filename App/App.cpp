#include <stdio.h>
#include <assert.h>
#include <sstream>
#include <cctype>
# include <unistd.h>
# include <pwd.h>
#include <sys/resource.h>
#include <iostream>
#include <omp.h>
#include <random>
# define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

#include "Server.h"
#include "FileProcessor.h"
#include <pbc/pbc.h>

size_t GT_SIZE = 0;
size_t ZR_SIZE = 0;
size_t G1_SIZE = 0;
size_t G2_SIZE = 0;


/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
    {
        SGX_ERROR_MEMORY_MAP_FAILURE,
        "Failed to reserve memory for the enclave.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
    	printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s", str);
}


// 把 std::string 转换为 unsigned char*
unsigned char* convertStringToUnsignedChar(std::string& str) {
    size_t len = str.size();
    unsigned char* buffer = (unsigned char*) malloc(len + 1);
    if (!buffer) {
        std::cerr << "Memory allocation failed" << std::endl;
        return nullptr;
    }
    memcpy(buffer, str.c_str(), len);
    buffer[len] = '\0';  // 确保以 NULL 结尾
    return buffer;
}


void get_pairing()
{
    pairing_t pairing; // 定义全局 pairing 变量

    char param_str[] = "type a   \
q 40132934874065581357639239301938089130039744463472639389591743372055069245229811691989086088125328594220615378634210894681862132537783020759006156856256486760853214375759294871087842511098137328669742901944483362556170153388301818039153709502326627974456159915072198235053718093631308481607762634120235579251 \
h 5986502056676971303894401875152023968506744561211054886102595589603460071084910131070137261543726329935522867827513637124526300709663875599084261056444276 \
r 6703903964971298549787012499102923063739682910296196688861780721860882015036773488400937149083451713845015929093243025426876941560715789883889358865432577 \
exp2 511  \
exp1 87   \
sign1 1  \
sign0 1";

    // 初始化pbc_param_t
    pbc_param_t par;
    pbc_param_init_set_str(par, param_str);
    pairing_init_pbc_param(pairing, par);

    printf("SET_STATIC_SIZE --- \n");
    element_t zr, g1, g2, gt;
    element_init_Zr(zr, pairing);
    element_init_G1(g1, pairing);
    element_init_G2(g2, pairing);
    element_init_GT(gt, pairing);
    element_random(zr);
    element_random(g1);
    element_random(g2);
    element_random(gt);
    ZR_SIZE = element_length_in_bytes(zr);
    G1_SIZE = element_length_in_bytes(g1);
    G2_SIZE = element_length_in_bytes(g2);
    GT_SIZE = element_length_in_bytes(gt);
}
/* Application entry */
int SGX_CDECL main(int argc, char *argv[]) {
    (void)(argc);
    (void)(argv);

    if (initialize_enclave() < 0) {
        printf("Enclave initialization failed.\n");
        return -1;
    }

    //Server server("http://0.0.0.0:8080/Transform2");
    //server.start();

    std::string PTC_str, HK_str;
    size_t PTC_len, HK_len, C1_len,TC_len;
    
    FileProcessor::processFile("Transform2_received.txt", PTC_str, PTC_len, HK_str, HK_len, C1_len);

    //std::cout << "PTC_str: " << PTC_str << std::endl;
    std::cout << "PTC_len: " << PTC_len << std::endl;
    //std::cout << "HK_str: " << HK_str << std::endl;
    std::cout << "HK_len: " << HK_len << std::endl;
    std::cout << "C1_len: " << C1_len << std::endl;
   

    // 调用 get_pairing 函数初始化 pairing
    get_pairing();

    // 打印 GT_SIZE
    cout << "GT_SIZE: " << GT_SIZE << endl;

    // 调用 Enclave 中的函数
    // 转换 std::string 为 unsigned char*
    // unsigned char* PTC_buf = convertStringToUnsignedChar(PTC_str);
    // unsigned char* HK_buf = convertStringToUnsignedChar(HK_str);
    // //unsigned char* TC_buf = (unsigned char*) malloc(PTC_len + HK_len + 1);  // 预分配

    // if (!PTC_buf || !HK_buf || !TC_buf) {
    //     std::cerr << "Memory allocation failed." << std::endl;
    //     return -1;
    // }

    // 调用 SGX Enclave
    // sgx_status_t ret = transform2(global_eid, &TC_buf, &TC_len, &HK_buf, HK_len, &PTC_buf, PTC_len);
    // if (ret != SGX_SUCCESS) {
    //     printf("Error: Transform2 failed.\n");
    //     return -1;
    // }

    // // 释放分配的内存
    // free(PTC_buf);
    // free(HK_buf);
    // free(TC_buf);

   

    sgx_destroy_enclave(global_eid);
    printf("Enclave destroyed.\n");

    return 0;
}