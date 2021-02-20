#ifndef B3B03750_263B_4E66_AC1D_8B942729270C
#define B3B03750_263B_4E66_AC1D_8B942729270C

#include "core.h"
#include "typedefs.h"

namespace xofo {

struct Camera {
  f32x4x4 prj;
  f32x4x4 ori;
  f32x4 pos;
  f32x4x4 view;
  f32x4 mouse_ray;
  Camera(f32 x, f32 y) : ori(1), pos(0, 0, 0, 1), mouse_ray(0, 0, 1, 0) { set_prj(x, y); }


  void set_prj(f32 x, f32 y) {
    f32 fov = 90 * RADIAN;
    prj = perspective(fov, fov * y / x, 0.001f, 400.f);
  }

  void update(f32x2 md, f32 fwd, f32 lr, f32 dt) {

    pos = (pos + ori[2] * fwd * dt + ori[0] * lr * dt);
    ori = ori * axis_angle(ori[0].xyz, md.y) * axis_angle(f32x3(0, 1, 0), md.x);

    view = ori.transpose();
    view[3] = -pos * view;
    view[3].w = 1;

    mouse_ray = norm(f32x4(mouse_norm() / f32x2(prj[0][0], prj[1][1]), -1, 0) * ori);

  }

  f32x4 right() const { return ori[0]; }
  f32x4 forward() const { return ori[2]; }
  f32x4 up() const { return ori[1]; }
};
}  // namespace xofo
#endif /* B3B03750_263B_4E66_AC1D_8B942729270C */
