#define VMA_IMPLEMENTATION
#include <xofo.h>

#include <GLFW/glfw3.h>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

using namespace xofo;
using namespace std;

static VkDescriptorPool g_imgui_pool;
static VkInstance g_instance;
static VkDevice g_dev;
static VkPhysicalDevice g_pdev;
static VkQueue g_queue;
static VmaAllocator g_allocator;
static struct GLFWwindow* g_glfw;
static VkSurfaceKHR g_surface;
static vec4 g_color = vec4{0.2, 0.2, 0.2, 1};
static vector<function<void(VkExtent2D)>> g_callbacks;
static vector<function<void()>> g_destructors;
static unordered_map<u64, VkSampler> g_samplers;

static vec2 g_mpos;
static vec2 g_mdelta;
static vec2 g_mnorm;
static i32 g_scroll_wheel;
static f64 g_time = 0;
static f64 g_dt = 0;

struct {
  VkCommandPool pool;
  VkSwapchainKHR swapchain;
  VkRenderPass renderpass;
  VkSurfaceFormatKHR fmt;
  VkExtent2D extent;
  Box<Image> depth_buffer;
  u32 curr;

  struct PerFrame {
    VkImage img;
    VkImageView view;
    VkFramebuffer framebuffer;
    VkCommandBuffer cmd;
    VkFence fence;
    VkSemaphore acquire;
    VkSemaphore present;
  };

  vector<PerFrame> frames;

  void init() {
    // commandpool
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                 VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};

    CHECKRE(vkCreateCommandPool(vk, &pool_info, 0, &pool));

    u32 count;
    VkSurfaceCapabilitiesKHR cap;

