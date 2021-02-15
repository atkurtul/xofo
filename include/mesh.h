#ifndef A5C13A51_878E_410E_B45F_954C97F8D972
#define A5C13A51_878E_410E_B45F_954C97F8D972

#include <cmath>
#include "image.h"
#include "pipeline.h"

namespace xofo {

struct Bbox {
  vec3 min = vec3(-INFINITY);
  vec3 max = vec3(+INFINITY);
};

struct Mesh {
  u64 vertex_offset;
  u64 index_offset;
  u64 indices;
  u64 vertices;
  u32 mat;

  void show() {
    ImGui::Text("Number of vertices: %lu", vertices);
    ImGui::Text("Number of indices: %lu", indices);
    ImGui::Text("Material index %u", mat);
  }
};

struct Material {
  Rc<Texture> diffuse;
  Rc<Texture> normal;
  Rc<Texture> metallic;
  vec3 color;

  Material() {
    static u8 black[4] = {0, 0, 0, 255};
    static u8 blue[4] = {0, 0, 255, 255};
    static u8 white[4] = {255, 255, 255, 255};
    static auto diffuse = Rc<Texture>(
        Texture::mk("default diffuse", white, VK_FORMAT_R8G8B8A8_SRGB, 1, 1));
    static auto normal = Rc<Texture>(
        Texture::mk("default normal", blue, VK_FORMAT_R8G8B8A8_SRGB, 1, 1));
    static auto metallic = Rc<Texture>(
        Texture::mk("default metallic", black, VK_FORMAT_R8G8B8A8_SRGB, 1, 1));
    xofo::at_exit([]() {
      diffuse.reset();
      normal.reset();
      metallic.reset();
    });
    this->diffuse = diffuse;
    this->normal = normal;
    this->metallic = metallic;
  }

  void show() {
    diffuse->show("Diffuse");
    normal->show("Normal");
    metallic->show("Metallic");
  }
};

}  // namespace xofo
#endif /* A5C13A51_878E_410E_B45F_954C97F8D972 */
