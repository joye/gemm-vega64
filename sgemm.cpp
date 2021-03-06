#include <iostream>
#include <hip/hip_runtime.h>
#include <hip/hip_runtime_api.h>
#include "outer_product.h"
#include "global_ops.h"
#include "shared_ops.h"
#include "dims.h"
#include <fstream>

__global__ void SGEMM(Float4 *A, Float4 *B, Float4 *C, Float4 *buffA, Float4 *buffB) {
  int tx = hipThreadIdx_x;
  int ty = hipThreadIdx_y;
  int bx = hipBlockIdx_x;
  int by = hipBlockIdx_y;

/**
* Initial order of instructions
* 1. Calculate A and B load indices
* 2. Load A and B into registers
* 3. Calculate C load indices
* 4. Load C to c[16]
* 5. Calculate lds read and write indices for A and B (this can move to point 3)
* 6. Wait for A and B loads
* 7. Write loaded A (a) and B (b) into lds
* 8. Wait for writes to lds
* 9. Read LDS to a0, a1, b0, b1
* 10. Wait for C
* 11. Wait for LDS reads of a0, a1, b0, b1
* 12. Operate on c[16], a0, a1, b0, b1
* 13. Write c[16] to C
*/


  Float4 a0, a1, a2, a3, b0, b1, b2, b3;
  Float4 rA, rB;
  Float4 c[16];

  int a_global_id = tx + (ty % 2) * 16 + (ty / 2) * dim_x4 + bx * 32 + by * 8192;
  int b_global_id = tx + (ty % 2) * 16 + (ty / 2) * dim_x4 + bx * 32 + by * 8192;

  global_load(A, rA, a_global_id);
  global_load(B, rB, b_global_id);
 
  int c0_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + dim_x4*0;
  int c1_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + dim_x4*1;
  int c2_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + dim_x4*2;
  int c3_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + dim_x4*3;

  int c4_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + dim_x4*0 + half_tilex4;
  int c5_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + dim_x4*1 + half_tilex4;
  int c6_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + dim_x4*2 + half_tilex4;
  int c7_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + dim_x4*3 + half_tilex4;

  int c8_id  = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + half_tile * dim_x4 + dim_x4*0;
  int c9_id  = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + half_tile * dim_x4 + dim_x4*1;
  int c10_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + half_tile * dim_x4 + dim_x4*2;
  int c11_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + half_tile * dim_x4 + dim_x4*3;

  int c12_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + half_tile * dim_x4 + half_tilex4 + dim_x4*0;
  int c13_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + half_tile * dim_x4 + half_tilex4 + dim_x4*1;
  int c14_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + half_tile * dim_x4 + half_tilex4 + dim_x4*2;
  int c15_id = tx + ty * dim_x + bx * tilex4 + by * dim_x4 * tile + half_tile * dim_x4 + half_tilex4 + dim_x4*3;

  global_load(C, c[0], c0_id);
  global_load(C, c[1], c1_id);
  global_load(C, c[2], c2_id);
  global_load(C, c[3], c3_id);

  global_load(C, c[4], c4_id);
  global_load(C, c[5], c5_id);
  global_load(C, c[6], c6_id);
  global_load(C, c[7], c7_id);

  global_load(C, c[8], c8_id);
  global_load(C, c[9], c9_id);
  global_load(C, c[10], c10_id);
  global_load(C, c[11], c11_id);

  global_load(C, c[12], c12_id);
  global_load(C, c[13], c13_id);
  global_load(C, c[14], c14_id);
  global_load(C, c[15], c15_id);

  int a_shared_id = (tx + (ty % 2) * 16 + (ty / 2) * 32);
  int b_shared_id = (tx + (ty % 2) * 16 + (ty / 2) * 32);

/*
  uint32_t redA_read_id0 = tx;
  uint32_t redA_read_id1 = (tx + 16);
  uint32_t redB_read_id0 = ty;
  uint32_t redB_read_id1 = (ty + 16);

  __shared__ Float4 sA[256];
  __shared__ Float4 sB[256];
  sA[a_shared_id] = a;
  sB[b_shared_id] = b;

  a0 = sA[redA_read_id0];
  a1 = sA[redA_read_id1];

  b0 = sB[redB_read_id0];
  b1 = sB[redB_read_id1];
*/

  uint32_t redA, redB, blueA, blueB;
  shared_init(redA, redB, blueA, blueB);

  uint32_t redA_write_id = redA + a_shared_id*16;
  uint32_t redB_write_id = redB + b_shared_id*16;
  uint32_t blueA_write_id = blueA + a_shared_id*16;
  uint32_t blueB_write_id = blueB + a_shared_id*16;

  uint32_t redA_read_id0 = redA + tx * 16;
  uint32_t redA_read_id1 = redA + (tx + 16) * 16;
  uint32_t redB_read_id0 = redB + ty * 16;
  uint32_t redB_read_id1 = redB + (ty + 16) * 16;

  uint32_t blueA_read_id0 = blueA + tx * 16;
  uint32_t blueA_read_id1 = blueA + (tx + 16) * 16;
  uint32_t blueB_read_id0 = blueB + tx * 16;
  uint32_t blueB_read_id1 = blueB + (tx + 16) * 16;

/**
* Wait for A and B
  As 16 is too high, wait for C too
  vmcnt<0>();
*/
  vmcnt<0>();

  shared_write_b128(rA, redA_write_id);
  shared_write_b128(rB, redB_write_id);
  lgkmcnt<0>();

  shared_read_b128(a0, redA_read_id0);
  shared_read_b128(a1, redA_read_id1);
  shared_read_b128(b0, redB_read_id0);
  shared_read_b128(b1, redB_read_id1);
  lgkmcnt<0>();

  outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
  outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
  outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
  outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

  redA_read_id0 += 512;
  redA_read_id1 += 512;
  redB_read_id0 += 512;
  redB_read_id1 += 512;

  shared_read_b128(a0, redA_read_id0);
  shared_read_b128(a1, redA_read_id1);
  shared_read_b128(b0, redB_read_id0);
  shared_read_b128(b1, redB_read_id1);
  lgkmcnt<0>();

  outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
  outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
  outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
  outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

  redA_read_id0 += 512;
  redA_read_id1 += 512;
  redB_read_id0 += 512;
  redB_read_id1 += 512;

  shared_read_b128(a0, redA_read_id0);
  shared_read_b128(a1, redA_read_id1);
  shared_read_b128(b0, redB_read_id0);
  shared_read_b128(b1, redB_read_id1);
  lgkmcnt<0>();

  outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
  outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
  outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
  outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);