    CHECKRE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk, g_surface, &cap));

    extent = cap.currentExtent;

    CHECKRE(vkGetPhysicalDeviceSurfaceFormatsKHR(vk, g_surface, &count, 0));

    vector<VkSurfaceFormatKHR> formats(count);

    CHECKRE(vkGetPhysicalDeviceSurfaceFormatsKHR(vk, g_surface, &count,
                                                 formats.data()));
    fmt = formats[0];
    // fmt.format = VK_FORMAT_B8G8R8A8_UNORM;

    CHECKRE(
        vkGetPhysicalDeviceSurfacePresentModesKHR(vk, g_surface, &count, 0));

    VkPresentModeKHR mod = VK_PRESENT_MODE_IMMEDIATE_KHR;
    vector<VkPresentModeKHR> mods(count);

    CHECKRE(vkGetPhysicalDeviceSurfacePresentModesKHR(vk, g_surface, &count,
                                                      mods.data()));

    for (int i = 0; i < count; ++i) {
      if (mods[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        mod = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }
    }

    u32 supported;

    CHECKRE(vkGetPhysicalDeviceSurfaceSupportKHR(vk, 0, g_surface, &supported));

    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = g_surface,
        .minImageCount = cap.minImageCount + 1,
        .imageFormat = fmt.format,
        .imageColorSpace = fmt.colorSpace,
        //.imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        //.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = cap.maxImageArrayLayers,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = mod,
        .clipped = 1,
        .oldSwapchain = 0,
    };

    CHECKRE(vkCreateSwapchainKHR(vk, &swapchain_info, 0, &swapchain));

    depth_buffer =
        Image::mk(VK_FORMAT_D32_SFLOAT, Image::DepthStencil | Image::Transient,
                  extent, 1, Image::Type::DepthBuffer);

    VkAttachmentDescription desc[2] = {
        {
            .format = fmt.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        {
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        }};

    VkAttachmentReference ref[2] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        }};

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ref[0],
        .pDepthStencilAttachment = &ref[1],
    };

    VkRenderPassCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = desc,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    CHECKRE(vkCreateRenderPass(vk, &info, 0, &renderpass));

    // frames
    {
      CHECKRE(vkGetSwapchainImagesKHR(vk, swapchain, &count, 0));
      vector<VkImage> images(count);
      vector<VkCommandBuffer> cmd(count);
      frames.resize(count);
      CHECKRE(vkGetSwapchainImagesKHR(vk, swapchain, &count, images.data()));
      VkCommandBufferAllocateInfo info = {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = pool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = count,
      };

      CHECKRE(vkAllocateCommandBuffers(vk, &info, cmd.data()));

      VkFenceCreateInfo fence = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                 .flags = VK_FENCE_CREATE_SIGNALED_BIT};
      VkSemaphoreCreateInfo sp = {.sType =
                                      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      VkImageViewCreateInfo view = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = fmt.format,
          .subresourceRange = {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .levelCount = 1,
              .layerCount = 1,
          }};

      VkFramebufferCreateInfo framebuffer = {
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
          .renderPass = renderpass,
          .attachmentCount = 2,
          .width = extent.width,
          .height = extent.height,
          .layers = 1,

      };

      for (int i = 0; i < count; ++i) {
        frames[i].cmd = cmd[i];
        view.image = frames[i].img = images[i];
        CHECKRE(vkCreateImageView(vk, &view, 0, &frames[i].view));
        VkImageView attachments[] = {frames[i].view, depth_buffer->view};
        framebuffer.pAttachments = attachments;
        CHECKRE(
            vkCreateFramebuffer(vk, &framebuffer, 0, &frames[i].framebuffer));
        CHECKRE(vkCreateFence(vk, &fence, 0, &frames[i].fence));
        CHECKRE(vkCreateSemaphore(vk, &sp, 0, &frames[i].acquire));
        CHECKRE(vkCreateSemaphore(vk, &sp, 0, &frames[i].present));
      }
    }

    curr = frames.size() - 1;
  }

  void free_frames() {
    vector<VkCommandBuffer> buff(frames.size());
    for (u32 i = 0; i < frames.size(); ++i) {
      buff[i] = frames[i].cmd;

      vkDestroyFramebuffer(vk, frames[i].framebuffer, 0);

      vkDestroyImageView(vk, frames[i].view, 0);

      vkDestroyFence(vk, frames[i].fence, 0);

      vkDestroySemaphore(vk, frames[i].acquire, 0);

      vkDestroySemaphore(vk, frames[i].present, 0);
    }
    vkFreeCommandBuffers(vk, pool, buff.size(), buff.data());
  }

  void free() {
    vkDestroySwapchainKHR(vk, swapchain, 0);

    free_frames();

    depth_buffer.reset();

    vkDestroyCommandPool(vk, pool, 0);

    vkDestroyRenderPass(vk, renderpass, 0);
  }
} g_res;

VulkanProxy::operator VkInstance() {
  return g_instance;
}
VulkanProxy::operator VkDevice() {
  return g_dev;
}
VulkanProxy::operator VkPhysicalDevice() {
  return g_pdev;
}
VulkanProxy::operator VmaAllocator() {
  return g_allocator;
}

VulkanProxy::operator VkCommandBuffer() {
  return g_res.frames[g_res.curr].cmd;
}

