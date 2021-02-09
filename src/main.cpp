
#include <buffer.h>
#include <camera.h>
#include <mesh_loader.h>
#include <pipeline.h>
#include <vk.h>
#include <vulkan/vulkan_core.h>
#include <cstring>
#include <memory>

int main() {

  Pipeline pipeline("shaders/shader");

  VkDescriptorSet set = pipeline.alloc_set(0);
  Buffer uniform(1024, Buffer::Uniform, Buffer::Mapped);

  u64 isz = 0, vsz = 0;

  Box<Buffer> buffer;
  Texture texture("models/car/posx.jpg", VK_FORMAT_R8G8B8A8_SRGB);

  VkDescriptorSet texture_set = pipeline.alloc_set(1);
  texture.image->bind_to_set(texture_set);
  {
    // MeshLoader mesh_loader("teapot.obj", 32);
    // MeshLoader mesh_loader("batman/0.obj", 32);
    MeshLoader mesh_loader("models/car/car.obj", 32);
    mesh_loader.tex = 12;
    mesh_loader.norm = 20;
    u64 buffer_size = mesh_loader.import();
    isz = mesh_loader.isz;
    vsz = mesh_loader.vsz;

    buffer = make_unique<Buffer>(buffer_size,
                                 Buffer::Vertex | Buffer::Index | Buffer::Dst,
                                 Buffer::Unmapped);

    Buffer staging(buffer_size, Buffer::Src, Buffer::Mapped);
    mesh_loader.load(staging.mapping);

    vk.execute([&](auto cmd) {
      VkBufferCopy reg = {0, 0, buffer_size};
      vkCmdCopyBuffer(cmd, staging, *buffer, 1, &reg);
    });
  }

  uniform.bind_to_set(set);
  // uniform.copy(mat::perspective(90 * RADIAN, 800, 600, 0.01, 4000), 64);

  u32 prev, curr = vk.res.frames.size() - 1;

  Camera cam(800, 600);

  while (vk.win.poll()) {
    auto mdelta = vk.win.mdelta * -0.0015f;
    f32 w = vk.win.get_key('W');
    f32 a = vk.win.get_key('A');
    f32 s = vk.win.get_key('S');
    f32 d = vk.win.get_key('D');

    uniform.copy(cam.update(mdelta, s - w, d - a));

    vk.draw([&](auto cmd) {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

      VkDescriptorSet sets[] = {set, texture_set};
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline, 0,
                              2, sets, 0, 0);

      u64 offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &buffer->buffer, &offset);
      vkCmdBindIndexBuffer(cmd, *buffer, offset + vsz, VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(cmd, isz / 4, 1, 0, 0, 0);
    });
  }
  CHECKRE(vkDeviceWaitIdle(vk));
}
