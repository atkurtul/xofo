#ifndef E7EA5D0B_5989_44BF_A123_639B65E5D7ED
#define E7EA5D0B_5989_44BF_A123_639B65E5D7ED
#include <typedefs.h>
#include <vulkan/vulkan_core.h>
#include <memory>
#include <optional>



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
    VkResult re = (expr);                                                      \
    if (re) {                                                                  \
      printf("Error: %s\n %s:%d\n", vk_result_string(re), __FILE__, __LINE__); \
      abort();                                                                 \
    }                                                                          \
  }

#endif /* E7EA5D0B_5989_44BF_A123_639B65E5D7ED */