static void init_imgui() {
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();
  ImGui::StyleColorsLight();

  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad
  // Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enable Docking
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForVulkan(g_glfw, true);
  ImGui_ImplVulkan_InitInfo init_info = {
      .Instance = vk,
      .PhysicalDevice = vk,
      .Device = vk,
      .Queue = g_queue,
      .MinImageCount = (u32)g_res.frames.size(),
      .ImageCount = (u32)g_res.frames.size(),
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

    CHECKRE(vkCreateDescriptorPool(vk, &info, 0, &g_imgui_pool));
    init_info.DescriptorPool = g_imgui_pool;
  }

  ImGui_ImplVulkan_Init(&init_info, g_res.renderpass);

  {
    xofo::execute([](auto cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
  }
}

void xofo::recreate() {
  vkDeviceWaitIdle(vk);
  g_res.free();
  g_res.init();

  for (auto& callback : g_callbacks) {
    callback(g_res.extent);
  }
}

void xofo::execute(function<void(VkCommandBuffer cmd)> const& f) {
  VkCommandBufferAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = g_res.pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VkCommandBuffer cmd;

  CHECKRE(vkAllocateCommandBuffers(vk, &info, &cmd));

  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  CHECKRE(vkBeginCommandBuffer(cmd, &begin_info));

  f(cmd);

  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
  };

  VkFenceCreateInfo fence_info = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  VkFence fence;

  CHECKRE(vkCreateFence(vk, &fence_info, 0, &fence));

  CHECKRE(vkEndCommandBuffer(cmd));

  CHECKRE(vkQueueSubmit(g_queue, 1, &submit_info, fence));

  CHECKRE(vkWaitForFences(vk, 1, &fence, 1, -1));

  vkDestroyFence(vk, fence, 0);

  vkFreeCommandBuffers(vk, g_res.pool, 1, &cmd);
}

VkExtent2D xofo::extent() {
  return g_res.extent;
}

void xofo::register_recreation_callback(function<void(VkExtent2D)> const& f) {
  g_callbacks.push_back(f);
}

VkRenderPass xofo::renderpass() {
  return g_res.renderpass;
}

u32 xofo::buffer_count() {
  return g_res.frames.size();
}
void ShowExampleMenuFile();
void xofo::draw(function<void(VkCommandBuffer)> const& f,
                function<void()> const& imgui) {
  // imgui
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  if (0 & ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      ShowExampleMenuFile();
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "CTRL+Z")) {
      }
      if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
      }  // Disabled item
      ImGui::Separator();
      if (ImGui::MenuItem("Cut", "CTRL+X")) {
      }
      if (ImGui::MenuItem("Copy", "CTRL+C")) {
      }
      if (ImGui::MenuItem("Paste", "CTRL+V")) {
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  imgui();

  ImGui::Render();
  u32 prev = g_res.curr;
  while (vkAcquireNextImageKHR(vk, g_res.swapchain, -1,
                               g_res.frames[prev].acquire, 0,
                               &g_res.curr) != VK_SUCCESS) {
    xofo::recreate();
  }

  auto& curr = g_res.frames[g_res.curr];
  {
    VkCommandBufferBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    CHECKRE(vkWaitForFences(vk, 1, &curr.fence, 1, -1));

    CHECKRE(vkResetFences(vk, 1, &curr.fence));

    vkResetCommandBuffer(curr.cmd,
                         VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    CHECKRE(vkBeginCommandBuffer(curr.cmd, &info));
  }
  {
    VkClearValue clear[] = {
        {.color = {.float32 = {g_color.x, g_color.y, g_color.z, g_color.w}}},
        {.depthStencil = {.depth = 1.f, .stencil = 1}},
    };

    VkRenderPassBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = g_res.renderpass,
        .framebuffer = curr.framebuffer,
        .renderArea = {.extent = xofo::extent()},
        .clearValueCount = 2,
        .pClearValues = clear,
    };
    vkCmdBeginRenderPass(curr.cmd, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  f(curr.cmd);
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), curr.cmd);

  {
    u32 stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    vkCmdEndRenderPass(curr.cmd);

    CHECKRE(vkEndCommandBuffer(curr.cmd));

    VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &g_res.frames[prev].acquire,
        .pWaitDstStageMask = &stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &curr.cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &curr.present,
    };
    CHECKRE(vkQueueSubmit(g_queue, 1, &info, curr.fence));
  }

  {
    VkPresentInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &curr.present,
        .swapchainCount = 1,
        .pSwapchains = &g_res.swapchain,
        .pImageIndices = &g_res.curr,
    };

    vkQueuePresentKHR(g_queue, &info);
  }
}

