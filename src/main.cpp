
#include <vulkan/vulkan_core.h>
#include <xofo.h>
#include <mango/image/surface.hpp>
#include "core.h"
#include "imgui.h"

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
    pos = vec4{mango::clamp(float((rand() % 127) - 127) / 127, -1.f, 1.f),
               mango::clamp(float((rand() % 127) - 127) / 127, -1.f, 1.f),
               mango::clamp(float((rand() % 127) - 127) / 127, -1.f, 1.f), 1};
  }
};

void integrate_lights(f32 dt, vector<Light>& lights, vector<vec3>& meta) {
  u32 i = 0;
  for (auto& light : meta) {
    lights.at(i).integrate(light, dt);
  }
}

void make_lights(u32 n, f32 r, vector<Light>& lights, vector<vec3>& meta) {
  n = 1;
  lights.clear();
  lights.resize(n);
  meta.resize(n);
  for (auto& meta : meta) {
    meta.x = float(rand() % 255) / 255;
    meta.y = 0.01 + 0.5 * float(rand() % 255) / 255;
    meta.z = r * (float(rand() % 255) / 255.f + 0.01);
  }
}
using namespace xofo;

struct CubeMap {
  Box<Texture> texture;
  Box<Buffer> buffer;
  VkDescriptorSet set;
  Mesh mesh;

  CubeMap(Pipeline& pipeline) {
    MeshLoader mesh_loader("models/cube.gltf",
                           pipeline.inputs.per_vertex.stride);
    mesh_loader.tex = 12;
    auto meshes = mesh_loader.import(0x1000000);
    if (meshes.size() != 1)
      abort();

    mesh = meshes[0];

    buffer = Buffer::mk(mesh_loader.size,
                        Buffer::Vertex | Buffer::Index | Buffer::Dst,
                        Buffer::Unmapped);

    auto staging = Buffer::mk(mesh_loader.size, Buffer::Src, Buffer::Mapped);
    mesh_loader.load_geometry(staging->mapping);

    xofo::execute([&](auto cmd) {
      VkBufferCopy reg = {0, 0, mesh_loader.size};
      vkCmdCopyBuffer(cmd, *staging, *buffer, 1, &reg);
    });

    set = pipeline.alloc_set(0);
    texture = load_cubemap_single_file("models/Daylight.png",
                                       VK_FORMAT_R8G8B8A8_SRGB);
    texture->bind_to_set(set, 0);
  };

  void draw(Pipeline& pipeline, mat mat, VkCommandBuffer cmd = vk) {
    pipeline.push(mat, 128);
    pipeline.bind_set(set);

    buffer->bind_vertex();
    vkCmdBindIndexBuffer(cmd, *buffer, mesh.index_offset, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh.indices >> 2, 1, 0, 0, 0);
  }
};

void remake_lights(u32 n, vector<Light>& lights, vector<vec3>& metas) {
  lights = vector<Light>(n);
  metas = vector<vec3>(n);
  for (auto& m : metas) {
    m.x = 1;
    m.y = 1;
    m.z = 16;
  }
}

int main() {
  xofo::init();
  auto pipeline = Pipeline::mk("shaders/shader");
  auto skybox = Pipeline::mk("shaders/skybox");
  skybox->depth_write = 0;
  //  skybox->depth_test = 0;
  skybox->reset();

  CubeMap cube_map(*skybox);

  auto grid_pipe = Pipeline::mk("shaders/grid");
  grid_pipe->mode = (VK_POLYGON_MODE_LINE);
  grid_pipe->topology = (VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
  grid_pipe->line_width = (1.2f);
  grid_pipe->reset();

  auto uniform =
      xofo::Buffer::mk(65536, xofo::Buffer::Uniform, xofo::Buffer::Mapped);

  auto cam_set = uniform->bind_to_set(pipeline->alloc_set(0), 0);

  // Model model0("models/sponza/sponza.gltf", *pipeline);

  // Model model1("models/doom/0.obj", *pipeline);
  // Model model2("models/batman/0.obj", *pipeline);
  // Model model3("models/heavy_assault_rifle/scene.gltf", *pipeline);
  // model0.scale = 0.1;

  Model model0("models/Package/AncientTemple.obj", *pipeline);

  xofo::Camera cam(1600, 900);

  xofo::register_recreation_callback(
      [&](auto extent) { cam.set_prj(extent.width, extent.height); });

  mat m3(0.1);
  vec4 ax(0, 0, 0, 1);
  vec3 xxf(0);

  vector<Light> lights;
  vector<vec3> metas;
  u32 n_lights = 32;
  remake_lights(n_lights, lights, metas);

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
    u32 i = 0;
    for (auto& light : lights) {
      light.integrate(metas[i++], dt);
    }

    f32 w = xofo::get_key(Key::W);
    f32 a = xofo::get_key(Key::A);
    f32 s = xofo::get_key(Key::S);
    f32 d = xofo::get_key(Key::D);
    cam.update(mdelta, s - w, d - a, dt * 5 * speed);

    uniform->copy(cam.pos);
    uniform->copy(vec4i(n_lights), 16);
    uniform->copy(lights.size() * sizeof(Light), lights.data(), 32);
    auto inv_view = mango::inverse(cam.view);
    if (get_key(Key::E) | 1) {
      auto xf = mat(1);
      xf.translate(xxf + vec3(0.1, -0.2, -0.4));
      auto xx = mango::AngleAxis(-180 * RADIAN, vec3(0, 1, 1)) *
                mango::AngleAxis(ax.w, ax.xyz);
      m3 = mat(0.015) * xx * xf * inv_view;
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
          pipeline->bind_set(cam_set);

          model0.draw(*pipeline, mat(1));
          // model1.draw(cmd, *pipeline, mat(1));
          // model2.draw(cmd, *pipeline, mat(1));
          // model3.draw(cmd, *pipeline, m3);

          grid_pipe->bind();
          grid_buffer->bind_vertex();
          vkCmdDraw(vk, 8008, 1, 0, 0);
        },
        [&]() {
          imgui_win("Recompiler", [&]() {
            if (ImGui::Button("Recompile")) {
              pipeline->recompile();
            }
          });
          imgui_win("Pipeline state", [&]() {
            ImGui::Text("%5f", 1.f / dt);
            auto fwd = cam.view[2];
            ImGui::Text("%5f %5f %5f", (f32)fwd.x, (f32)fwd.y, (f32)fwd.z);
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
            ImGui::DragFloat("Speed", &speed, 0.05, 0.5f, 100.f, "%f", ImGuiSliderFlags_Logarithmic);
            if (ImGui::SliderInt("Lights", (i32*)&n_lights, 0, 128)) {
              remake_lights(n_lights, lights, metas);
            }

            ImGui::DragFloat4("Angle Axis  0", (f32*)&ax, 0.005, -10, 10);
            ImGui::DragFloat3("Offset", (f32*)&xxf, 0.05, -2, 2);
          });
        });
  }
  CHECKRE(vkDeviceWaitIdle(xofo::vk));
}
