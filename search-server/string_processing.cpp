#include "string_processing.h"
std::vector<std::string_view> SplitIntoWords(const std::string_view & text) {
  std::vector<std::string_view> words;
  auto pointer = &text[0];
  int length = 0;
  for (const char& c : text) {
    if (c == ' ') {
      if (length != 0) {
        words.emplace_back(pointer, length);
        length = 0;
      }
    } else {
        if (length == 0) {
            pointer = &c;
        }
      length += 1;
    }
  }
  if (length != 0) {
    words.emplace_back(pointer, length);
  }

  return words;
}