void xofo::set_cursor_shape(int shape) {
  static GLFWcursor* cursors[] = {
      glfwCreateStandardCursor(GLFW_ARROW_CURSOR),
      glfwCreateStandardCursor(GLFW_IBEAM_CURSOR),
      glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR),
      glfwCreateStandardCursor(GLFW_HAND_CURSOR),
      glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR),
      glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR),
      glfwCreateStandardCursor(GLFW_CONNECTED),
      glfwCreateStandardCursor(GLFW_DISCONNECTED),
      glfwCreateStandardCursor(GLFW_JOYSTICK_HAT_BUTTONS),
      glfwCreateStandardCursor(GLFW_COCOA_CHDIR_RESOURCES),
  };
  glfwSetCursor(g_glfw, cursors[shape]);
}

void xofo::resize(VkExtent2D v) {
  glfwSetWindowSize(g_glfw, v.width, v.height);
  xofo::recreate();
}

bool xofo::get_key(enum Key key) {
  return glfwGetKey(g_glfw, (i32)key);
}

bool xofo::mbutton(int lr) {
  return glfwGetMouseButton(g_glfw, lr);
}

int xofo::mscroll_wheel() {
  return g_scroll_wheel;
}

void xofo::hide_mouse(bool state) {
  glfwSetInputMode(g_glfw, GLFW_CURSOR,
                   state ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

int xofo::poll() {
  g_scroll_wheel = 0;
  {
    f64 curr = glfwGetTime();
    g_dt = curr - g_time;
    g_time = curr;
  }
  {
    f64 x, y;
    glfwGetCursorPos(g_glfw, &x, &y);
    vec2 curr = vec2{(f32)x, (f32)y};
    g_mdelta = curr - g_mpos;
    g_mpos = curr;
    g_mnorm =
        curr / vec2{(f32)g_res.extent.width, (f32)g_res.extent.height} * 2.f -
        1.f;
  }

  glfwPollEvents();
  return !glfwWindowShouldClose(g_glfw) & !glfwGetKey(g_glfw, GLFW_KEY_ESCAPE);
}

f64 xofo::aspect_ratio() {
  return (f64)g_res.extent.width / g_res.extent.height;
}

f64 xofo::dt() {
  return g_dt;
}

vec2 xofo::mouse_delta() {
  return g_mdelta;
}

vec2 xofo::mouse_norm() {
  return g_mnorm;
}

vec2 xofo::mouse_pos() {
  return g_mpos;
}

void deinit() {
  for (auto& dtor : g_destructors) {
    dtor();
  }

  CHECKRE(vkDeviceWaitIdle(vk));

  // imgui
  {
    vkDestroyDescriptorPool(vk, g_imgui_pool, 0);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  g_res.free();

  glfwDestroyWindow(g_glfw);
  vkDestroySurfaceKHR(vk, g_surface, 0);

  vmaDestroyAllocator(vk);

  for (auto [k, v] : g_samplers) {
    vkDestroySampler(vk, v, 0);
  }

  vkDestroyDevice(vk, 0);

  vkDestroyInstance(g_instance, 0);
}

void xofo::init() {
  u32 count;

  VkApplicationInfo app = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_2,
  };

  const char* layer[] = {"VK_LAYER_KHRONOS_validation"};

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  auto ext = glfwGetRequiredInstanceExtensions(&count);

  VkInstanceCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app,
      .enabledLayerCount = 1,
      .ppEnabledLayerNames = layer,
      .enabledExtensionCount = count,
      .ppEnabledExtensionNames = ext,
  };

  CHECKRE(vkCreateInstance(&info, 0, &g_instance));

  CHECKRE(vkEnumeratePhysicalDevices(g_instance, &count, 0));

  vector<VkPhysicalDevice> buff(count);

  CHECKRE(vkEnumeratePhysicalDevices(g_instance, &count, buff.data()));

  g_pdev = buff.front();

  // device
  {
    float prio = 1.f;

    VkDeviceQueueCreateInfo qinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = 1,
        .pQueuePriorities = &prio};

    const char* ext[] = {"VK_KHR_swapchain"};

    VkPhysicalDeviceFeatures features = {
        .sampleRateShading = 1,
        .fillModeNonSolid = 1,
        .wideLines = 1,
        .samplerAnisotropy = 1,
        .vertexPipelineStoresAndAtomics = 1,
    };

    VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qinfo,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = layer,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = ext,
        .pEnabledFeatures = &features,
    };

    CHECKRE(vkCreateDevice(vk, &info, 0, &g_dev));

    vkGetDeviceQueue(vk, 0, 0, &g_queue);

    VmaAllocatorCreateInfo allocator_info = {
        .physicalDevice = g_pdev,
        .device = g_dev,
        .instance = g_instance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };

    CHECKRE(vmaCreateAllocator(&allocator_info, &g_allocator));
  }

  // window
  {
    g_glfw = glfwCreateWindow(1600, 900, "window", 0, 0);
    glfwSetInputMode(g_glfw, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    // glfwSetInputMode(glfw, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    CHECKRE(glfwCreateWindowSurface(g_instance, g_glfw, 0, &g_surface));

    glfwSetDropCallback(g_glfw, [](auto win, auto i, auto str) {
      cout << "Dropped : " << *str << "\n";
    });

    glfwSetScrollCallback(g_glfw,
                          [](auto win, auto x, auto y) { g_scroll_wheel = y; });
  }

  g_res.init();

  init_imgui();

  atexit(deinit);
}

