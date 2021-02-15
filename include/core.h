#ifndef E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3
#define E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "typedefs.h"
#include "vk_mem_alloc.h"

#include <imgui.h>
#include <imgui_internal.h>

template <class T>
using Box = std::unique_ptr<T>;
template <class T>
using Rc = std::shared_ptr<T>;
template <class T>
using Opt = std::optional<T>;

const char* vk_result_string(VkResult re);
const char* desc_type_string(VkDescriptorType ty);
#define CHECKRE(expr)                                                          \
  {                                                                            \
    while (VkResult re = (expr)) {                                             \
      printf("Error: %s\n %s:%d\n", vk_result_string(re), __FILE__, __LINE__); \
      abort();                                                                 \
    }                                                                          \
  }

#define ARRSIZE(arr) (sizeof(arr) / sizeof(arr[0]))

namespace xofo {
enum class Key;

struct ShaderResource {};

void init();

void at_exit(std::function<void()> const& f);

VkExtent2D extent();
void recreate();
void execute(std::function<void(VkCommandBuffer cmd)> const& f);

void register_recreation_callback(std::function<void(VkExtent2D)> const& f);
void draw(std::function<void(VkCommandBuffer)> const&,
          std::function<void()> const& imgui);

VkRenderPass renderpass();
u32 buffer_count();

void set_cursor_shape(int shape);
void resize(VkExtent2D x);
bool get_key(Key);
bool mbutton(int lr);
i32 mscroll_wheel();
void hide_mouse(bool state);

i32 poll();
f64 dt();
f64 aspect_ratio();
vec2 mouse_delta();
vec2 mouse_norm();
vec2 mouse_pos();

VkSampler create_sampler(VkSamplerAddressMode mode, uint mip);

static struct VulkanProxy {
  operator VkInstance();
  operator VkDevice();
  operator VkPhysicalDevice();
  operator VmaAllocator();
  operator VkCommandBuffer();
} vk;

enum class Key {
  SPACE = 32,
  APOSTROPHE = 39,
  COMMA = 44,
  MINUS = 45,
  PERIOD = 46,
  SLASH = 47,
  N0 = 48,
  N1 = 49,
  N2 = 50,
  N3 = 51,
  N4 = 52,
  N5 = 53,
  N6 = 54,
  N7 = 55,
  N8 = 56,
  N9 = 57,
  SEMICOLON = 59,
  EQUAL = 61,
  A = 65,
  B = 66,
  C = 67,
  D = 68,
  E = 69,
  F = 70,
  G = 71,
  H = 72,
  I = 73,
  J = 74,
  K = 75,
  L = 76,
  M = 77,
  N = 78,
  O = 79,
  P = 80,
  Q = 81,
  R = 82,
  S = 83,
  T = 84,
  U = 85,
  V = 86,
  W = 87,
  X = 88,
  Y = 89,
  Z = 90,
  LEFT_BRACKET = 91,
  BACKSLASH = 92,
  RIGHT_BRACKET = 93,
  GRAVE_ACCENT = 96,
  WORLD_1 = 161,
  WORLD_2 = 162,
  ESCAPE = 256,
  ENTER = 257,
  TAB = 258,
  BACKSPACE = 259,
  INSERT = 260,
  DELETE = 261,
  RIGHT = 262,
  LEFT = 263,
  DOWN = 264,
  UP = 265,
  PAGE_UP = 266,
  PAGE_DOWN = 267,
  HOME = 268,
  END = 269,
  CAPS_LOCK = 280,
  SCROLL_LOCK = 281,
  NUM_LOCK = 282,
  PRINT_SCREEN = 283,
  PAUSE = 284,
  F1 = 290,
  F2 = 291,
  F3 = 292,
  F4 = 293,
  F5 = 294,
  F6 = 295,
  F7 = 296,
  F8 = 297,
  F9 = 298,
  F10 = 299,
  F11 = 300,
  F12 = 301,
  F13 = 302,
  F14 = 303,
  F15 = 304,
  F16 = 305,
  F17 = 306,
  F18 = 307,
  F19 = 308,
  F20 = 309,
  F21 = 310,
  F22 = 311,
  F23 = 312,
  F24 = 313,
  F25 = 314,
  KP_0 = 320,
  KP_1 = 321,
  KP_2 = 322,
  KP_3 = 323,
  KP_4 = 324,
  KP_5 = 325,
  KP_6 = 326,
  KP_7 = 327,
  KP_8 = 328,
  KP_9 = 329,
  KP_DECIMAL = 330,
  KP_DIVIDE = 331,
  KP_MULTIPLY = 332,
  KP_SUBTRACT = 333,
  KP_ADD = 334,
  KP_ENTER = 335,
  KP_EQUAL = 336,
  LEFT_SHIFT = 340,
  LEFT_CONTROL = 341,
  LEFT_ALT = 342,
  LEFT_SUPER = 343,
  RIGHT_SHIFT = 344,
  RIGHT_CONTROL = 345,
  RIGHT_ALT = 346,
  RIGHT_SUPER = 347,
  MENU = 348,
};

}  // namespace xofo

#endif /* E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3 */
