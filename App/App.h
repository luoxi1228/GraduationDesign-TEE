#ifndef _APP_H_
#define _APP_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "sgx_error.h"       /* sgx_status_t */
#include "sgx_eid.h"     /* sgx_enclave_id_t */
#include "pbc/pbc.h"

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

# define ENCLAVE_FILENAME "enclave.signed.so"

extern sgx_enclave_id_t global_eid;    /* global enclave id */

#if defined(__cplusplus)
extern "C" {
#endif
//放函数声
extern size_t ZR_SIZE;
extern size_t G1_SIZE;
extern size_t G2_SIZE;
extern size_t GT_SIZE;
extern size_t C1_len;
void get_pairing(pairing_t pairing);    

#if defined(__cplusplus)
}
#endif

#endif /* !_APP_H_ */