void xofo::at_exit(const std::function<void()>& f) {
  g_destructors.push_back(f);
}

VkSampler xofo::create_sampler(VkSamplerAddressMode mode, uint mip) {
  u64 key = ((u64)mode << 32) | mip;
  auto existing = g_samplers.find(key);
  if (existing != g_samplers.end())
    return existing->second;

  VkSamplerCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = mode,
      .addressModeV = mode,
      .addressModeW = mode,
      .mipLodBias = 0.0f,
      .anisotropyEnable = 1,
      .maxAnisotropy = 16,
      .compareOp = VK_COMPARE_OP_NEVER,
      .maxLod = (float)mip,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
  };

  VkSampler sampler;
  CHECKRE(vkCreateSampler(vk, &info, 0, &sampler));
  return g_samplers[key] = sampler;
}

void ShowExampleMenuFile()
{
    ImGui::MenuItem("(demo menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {}
    if (ImGui::MenuItem("Open", "Ctrl+O")) {}
    if (ImGui::BeginMenu("Open Recent"))
    {
        ImGui::MenuItem("fish_hat.c");
        ImGui::MenuItem("fish_hat.inl");
        ImGui::MenuItem("fish_hat.h");
        if (ImGui::BeginMenu("More.."))
        {
            ImGui::MenuItem("Hello");
            ImGui::MenuItem("Sailor");
            if (ImGui::BeginMenu("Recurse.."))
            {
                ShowExampleMenuFile();
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {}
    if (ImGui::MenuItem("Save As..")) {}

    ImGui::Separator();
    if (ImGui::BeginMenu("Options"))
    {
        static bool enabled = true;
        ImGui::MenuItem("Enabled", "", &enabled);
        ImGui::BeginChild("child", ImVec2(0, 60), true);
        for (int i = 0; i < 10; i++)
            ImGui::Text("Scrolling Text %d", i);
        ImGui::EndChild();
        static float f = 0.5f;
        static int n = 0;
        ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
        ImGui::InputFloat("Input", &f, 0.1f);
        ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Colors"))
    {
        float sz = ImGui::GetTextLineHeight();
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImGui::GetColorU32((ImGuiCol)i));
            ImGui::Dummy(ImVec2(sz, sz));
            ImGui::SameLine();
            ImGui::MenuItem(name);
        }
        ImGui::EndMenu();
    }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options") twice.
    // In a real code-base using it would make senses to use this feature from very different code locations.
    if (ImGui::BeginMenu("Options")) // <-- Append!
    {
        static bool b = true;
        ImGui::Checkbox("SomeOption", &b);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Disabled", false)) // Disabled
    {
        IM_ASSERT(0);
    }
    if (ImGui::MenuItem("Checked", NULL, true)) {}
    if (ImGui::MenuItem("Quit", "Alt+F4")) {}
}