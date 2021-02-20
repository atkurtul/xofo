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

  Cube() {
    if (!buffer) {
      f32x3 vertices[] = {
          f32x3(-1, -1, -1),  // 0
          f32x3(+1, -1, -1),  // 1
          f32x3(+1, +1, -1),  // 2
          f32x3(-1, +1, -1),  // 3
          f32x3(-1, -1, +1),  // 4
          f32x3(+1, -1, +1),  // 5
          f32x3(+1, +1, +1),  // 6
          f32x3(-1, +1, +1),  // 7
      };
      u32 indices[36] = {0, 2, 1, 0, 3, 2, 1, 2, 6, 6, 5, 1, 4, 5, 6, 6, 7, 4,
                         2, 3, 6, 6, 3, 7, 0, 7, 3, 0, 4, 7, 0, 1, 5, 0, 5, 4};

      buffer = Buffer::unmapped({{sizeof(indices), (u8*)indices},
                                 {sizeof(vertices), (u8*)vertices}})
                   .release();

      xofo::at_exit([]() { delete buffer; });
    }
  }

  void draw(std::vector<f32x4x4> const& boxes, Pipeline& pipeline, VkCommandBuffer cmd = vk){

    buffer->bind_vertex(vertex_offset);
    buffer->bind_index();

    for (auto& box : boxes) {
      pipeline.push(box, 128);
      vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
    }

  }
};

}  // namespace xofo

#endif /* A89581E3_7D6F_445A_A2AA_76AA2E69075C */
