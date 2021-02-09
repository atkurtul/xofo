#include <mesh_loader.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

MeshLoader::MeshLoader(const char* file, u32 stride)
    : file(file),
      stride(stride),
      importer(new Assimp::Importer),
      scene(0),
      tex(0),
      norm(0),
      norm1(0),
      norm2(0) {}

MeshLoader::~MeshLoader() {
  delete importer;
}

u64 MeshLoader::import() {
  scene = importer->ReadFile(file, aiProcess_Triangulate | 
  aiProcess_FlipUVs |
  aiProcess_GenSmoothNormals |
  aiProcess_CalcTangentSpace |
  aiProcess_LimitBoneWeights);
  
  isz = 0;
  vsz = 0;
  for (u32 i = 0; i < scene->mNumMeshes; ++i) {
    struct aiMesh* mesh = scene->mMeshes[i];
    isz += mesh->mNumFaces * 12;
    vsz += mesh->mNumVertices * stride;
  }
  return isz + vsz;
}

void MeshLoader::load(void* buffer) {
  auto vertices = (char*)buffer;
  auto indices = vertices + vsz;
  u32 offset = 0;
  u32 ioffset = 0;
  for (u32 i = 0; i < scene->mNumMeshes; ++i) {
    struct aiMesh* mesh = scene->mMeshes[i];
    for (u32 j = 0; j < mesh->mNumFaces; ++j) {
      void* aidx = mesh->mFaces[j].mIndices;
      memcpy(indices + ioffset, aidx, 12);
      ioffset += 12;
    }
    u32 size = mesh->mNumVertices;
    for (u32 j = 0; j < mesh->mNumVertices; ++j) {
      u32 curr = offset * stride;
      memcpy(vertices + curr, mesh->mVertices + j, 12);
      if (tex && mesh->mTextureCoords[0])
        memcpy(vertices + curr + tex, mesh->mTextureCoords[0] + j, 8);
      if (norm && mesh->mNormals)
        memcpy(vertices + curr + norm, mesh->mNormals + j, 12);
      if (norm1 && mesh->mTangents)
        memcpy(vertices + curr + norm1, mesh->mTangents + j, 12);
      if (norm2 && mesh->mBitangents)
        memcpy(vertices + curr + norm2, mesh->mBitangents + j, 12);
      offset++;
    }
  }
}
