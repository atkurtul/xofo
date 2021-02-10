#ifndef B3B03750_263B_4E66_AC1D_8B942729270C
#define B3B03750_263B_4E66_AC1D_8B942729270C

#include <typedefs.h>
#include <stdio.h>
#include <mango/math/matrix_float4x4.hpp>
#include <mango/math/quaternion.hpp>

struct Camera {
  mat prj;
  mat ori;
  vec4 pos;

  Camera(f32 x, f32 y) : ori(1), pos(ori[3]) {
    set_prj(x,y);
  }
  
  void set_prj(f32 x, f32 y) {
    f32 fov = 90 * RADIAN;
    prj = mango::vulkan::perspective(fov, fov * y / x, 0.01, 4000);
  }

  mat update(vec2 md, f32 fwd, f32 lr) {
    pos = (pos + ori[2] * fwd * 0.0005f + ori[0] * lr * 0.0005f);
    ori = ori * mango::AngleAxis(md.y, ori[0].xyz) *
    
    mango::AngleAxis(md.x, vec3(0,1,0));
    
    mat re = mango::transpose(ori);
    re[3] = -pos * re;
    re[3].w  = 1;

    return re * prj;
  }
};

#endif /* B3B03750_263B_4E66_AC1D_8B942729270C */
