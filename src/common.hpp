#pragma once

#include <filesystem>
#include <memory_resource>
#include <optional>
#include <forward_list>
#include <string>
#include <string_view>
#include <compare>
#include <cassert>
#include <iterator>

#ifdef __EDG__
  #include "../subprojects/tlexpected/include/tl/expected.hpp"
  #include <ciso646>
#else
  #include "tl/expected.hpp"
#endif

#include "vocabulary/whimsy_str_view.hpp"
#include "vocabulary/scope.hpp"
#include "PP/op_and_punc_table.hpp"

#define  fn [[nodiscard]] auto
#define sfn [[nodiscard]] static auto
#define ifn [[nodiscard]] inline auto
#define cfn [[nodiscard]] constexpr auto
#define ffn [[nodiscard]] friend auto

namespace kusabira {
  namespace fs = std::filesystem;

  using u8_pmralloc = std::pmr::polymorphic_allocator<char8_t>;

  using maybe_u8str = std::optional<std::pmr::u8string>;

  template<typename T, typename E>
  using expected = tl::expected<T, E>;

  inline std::pmr::monotonic_buffer_resource def_mr{1024ul};
}

namespace kusabira {

  /**
  * @brief 成功を表すexpectedを返す
  * @details returnで使うことを想定
  */
  template<typename T>
  cfn ok(T&& t) -> T&& {
    return std::forward<T>(t);
  }

  /**
  * @brief 失敗を表すexpectedを返す
  * @details returnで使うことを想定
  */
  template<typename E>
  cfn error(E&& e) -> tl::unexpected<E> {
    return tl::unexpected<E>{std::forward<E>(e)};
  }

  /**
  * @brief イテレータをデリファンレンスする
  * @param it イテレータ
  * @details *it
  */
  template<typename I>
  cfn deref(I&& it) -> typename std::iterator_traits<std::remove_cvref_t<I>>::reference {
    return *it;
  }

  /**
  * @brief イテレータをデリファンレンスしてからインクリメントする
  * @param it イテレータ
  * @details std::move(*it), ++i
  */
  template<typename I>
  cfn deref_inc(I&& it) -> typename std::iterator_traits<std::remove_cvref_t<I>>::value_type {
    auto value = std::move(*it);
    ++it;
    return value;
  }
}

namespace kusabira::PP {

  /**
  * @brief ソースコードの論理行での1行を表現する型
  * @detail 改行継続（バックスラッシュ+改行）後にソースファイルの物理行との対応を取るためのもの
  */
  struct logical_line {

    //論理1行の文字列
    std::pmr::u8string line;

    //物理行番号
    const std::size_t phisic_line_num;

    //論理行番号
    const std::size_t logical_line_num;

    //1行毎の文字列長、このvectorの長さ=継続行数
    std::pmr::vector<std::size_t> line_offset;

    logical_line(std::size_t pline_num, std::size_t lline_num)
        : line{&kusabira::def_mr}
        , phisic_line_num{pline_num}
        , logical_line_num{lline_num}
        , line_offset{&kusabira::def_mr}
    {}

    logical_line(logical_line &&) = default;
    logical_line &operator=(logical_line &&) = default;
  };

