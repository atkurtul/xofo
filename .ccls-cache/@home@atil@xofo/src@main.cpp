#include <xofo.h>
#include "imgui.h"

using namespace std;
using namespace xofo;

struct CubeMap {
  Box<Texture> texture;
  Box<Buffer> buffer;
  Mesh mesh;

  CubeMap(Pipeline& pipeline) {
    MeshLoader mesh_loader("models/cube.gltf",
                           pipeline.inputs.per_vertex.stride);
    mesh_loader.tex = 12;
    auto meshes = mesh_loader.import(0x1000000);
    if (meshes.size() != 1)
      abort();

    mesh = meshes[0];

    buffer = Buffer::mk(mesh_loader.size,
                        Buffer::Vertex | Buffer::Index | Buffer::Dst,
                        Buffer::Unmapped);

    auto staging = Buffer::mk(mesh_loader.size, Buffer::Src, Buffer::Mapped);
    mesh_loader.load_geometry(staging->mapping);

    xofo::execute([&](auto cmd) {
      VkBufferCopy reg = {0, 0, mesh_loader.size};
      vkCmdCopyBuffer(cmd, *staging, *buffer, 1, &reg);
    });

    texture = load_cubemap_single_file("models/Daylight.png",
                                       VK_FORMAT_R8G8B8A8_SRGB);
    // set = pipeline.create_set(0, texture.get());
    // texture->bind_to_set(set, 0);
  };

  void draw(Pipeline& pipeline, mat mat, VkCommandBuffer cmd = vk) {
    pipeline.push(mat, 128);
    auto set = pipeline.get_set([&](auto set) { texture->bind_to_set(set, 0); },
                                0, texture.get());
    pipeline.bind_set(set, 0);

    buffer->bind_vertex();
    vkCmdBindIndexBuffer(cmd, *buffer, mesh.index_offset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, mesh.indices >> 2, 1, 0, 0, 0);
  }
};

template <class V>
bool imgui_combo(const char* mode_name,
                 vector<pair<string, V>> const& map,
                 u32& selected) {
  bool re = false;
  if (ImGui::BeginCombo(mode_name, map[selected].first.data())) {
    u32 i = 0;
    for (auto& [k, v] : map) {
      bool is_selected = i == selected;
      if (ImGui::Selectable(map[i].first.data(), is_selected)) {
        selected = i;
        re = true;
      }
      if (is_selected)
        ImGui::SetItemDefaultFocus();
      i++;
    }
    ImGui::EndCombo();
  }
  return re;
}

void show_pipeline_state(Pipeline& pipeline) {
  ImGui::Begin(pipeline.shader.data());
  if (imgui_combo("Cull mode", CullModeFlagBits_map,
                  (u32&)pipeline.state.culling) |
      imgui_combo("Polygon mode", PolygonMode_map,
                  (u32&)pipeline.state.polygon) |
      imgui_combo("Topology", PrimitiveTopology_map,
                  (u32&)pipeline.state.topology) |
      ImGui::DragFloat("Line width", &pipeline.state.line_width, 0.1, 0.1, 32) |
      ImGui::Checkbox("Depth test", &pipeline.state.depth_test) |
      ImGui::Checkbox("Depth write", &pipeline.state.depth_write)) {
    cout << "Cull mode: " << pipeline.state.culling << "\n";
    pipeline.reset();
  }
  ImGui::End();
}

void show_mesh_state(Mesh const& mesh) {
  ImGui::Text("Number of vertices: %lu", mesh.vertices);
  ImGui::Text("Number of indices: %lu", mesh.indices);
  ImGui::Text("Material index %u", mesh.mat);
}

void show_model(Model const& model) {}

