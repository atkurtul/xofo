
#include <buffer.h>
#include <camera.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <mesh_loader.h>
#include <pipeline.h>
#include <vk.h>
#include <vulkan/vulkan_core.h>
#include <cstring>
#include <memory>

int main() {
  Box<Pipeline> pipeline(new Pipeline("shaders/shader"));

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
  // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
  // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForVulkan(vk.win.glfw, true);
  ImGui_ImplVulkan_InitInfo init_info = {
      .Instance = vk.instance,
      .PhysicalDevice = vk.pdev,
      .Device = vk.dev,
      .Queue = vk.queue,
      .MinImageCount = (u32)vk.res.frames.size(),
      .ImageCount = (u32)vk.res.frames.size(),
  };
  {
    VkDescriptorPoolSize pools[3] = {
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1024,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1024,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1024,
        }};

    VkDescriptorPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1024,
        .poolSizeCount = 3,
        .pPoolSizes = pools,
    };

    VkDescriptorPool pool;
    CHECKRE(vkCreateDescriptorPool(vk, &info, 0, &pool));
    init_info.DescriptorPool = pool;
  }

  ImGui_ImplVulkan_Init(&init_info, vk.res.renderpass);

  auto uniform = Buffer::mk(1024, Buffer::Uniform, Buffer::Mapped);

  Box<Buffer> buffer;
  // Texture texture("models/car/posx.jpg", VK_FORMAT_R8G8B8A8_SRGB);
  // auto texture = Texture::mk("blue.png", VK_FORMAT_R8G8B8A8_SRGB);

  vector<Mesh> meshes;
  vector<Material> mats;
  vector<VkDescriptorSet> sets;

  {
    // MeshLoader mesh_loader("teapot.obj", 32);
    // MeshLoader mesh_loader("models/scene_Scene.fbx", 32);
    MeshLoader mesh_loader("models/lightning/0.obj", 32);
    //MeshLoader mesh_loader("models/doom/0.obj", 32);
    // MeshLoader mesh_loader("models/car/car.obj", 32);
    mesh_loader.tex = 12;
    mesh_loader.norm = 20;
    meshes = mesh_loader.import();

    buffer = Buffer::mk(mesh_loader.size,
                        Buffer::Vertex | Buffer::Index | Buffer::Dst,
                        Buffer::Unmapped);

    auto staging = Buffer::mk(mesh_loader.size, Buffer::Src, Buffer::Mapped);
    mats = mesh_loader.load(staging->mapping);
    sets.reserve(mats.size());
    for (auto& mat : mats) {
      auto set = pipeline->alloc_set(1);
      if (mat.diffuse.has_value())
        mat.diffuse.value()->bind_to_set(set, 0);
      if (mat.normal.has_value())
        mat.normal.value()->bind_to_set(set, 1);
      sets.push_back(set);
    }

    vk.execute([&](auto cmd) {
      VkBufferCopy reg = {0, 0, mesh_loader.size};
      vkCmdCopyBuffer(cmd, *staging, *buffer, 1, &reg);
    });
  }

  u32 prev, curr = vk.res.frames.size() - 1;

  auto cam_set = uniform->bind_to_set(pipeline->alloc_set(0), 0);

  Camera cam(1600, 900);

  vk.register_callback([&]() {
    cam.set_prj(vk.res.extent.width, vk.res.extent.height);
    pipeline->reset();
  });

  // Upload Fonts
  {
    vk.execute([](auto cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
  }

  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  while (vk.win.poll()) {
    auto mdelta = vk.win.mdelta * -0.0015f;
    f32 w = vk.win.get_key('W');
    f32 a = vk.win.get_key('A');
    f32 s = vk.win.get_key('S');
    f32 d = vk.win.get_key('D');

    uniform->copy(cam.update(mdelta, s - w, d - a));

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in
    // ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear
    // ImGui!).
    if (show_demo_window)
      ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair
    // to created a named window.
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!"
                                      // and append into it.

      ImGui::Text("This is some useful text.");  // Display some text (you can
                                                 // use a format strings too)
      ImGui::Checkbox(
          "Demo Window",
          &show_demo_window);  // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat(
          "float", &f, 0.0f,
          1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3(
          "clear color",
          (float*)&clear_color);  // Edit 3 floats representing a color

      if (ImGui::Button(
              "Button"))  // Buttons return true when clicked (most widgets
                          // return true when edited/activated)
        counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window) {
      ImGui::Begin(
          "Another Window",
          &show_another_window);  // Pass a pointer to our bool variable (the
                                  // window will have a closing button that will
                                  // clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me"))
        show_another_window = false;
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized =
        (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    // if (!is_minimized) {
    //   memcpy(&wd->ClearValue.color.float32[0], &clear_color, 4 * sizeof(float));
    //   FrameRender(wd, draw_data);
    //   FramePresent(wd);
    // }

    vk.draw([&](auto cmd) {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline,
                              0, 1, &cam_set, 0, 0);

      for (auto& mesh : meshes) {

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline,
                                1, 1, &sets.at(mesh.mat), 0, 0);
        vkCmdBindVertexBuffers(cmd, 0, 1, &buffer->buffer, &mesh.vertex_offset);
        vkCmdBindIndexBuffer(cmd, *buffer, mesh.index_offset,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.indices / 4, 1, 0, 0, 0);
      }

                    // Record dear imgui primitives into command buffer
        ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
    });
  }
  CHECKRE(vkDeviceWaitIdle(vk));
}
