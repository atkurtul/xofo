#ifndef CD090ED5_EE44_479B_B858_96E605ED8117
#define CD090ED5_EE44_479B_B858_96E605ED8117

#include "mesh_loader.h"

#include <vulkan/vulkan_core.h>
#include "buffer.h"
#include "pipeline.h"

namespace xofo {

struct Model {
  std::string origin;
  std::vector<Mesh> meshes;
  std::vector<Material> mats;
  std::vector<VkDescriptorSet> sets;

  Rc<Buffer> buffer;
  vec3 pos;
  f32 scale;
  Bbox box;

  Model(const char* path, Pipeline& pipeline) : origin(path), scale(1), pos(0) {
    MeshLoader mesh_loader(path, pipeline.inputs.per_vertex.stride);
    mesh_loader.tex = 12;
    mesh_loader.norm0 = 20;
    mesh_loader.norm1 = 32;
    mesh_loader.norm2 = 44;
    meshes = mesh_loader.import();
    buffer = Buffer::mk(mesh_loader.size,
                        Buffer::Vertex | Buffer::Index | Buffer::Dst,
                        Buffer::Unmapped);

    auto staging = Buffer::mk(mesh_loader.size, Buffer::Src, Buffer::Mapped);
    mats = mesh_loader.load_materials();
    box = mesh_loader.load_geometry(staging->mapping);

    xofo::execute([&](auto cmd) {
      VkBufferCopy reg = {0, 0, mesh_loader.size};
      vkCmdCopyBuffer(cmd, *staging, *buffer, 1, &reg);
    });
  }

  void draw(Pipeline& pipeline, mat mat, VkCommandBuffer cmd = vk) {
    pipeline.push(mat, 128);
    for (auto& mesh : meshes) {
      pipeline.bind_set(
          [&](auto set) {
            if (pipeline.has_binding(1, 2))
              mats[mesh.mat].metallic->bind_to_set(set, 2);
            if (pipeline.has_binding(1, 1))
              mats[mesh.mat].normal->bind_to_set(set, 1);
            if (pipeline.has_binding(1, 0))
              mats[mesh.mat].diffuse->bind_to_set(set, 0);
          },
          {mats[mesh.mat].metallic.get(), mats[mesh.mat].normal.get(),
           mats[mesh.mat].diffuse.get()},
          1, cmd);

      buffer->bind_vertex(mesh.vertex_offset);
      buffer->bind_index(mesh.index_offset);
      vkCmdDrawIndexed(cmd, mesh.indices >> 2, 1, 0, 0, 0);
    }
  }
};

}  // namespace xofo
#endif /* CD090ED5_EE44_479B_B858_96E605ED8117 */
