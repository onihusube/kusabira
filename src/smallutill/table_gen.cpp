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
    //%:, %>の時だけ受理
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

//>から始まる記号列のテーブル
int table4(unsigned char c) {
  if (c == '=') {
    //>=
    return 0;
  } else if (c == '>') {
    //>>=を判定するため
    return 1;
  } else {
    //その他の記号
    return -1;
  }
}

//+から始まる記号列のテーブル
int table5(unsigned char c) {
  if (c == '=' || c == '+') {
    //+=, ++
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

//-から始まる記号列のテーブル
int table6(unsigned char c) {
  if (c == '=' || c == '-') {
    //-=, --
    return 0;
  } else if (c == '>') {
    //->, ->*をチェック
    return 12;
  } else {
    //その他の記号
    return -1;
  }
}

//&から始まる記号列のテーブル
int table7(unsigned char c) {
  if (c == '=' || c == '&') {
    //&=, &&
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

//|から始まる記号列のテーブル
int table8(unsigned char c) {
  if (c == '=' || c == '|') {
    //|=, ||
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

//:から始まる記号列のテーブル
int table9(unsigned char c) {
  if (c == '>'/* || c == ':'*/) {
    //:>, ::とは認識しない
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

//.から始まる記号列のテーブル
int table10(unsigned char c) {
  if (c == '*') {
    //.*
    return 0;
  } else if (c == '.') {
    //...(elipsis)のチェック
    return 13;
  } else {
    //その他の記号
    return -1;
  }
}

//<=>のテーブル
int table11(unsigned char c) {
  if (c == '>') {
    //<=>
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

//->*のテーブル
int table12(unsigned char c) {
  if (c == '*') {
    //->*
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

//...のテーブル
int table13(unsigned char c) {
  if (c == '.') {
    //...
    return 0;
  } else {
    //その他の記号
    return -1;
  }
}

void table_generate(int(*table)(unsigned char), int tilde = -1) {
  std::cout << "{";
  //制御文字33文字を飛ばす、!からスタート
  //チルダより後ろは無視
  for (unsigned int i = 33; i < 126; ++i) {
    int out = -1;
    auto c = static_cast<unsigned char>(i);

    if (std::ispunct(c)) {
      out = table(c);
    }
    
    std::cout << out << ", ";
  }
  //最後のチルダについての出力、チルダは1文字記号
  std::cout << tilde << "}";
}

int main()
{
  table_generate(table0, 0);
  std::cout << "," << std::endl;
  table_generate(table1);
  std::cout << "," << std::endl;
  table_generate(table2);
  std::cout << "," << std::endl;
  table_generate(table3);
  std::cout << "," << std::endl;
  table_generate(table4);
  std::cout << "," << std::endl;
  table_generate(table5);
  std::cout << "," << std::endl;
  table_generate(table6);
  std::cout << "," << std::endl;
  table_generate(table7);
  std::cout << "," << std::endl;
  table_generate(table8);
  std::cout << "," << std::endl;
  table_generate(table9);
  std::cout << "," << std::endl;
  table_generate(table10);
  std::cout << "," << std::endl;
  table_generate(table11);
  std::cout << "," << std::endl;
  table_generate(table12);
  std::cout << "," << std::endl;
  table_generate(table13);
}
