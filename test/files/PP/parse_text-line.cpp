
int main() {
    int n = 10; //comment
  double d = 1.0;

  int add = n + 10;

  /*
  block comment

  */char c = 'a';

  char8_t u8str[] = u8"test string";

  char rawstr[] = R"+*(test raw string)+*";

  auto udl1 = "test udl"sv;
  auto udl2 = U"test udl"s;
  auto udl3 = L"test udl"sv;
  auto udl4 = u8"test udl"sv;
  auto udl5 = u"test udl"sv;
  auto udl6 = u8R"(test udl
newline)"_udl;

  constexpr int num = 123'456'789;

  float f1 = 1.0E-8f;
  volatile float f2 = .1F;
  double d1 = 0x1.2p3;
  double d2 = 1.;
  long double d3 = 0.1e-1L;
  long double d4 = .1e-1l;
  const double d5 = 3.1415_udl;

  return n;
}