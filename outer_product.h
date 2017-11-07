#pragma once

typedef float Float4 __attribute__((ext_vector_type(4)));

inline __device__ void outerProduct4x4(Float4 &a, Float4 &b, Float4 &c0, Float4 &c1, Float4 &c2, Float4 &c3) {
  asm volatile(
      "\n \
      v_mac_f32 %0, %4, %5 \n \
      v_mac_f32 %1, %4, %6 \n \
      v_mac_f32 %2, %4, %7 \n \
      v_mac_f32 %3, %4, %8 \n \
      "
      :"=v"(c0.x),"=v"(c0.y),"=v"(c0.z),"=v"(c0.w)
      :"v"(a.x),"v"(b.x),"v"(b.y),"v"(b.z),"v"(b.w)
  );
  asm volatile(
      "\n \
      v_mac_f32 %0, %4, %5 \n \
      v_mac_f32 %1, %4, %6 \n \
      v_mac_f32 %2, %4, %7 \n \
      v_mac_f32 %3, %4, %8 \n \
      "
      :"=v"(c0.x),"=v"(c0.y),"=v"(c0.z),"=v"(c0.w)
      :"v"(a.y),"v"(b.x),"v"(b.y),"v"(b.z),"v"(b.w)
  );
  asm volatile(
      "\n \
      v_mac_f32 %0, %4, %5 \n \
      v_mac_f32 %1, %4, %6 \n \
      v_mac_f32 %2, %4, %7 \n \
      v_mac_f32 %3, %4, %8 \n \
      "
      :"=v"(c0.x),"=v"(c0.y),"=v"(c0.z),"=v"(c0.w)
      :"v"(a.z),"v"(b.x),"v"(b.y),"v"(b.z),"v"(b.w)
  );
  asm volatile(
      "\n \
      v_mac_f32 %0, %4, %5 \n \
      v_mac_f32 %1, %4, %6 \n \
      v_mac_f32 %2, %4, %7 \n \
      v_mac_f32 %3, %4, %8 \n \
      "
      :"=v"(c0.x),"=v"(c0.y),"=v"(c0.z),"=v"(c0.w)
      :"v"(a.w),"v"(b.x),"v"(b.y),"v"(b.z),"v"(b.w)
  );
}