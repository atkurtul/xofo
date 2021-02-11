#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/Importer.hpp>

#include <mesh_loader.h>

#include <iostream>
#include <unordered_map>

using namespace std;
using namespace xofo;

MeshLoader::MeshLoader(const char* file, u32 stride)
    : file(file),
      stride(stride),
      importer(new Assimp::Importer),
      scene(0),
      tex(0),
      norm0(0),
      norm1(0),
      norm2(0) {}

MeshLoader::~MeshLoader() {
  delete importer;
}

std::vector<Mesh> MeshLoader::import(u32 flags) {
  scene = importer->ReadFile(file, aiProcess_Triangulate | aiProcess_FlipUVs |
                                       aiProcess_GenSmoothNormals |
                                       aiProcess_CalcTangentSpace |
                                       aiProcess_LimitBoneWeights | flags);
  if (!scene)
    abort();
  size = 0;
  std::vector<Mesh> meshes;
  for (u32 i = 0; i < scene->mNumMeshes; ++i) {
    struct aiMesh* mesh = scene->mMeshes[i];

    u64 indices = mesh->mNumFaces * 12;
    u64 vertices = mesh->mNumVertices * stride;
    Mesh mm = {
        .vertex_offset = size,
        .index_offset = size + vertices,
        .indices = indices,
        .vertices = vertices,
        .mat = mesh->mMaterialIndex,
    };
    size += indices + vertices;
    meshes.push_back(mm);
  }

  return meshes;
}

void MeshLoader::load_geometry(char* buffer) {
  for (u32 i = 0; i < scene->mNumMeshes; ++i) {
    struct aiMesh* mesh = scene->mMeshes[i];
    for (u32 j = 0; j < mesh->mNumVertices; ++j) {
      memcpy(buffer, mesh->mVertices + j, 12);

      if (tex && mesh->mTextureCoords[0])
        memcpy(buffer + tex, &mesh->mTextureCoords[0][j], 8);

      if (norm0 && mesh->mNormals)
        memcpy(buffer + norm0, mesh->mNormals + j, 12);

      if (norm1 && mesh->mTangents)
        memcpy(buffer + norm1, mesh->mTangents + j, 12);

      if (norm2 && mesh->mBitangents)
        memcpy(buffer + norm2, mesh->mBitangents + j, 12);
      buffer += stride;
    }

    for (u32 j = 0; j < mesh->mNumFaces; ++j) {
      void* aidx = mesh->mFaces[j].mIndices;
      memcpy(buffer, aidx, 12);
      buffer += 12;
    }
  }
}
std::vector<Material> MeshLoader::load_materials() {
  unordered_map<string, Rc<Texture>> textures;
  std::vector<Material> mats;

  string base = file;

  base.erase(base.begin() + base.find_last_of("/") + 1, base.end());

  for (u32 i = 0; i < scene->mNumMaterials; ++i) {
    Material mmat;
    aiString path;
    auto mat = scene->mMaterials[i];

    aiColor3D color;
    if (aiReturn_SUCCESS == mat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
      mmat.color = vec3{color.r, color.g, color.b};
    }

    if (aiReturn_SUCCESS == mat->GetTexture(aiTextureType_DIFFUSE, 0, &path)) {
      string rpath = base + path.C_Str();
      if (textures.find(path.C_Str()) == textures.end()) {
        textures[path.C_Str()] =
            Rc<Texture>(Texture::mk(rpath, VK_FORMAT_R8G8B8A8_SRGB));
      }
      mmat.diffuse = textures[path.C_Str()];
    }

    if (aiReturn_SUCCESS == mat->GetTexture(aiTextureType_HEIGHT, 0, &path)) {
      string rpath = base + path.C_Str();
      if (textures.find(path.C_Str()) == textures.end()) {
        textures[path.C_Str()] =
            Rc<Texture>(Texture::mk(rpath, VK_FORMAT_R8G8B8A8_SRGB));
      }
      mmat.normal = textures[path.C_Str()];
    }

    mats.push_back(mmat);
  }

  return mats;
}
