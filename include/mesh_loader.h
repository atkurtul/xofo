#ifndef C38E0870_5656_4905_97E9_318751A4B327
#define C38E0870_5656_4905_97E9_318751A4B327

#include "mesh.h"

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
  std::vector<xofo::Mesh>  import(u32 flags = 0);

  std::vector<xofo::Material> load_materials();

  xofo::Bbox load_geometry(u8* buffer);

  ~MeshLoader();
};

#endif /* C38E0870_5656_4905_97E9_318751A4B327 */
