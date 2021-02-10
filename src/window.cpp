
#include <vk.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>


std::vector<const char*> Window::init() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  uint32_t count;
  auto pp = glfwGetRequiredInstanceExtensions(&count);
  return std::vector<const char*>(pp, pp + count);
}

VkResult Window::create_surface(VkInstance instance, int x, int y) {
  glfw = glfwCreateWindow(x, y, "window", 0, 0);
  glfwSetInputMode(glfw, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
  // glfwSetInputMode(glfw, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  return glfwCreateWindowSurface(instance, glfw, 0, &surface);
}

void Window::free() {
  glfwDestroyWindow(glfw);
  vkDestroySurfaceKHR(vk, surface, 0);
}

void Window::hide_mouse() {
  glfwSetInputMode(glfw, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void Window::unhide_mouse() {
  glfwSetInputMode(glfw, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::resize(VkExtent2D v) {
  glfwSetWindowSize(glfw, v.width, v.height);
  vk.recreate();
}

bool Window::poll() {
  static double timer = 0;
  timer += dt;
  if (timer > 0.4 && glfwGetKey(glfw, GLFW_KEY_SPACE)) {
    auto input = glfwGetInputMode(glfw, GLFW_CURSOR);
    glfwSetInputMode(glfw, GLFW_CURSOR,
                     input == GLFW_CURSOR_DISABLED ? GLFW_CURSOR_NORMAL
                                                   : GLFW_CURSOR_DISABLED);
    timer = 0;
    mdelta = {};
    double x, y;
    glfwGetCursorPos(glfw, &x, &y);
    mpos = vec2{(f32)x, (f32)y};
  }
  {
    double curr = glfwGetTime();
    dt = curr - time;
    time = curr;
  }
  {
    double x, y;
    glfwGetCursorPos(glfw, &x, &y);
    vec2 curr = vec2{(f32)x, (f32)y};
    mdelta = curr - mpos;
    mpos = curr;
  }

  glfwPollEvents();
  return !glfwWindowShouldClose(glfw) & !glfwGetKey(glfw, GLFW_KEY_ESCAPE);
}

bool Window::get_key(char key) {
  return glfwGetKey(glfw, key);
}

bool Window::mbutton(int lr) {
  return glfwGetMouseButton(glfw, lr);
}
