#ifndef C38E0870_5656_4905_97E9_318751A4B327
#define C38E0870_5656_4905_97E9_318751A4B327
#include <typedefs.h>

namespace Assimp {
  class Importer;
}


struct MeshLoader {
  Assimp::Importer* importer;
  const char* file;
  const class aiScene* scene;
  u32 stride;
  u32 tex, norm, norm1, norm2;
  u32 isz;
  u32 vsz;
  MeshLoader(const char* file, u32 stride);
  u64 import();
  u64 size() { return isz + vsz; }
  void load(void* buffer);
  ~MeshLoader();
};

#endif /* C38E0870_5656_4905_97E9_318751A4B327 */
