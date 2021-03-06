#include <xofo.h>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include <assimp/Importer.hpp>

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
  auto t0 = Clock::now();
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
  auto t1 = Clock::now();
  cout << file << " Loaded Meshes in: "<< chrono::duration_cast<chrono::milliseconds>(t1-t0).count() << "\n";
  return meshes;
}

Bbox MeshLoader::load_geometry(u8* buffer) {
  //auto t0 = Clock::now();
  Bbox box = { };

  for (u32 i = 0; i < scene->mNumMeshes; ++i) {
    struct aiMesh* mesh = scene->mMeshes[i];
    for (u32 j = 0; j < mesh->mNumVertices; ++j) {

      memcpy(buffer, mesh->mVertices + j, 12);
      f32x3 pos = *(f32x3*)(mesh->mVertices + j);
      box.min = xofo::min(pos, box.min);
      box.max = xofo::max(pos, box.max);

      if (tex && mesh->mTextureCoords[0])
        memcpy(buffer + tex, &mesh->mTextureCoords[0][j], 8);

      if (norm0 && mesh->mNormals)
        memcpy(buffer + norm0, mesh->mNormals + j, 12);

      if (norm1 && mesh->mTangents)
        memcpy(buffer + norm1, mesh->mTangents + j, 12);

      if (norm2 && mesh->mBitangents)
        memcpy(buffer + norm2, mesh->mBitangents + j, 12);
      buffer += stride;


      if (mesh->HasVertexColors(0)) {
        cerr << "Has colors: " << file <<"\n";
      }
    }

    for (u32 j = 0; j < mesh->mNumFaces; ++j) {
      void* aidx = mesh->mFaces[j].mIndices;
      memcpy(buffer, aidx, 12);
      buffer += 12;
    }
  }
  
  //auto t1 = Clock::now();
  //cout  << file << " Loaded geometry in: "<< chrono::duration_cast<chrono::milliseconds>(t1-t0).count() << "\n";
  return box;
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
      mmat.color = f32x3(color.r, color.g, color.b);
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
