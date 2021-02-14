#ifndef CD090ED5_EE44_479B_B858_96E605ED8117
#define CD090ED5_EE44_479B_B858_96E605ED8117

#include "mesh_loader.h"

#include "buffer.h"
#include "pipeline.h"

namespace xofo {

struct Model {
  std::string origin;
  std::vector<Mesh> meshes;
  std::vector<Material> mats;
  // std::vector<VkDescriptorSet> sets;
  Rc<Buffer> buffer;
  vec3 pos;
  f32 scale;

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
    mesh_loader.load_geometry(staging->mapping);

    xofo::execute([&](auto cmd) {
      VkBufferCopy reg = {0, 0, mesh_loader.size};
      vkCmdCopyBuffer(cmd, *staging, *buffer, 1, &reg);
    });
  }

  void draw(Pipeline& pipeline, mat mat, VkCommandBuffer cmd = vk) {
    pipeline.push(mat, 128);

    for (auto& mesh : meshes) {
      for (auto& mat : mats) {
        auto set = pipeline.get_set(
            [&](auto set) {
              switch (pipeline.set_layouts[1].bindings.size()) {
                default:
                case 3:
                  mat.metallic->bind_to_set(set, 2);
                case 2:
                  mat.normal->bind_to_set(set, 1);
                case 1:
                  mat.diffuse->bind_to_set(set, 0);
                case 0:;
              }
            },
            1, mat.metallic.get(), mat.normal.get(), mat.diffuse.get());
        pipeline.bind_set(set, 1);
      }
      vkCmdBindVertexBuffers(cmd, 0, 1, &buffer->buffer, &mesh.vertex_offset);
      vkCmdBindIndexBuffer(cmd, *buffer, mesh.index_offset,
                           VK_INDEX_TYPE_UINT32);
      vkCmdDrawIndexed(cmd, mesh.indices / 4, 1, 0, 0, 0);
    }
  }
};

}  // namespace xofo
#endif /* CD090ED5_EE44_479B_B858_96E605ED8117 */
