#include <xofo.h>


using namespace std;
using namespace xofo;

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
    pipeline.reset();
  }
  ImGui::End();
}

void show_mesh_state(Mesh const& mesh) {
  ImGui::Text("Number of vertices: %lu", mesh.vertices);
  ImGui::Text("Number of indices: %lu", mesh.indices);
  ImGui::Text("Material index %u", mesh.mat);
}

void show_texture(const char* name, Texture const& tex) {
  ImGui::Text("%s: %s", name, tex.origin.data());
  ImGui::Text("Width: %4u Height: %4u", tex.width, tex.height);
}

void show_material(Material const& mat) {
  show_texture("Diffuse", *mat.diffuse);
  show_texture("Normal", *mat.normal);
  show_texture("Metallic", *mat.metallic);
}

void show_model(Model const& model) {
  ImGui::Begin(model.origin.data());
  u32 idx = 0;
  for (auto mesh : model.meshes) {
    if (ImGui::TreeNode(("Mesh [" + to_string(idx++) + "]").data())) {
      show_mesh_state(mesh);
      show_material(model.mats[mesh.mat]);
      ImGui::TreePop();
    }

  }

  ImGui::End();
}

int main() {
  xofo::init();
  auto pipeline = Pipeline::mk("shaders/shader");
  auto skybox = Pipeline::mk("shaders/skybox", PipelineState{.depth_write = 0});

  CubeMap cube_map(*skybox);

  auto grid_pipe = Pipeline::mk(
      "shaders/grid", PipelineState{.polygon = VK_POLYGON_MODE_LINE,
                                    .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                                    .line_width = 1.2f});

  auto uniform =
      xofo::Buffer::mk(65536, xofo::Buffer::Uniform, xofo::Buffer::Mapped);

  Model model0("models/sponza/sponza.gltf", *pipeline);
  Model model1("models/doom/0.obj", *pipeline);
  Model model2("models/batman/0.obj", *pipeline);
  // Model model3("models/heavy_assault_rifle/scene.gltf", *pipeline);
  // model0.scale = 0.1;

  //Model model0("models/Package/AncientTemple.obj", *pipeline);

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

    xofo::draw(
        [&](auto) {
          skybox->bind();
          skybox->push(cam.view);
          skybox->push(cam.prj, 64);

          auto id = mat(1);
          id.translate(cam.pos.xyz);
          cube_map.draw(*skybox, id);

          pipeline->bind();
          pipeline->bind_set([&](auto set) { uniform->bind_to_set(set, 0); },
                             {uniform.get()});

          model0.draw(*pipeline, mat(0.1));
          model1.draw(*pipeline, mat(1));
          model2.draw(*pipeline, mat(1));

          grid_pipe->bind();
          grid_buffer->bind_vertex();
          vkCmdDraw(vk, 8004, 1, 0, 0);
        },
        [&]() {
          using namespace ImGui;
          BeginMainMenuBar();
          {
            if (BeginMenu("File")) {
              void ShowExampleMenuFile();
              ShowExampleMenuFile();
              EndMenu();
            }
          }

          for (auto pipe : Pipeline::pipelines) {
            show_pipeline_state(*pipe);
          }
          show_model(model1);
        });
  }
  CHECKRE(vkDeviceWaitIdle(xofo::vk));
}
