#ifndef B3B03750_263B_4E66_AC1D_8B942729270C
#define B3B03750_263B_4E66_AC1D_8B942729270C

#include "core.h"
#include "typedefs.h"

namespace xofo {

struct Camera {
  mat prj;
  mat ori;
  vec4 pos;
  mat view;
  vec4 mouse_ray;
  Camera(f32 x, f32 y) : ori(1), pos(0, 0, 0, 1), mouse_ray(0, 0, 1, 0) { set_prj(x, y); }


  void set_prj(f32 x, f32 y) {
    f32 fov = 90 * RADIAN;
    prj = perspective(fov, fov * y / x, 0.001, 400);
  }

  void update(vec2 md, f32 fwd, f32 lr, f32 dt) {

    pos = (pos + ori[2] * fwd * dt + ori[0] * lr * dt);
    ori = ori * angax(md.y, ori[0].xyz) * angax(md.x, vec3(0, 1, 0));

    view = transpose(ori);
    view[3] = -pos * view;
    view[3].w = 1;

    mouse_ray = mango::normalize(vec4(mouse_norm() / vec2(prj[0][0], prj[1][1]), -1, 0) * ori);

  }

  vec4 right() const { return ori[0]; }
  vec4 forward() const { return ori[2]; }
  vec4 up() const { return ori[1]; }
};
}  // namespace xofo
#endif /* B3B03750_263B_4E66_AC1D_8B942729270C */
