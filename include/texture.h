#ifndef BB9D162E_4A9D_444C_B588_D0E6029AEB13
#define BB9D162E_4A9D_444C_B588_D0E6029AEB13

#include <image.h>
#include <string>
#include <memory>


struct Texture : Image  {
  static Box<Texture> mk(std::string file, VkFormat format);
  static Box<Texture> mk(void* data, VkFormat format, u32 width, u32 height);
};


#endif /* BB9D162E_4A9D_444C_B588_D0E6029AEB13 */
