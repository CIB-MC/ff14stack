#include "base32.hpp"
int base32::encodedStringToByteArray(std::string encoded_str, unsigned char **result, size_t *result_size, bool loose) {
  int strLen = encoded_str.length();
  const char *encodedChars = encoded_str.c_str();
  char *validChars = (char *)malloc(sizeof(char) * (size_t)strLen);
  if (validChars == NULL) return 1;

  int validCharsCount = 0;
  for (int i = 0; i < strLen; i++) {
    char inspectee = encodedChars[i];
    if(
      (0x41 <= inspectee && inspectee <= 0x5a) || // if "A to Z"
      (0x32 <= inspectee && inspectee <= 0x37) // if "2 to 7"
    ) {
      validChars[validCharsCount++] = inspectee;
    } else if (0x3d == inspectee) {
      break; // end check if equal.
    } else if (loose && (0x20 == inspectee)) {
      continue; // ignore space if loose mode.
    } else {
      return 2; // Invalid charactor is included.
    }
  }

  size_t fiveBitArraySize = (size_t)(nextMultiple(validCharsCount * 5, 40) * sizeof(short));
  short *fiveBitArray = (short *)malloc(fiveBitArraySize);
  if (fiveBitArray == NULL) return 3;
  memset(fiveBitArray, 0, fiveBitArraySize);

  for(int i = 0; i < validCharsCount; i++) {
    char validChar = validChars[i];
    if(0x41 <= validChar && validChar <= 0x5a){
      fiveBitArray[i] = validChar - 0x41;
    } else if (0x32 <= validChar && validChar <= 0x37) {
      fiveBitArray[i] = validChar - 0x32 + 26;
    } else {
      return 4;
    }
  }
  free(validChars);
  std::string bitsStr = "";
  for (int i = 0; i < validCharsCount; i++) {
    bitsStr += getBinString(fiveBitArray[i], 5);
  }
  free(fiveBitArray);

  unsigned char* res = NULL;
  stringBinToByteArray(bitsStr, &res);
  size_t bytesLength = bitsStr.length() / 8;
  (*result) = res;
  (*result_size) = bytesLength;

  return 0;
}

int base32::nextMultiple(int num, int multi) {
  int current_multi = num / multi;
  return multi * (current_multi + 1);
}

std::string base32::getBinString(short num, int digit) {
  std::string result = "";
  unsigned short mask = 32768;
  mask = mask >> (16 - digit);
  for (int i = 16 - digit; i < 16; i++) {
    result += ((num & mask) == mask) ? "1" : "0";
    mask = mask >> 1;
  }
  return result;
}

int base32::stringBinToByteArray(std::string stringBin, unsigned char** byteArray) {
  if(stringBin.length() % 8 != 0) return 1;
  unsigned char* res = (unsigned char*)malloc(sizeof(unsigned char) * (stringBin.length() / 8));
  if(res == NULL) return 2;
  for (int i = 0; i < stringBin.length(); i += 8) {
    res[i / 8] = (
      (stringBin.compare(i + 0, 1, "1") ? 0 : 128) +
      (stringBin.compare(i + 1, 1, "1") ? 0 : 64) +
      (stringBin.compare(i + 2, 1, "1") ? 0 : 32) +
      (stringBin.compare(i + 3, 1, "1") ? 0 : 16) +
      (stringBin.compare(i + 4, 1, "1") ? 0 : 8) +
      (stringBin.compare(i + 5, 1, "1") ? 0 : 4) +
      (stringBin.compare(i + 6, 1, "1") ? 0 : 2) +
      (stringBin.compare(i + 7, 1, "1") ? 0 : 1)
    );
  }
  (*byteArray) = res;
  return 0;
}
