
#include <buffer.h>
#include <camera.h>
#include <mesh_loader.h>
#include <pipeline.h>
#include <vk.h>
#include <vulkan/vulkan_core.h>
#include <cstring>
#include <memory>

int main() {
  Box<Pipeline> pipeline(new Pipeline("shaders/shader"));

  auto uniform = Buffer::mk(1024, Buffer::Uniform, Buffer::Mapped);

  Box<Buffer> buffer;
  // Texture texture("models/car/posx.jpg", VK_FORMAT_R8G8B8A8_SRGB);
  // auto texture = Texture::mk("blue.png", VK_FORMAT_R8G8B8A8_SRGB);

  vector<Mesh> meshes;
  vector<Material> mats;
  vector<VkDescriptorSet> sets;

  {
    // MeshLoader mesh_loader("teapot.obj", 32);
    // MeshLoader mesh_loader("models/scene_Scene.fbx", 32);
    MeshLoader mesh_loader("models/wmaker/0.obj", 32);
    // MeshLoader mesh_loader("models/car/car.obj", 32);
    mesh_loader.tex = 12;
    mesh_loader.norm = 20;
    meshes = mesh_loader.import();

    buffer = Buffer::mk(mesh_loader.size,
                        Buffer::Vertex | Buffer::Index | Buffer::Dst,
                        Buffer::Unmapped);

    auto staging = Buffer::mk(mesh_loader.size, Buffer::Src, Buffer::Mapped);
    mats = mesh_loader.load(staging->mapping);
    sets.reserve(mats.size());
    for (auto& mat : mats) {
      auto set = pipeline->alloc_set(1);
      if (mat.diffuse.has_value())
        mat.diffuse.value()->bind_to_set(set, 0);
      if (mat.normal.has_value())
        mat.normal.value()->bind_to_set(set, 1);
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

  while (vk.win.poll()) {
    auto mdelta = vk.win.mdelta * -0.0015f;
    f32 w = vk.win.get_key('W');
    f32 a = vk.win.get_key('A');
    f32 s = vk.win.get_key('S');
    f32 d = vk.win.get_key('D');

    uniform->copy(cam.update(mdelta, s - w, d - a));

    vk.draw([&](auto cmd) {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline,
                              0, 1, &cam_set, 0, 0);

      for (auto& mesh : meshes) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline,
                                1, 1, &sets.at(mesh.mat), 0, 0);
        vkCmdBindVertexBuffers(cmd, 0, 1, &buffer->buffer, &mesh.vertex_offset);
        vkCmdBindIndexBuffer(cmd, *buffer, mesh.index_offset,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.indices / 4, 1, 0, 0, 0);
      }
    });
  }
  CHECKRE(vkDeviceWaitIdle(vk));
}