/**
Prefetch to blue lds
*/

/**
* Unroll loop for unroll_factor
* Do global_load 
* Do lds_write
*/

/**
* global to registers
* registers to lds
* read i0
* loop starts
* Do global to register (for next iter)
* do iter0
* read iter2
* do iter1
* read iter3
* do iter2
* ....
* read iter6
* do iter5
* vmcnt() (for next iter)
* registers to lds (for next iter)
* read iter7
* do iter6
* lgkmcnt<>(); (wait for registers to lds)
* read iter 0
* do iter7
*/
/*
shared_write_b128(rA, redA_write_id);
shared_write_b128(rB, redB_write_id);
lgkmcnt<0>();
shared_read_b128(a0, redA_read_id0);
shared_read_b128(a1, redA_read_id1);
shared_read_b128(b0, redB_read_id0);
shared_read_b128(b1, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;
shared_read_b128(a2, redA_read_id0);
shared_read_b128(a3, redA_read_id1);
shared_read_b128(b2, redB_read_id0);
shared_read_b128(b3, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;

for(int i=0;i<2;i++) {

a_global_id += 8*dim_x4;
b_global_id += 8*dim_x4;

global_load(A, rA, a_global_id);
global_load(B, rB, b_global_id);
lgkmcnt<4>();

//outerproduct(a0, a1, b0, b1);
outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

// shared_read_b128(a0, a1, b0, b1, redA, redB);
shared_read_b128(a0, redA_read_id0);
shared_read_b128(a1, redA_read_id1);
shared_read_b128(b0, redB_read_id0);
shared_read_b128(b1, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;
lgkmcnt<4>();

//outerproduct(a2, a3, b2, b3);
outerProduct4x4(a2, b2, c[0], c[1], c[2], c[3]);
outerProduct4x4(a2, b3, c[4], c[5], c[6], c[7]);
outerProduct4x4(a3, b2, c[8], c[9], c[10], c[11]);
outerProduct4x4(a3, b3, c[12], c[13], c[14], c[15]);

//shared_read_b128(a2, a3, b2, b3, redA, redB);
shared_read_b128(a2, redA_read_id0);
shared_read_b128(a3, redA_read_id1);
shared_read_b128(b2, redB_read_id0);
shared_read_b128(b3, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;
lgkmcnt<4>();
//outerproduct(a0, a1, b0, b1);
outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

//shared_read_b128(a0, a1, b0, b1, redA, redB);
shared_read_b128(a0, redA_read_id0);
shared_read_b128(a1, redA_read_id1);
shared_read_b128(b0, redB_read_id0);
shared_read_b128(b1, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;
lgkmcnt<4>();
//outerproduct(a2, a3, b2, b3);
outerProduct4x4(a2, b2, c[0], c[1], c[2], c[3]);
outerProduct4x4(a2, b3, c[4], c[5], c[6], c[7]);
outerProduct4x4(a3, b2, c[8], c[9], c[10], c[11]);
outerProduct4x4(a3, b3, c[12], c[13], c[14], c[15]);

//shared_read_b128(a2, a3, b2, b3, redA, redB);
shared_read_b128(a2, redA_read_id0);
shared_read_b128(a3, redA_read_id1);
shared_read_b128(b2, redB_read_id0);
shared_read_b128(b3, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;
lgkmcnt<4>();
//outerproduct(a0, a1, b0, b1);
outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

//shared_read_b128(a0, a1, b0, b1, redA, redB);
shared_read_b128(a0, redA_read_id0);
shared_read_b128(a1, redA_read_id1);
shared_read_b128(b0, redB_read_id0);
shared_read_b128(b1, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;
lgkmcnt<4>();
//outerproduct(a2, a3, b2, b3);
outerProduct4x4(a2, b2, c[0], c[1], c[2], c[3]);
outerProduct4x4(a2, b3, c[4], c[5], c[6], c[7]);
outerProduct4x4(a3, b2, c[8], c[9], c[10], c[11]);
outerProduct4x4(a3, b3, c[12], c[13], c[14], c[15]);

//shared_read_b128(a2, a3, b2, b3, redA, redB);
shared_read_b128(a2, redA_read_id0);
shared_read_b128(a3, redA_read_id1);
shared_read_b128(b2, redB_read_id0);
shared_read_b128(b3, redB_read_id1);
redA_read_id0 = redA + tx *16;
redA_read_id1 = redA + (tx + 16) * 16;
redB_read_id0 = redB + tx * 16;
redB_read_id1 = redB + (tx + 16) * 16;
vmcnt<0>();
shared_write_b128(rA, blueA);
shared_write_b128(rB, blueB);
lgkmcnt<6>();
//outerproduct(a0, a1, b0, b1);
outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

//shared_read_b128(a0, a1, b0, b1, blueA, blueB);
shared_read_b128(a0, blueA_read_id0);
shared_read_b128(a1, blueA_read_id1);
shared_read_b128(b0, blueB_read_id0);
shared_read_b128(b1, blueB_read_id1);
blueA_read_id0 += 512;
blueA_read_id1 += 512;
blueB_read_id0 += 512;
blueB_read_id1 += 512;

lgkmcnt<4>();
//outerproduct(a2, a3, b2, b3);
outerProduct4x4(a2, b2, c[0], c[1], c[2], c[3]);
outerProduct4x4(a2, b3, c[4], c[5], c[6], c[7]);
outerProduct4x4(a3, b2, c[8], c[9], c[10], c[11]);
outerProduct4x4(a3, b3, c[12], c[13], c[14], c[15]);

//shared_read_b128(a2, a3, b2, b3, blueA, blueB);
shared_read_b128(a2, blueA_read_id0);
shared_read_b128(a3, blueA_read_id1);
shared_read_b128(b2, blueB_read_id0);
shared_read_b128(b3, blueB_read_id1);
blueA_read_id0 += 512;
blueA_read_id1 += 512;
blueB_read_id0 += 512;
blueB_read_id1 += 512;

a_global_id += 8 * dim_x4;
b_global_id += 8 * dim_x4;

global_load(A, rA, a_global_id);
global_load(B, rB, b_global_id);

lgkmcnt<4>();
//outerproduct(a0, a1, b0, b1);
outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

//shared_read_b128(a0, a1, b0, b1, blueA, blueB);
shared_read_b128(a0, blueA_read_id0);
shared_read_b128(a1, blueA_read_id1);
shared_read_b128(b0, blueB_read_id0);
shared_read_b128(b1, blueB_read_id1);
blueA_read_id0 += 512;
blueA_read_id1 += 512;
blueB_read_id0 += 512;
blueB_read_id1 += 512;

lgkmcnt<4>();
//outerproduct(a2, a3, b2, b3);
outerProduct4x4(a2, b2, c[0], c[1], c[2], c[3]);
outerProduct4x4(a2, b3, c[4], c[5], c[6], c[7]);
outerProduct4x4(a3, b2, c[8], c[9], c[10], c[11]);
outerProduct4x4(a3, b3, c[12], c[13], c[14], c[15]);

//shared_read_b128(a2, a3, b2, b3, blueA, blueB);
shared_read_b128(a2, blueA_read_id0);
shared_read_b128(a3, blueA_read_id1);
shared_read_b128(b2, blueB_read_id0);
shared_read_b128(b3, blueB_read_id1);
blueA_read_id0 += 512;
blueA_read_id1 += 512;
blueB_read_id0 += 512;
blueB_read_id1 += 512;

lgkmcnt<4>();
//outerproduct(a0, a1, b0, b1);
outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

//shared_read_b128(a0, a1, b0, b1, blueA, blueB);
shared_read_b128(a0, blueA_read_id0);
shared_read_b128(a1, blueA_read_id1);
shared_read_b128(b0, blueB_read_id0);
shared_read_b128(b1, blueB_read_id1);
blueA_read_id0 += 512;
blueA_read_id1 += 512;
blueB_read_id0 += 512;
blueB_read_id1 += 512;

lgkmcnt<4>();
//outerproduct(a2, a3, b2, b3);
outerProduct4x4(a2, b2, c[0], c[1], c[2], c[3]);
outerProduct4x4(a2, b3, c[4], c[5], c[6], c[7]);
outerProduct4x4(a3, b2, c[8], c[9], c[10], c[11]);
outerProduct4x4(a3, b3, c[12], c[13], c[14], c[15]);

//shared_read_b128(a2, a3, b2, b3, blueA, blueB);
shared_read_b128(a2, blueA_read_id0);
shared_read_b128(a3, blueA_read_id1);
shared_read_b128(b2, blueB_read_id0);
shared_read_b128(b3, blueB_read_id1);
blueA_read_id0 += 512;
blueA_read_id1 += 512;
blueB_read_id0 += 512;
blueB_read_id1 += 512;

lgkmcnt<4>();
//outerproduct(a0, a1, b0, b1);
outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

//shared_read_b128(a0, a1, b0, b1, blueA, blueB);
shared_read_b128(a0, blueA_read_id0);
shared_read_b128(a1, blueA_read_id1);
shared_read_b128(b0, blueB_read_id0);
shared_read_b128(b1, blueB_read_id1);
blueA_read_id0 += 512;
blueA_read_id1 += 512;
blueB_read_id0 += 512;
blueB_read_id1 += 512;

lgkmcnt<4>();
//outerproduct(a2, a3, b2, b3);
outerProduct4x4(a2, b2, c[0], c[1], c[2], c[3]);
outerProduct4x4(a2, b3, c[4], c[5], c[6], c[7]);
outerProduct4x4(a3, b2, c[8], c[9], c[10], c[11]);
outerProduct4x4(a3, b3, c[12], c[13], c[14], c[15]);

//shared_read_b128(a2, a3, b2, b3, blueA, blueB);
shared_read_b128(a2, blueA_read_id0);
shared_read_b128(a3, blueA_read_id1);
shared_read_b128(b2, blueB_read_id0);
shared_read_b128(b3, blueB_read_id1);
blueA_read_id0 = blueA + tx * 16;
blueA_read_id1 = blueA + (tx + 16) * 16;
blueB_read_id0 = blueB + tx * 16;
blueB_read_id1 = blueB + (tx + 16) * 16;

vmcnt<0>();
shared_write_b128(rA, redA);
shared_write_b128(rB, redB);
lgkmcnt<6>();
//outerproduct(a0, a1, b0, b1);
outerProduct4x4(a0, b0, c[0], c[1], c[2], c[3]);
outerProduct4x4(a0, b1, c[4], c[5], c[6], c[7]);
outerProduct4x4(a1, b0, c[8], c[9], c[10], c[11]);
outerProduct4x4(a1, b1, c[12], c[13], c[14], c[15]);

//shared_read_b128(a0, a1, b0, b1, redA, redB);
shared_read_b128(a0, redA_read_id0);
shared_read_b128(a1, redA_read_id1);
shared_read_b128(b0, redB_read_id0);
shared_read_b128(b1, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;

lgkmcnt<4>();
//outerproduct(a2, a3, b2, b3);
outerProduct4x4(a2, b2, c[0], c[1], c[2], c[3]);
outerProduct4x4(a2, b3, c[4], c[5], c[6], c[7]);
outerProduct4x4(a3, b2, c[8], c[9], c[10], c[11]);
outerProduct4x4(a3, b3, c[12], c[13], c[14], c[15]);

//shared_read_b128(a2, a3, b2, b3, redA, redB);
shared_read_b128(a0, redA_read_id0);
shared_read_b128(a1, redA_read_id1);
shared_read_b128(b0, redB_read_id0);
shared_read_b128(b1, redB_read_id1);
redA_read_id0 += 512;
redA_read_id1 += 512;
redB_read_id0 += 512;
redB_read_id1 += 512;
}
*/

  global_store(C, c[0], c0_id);
  global_store(C, c[1], c1_id);
  global_store(C, c[2], c2_id);
  global_store(C, c[3], c3_id);

  global_store(C, c[4], c4_id);
  global_store(C, c[5], c5_id);
  global_store(C, c[6], c6_id);
  global_store(C, c[7], c7_id);

  global_store(C, c[8], c8_id);
  global_store(C, c[9], c9_id);
  global_store(C, c[10], c10_id);
  global_store(C, c[11], c11_id);

  global_store(C, c[12], c12_id);
  global_store(C, c[13], c13_id);
  global_store(C, c[14], c14_id);
  global_store(C, c[15], c15_id);

  buffA[c0_id] = a0;
  buffB[c0_id] = b0;

}


