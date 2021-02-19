#include <xofo.h>
#include "imgui.h"

using namespace std;
using namespace xofo;

void draw_vec4(const char* name, vec4 v) {
  ImGui::Text("%s %5f %5f %5f %5f", name, (f32)v.x, (f32)v.y, (f32)v.z,
              (f32)v.w);
}

struct Xform {
  mat xform;
  vec3 vel;

  Xform(vec3 pos, vec3 vel) : xform(0.1), vel(vel) { xform.translate(pos); }

  mat const& integrate(f32 dt) {
    xform.translate(vel * dt);
    return xform;
  }
};

int main() {
  xofo::init();

  auto pipeline = Pipeline::mk("shaders/shader");

  auto skybox = Pipeline::mk("shaders/skybox", PipelineState{.depth_write = 0});

  CubeMap cube_map(*skybox);

  Model cube("models/cube.gltf", *pipeline);

  vector<Xform> xforms;

  auto grid_pipe = Pipeline::mk(
      "shaders/grid", PipelineState{.polygon = VK_POLYGON_MODE_LINE,
                                    .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                                    .line_width = 1.2f});

  auto uniform = Buffer::mk(65536, Buffer::Uniform, Buffer::Mapped);

  Model model0("models/sponza/sponza.gltf", *pipeline);
  Model model1("models/doom/0.obj", *pipeline);
  Model model2("models/batman/0.obj", *pipeline);

  xofo::Camera cam(1600, 900);

  xofo::register_recreation_callback(
      [&](auto extent) { cam.set_prj(extent.width, extent.height); });

  mat m3(0.1);
  vec4 ax(0, 0, 0, 1);
  vec3 xxf(0);

  auto grid_buffer = Buffer::mk(65536, Buffer::Vertex, Buffer::Mapped);
  for (u32 i = 0; i <= 2000; ++i) {
    vec2* verts = (vec2*)grid_buffer->mapping;
    i32 half = 1000;
    f32 x = (f32)i - half;

    verts[i * 4 + 0] = vec2(x, half);
    verts[i * 4 + 1] = vec2(x, -half);
    verts[i * 4 + 2] = vec2(half, x);
    verts[i * 4 + 3] = vec2(-half, x);
  }

  f32 speed = 1.f;

  while (xofo::poll()) {
    f64 dt = xofo::dt();
    auto mdelta = mouse_delta() * -0.0012f;

    if (xofo::mbutton(1))
      xofo::hide_mouse(1);
    else {
      xofo::hide_mouse(0);
      mdelta = {};
    }

    f32 w = xofo::get_key(Key::W);
    f32 a = xofo::get_key(Key::A);
    f32 s = xofo::get_key(Key::S);
    f32 d = xofo::get_key(Key::D);
    cam.update(mdelta, s - w, d - a, dt * 5 * speed);

    uniform->copy(cam.pos);
    {
      static float timer = 0;
      if ((timer += dt) > 0.2 && mbutton(0)) {
        timer = 0;
        xforms.emplace_back(cam.pos.xyz, cam.mouse_ray.xyz);
        //xforms.emplace_back(cam.pos.xyz, -cam.ori[2].xyz);
      }
    }

    xofo::draw(
        [&](auto) {
          skybox->bind();
          skybox->push(cam.view);
          skybox->push(cam.prj, 64);

          auto id = mat(1);
          id.translate(cam.pos.xyz);
          cube_map.draw(*skybox, id);

          pipeline->bind();
          pipeline->bind_set([&](auto set) { uniform->bind_to_set(set, 0); },
                             {uniform.get()});

          model0.draw(*pipeline, mat(0.1));
          model1.draw(*pipeline, mat(1));
          model2.draw(*pipeline, mat(1));

          for (auto& xform : xforms) {
            cube.draw(*pipeline, xform.integrate(10 * dt));
          }

          grid_pipe->bind();
          grid_buffer->bind_vertex();
          vkCmdDraw(vk, 8004, 1, 0, 0);
        },
        [&]() {
          using namespace ImGui;
          ImGui::BeginMainMenuBar();
          {
            {
              static float timer = 0, dtt = dt;
              if ((timer += dt) > 0.2)
                timer = 0, dtt = 1.f / dt;
              ImGui::Text("%f\n", dtt);
            }

            if (ImGui::BeginMenu("File")) {
              ImGui::EndMenu();
            }
          }

          ImGui::Begin("Dirs");
          draw_vec4("Mouse ray", cam.mouse_ray);
          draw_vec4("Forward", cam.ori[2]);
          ImGui::End();

          ImGui::EndMainMenuBar();

          for (auto pipe : Pipeline::pipelines) {
            pipe->show();
          }

          model2.show();
        });
  }
  CHECKRE(vkDeviceWaitIdle(xofo::vk));
}
