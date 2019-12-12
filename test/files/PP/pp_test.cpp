#include <iostream>
#include<cmath>
#include "hoge.hpp"
import <type_traits>

#define N 10

int main() {
  int n = 0;//line comment
  /*    
  Block
  Comment
  */
  auto str = u8"string"sv;
  auto rawstr = R"(raw string)"_udl;
  auto rawstr2 = R"+*(raw
string
literal
new line)+*"_udl;
  float f = 1.0E-8f;
}