int main() {
  hipSetDevice(1);
  std::vector<Float4> a(dim_x4*dim_y), b(dim_x4*dim_y), c(dim_x4*dim_y);
  std::fill(c.begin(), c.end(), 0.0f);
  float *_a = reinterpret_cast<float*>(a.data());
  float *_b = reinterpret_cast<float*>(b.data());
  float *_c = reinterpret_cast<float*>(c.data());
  for(int j=0;j<dim_y;j++) {
    for(int i=0;i<dim_x;i++) {
      _a[i + j *dim_x] = (i + j * dim_x)*1.0f + 1.0f;
      if(i == j) {
        _b[i + j * dim_x] = 1.0f;
      } else {
        _b[i + j * dim_x] = 0.0f;
      }
    }
  }
  Float4 *Ad, *Bd, *Cd;
  Float4 *buffA, *buffB;
  hipHostMalloc(&buffA, size);
  hipHostMalloc(&buffB, size);
  hipMalloc(&Ad, size);
  hipMalloc(&Bd, size);
  hipMalloc(&Cd, size);
  hipMemcpy(Ad, a.data(), size, hipMemcpyHostToDevice);
  hipMemcpy(Bd, b.data(), size, hipMemcpyHostToDevice);
  hipMemcpy(Cd, c.data(), size, hipMemcpyHostToDevice);
  auto start = std::chrono::high_resolution_clock::now();
  hipLaunchKernelGGL(SGEMM, dim3(dim_x4/(2*16),dim_y4/(2*16),1), dim3(16,16,1), 4*sizeof(float)*8*128, 0, Ad, Bd, Cd, buffA, buffB);
  hipDeviceSynchronize();
  auto stop = std::chrono::high_resolution_clock::now();
  double sec = std::chrono::duration_cast<std::chrono::duration<double>>(stop - start).count();
  std::cout<<sec<<std::endl;
  double flops = (double)(4096)*(double)(4096)*(double)(4096);;
  std::cout<<"Throughput: "<<flops/1.0E9<<std::endl;
  hipMemcpy(c.data(), Cd, size, hipMemcpyDeviceToHost);
for(int i=0;i<8;i++) {
  std::cout<<buffA[i].x<<" "<<buffB[i].x<<std::endl;
  std::cout<<buffA[i].y<<" "<<buffB[i].y<<std::endl;
  std::cout<<buffA[i].z<<" "<<buffB[i].z<<std::endl;
  std::cout<<buffA[i].w<<" "<<buffB[i].w<<std::endl;
}
    std::ofstream outfile;
    outfile.open("outfile.txt");
  for(int j=0;j<dim_y;j++) {
    for(int i=0;i<dim_x;i++) {
        outfile << _c[j+i*dim_y] <<" ";
/*
      if(_c[j+i*dim_y] - float(i+j*dim_x)+1.0f > 0.1f) {
        std::cout<<"Bad Output at: "<<i<<" "<<j<<" got: "<<_c[i+j*dim_x]<<" expected: "<<float(i+j*dim_x)+1.0f<<std::endl;
        return 0;
      }
*/
    }
        outfile <<"\n\n\n";
  }
  outfile.close();
  std::cout<<c[10].x<<std::endl;
}
