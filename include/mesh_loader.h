#ifndef C38E0870_5656_4905_97E9_318751A4B327
#define C38E0870_5656_4905_97E9_318751A4B327
#include <memory>
#include <typedefs.h>
#include <texture.h>

namespace Assimp {
  class Importer;
}

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
  vec3 color;
  Material() : diffuse(), normal(), color() {}
};

struct MeshLoader {
  Assimp::Importer* importer;
  const char* file;
  const class aiScene* scene;
  u32 stride;
  u32 tex, norm, norm1, norm2;
  u32 size;

  MeshLoader(const char* file, u32 stride);
  std::vector<Mesh>  import();

  std::vector<Material> load(char* buffer);
  ~MeshLoader();
};

#endif /* C38E0870_5656_4905_97E9_318751A4B327 */
