#ifndef C38E0870_5656_4905_97E9_318751A4B327
#define C38E0870_5656_4905_97E9_318751A4B327
#include <memory>
#include <typedefs.h>
#include <mesh.h>

namespace Assimp {
  class Importer;
}

struct MeshLoader {
  Assimp::Importer* importer;
  const char* file;
  const class aiScene* scene;
  u32 stride;
  u32 tex, norm0, norm1, norm2;
  u32 size;

  MeshLoader(const char* file, u32 stride);
  std::vector<Mesh>  import();

  std::vector<Material> load(char* buffer);
  ~MeshLoader();
};

#endif /* C38E0870_5656_4905_97E9_318751A4B327 */
