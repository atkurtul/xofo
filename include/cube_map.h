#ifndef CC7C02E2_3303_42F2_B2CE_B276CA93A3A6
#define CC7C02E2_3303_42F2_B2CE_B276CA93A3A6

#include "buffer.h"
#include "image.h"
#include "mesh_loader.h"
#include "pipeline.h"

namespace xofo {

struct CubeMap {
  Box<Texture> texture;
  Box<Buffer> buffer;
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

    texture = Texture::load_cubemap_single_file("models/Daylight.png",
                                                VK_FORMAT_R8G8B8A8_SRGB);

    // set = pipeline.create_set(0, texture.get());
    // texture->bind_to_set(set, 0);
  };

  void draw(Pipeline& pipeline, mat mat, VkCommandBuffer cmd = vk) {
    pipeline.push(mat, 128);
    pipeline.bind_set([&](auto set) { texture->bind_to_set(set, 0); },
                      {texture.get()}, 0, cmd);

    buffer->bind_vertex();
    vkCmdBindIndexBuffer(cmd, *buffer, mesh.index_offset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, mesh.indices >> 2, 1, 0, 0, 0);
  }
};
}  // namespace xofo
#endif /* CC7C02E2_3303_42F2_B2CE_B276CA93A3A6 */
