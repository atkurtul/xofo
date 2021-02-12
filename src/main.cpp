
#include <vulkan/vulkan_core.h>
#include <xofo.h>
#include <mango/image/surface.hpp>

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
    MeshLoader mesh_loader("models/cube.gltf", pipeline.stride);
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
    texture = load_cubemap2("models/car", VK_FORMAT_R8G8B8A8_SRGB);
    texture->bind_to_set(set, 0);
  };

  void draw(VkCommandBuffer cmd, VkPipelineLayout layout, mat mat) {
    vkCmdPushConstants(cmd, layout, 17, 128, 64, &mat);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1,
                            &set, 0, 0);

    vkCmdBindVertexBuffers(cmd, 0, 1, &buffer->buffer, &mesh.vertex_offset);
    vkCmdBindIndexBuffer(cmd, *buffer, mesh.index_offset, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh.indices / 4, 1, 0, 0, 0);
  }
};

struct Dummy {
  Box<Buffer> buffer;
  Mesh mesh;

  Dummy(Pipeline& pipeline) {
    MeshLoader mesh_loader("models/cube.gltf", pipeline.stride);
    mesh_loader.tex = 12;
    mesh_loader.norm0 = 20;
    mesh_loader.norm1 = 32;
    mesh_loader.norm2 = 44;

  }
};

int main() {
  xofo::init();
  auto pipeline = Pipeline::mk("shaders/shader");
  auto skybox = Pipeline::mk("shaders/skybox");
  skybox->depth_write = 0;
  //  skybox->depth_test = 0;
  skybox->reset();
  CubeMap cube_map(*skybox);
  auto uniform =
      xofo::Buffer::mk(65536, xofo::Buffer::Uniform, xofo::Buffer::Mapped);

  xofo::Model model0("models/sponza/sponza.gltf", *pipeline);
  xofo::Model model1("models/doom/0.obj", *pipeline);
  xofo::Model model2("models/batman/0.obj", *pipeline);
  xofo::Model model3("models/heavy_assault_rifle/scene.gltf", *pipeline);
  model0.scale = 0.1;

  auto cam_set = uniform->bind_to_set(pipeline->alloc_set(0), 0);

  xofo::Camera cam(1600, 900);

  xofo::register_recreation_callback([&](auto extent) {
    cam.set_prj(extent.width, extent.height);
    pipeline->reset();
    skybox->reset();
  });

  mat m3(0.1);
  vec4 ax(0, 0, 0, 1);
  vec3 xxf(0);

  Light light;
  vec3 meta = vec3{0.f, 1.f, 16.f};

  while (xofo::poll()) {
    double dt = xofo::dt();
    auto mdelta = xofo::mdelta() * -0.0012f;

    if (xofo::mbutton(1))
      xofo::hide_mouse(1);
    else {
      xofo::hide_mouse(0);
      mdelta = {};
    }

    light.integrate(meta, dt);

    f32 w = xofo::get_key('W');
    f32 a = xofo::get_key('A');
    f32 s = xofo::get_key('S');
    f32 d = xofo::get_key('D');
    cam.update(mdelta, s - w, d - a, dt * 5);

    uniform->copy(cam.pos);
    uniform->copy(light, 16);
    auto inv_view = mango::inverse(cam.view);
    if (xofo::get_key('E') | 1) {
      auto xf = mat(1);
      xf.translate(xxf + vec3(0.1, -0.2, -0.4));
      auto xx = mango::AngleAxis(-180 * RADIAN, vec3(0, 1, 1)) *
                mango::AngleAxis(ax.w, ax.xyz);
      m3 = mat(0.015) * xx * xf * inv_view;
    }

    xofo::draw(
        [&](auto cmd) {
          vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *skybox);
          vkCmdPushConstants(cmd, *pipeline, 17, 0, 64, &cam.view);
          vkCmdPushConstants(cmd, *pipeline, 17, 64, 64, &cam.prj);
          auto id = mat(1);
          id.translate(cam.pos.xyz);
          cube_map.draw(cmd, *skybox, id);

          vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
          vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  *pipeline, 0, 1, &cam_set, 0, 0);

          vkCmdPushConstants(cmd, *pipeline, 17, 0, 64, &cam.view);
          vkCmdPushConstants(cmd, *pipeline, 17, 64, 64, &cam.prj);

          model0.draw(cmd, *pipeline, mat(0.01));
          model1.draw(cmd, *pipeline, mat(1));
          model2.draw(cmd, *pipeline, mat(1));
          model3.draw(cmd, *pipeline, m3);
        },
        [&]() {
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
            if (ImGui::Button("Recompile")) {
              pipeline->recompile();
            }
            ImGui::DragFloat4("Angle Axis  0", (float*)&ax, 0.005, -10, 10);
            ImGui::DragFloat3("Offset", (float*)&xxf, 0.05, -2, 2);

            ImGui::DragFloat3("Light color", (float*)&light.color, 0.05, 0,
                              1.f);
            ImGui::DragFloat("Light speed", &meta.y, 0.05, 0, 16.f);
            ImGui::DragFloat("Light radius", &meta.z, 0.05, 0, 16.f);
          });
        });
  }
  CHECKRE(vkDeviceWaitIdle(xofo::vk));
}
