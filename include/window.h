#ifndef AF397561_55AE_459A_8D47_6FDBECDDE96F
#define AF397561_55AE_459A_8D47_6FDBECDDE96F
#include <util.h>
#include <vector>


struct Window {
  struct GLFWwindow* glfw;
  vec2 mpos;
  vec2 mdelta;
  double time = 0;
  double dt = 0;
  VkSurfaceKHR surface;

  std::vector<const char*> init();
  VkResult create_surface(VkInstance instance, int x, int y);

  void free();
  bool poll();

  bool get_key(char);
};
#endif /* AF397561_55AE_459A_8D47_6FDBECDDE96F */
