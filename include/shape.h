#ifndef A89581E3_7D6F_445A_A2AA_76AA2E69075C
#define A89581E3_7D6F_445A_A2AA_76AA2E69075C

#include <buffer.h>
#include <cstring>
#include <vulkan/vulkan_core.h>
#include "core.h"
#include "mesh.h"

namespace xofo {

struct Cube {
  inline static Buffer* buffer = 0;
  inline static constexpr u32 vertex_offset = 144;
  Bbox scale;

  Cube(Bbox box) : scale(box) {
    if (!buffer) {
      vec3 vertices[] = {
          vec3(-1, -1, -1),  // 0
          vec3(+1, -1, -1),  // 1
          vec3(+1, +1, -1),  // 2
          vec3(-1, +1, -1),  // 3
          vec3(-1, -1, +1),  // 4
          vec3(+1, -1, +1),  // 5
          vec3(+1, +1, +1),  // 6
          vec3(-1, +1, +1),  // 7
      };
      u32 indices[36] = {0, 2, 1, 0, 3, 2, 1, 2, 6, 6, 5, 1, 4, 5, 6, 6, 7, 4,
                         2, 3, 6, 6, 3, 7, 0, 7, 3, 0, 4, 7, 0, 1, 5, 0, 5, 4};

      buffer = Buffer::unmapped({{sizeof(indices), (u8*)indices},
                                 {sizeof(vertices), (u8*)vertices}})
                   .release();

      xofo::at_exit([]() { delete buffer; });
    }
  }

  void draw(Pipeline& pipeline, mat mat, VkCommandBuffer cmd = vk){
    pipeline.push(mat, 128);
    // for (auto& mesh : meshes) {
    //   pipeline.bind_set(
    //       [&](auto set) {
    //         if (pipeline.has_binding(1, 2))
    //           mats[mesh.mat].metallic->bind_to_set(set, 2);
    //         if (pipeline.has_binding(1, 1))
    //           mats[mesh.mat].normal->bind_to_set(set, 1);
    //         if (pipeline.has_binding(1, 0))
    //           mats[mesh.mat].diffuse->bind_to_set(set, 0);
    //       },
    //       {mats[mesh.mat].metallic.get(), mats[mesh.mat].normal.get(),
    //        mats[mesh.mat].diffuse.get()},
    //       1, cmd);

    //   buffer->bind_vertex(mesh.vertex_offset);
    //   buffer->bind_index(mesh.index_offset);
    //   vkCmdDrawIndexed(cmd, mesh.indices >> 2, 1, 0, 0, 0);
    // }
  }
};

}  // namespace xofo

#endif /* A89581E3_7D6F_445A_A2AA_76AA2E69075C */