int main() {

  
  xofo::init();
  auto pipeline = Pipeline::mk("shaders/shader");
  auto skybox = Pipeline::mk("shaders/skybox", {.depth_write = 0});

  CubeMap cube_map(*skybox);

  auto grid_pipe =
      Pipeline::mk("shaders/grid", {.polygon = VK_POLYGON_MODE_LINE,
                                    .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                                    .line_width = 1.2f});

  auto uniform =
      xofo::Buffer::mk(65536, xofo::Buffer::Uniform, xofo::Buffer::Mapped);

  // Model model0("models/sponza/sponza.gltf", *pipeline);

  // Model model1("models/doom/0.obj", *pipeline);
  // Model model2("models/batman/0.obj", *pipeline);
  // Model model3("models/heavy_assault_rifle/scene.gltf", *pipeline);
  // model0.scale = 0.1;

  Model model0("models/Package/AncientTemple.obj", *pipeline);

  xofo::Camera cam(1600, 900);

  xofo::register_recreation_callback(
      [&](auto extent) { cam.set_prj(extent.width, extent.height); });

  mat m3(0.1);
  vec4 ax(0, 0, 0, 1);
  vec3 xxf(0);

  auto grid_buffer = Buffer::mk(65536, Buffer::Vertex, Buffer::Mapped);
  for (u32 i = 0; i <= 2000; ++i) {
    vec2* verts = (vec2*)grid_buffer->mapping;
    i32 half = 1000;
    f32 x = (f32)i - half;

    verts[i * 4 + 0] = vec2(x, half);
    verts[i * 4 + 1] = vec2(x, -half);
    verts[i * 4 + 2] = vec2(half, x);
    verts[i * 4 + 3] = vec2(-half, x);
  }
  f32 speed = 1.f;
  while (xofo::poll()) {
    f64 dt = xofo::dt();
    auto mdelta = mouse_delta() * -0.0012f;

    if (xofo::mbutton(1))
      xofo::hide_mouse(1);
    else {
      xofo::hide_mouse(0);
      mdelta = {};
    }

    f32 w = xofo::get_key(Key::W);
    f32 a = xofo::get_key(Key::A);
    f32 s = xofo::get_key(Key::S);
    f32 d = xofo::get_key(Key::D);
    cam.update(mdelta, s - w, d - a, dt * 5 * speed);

    uniform->copy(cam.pos);

    auto inv_view = mango::inverse(cam.view);
    if (get_key(Key::E) | 1) {
      auto xf = mat(1);
      xf.translate(xxf + vec3(0.1, -0.2, -0.4));
      auto xx = mango::AngleAxis(-180 * RADIAN, vec3(0, 1, 1)) *
                mango::AngleAxis(ax.w, ax.xyz);
      m3 = mat(0.015) * xx * xf * inv_view;
    }

    xofo::draw(
        [&](auto) {
          skybox->bind();
          skybox->push(cam.view);
          skybox->push(cam.prj, 64);

          auto id = mat(1);
          id.translate(cam.pos.xyz);
          cube_map.draw(*skybox, id);

          pipeline->bind();

          auto cam_set =
              pipeline->get_set([&](auto set) { uniform->bind_to_set(set, 0); },
                                0, uniform.get());
          pipeline->bind_set(cam_set, 0);
          model0.draw(*pipeline, mat(1));
          // model1.draw(cmd, *pipeline, mat(1));
          // model2.draw(cmd, *pipeline, mat(1));
          // model3.draw(cmd, *pipeline, m3);

          grid_pipe->bind();
          grid_buffer->bind_vertex();
          vkCmdDraw(vk, 8008, 1, 0, 0);
        },
        [&]() {
          using namespace ImGui;
          static bool show_pipeline = false;
          if (show_pipeline && Begin("Create pipeline", &show_pipeline)) {
            if (Button("Create")) {
              show_pipeline = false;
            }
            static u32 cull = 0;
            static u32 polygon = 0;
            static u32 topology = 0;
            static f32 line_width = 1.f;
            static bool depth_test = true;
            static bool depth_write = true;
            static char shader_path[256] = {};
            imgui_combo("Cull mode", CullModeFlagBits_map, cull);
            imgui_combo("Polygon mode", PolygonMode_map, polygon);
            imgui_combo("Topology", PrimitiveTopology_map, topology);
            DragFloat("Line width", &line_width, 0.1, 0.1, 32);
            Checkbox("Depth test", &depth_test);
            Checkbox("Depth write", &depth_write);
            InputText("Shader path", shader_path, 256);
            End();
          }

          BeginMainMenuBar();
          {
            if (BeginMenu("File")) {
              void ShowExampleMenuFile();
              ShowExampleMenuFile();
              EndMenu();
            }
            if (BeginMenu("New")) {
              if (MenuItem("Pipeline")) {
                show_pipeline = true;
              }
              EndMenu();
            }
            EndMenuBar();
          }

          for (auto pipe : Pipeline::pipelines) {
            show_pipeline_state(*pipe);
          }
          ImGui::Begin(model0.origin.data());
          {
            u32 idx = 0;
            for (auto mesh : model0.meshes) {
              if (TreeNode(("Mesh [" + to_string(idx++) + "]").data())) {
                show_mesh_state(mesh);
              }
            }
            ImGui::End();
          }
        });
  }
  CHECKRE(vkDeviceWaitIdle(xofo::vk));
}
