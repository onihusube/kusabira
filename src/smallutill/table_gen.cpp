#include <iostream>
#include <cstdint>
#include <cctype>

//1文字目の入力に対するテーブル生成
int table0(unsigned char c) {
  if (c == '{' || c == '}' || c == '[' || c == ']' || c == '#' || c == '(' || c == ')' || c == ';' || c == '?' || c == ',' || c == '~') {
    //1文字記号
    return 0;
  } else if (c == '=' || c == '*' || c == '/' || c == '^' || c == '!' ) {
    //2文字目に=のみが来る可能性がある、さもなければ1文字記号
    return 1;
  } else if (c == '%') {
    //%から始まる記号
    return 2;
  } else if (c == '<') {
    //<から始まる記号
    return 3;
  } else if (c == '>') {
    //>から始まる記号
    return 4;
  } else if (c == '+') {
    //+から始まる記号
    return 5;
  } else if (c == '-') {
    //-から始まる記号
    return 6;
  } else if (c == '&') {
    //&から始まる記号
    return 7;
  } else if (c == '|') {
    //|から始まる記号
    return 8;
  } else if (c == ':') {
    //:から始まる記号
    return 9;
  } else if (c == '.') {
    //.から始まる記号
    return 10;
  } else {
    //その他の記号
    return -1;
  }
}

//2文字目が=である記号のテーブル
int table1(unsigned char c) {
  if (c == '=') {
    //=の時だけ受理
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

//%から始まる記号列のテーブル
int table2(unsigned char c) {
  if (c == ':' || c == '>' || c == '=') {
    //=の時だけ受理
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

//<から始まる記号列のテーブル
int table3(unsigned char c) {
  if (c == ':' || c == '%') {
    //代替トークン
    return 0;
  } else if (c == '<') {
    //<<=を判定するため
    return 1;
  } else if (c == '=') {
    //<=>を判定するため
    return 11;
  } else {
    //その他の記号
    return -1;
  }
}

void table_generate(int(*table)(unsigned char)) {
  std::cout << "{";
  for (unsigned int i = 0; i < 127; ++i) {
    int out = -1;
    auto c = static_cast<unsigned char>(i);

    if (std::ispunct(c)) {
      out = table(c);
    }
    
    std::cout << out << ", ";
  }
  std::cout << -1 << "}";
}

int main()
{
  table_generate(table0);
  std::cout << "," << std::endl;
  table_generate(table1);
  std::cout << "," << std::endl;
  table_generate(table2);
}

