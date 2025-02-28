#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <assert.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

int printf(const char* fmt, ...);
//void ecall_encrypt([in, string] const char* input, [out, string] char* output, size_t len);

#if defined(__cplusplus)
}
#endif

#endif /* !_ENCLAVE_H_ */