#include <M5StickCPlus.h>
#include <string>
namespace base32 {
  int encodedStringToByteArray(std::string, unsigned char **result, size_t *result_size, bool loose = true);
  int nextMultiple(int num, int multi);
  std::string getBinString(short num, int digit = 16);
  int stringBinToByteArray(std::string stringBin, unsigned char **byteArray);
}
