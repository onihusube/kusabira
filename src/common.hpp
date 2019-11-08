#pragma once

#include <filesystem>
#include <memory_resource>
#include <optional>
#include <system_error>

#define  fn [[nodiscard]] auto
#define ifn [[nodiscard]] inline auto
#define cfn [[nodiscard]] constexpr auto

namespace kusabira {
  namespace fs = std::filesystem;

  using u8_pmralloc = std::pmr::polymorphic_allocator<char8_t>;

  using maybe_u8str = std::optional<std::pmr::u8string>;

  inline std::pmr::monotonic_buffer_resource def_mr{1024ul};
}

namespace kusabira::PP
{
  /**
  * @brief オートマトンの受理状態を表現する
  * @detail 受理状態か否か、トークナイズエラーが起きたかどうか
  * @detail Unacceptedを境界に、それより小さければエラー、大きければカテゴライズ完了
  * @detail トークナイズ後に識別を容易にするためのもの
  * @detail カテゴリは規格書 5.4 Preprocessing tokens [lex.pptoken]のプリプロセッシングトークン分類とは一致していない（同じものを対象としてはいる）
  */
  enum class pp_tokenize_status : std::int8_t {
    
    UnknownError,
    FailedRawStrLiteralRead,            //生文字列リテラルの読み取りに失敗した、バグの可能性が高い
    RawStrLiteralDelimiterOver16Chars,  //生文字列リテラルデリミタの長さが16文字を超えた
    RawStrLiteralDelimiterInvalid,      //生文字列リテラルデリミタに現れてはいけない文字が現れた
    UnexpectedNewLine,                  //予期しない改行入力があった

    Unaccepted = 0,  //非受理状態、トークンの途中

    Whitespaces,    //ホワイトスペースの列
    LineComment,    //行コメント
    BlockComment,   //コメントブロック、C形式コメント
    Identifier,     //識別子、文字列リテラルのプレフィックス、ユーザー定義リテラル、ヘッダ名、importを含む
    NumberLiteral,  //数値リテラル、サフィックス、ユーザー定義リテラルは含まない
    StringLiteral,  //文字・文字列リテラル、LRUu8等のプレフィックスを含むがユーザ定義リテラルは含まない
    RawStrLiteral,  //生文字列リテラル
    OPorPunc,       //記号列、それが演算子として妥当であるかはチェックしていない
    OtherChar       //その他の非空白文字の一文字
  };


  struct pp_tokenize_result {
    pp_tokenize_status status;

    explicit operator bool() const noexcept {
      return pp_tokenize_status::Unaccepted == this->status;
    }

    fn operator==(pp_tokenize_status status) const noexcept -> bool {
      return this->status == status;
    }
  };

} // namespace kusabira::PP
