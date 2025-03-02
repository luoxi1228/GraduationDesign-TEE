#include "Enclave.h"
#include "Enclave_t.h" /* print_string */
#include <stdarg.h>
#include <stdio.h> /* vsnprintf */
#include <string.h>

#include "pbc_sgx/pbc.h"

size_t GT_SIZE = 0;
size_t ZR_SIZE = 0;
size_t G1_SIZE = 0;
size_t G2_SIZE = 0;
size_t C1_len = 0;

pairing_t pairing; // 定义全局 pairing 变量

int printf(const char* fmt, ...)
{
    char buf[BUFSIZ] = { '\0' };
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return (int)strnlen(buf, BUFSIZ - 1) + 1;
}
void printf_element(element_t G ,int addition){
    unsigned char buffer[512];
    int len=element_to_bytes(buffer,G);
    ocall_element_printf(buffer, len, addition);

}
void get_pairing()
{
    
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

// string -> struct HK
void deserl_HK(element_t& gamma_1 ,element_t& gamma_2, unsigned char* str, size_t str_count)
{

    //printf("HK de-serialization --- \n");
    unsigned char *pointer = str;
    element_from_bytes(gamma_1, pointer);
    pointer = pointer + ZR_SIZE;
    element_from_bytes(gamma_2, pointer);
    
}

// string -> struct PTC
void deserl_PTC(element_t& C0, unsigned char* C1, size_t C1_size, element_t& CP1, element_t& CP2, unsigned char* str, size_t str_count)
{
    unsigned char* pointer = str;
    memcpy(C1, pointer, C1_size);
    pointer += C1_size;
    element_from_bytes(C0, pointer);
    pointer += GT_SIZE;
    element_from_bytes(CP1, pointer);
    pointer += GT_SIZE;
    element_from_bytes(CP2, pointer);
}

// struct TC -> string
void serl_TC(unsigned char** str, size_t* str_count, element_t T0, unsigned char* T1, size_t T1_size, element_t T2)
{
    unsigned char* pointer = *str;
    memcpy(pointer, T1, T1_size);
    pointer += T1_size;
    element_to_bytes(pointer, T0);
    pointer += element_length_in_bytes(T0);
    element_to_bytes(pointer, T2);
}

void transform2(unsigned char** tc_str, size_t* tc_str_count,
                 unsigned char** hk_str, size_t* hk_str_count,
                 unsigned char** ptc_str, size_t* ptc_str_count)
{
    get_pairing();
    ocall_get_C1_len(&C1_len);

    element_t gamma_1, gamma_2;
    element_init_Zr(gamma_1, pairing);
    element_init_Zr(gamma_2, pairing);
    deserl_HK(gamma_1, gamma_2, *hk_str, *hk_str_count);

    element_t C0, CP1, CP2;
    size_t C1_len = 0;
    ocall_get_C1_len(&C1_len);   
    unsigned char C1[C1_len];
    element_init_GT(C0, pairing);
    element_init_GT(CP1, pairing);
    element_init_GT(CP2, pairing);
    deserl_PTC(C0, C1, C1_len, CP1, CP2, *ptc_str, *ptc_str_count);

    element_t T0, T2;
    unsigned char T1[C1_len];
    element_init_GT(T0, pairing);
    element_init_GT(T2, pairing);

    element_t result_1, result_2;
    element_init_GT(result_1, pairing);
    element_init_GT(result_2, pairing);
    element_pow_zn(result_1, CP1, gamma_1);
    element_pow_zn(result_2, CP2, gamma_2);
    // printf_element(result_1,3);
    // printf_element(result_2,3);

    element_set(T0, C0);
    memcpy(T1, C1, C1_len);
    element_set(T2, result_1);

    // 计算比对结果
    uint8_t reg = (element_cmp(result_1, result_2) == 0) ? 1 : 0;
    printf("Transform2 %s\n", reg ? "succeed!" : "failed!");

    // **先调用 serl_TC 进行序列化**
    serl_TC(tc_str, tc_str_count, T0, T1, C1_len, T2);

    // // **重新分配 tc_str 以增加 1 字节存储 reg**
    // *tc_str = (unsigned char*)realloc(*tc_str, *tc_str_count + 1);
    // if (!(*tc_str)) {
    //     printf("Memory reallocation for tc_str failed!\n");
    //     return;
    // }

    // // **将 reg 存入最后 1 字节**
    // (*tc_str)[*tc_str_count] = reg;

    // // **更新 tc_str_count**
    // *tc_str_count += 1;

    element_clear(gamma_1);
    element_clear(gamma_2);
    element_clear(C0);
    element_clear(CP1);
    element_clear(CP2);
    element_clear(T0);
    element_clear(T2);
    element_clear(result_1);
    element_clear(result_2);
}




