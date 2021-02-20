#include "core.h"
#include <camera.h>

using namespace xofo;

void Camera::update(f32x2 md, f32 fwd, f32 lr, f32 dt) {
  pos = (pos + ori[2] * fwd * dt + ori[0] * lr * dt);
  ori = ori * axis_angle(ori[0].xyz, md.y) * axis_angle(f32x3(0, 1, 0), md.x);

  view = transpose(ori);
  view[3] = -pos * view;
  view[3].w = 1;

  // auto tmp = mouse_norm() / f32x2(prj[0][0], prj[1][1]);
  // mouse_ray = norm(f32x4(tmp , -1, 1) * ori);

  auto n = f32x4(mouse_norm() * 1.5f, -1, 1) * inverse(prj);

  mouse_ray = f32x4(n.xy, -1, 0) * ori;
}