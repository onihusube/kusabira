#pragma once

#include <filesystem>
#include <memory_resource>
#include <optional>
#include <forward_list>
#include <system_error>
#ifdef __cpp_impl_three_way_comparison
#include <compare>
#endif

#ifdef __EDG__
  #include "../subprojects/tlexpected/include/tl/expected.hpp"
#else
  #include "tl/expected.hpp"
#endif

#define  fn [[nodiscard]] auto
#define ifn [[nodiscard]] inline auto
#define cfn [[nodiscard]] constexpr auto
#define ffn [[nodiscard]] friend auto

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
    
    UnknownError = std::numeric_limits<std::int8_t>::min(),
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
    DuringRawStr,   //生文字列リテラルの途中、改行時
    OPorPunc,       //演算子等の記号列
    OtherChar,      //その他の非空白文字の一文字
    NewLine,        //改行
    Empty           //空のトークン
  };


  /**
  * @brief pp_tokenize_statusの薄いラッパー
  * @detail テンプレートメソッド（NVI）のように、内部の変更の漏出を抑えるためのもの
  */
  struct pp_tokenize_result {
    pp_tokenize_status status;

    /**
    * @brief 現在の値は受理状態か否か
    * @detail エラーも1つの受理状態として扱う
    * @return 受理状態ならtrue
    */
    explicit operator bool() const noexcept {
      return pp_tokenize_status::Unaccepted != this->status;
    }

#ifdef __cpp_impl_three_way_comparison
    auto operator<=>(const pp_tokenize_result&) const = default;
    bool operator==(const pp_tokenize_result&) const = default;

    fn operator<=>(const pp_tokenize_status ext_status) const noexcept -> std::strong_ordering {
      return this->status <=> ext_status;
    }
#else

    fn operator==(const pp_tokenize_result& that) const noexcept -> bool {
      return this->status == that.status;
    }

    fn operator<(const pp_tokenize_status ext_status) const noexcept -> bool {
      return this->status < ext_status;
    }

    ffn operator<(const pp_tokenize_status ext_status, const pp_tokenize_result& that) noexcept -> bool {
      return ext_status < that.status;
    }
#endif

    fn operator==(const pp_tokenize_status ext_status) const noexcept -> bool {
      return this->status == ext_status;
    }
  };


  /**
  * @brief ソースコードの論理行での1行を表現する型
  * @detail 改行継続（バックスラッシュ+改行）後にソースファイルの物理行との対応を取るためのもの
  */
  struct logical_line {

    //()集成体初期化がポータブルになったならこのコンストラクタは消える定め
    logical_line(u8_pmralloc &alloc, std::size_t line_num)
        : line{alloc}
        , phisic_line{line_num}
        , line_offset{alloc}
    {}

    //論理1行の文字列
    std::pmr::u8string line;

    //元の開始行
    std::size_t phisic_line;

    //1行毎の文字列長、このvectorの長さ=継続行数
    std::pmr::vector<std::size_t> line_offset;
  };


  /**
  * @brief 字句トークン1つを表現する型
  */
  struct lex_token {
    using line_iterator = std::pmr::forward_list<logical_line>::const_iterator;

    //()集成体初期化がポータブルになったならこのコンストラクタは消える定め
    lex_token(kusabira::PP::pp_tokenize_result result, std::u8string_view view, line_iterator line)
      : kind{ std::move(result) }
      , token{ std::move(view) }
      , srcline_ref{ std::move(line) }
    {}

    //トークン種別
    kusabira::PP::pp_tokenize_result kind;

    //トークン文字列
    std::u8string_view token;

    //対応する論理行オブジェクトへのイテレータ
    line_iterator srcline_ref;
  };

} // namespace kusabira::PP