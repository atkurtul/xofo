#include <vk.h>
#include <vulkan/vulkan_core.h>
#include <cstring>
#include <memory>

Vk vk;

void Resources::init() {
  auto surface = vk.win.surface;

  // commandpool
  VkCommandPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
               VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT};
  
  CHECKRE(vkCreateCommandPool(vk, &pool_info, 0, &pool));

  u32 count;
  VkSurfaceCapabilitiesKHR cap;
  
  CHECKRE(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk, surface, &cap));
  
  extent = cap.currentExtent;
  
  CHECKRE(vkGetPhysicalDeviceSurfaceFormatsKHR(vk, surface, &count, 0));
  
  vector<VkSurfaceFormatKHR> formats(count);
  
  CHECKRE(vkGetPhysicalDeviceSurfaceFormatsKHR(vk, surface, &count,
                                               formats.data()));
  fmt = formats[0];
  //fmt.format = VK_FORMAT_B8G8R8A8_UNORM;

  CHECKRE(vkGetPhysicalDeviceSurfacePresentModesKHR(vk, surface, &count, 0));
  
  VkPresentModeKHR mod = VK_PRESENT_MODE_IMMEDIATE_KHR;
  vector<VkPresentModeKHR> mods(count);
  
  CHECKRE(vkGetPhysicalDeviceSurfacePresentModesKHR(vk, surface, &count,
                                                    mods.data()));
  
  for (int i = 0; i < count; ++i) {
    if (mods[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      mod = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }

  u32 supported;
  
  CHECKRE(vkGetPhysicalDeviceSurfaceSupportKHR(vk, 0, surface, &supported));
  
  VkSwapchainCreateInfoKHR swapchain_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
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
  
  CHECKRE((vkCreateSwapchainKHR(vk, &swapchain_info, 0, &swapchain)));

  depth_buffer = Image::mk(
      VK_FORMAT_D32_SFLOAT, Image::DepthStencil | Image::Transient, extent, 1,
      VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

  // color_buffer =
  //     Image::create(vk, fmt.format, Image::Color | Image::Transient, extent,
  //     1,
  //                   VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

  // renderpass
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
    CHECKRE((vkGetSwapchainImagesKHR(vk, swapchain, &count, 0)));
    vector<VkImage> images(count);
    vector<VkCommandBuffer> cmd(count);
    frames.resize(count);
    CHECKRE((vkGetSwapchainImagesKHR(vk, swapchain, &count, images.data())));
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = count,
    };

    CHECKRE((vkAllocateCommandBuffers(vk, &info, cmd.data())));

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
      CHECKRE(vkCreateFramebuffer(vk, &framebuffer, 0, &frames[i].framebuffer));
      CHECKRE(vkCreateFence(vk, &fence, 0, &frames[i].fence));
      CHECKRE(vkCreateSemaphore(vk, &sp, 0, &frames[i].acquire));
      CHECKRE(vkCreateSemaphore(vk, &sp, 0, &frames[i].present));
    }
  }

  curr = frames.size() - 1;
}

void Resources::free_frames() {
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

void Resources::free() {
  vkDestroySwapchainKHR(vk, swapchain, 0);
  
  free_frames();
  
  depth_buffer.reset();
  
  vkDestroyCommandPool(vk, pool, 0);
  
  vkDestroyRenderPass(vk, renderpass, 0);
}

Vk::Vk() {
  u32 count;
  vector<const char*> ext = win.init();

  VkApplicationInfo app = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_2,
  };
  
  VkInstanceCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app,
      .enabledLayerCount = 1,
      .ppEnabledLayerNames = layer,
      .enabledExtensionCount = (u32)ext.size(),
      .ppEnabledExtensionNames = ext.data(),
  };
  
  CHECKRE(vkCreateInstance(&info, 0, &instance));

  CHECKRE(vkEnumeratePhysicalDevices(instance, &count, 0));
  
  vector<VkPhysicalDevice> buff(count);
  
  CHECKRE(vkEnumeratePhysicalDevices(instance, &count, buff.data()));
  
  pdev = buff.front();
  
  init_device();
  
  win.create_surface(instance, 1600, 900);
  
  res.init();
}

Vk::~Vk() {
  res.free();
  
  win.free();
  
  vmaDestroyAllocator(allocator);

  extern void destroy_samplers();
  
  destroy_samplers();
  
  vkDestroyDevice(dev, 0);

  vkDestroyInstance(instance, 0);
}

void Vk::init_device() {
  {
    float prio = 1.f;

    VkDeviceQueueCreateInfo qinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = 1,
        .pQueuePriorities = &prio};

    const char* ext[] = {"VK_KHR_swapchain"};

    VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qinfo,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = layer,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = ext,
    };

    CHECKRE(vkCreateDevice(pdev, &info, 0, &dev));

    vkGetDeviceQueue(dev, 0, 0, &queue);
  }
  
  {
    VmaAllocatorCreateInfo info = {
        .physicalDevice = pdev,
        .device = dev,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };

    CHECKRE(vmaCreateAllocator(&info, &allocator));
  }
}

VkCommandBuffer Vk::get_cmd() {
  VkCommandBufferAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = res.pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VkCommandBuffer cmd;

  CHECKRE(vkAllocateCommandBuffers(dev, &info, &cmd));

  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

  CHECKRE(vkBeginCommandBuffer(cmd, &begin_info));

  return cmd;
}

void Vk::submit_cmd(VkCommandBuffer cmd) {
  VkSubmitInfo info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
  };

  VkFenceCreateInfo fence_info = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  VkFence fence;

  CHECKRE(vkCreateFence(dev, &fence_info, 0, &fence));

  CHECKRE(vkEndCommandBuffer(cmd));

  CHECKRE(vkQueueSubmit(queue, 1, &info, fence));

  CHECKRE(vkWaitForFences(dev, 1, &fence, 1, -1));

  vkDestroyFence(dev, fence, 0);

  vkFreeCommandBuffers(dev, res.pool, 1, &cmd);
}


void Vk::draw(function<void(VkCommandBuffer)> const& f) {
  u32 prev = res.curr;

  while (vkAcquireNextImageKHR(dev, res.swapchain, -1, res.frames[prev].acquire,
                               0, &res.curr) != VK_SUCCESS) {
    vkDeviceWaitIdle(dev);
    res.free();
    res.init();

    for (auto& callback : callbacks) {
      callback();
    }
  }

  auto& curr = res.frames[res.curr];
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
        {.color = {.float32 = {.2f, 0.4f, .8f, 0}}},
        {.depthStencil = {.depth = 1.f, .stencil = 1}},
    };

    VkRenderPassBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = res.renderpass,
        .framebuffer = curr.framebuffer,
        .renderArea = {.extent = res.extent},
        .clearValueCount = 2,
        .pClearValues = clear,
    };
    vkCmdBeginRenderPass(curr.cmd, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  f(curr.cmd);

  {
    u32 stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    vkCmdEndRenderPass(curr.cmd);
    
    CHECKRE(vkEndCommandBuffer(curr.cmd));
    
    VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &res.frames[prev].acquire,
        .pWaitDstStageMask = &stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &curr.cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &curr.present,
    };
    CHECKRE(vkQueueSubmit(queue, 1, &info, curr.fence));
  }
  
  {
    VkPresentInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &curr.present,
        .swapchainCount = 1,
        .pSwapchains = &res.swapchain,
        .pImageIndices = &res.curr,
    };
    
    vkQueuePresentKHR(queue, &info);
  }
}