  // プリプロセッシングトークンと字句トークン型をまとめるようにする変更中
  inline namespace v2 {

    enum class pp_token_category : std::int8_t {

      //トークナイズステータス
      UnknownError = std::numeric_limits<std::int8_t>::min(),
      FailedRawStrLiteralRead,            //生文字列リテラルの読み取りに失敗した、バグの可能性が高い
      RawStrLiteralDelimiterOver16Chars,  //生文字列リテラルデリミタの長さが16文字を超えた
      RawStrLiteralDelimiterInvalid,      //生文字列リテラルデリミタに現れてはいけない文字が現れた
      UnexpectedNewLine,                  //予期しない改行入力があった
      Unaccepted = -1,  //非受理状態、トークンの途中

      //プリプロセッシングトークン分類
      newline = 0,
      whitespace,
      empty,
      line_comment,
      block_comment,

      header_name,
      import_keyword,
      export_keyword,
      module_keyword,
      identifier,
      pp_number,
      charcter_literal,
      user_defined_charcter_literal,

      string_literal,
      user_defined_string_literal,
      //生文字列リテラル識別のため・・・
      raw_string_literal,
      user_defined_raw_string_literal,

      //生文字列リテラルの途中、改行時
      during_raw_string_literal,

      op_or_punc,
      other_character,

      //可変引数が空の時の__VA_ARGS__と__VA_OPT__の置換先
      placemarker_token
    };

    /**
    * @brief pp_token_categoryに演算子オーバーロードを追加する
    * @details 名前空間スコープに演算子オーバーロードをばら撒かないため
    */
    struct add_op {

      pp_token_category& cat;

      /**
      * @brief プリプロセッシングトークンカテゴリを連結する
      * @details 結果がfalseなら変更されない
      * @return 連結が妥当か否かを示すbool値
      */
      [[nodiscard]]
      friend constexpr auto operator+=(add_op lhs, pp_token_category rhs) -> bool {

        assert(pp_token_category::identifier <= lhs.cat and pp_token_category::identifier <= rhs);

        switch (lhs.cat) {
        case pp_token_category::identifier:
          if (rhs == pp_token_category::identifier) {
            //識別子と識別子 -> 識別子
            return true;
          }
          if (rhs == pp_token_category::pp_number) {
            //識別子と数値 -> 識別子
            return true;
          }
          break;
        case pp_token_category::pp_number:
          if (rhs == pp_token_category::pp_number) {
            //数値と数値 -> 数値
            return true;
          }
          if (rhs == pp_token_category::identifier) {
            //数値と識別子 -> ユーザー定義数値リテラル
            return true;
          }
          break;
        case pp_token_category::op_or_punc:
          if (rhs == pp_token_category::op_or_punc) {
            //記号同士 -> 記号（妥当性は外でチェック
            return true;
          }
          break;
        case pp_token_category::charcter_literal: [[fallthrough]];
        case pp_token_category::string_literal: [[fallthrough]];
        case pp_token_category::raw_string_literal:
          if (rhs == pp_token_category::identifier) {
            //文字/文字列リテラルと識別子 -> ユーザー定義文字/文字列リテラル
            using enum_int = std::underlying_type_t<pp_token_category>;
            lhs.cat = static_cast<pp_token_category>(static_cast<enum_int>(lhs.cat) + enum_int(1u));
            return true;
          }
          break;
        case pp_token_category::user_defined_charcter_literal: [[fallthrough]];
        case pp_token_category::user_defined_string_literal: [[fallthrough]];
        case pp_token_category::user_defined_raw_string_literal:
          if (rhs == pp_token_category::identifier) {
            //ユーザー定義文字/文字列リテラルと識別子 -> ユーザー定義文字/文字列リテラル
            return true;
          }
        default:
          break;
        }

        //それ以外はとりあえずダメ
        return false;
      }
    };


    /**
    * @brief プリプロセッシングトークン1つを表現する
    */
    struct pp_token {

      using line_iterator = std::pmr::forward_list<logical_line>::const_iterator;

      //プリプロセッシングトークン種別
      pp_token_category category;

      //プリプロセッシングトークン文字列
      kusabira::vocabulary::whimsy_str_view<> token;

      //トークンの論理行での位置
      std::size_t column;

      //対応する論理行オブジェクトへのイテレータ
      line_iterator srcline_ref;

      //構成するトークン列、結合した場合などにここに直列していく
      std::pmr::forward_list<pp_token> composed_tokens;

      //生成されたトークンであるか否か、生成された場合ソースコードコンテキストに関する情報を持たない
      bool is_generated = false;

      /**
      * @brief コンストラクタ
      * @param cat プリプロセッシングトークンのカテゴリ
      * @param view トークン文字列、std::stringを入れたいときは初期化後に明示的に代入する
      * @param col 論理行上での位置（先頭からの文字数）
      * @param line 論理行オブジェクトへの参照（イテレータ）
      */
      pp_token(pp_token_category cat, std::u8string_view view, std::size_t col, line_iterator line)
        : category{ cat }
        , token{ view }
        , column{ col }
        , srcline_ref{ std::move(line) }
        , composed_tokens{ &kusabira::def_mr}
      {}

      /**
      * @brief コンストラクタ
      * @param cat プリプロセッシングトークンのカテゴリ
      * @param view トークン文字列、std::stringを入れたいときは初期化後に明示的に代入する
      */
      explicit pp_token(pp_token_category cat, std::u8string_view view = {})
        : category{ cat }
        , token{view}
        , column{0}
        , srcline_ref{}
        , composed_tokens{ &kusabira::def_mr }
        , is_generated{true}
      {}

      /**
      * @brief コピーコンストラクタ
      * @details polymorphic_allocatorのmemory_resourceがコピーによって伝播しないのを防ぐために定義
      */
      pp_token(const pp_token& other)
        : category{other.category}
        , token {other.token}
        , column{ other.column }
        , srcline_ref{ other.srcline_ref }
        , composed_tokens{other.composed_tokens, &kusabira::def_mr}
        , is_generated{other.is_generated}
      {}

      /**
      * @brief コピー代入演算子
      * @details コピーコンストラクタと同様の理由による
      */
      pp_token& operator=(const pp_token& other) {
        if (this != &other) {
          pp_token copy = other;
          *this = std::move(copy);
        }
        return *this;
      }

      /**
      * @brief ムーブコンストラクタ
      * @details ムーブ時はmemory_resourceが伝播するのでdefault定義
      */
      pp_token(pp_token&&) = default;
      pp_token& operator=(pp_token&&) = default;

      /**
      * @brief 同値比較演算子
      */
      ffn operator==(const pp_token& lhs, const pp_token& rhs) noexcept -> bool {
        return lhs.category == rhs.category && lhs.token == rhs.token/* && lhs.column == rhs.column*/;
      }

      /**
      * @brief バックスラッシュによる行継続が行われているかを判定する
      * @detail 現在の行は複数の物理行から構成されているかどうか？を取得
      * @return trueなら論理行は複数の物理行で構成される
      */
      fn is_multiple_phlines() const -> bool {
        assert(not is_generated);
        return 0u < (*srcline_ref).line_offset.size();
      }

      /**
      * @brief 対応する論理行全体の文字列を取得する
      * @return 論理行文字列へのconstな参照
      */
      fn get_line_string() const -> const std::pmr::u8string& {
        assert(not is_generated);
        return (*srcline_ref).line;
      }

      /**
      * @brief 対応する物理行上の位置を取得する
      * @return {行, 列}のペア
      */
      fn get_phline_pos() const -> std::pair<std::size_t, std::size_t> {
        assert(not is_generated);

        if (0u < (*srcline_ref).line_offset.size()) {
          //物理行とのズレ
          std::size_t offset{};
          //確認した行までの文字数
          std::size_t total_len{};

          //line_offsetの各要素は行継続前の各物理行の文字数（2文字なら2、100文字なら100）が入っている
          for (auto len : (*srcline_ref).line_offset) {
            //今の行数までのトータル文字数の間に収まっていれば、その行にあったという事
            if (this->column < total_len + len) break;
            //今の行までのトータル文字数を減ずることでその行での本来の文字位置を算出する
            //total_lenの更新はチェックの後
            total_len += len;
            ++offset;
          }

          return { (*srcline_ref).phisic_line_num + offset, this->column - total_len };
        } else {
          return {(*srcline_ref).phisic_line_num, this->column};
        }
      }

      /**
      * @brief 対応する論理行番号を取得する
      * @return 論理行数
      */
      fn get_logicalline_num() const -> std::size_t {
        assert(not is_generated);
        return (*srcline_ref).logical_line_num;
      }


      /**
      * @brief 2つのプリプロセッシングトークンを結合する
      * @details 左辺←右辺に結合、結合が妥当でなくても引数は変更される
      * @param lhs 結合されるトークン、直接変更される（結合が不正でも変更されている）
      * @param rhs 結合するトークン、ムーブす（結合が妥当である時のみ破壊的変更が起きる）
      * @return 結合が妥当であるか否か
      */
      friend auto operator+=(pp_token& lhs, pp_token&& rhs) -> bool {
        /*不正なトークンの結合は未定義動作、とりあえず戻り値で表現
          結合できるのは
          - 識別子と識別子 -> 識別子
          - 数値と数値 -> 数値
          - 記号同士（内容による） -> 記号
          - 識別子と数値 -> 識別子
          - 識別子と（ユーザー定義）文字/文字列リテラル -> 文字/文字列リテラル
          - 文字/文字列リテラルと識別子 -> ユーザー定義文字/文字列リテラル
          - 数値と識別子 -> ユーザー定義数値リテラル
          - ユーザー定義文字/文字列リテラルと識別子 -> ユーザー定義文字/文字列リテラル
          - ユーザー定義数値リテラルと識別子 -> ユーザー定義数値リテラル
        */

        //プレイスメーカートークンが右辺にある時
        if (rhs.category == pp_token_category::placemarker_token) {
          //何もしなくていい
          return true;
        }
        //左辺にあるとき
        if (lhs.category == pp_token_category::placemarker_token) {
          lhs = std::move(rhs);
          return true;
        }

        //構成文字列の連結
        auto&& tmp_str = lhs.token.to_string();
        tmp_str.append(rhs.token);
        lhs.token = std::move(tmp_str);

        if (bool is_concatenated = add_op{lhs.category} += rhs.category; not is_concatenated) {
          //特別扱い
          if (lhs.category != pp_token_category::identifier) return false;
          if (not (pp_token_category::charcter_literal <= rhs.category and rhs.category <= pp_token_category::user_defined_raw_string_literal)) return false;

          //識別子と（ユーザー定義）文字/文字列リテラル -> （ユーザー定義）文字/文字列リテラル
          const auto str = lhs.token.to_view();
          auto index = 0ull;
          bool is_rowstr = false;

          if (str[index] == u8'u') {
            ++index;
            if (str[index] != u8'8') return false;
            ++index;
          } else if (str[index] == u8'U') {
            ++index;
          } else if (str[index] == u8'L') {
            ++index;
          }
          if (str[index] == u8'R') {
            ++index;
            is_rowstr = true;
          }
          if (str[index] != u8'"' and str[index] != u8'\'') return false;
          //文字/文字列リテラルとして妥当だった

          if (is_rowstr) {
            //文字リテラルが来てたらおかしい
            assert(pp_token_category::string_literal <= rhs.category);

            //生文字列リテラル化
            using enum_int = std::underlying_type_t<pp_token_category>;
            lhs.category = static_cast<pp_token_category>(static_cast<enum_int>(rhs.category) + enum_int(2u));
          }
        } else if (lhs.category == pp_token_category::op_or_punc) {
          //記号の妥当性のチェック
          auto row = 0;
          const auto str = lhs.token.to_view();
          const auto len = str.length();
          auto index = 0ull;
          do {
            //トークナイザで使ってるテーブルを引いてチェックする
            row = table::ref_symbol_table(str[index], row);
            ++index;
          } while (0 < row and index < len);

          //ここに来る場合は妥当な記号列同士の連結になっているはずで連結結果が妥当なら文字列を全て見ることになる
          //その上で、テーブル参照結果が0以上なら妥当
          if (index != len or row < 0) return false;
        }

        //トークンを構成するトークン列の連結
        //lhsを構成するトークン > rhs > rhsを構成するトークン、の順序で直列化
        //lhs自身はそのままなので、長さは連結したトークン数-1になる
        std::pmr::forward_list<pp_token> tmp{&kusabira::def_mr};
        tmp.splice_after(tmp.before_begin(), std::move(rhs.composed_tokens));
        tmp.insert_after(tmp.before_begin(), std::move(rhs));
        tmp.splice_after(tmp.before_begin(), std::move(lhs.composed_tokens));
        lhs.composed_tokens = std::move(tmp);

        return true;
      }

    };

    static_assert(std::is_copy_constructible_v<pp_token>);
    static_assert(std::is_copy_assignable_v<pp_token>);
    static_assert(std::is_nothrow_move_constructible_v<pp_token>);
    static_assert(std::is_move_assignable_v<pp_token>);

  } // namespace v2

} // namespace kusabira::PP