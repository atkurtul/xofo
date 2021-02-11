#include <xofo.h>

using namespace std;


template <class F>
void imgui_win(const char* title, F&& f) {
  ImGui::Begin(title);
  f();
  ImGui::End();
}

struct Light {
  vec4 pos;
  vec4 color;

  void integrate(vec3& meta, f32 dt) {
    meta.x += dt * meta.y * 0.1;
    const float angle = 2 * 3.1415 * meta.x;
    pos = vec4(cosf(angle) * meta.z, 0, sinf(angle) * meta.z, 1);
  }

  Light() {
    color = vec4{mango::clamp(float(rand() % 255) / 255, 0.5f, 1.f),
                 mango::clamp(float(rand() % 255) / 255, 0.5f, 1.f),
                 mango::clamp(float(rand() % 255) / 255, 0.5f, 1.f), 1};
  }
};

void integrate_lights(f32 dt, vector<Light>& lights, vector<vec3>& meta) {
  u32 i = 0;
  for (auto& light : meta) {
    lights.at(i).integrate(light, dt);
  }
}

void make_lights(u32 n, f32 r, vector<Light>& lights, vector<vec3>& meta) {
  lights.clear();
  lights.resize(n);
  meta.resize(n);
  for (auto& meta : meta) {
    meta.x = float(rand() % 255) / 255;
    meta.y = 0.01 + 0.5 * float(rand() % 255) / 255;
    meta.z = r * (float(rand() % 255) / 255.f + 0.01);
  }
}

int main() {
  xofo::init();
  Box<xofo::Pipeline> pipeline(new xofo::Pipeline("shaders/shader"));

  auto uniform =
      xofo::Buffer::mk(65536, xofo::Buffer::Uniform, xofo::Buffer::Mapped);

  xofo::Model model0("models/sponza/sponza.gltf", *pipeline);
  xofo::Model model1("models/doom/0.obj", *pipeline);
  xofo::Model model2("models/batman/0.obj", *pipeline);
  xofo::Model model3("models/heavy_assault_rifle/scene.gltf", *pipeline);

  model0.scale = 0.01;
  u32 prev, curr = xofo::buffer_count() - 1;

  auto cam_set = uniform->bind_to_set(pipeline->alloc_set(0), 0);

  xofo::Camera cam(1600, 900);

  xofo::register_recreation_callback([&](auto extent) {
    cam.set_prj(extent.width, extent.height);
    pipeline->reset();
  });

  vector<Light> lights;
  vector<vec3> meta;
  u32 n_lights = 16;
  f32 radius = 8;
  make_lights(n_lights, radius, lights, meta);

  mat m3(0.1);
  vec4 ax(0, 0, 0, 1);
  vec3 xxf(0);

  string read_to_string(const char* file);

  while (xofo::poll()) {
    double dt = xofo::dt();

    integrate_lights(dt, lights, meta);

    auto mdelta = xofo::mdelta() * -0.0012f;
    if (!xofo::mbutton(1)) {
      mdelta = {};
      xofo::unhide_mouse();
    } else {
      xofo::hide_mouse();
    }
    f32 w = xofo::get_key('W');
    f32 a = xofo::get_key('A');
    f32 s = xofo::get_key('S');
    f32 d = xofo::get_key('D');
    cam.update(mdelta, s - w, d - a, dt * 5);

    uniform->copy(cam.pos);
    uniform->copy(vec4i(lights.size()), 16);
    uniform->copy(lights.size() * sizeof(vec4[2]), lights.data(), 32);

    if (xofo::get_key('E') | 1) {
      auto xf = mat(1);
      xf.translate(xxf + vec3(0.1, -0.2, -0.4));
      auto xx = mango::AngleAxis(-180 * RADIAN, vec3(0, 1, 1)) *
                mango::AngleAxis(ax.w, ax.xyz);
      m3 = mat(0.015) * xx * xf * mango::inverse(cam.view);
    }

    xofo::draw(
        [&](auto cmd) {
          vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
          vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  *pipeline, 0, 1, &cam_set, 0, 0);

          vkCmdPushConstants(cmd, *pipeline, 17, 0, 64, &cam.view);
          vkCmdPushConstants(cmd, *pipeline, 17, 64, 64, &cam.prj);
          model0.draw(cmd, *pipeline, mat(0.01));
          model1.draw(cmd, *pipeline, mat(1));
          auto id = mat(1);
          id.translate(-vec3(1, 0, 1));
          model2.draw(cmd, *pipeline, id);
          model3.draw(cmd, *pipeline, m3);
        },
        [&]() {
          imgui_win("Pipeline state", [&]() {
            ImGui::Text("%5f", 1.f / dt);
            if (ImGui::RadioButton("Polygon Fill",
                                   pipeline->mode == VK_POLYGON_MODE_FILL))
              pipeline->set_mode(VK_POLYGON_MODE_FILL);
            if (ImGui::RadioButton("Polygon Line",
                                   pipeline->mode == VK_POLYGON_MODE_LINE))
              pipeline->set_mode(VK_POLYGON_MODE_LINE);
            if (ImGui::RadioButton("Polygon Point",
                                   pipeline->mode == VK_POLYGON_MODE_POINT))
              pipeline->set_mode(VK_POLYGON_MODE_POINT);

            if (ImGui::Checkbox("Depth Test", &pipeline->depth_test))
              pipeline->reset();
            if (ImGui::Checkbox("Depth Write", &pipeline->depth_write))
              pipeline->reset();
            auto extent = xofo::extent();
            if (ImGui::SliderInt("Width", (i32*)&extent.width, 400, 1920) ||
                ImGui::SliderInt("Height", (i32*)&extent.height, 400, 1080)) {
              xofo::resize(extent);
            }
            if (ImGui::Button("Reload shaders")) {
              pipeline->recompile();
            }
            ImGui::DragFloat4("Angle Axis  0", (float*)&ax, 0.005, -10, 10);
            ImGui::DragFloat3("Offset", (float*)&xxf, 0.05, -2, 2);

            if (ImGui::SliderInt("Number of lights", (i32*)&n_lights, 0, 64) ||
                ImGui::SliderFloat("Average radius", &radius, 4, 1024)) {
              make_lights(n_lights, radius, lights, meta);
            }
          });
        });
  }
  CHECKRE(vkDeviceWaitIdle(xofo::vk));
}
