#ifndef A5C13A51_878E_410E_B45F_954C97F8D972
#define A5C13A51_878E_410E_B45F_954C97F8D972
#include <typedefs.h>
#include <texture.h>

struct Mesh {
  u64 vertex_offset;
  u64 index_offset;
  u64 indices;
  u64 vertices;
  u32 mat;
};


struct Material {
  Opt<Rc<Texture>> diffuse;
  Opt<Rc<Texture>> normal;
  Opt<Rc<Texture>> metallic;
  vec3 color;
  Material() : diffuse(), normal(), metallic(), color() {}
};


#endif /* A5C13A51_878E_410E_B45F_954C97F8D972 */
