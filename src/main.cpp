
#include <buffer.h>
#include <camera.h>
#include <imgui.h>

#include <mesh_loader.h>
#include <pipeline.h>
#include <vk.h>
#include <vulkan/vulkan_core.h>
#include <cstring>
#include <memory>
#include "typedefs.h"

template <class F>
void imgui_win(const char* title, F&& f) {
  ImGui::Begin(title);
  f();
  ImGui::End();
}

struct Light {
  vec4 pos;
  vec4 color;
};

void integrate_lights(f32 dt, vector<Light>& lights, vector<vec3>& meta) {
  u32 i = 0;
  for (auto& light : meta) {
    light.x += dt * light.y * 0.1;
    const float angle = 2 * 3.1415 * light.x;
    lights[i++].pos = vec4(cosf(angle) * light.z, 0, sinf(angle) * light.z, 1);
  }
}

void make_lights(u32 n, f32 r, vector<Light>& lights, vector<vec3>& meta) {
  lights.resize(n);
  meta.resize(n);

  for (auto& light : lights) {
    light.color = vec4{float(rand() % 255) / 255, float(rand() % 255) / 255,
                    float(rand() % 255) / 255, 1};
  }

  for (auto& light : meta) {
    light.x = float(rand() % 255) / 255;
    light.y = 0.01 + 0.5 * float(rand() % 255) / 255;
    light.z = r * (float(rand() % 255) / 255.f + 0.01);
  }
}

int main() {
  Box<Pipeline> pipeline(new Pipeline("shaders/shader"));

  auto uniform = Buffer::mk(65536, Buffer::Uniform, Buffer::Mapped);

  Box<Buffer> buffer;
  // Texture texture("models/car/posx.jpg", VK_FORMAT_R8G8B8A8_SRGB);
  // auto texture = Texture::mk("blue.png", VK_FORMAT_R8G8B8A8_SRGB);

  vector<Mesh> meshes;
  vector<Material> mats;
  vector<VkDescriptorSet> sets;

  {
    // MeshLoader mesh_loader("teapot.obj", 56);
    // MeshLoader mesh_loader("models/scene_Scene.fbx", 56);
    MeshLoader mesh_loader("models/lightning/0.obj", 56);
    // MeshLoader mesh_loader("models/doom/0.obj", 56);
    // MeshLoader mesh_loader("models/car/car.obj", 56);
    mesh_loader.tex = 12;
    mesh_loader.norm0 = 20;
    mesh_loader.norm1 = 32;
    mesh_loader.norm2 = 44;
    meshes = mesh_loader.import();

    buffer = Buffer::mk(mesh_loader.size,
                        Buffer::Vertex | Buffer::Index | Buffer::Dst,
                        Buffer::Unmapped);

    auto staging = Buffer::mk(mesh_loader.size, Buffer::Src, Buffer::Mapped);
    mats = mesh_loader.load(staging->mapping);
    sets.reserve(mats.size());
    for (auto& mat : mats) {
      auto set = pipeline->alloc_set(1);
      mat.diffuse.value()->bind_to_set(set, 0);
      mat.normal.value()->bind_to_set(set, 1);
      mat.metallic.value()->bind_to_set(set, 2);
      sets.push_back(set);
    }

    vk.execute([&](auto cmd) {
      VkBufferCopy reg = {0, 0, mesh_loader.size};
      vkCmdCopyBuffer(cmd, *staging, *buffer, 1, &reg);
    });
  }

  u32 prev, curr = vk.res.frames.size() - 1;

  auto cam_set = uniform->bind_to_set(pipeline->alloc_set(0), 0);

  Camera cam(1600, 900);

  vk.register_callback([&]() {
    cam.set_prj(vk.res.extent.width, vk.res.extent.height);
    pipeline->reset();
  });

  vector<Light> lights(32);
  vector<vec3> meta(32);
  u32 n_lights = 32;
  f32 radius = 8;
  make_lights(n_lights, radius, lights, meta);

  while (vk.win.poll()) {
    float dt = vk.win.dt;
    integrate_lights(dt, lights, meta);

    auto mdelta = vk.win.mdelta * -0.0015f;
    if (!vk.win.mbutton(1)) {
      mdelta = {};
      vk.win.unhide_mouse();
    } else {
      vk.win.hide_mouse();
    }
    f32 w = vk.win.get_key('W');
    f32 a = vk.win.get_key('A');
    f32 s = vk.win.get_key('S');
    f32 d = vk.win.get_key('D');

    uniform->copy(cam.update(mdelta, s - w, d - a, dt));
    uniform->copy(cam.pos, 64);
    uniform->copy(vec4i(lights.size()), 64 + 16);
    uniform->copy(lights.size() * sizeof(vec4[2]), lights.data(), 64 + 32);

    vk.draw(
        [&](auto cmd) {
          vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);
          auto id = mat(1);
          vkCmdPushConstants(cmd, *pipeline, 17, 0, 64, &id);
          vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  *pipeline, 0, 1, &cam_set, 0, 0);

          for (auto& mesh : meshes) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    *pipeline, 1, 1, &sets.at(mesh.mat), 0, 0);
            vkCmdBindVertexBuffers(cmd, 0, 1, &buffer->buffer,
                                   &mesh.vertex_offset);
            vkCmdBindIndexBuffer(cmd, *buffer, mesh.index_offset,
                                 VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, mesh.indices / 4, 1, 0, 0, 0);
          }
        },
        [&]() {
          imgui_win("Pipeline state", [&]() {
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
            auto extent = vk.res.extent;
            if (ImGui::SliderInt("Width", (i32*)&extent.width, 400, 1920) ||
                ImGui::SliderInt("Height", (i32*)&extent.height, 400, 1080)) {
              vk.win.resize(extent);
            }

            if (
              ImGui::SliderInt("Number of lights", (i32*)&n_lights, 0, 64) ||
              ImGui::SliderFloat("Average radius", &radius, 4, 1024) 
            ) {
              make_lights(n_lights, radius, lights, meta);
            }
          });
        });
  }
  CHECKRE(vkDeviceWaitIdle(vk));
}
