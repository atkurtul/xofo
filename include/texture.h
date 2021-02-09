#ifndef BB9D162E_4A9D_444C_B588_D0E6029AEB13
#define BB9D162E_4A9D_444C_B588_D0E6029AEB13

#include <image.h>
#include <string>
#include <memory>


struct Texture  {
  std::unique_ptr<Image> image;
  Texture(std::string const& file, VkFormat format);
};


#endif /* BB9D162E_4A9D_444C_B588_D0E6029AEB13 */
