#include "read_input_functions.h"

int ReadLineWithNumber() {
  int result;
  std::cin >> result;
  ReadLine();
  return result;
}
std::string ReadLine() {
  std::string s;
  getline(std::cin, s);
  return s;
}
