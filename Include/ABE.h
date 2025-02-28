#ifndef ABE_H
#define ABE_H

#include <pbc/pbc.h>

// 声明外部全局变量 GT_SIZE（或其他需要的大小）
extern size_t GT_SIZE;
extern size_t ZR_SIZE;
extern size_t G1_SIZE;
extern size_t G2_SIZE;

// 声明函数
void SETSTATICSIZE(pairing_t pairing);
void get_pairing();

// 其他需要的声明...

#endif // ABE_H
