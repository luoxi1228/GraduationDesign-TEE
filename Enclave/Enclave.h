#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <assert.h>
#include <stdlib.h>
#include <cstdio>
#if defined(__cplusplus)
extern "C" {
#endif

//添加函数声明
int printf(const char* fmt, ...);
extern size_t ZR_SIZE;
extern size_t G1_SIZE;
extern size_t G2_SIZE;
extern size_t GT_SIZE;
extern size_t cipher_C1_size;
extern size_t ptc_C1_size;
extern size_t tc_T1_size;

#if defined(__cplusplus)
}

#endif

#endif /* !_ENCLAVE_H_ */
