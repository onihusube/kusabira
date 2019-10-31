#pragma once

#include <variant>
#include <cctype>

#include "common.hpp"

namespace kusabira::sm {

  template<typename... Fs>
    struct overloaded : public Fs... {
      using Fs::operator()...;
    };

    template<typename... Fs>
    overloaded(Fs&&...) -> overloaded<std::decay_t<Fs>...>;

  /**
  * @brief 型リストに型が含まれているかを調べる
  * @tparam T 調べたい型
  * @tparam Ts 型リスト
  */
  template <typename T, typename... Ts>
  struct contains : std::disjunction<std::is_same<T, Ts>...> {};

  template <typename T, typename... Ts>
  struct contains<T, std::variant<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {};

  /**
  * @brief 状態遷移テーブルを定義するための型
  */
  template <typename... Rows>
  struct table;

  /**
  * @brief 状態遷移テーブルの行を表す型
  */
  template <typename SrcState, typename... Ts>
  struct row;

  /**
  * @brief 状態遷移テーブルから現状態の遷移リストを取り出す、実装部
  * @detail 与えられたテーブルを行ごとに調べていく
  * @detail プライマリテンプレート、最後まで掘り返したが見つからなかった時
  */
  template <typename SrcState, typename... Ts>
  struct table_search_impl
  {
    //テーブルには指定された状態から始まる遷移は見つからなかった
    static constexpr bool value = false;

    using type = row<void>;
  };

  /**
  * @brief 状態遷移テーブルから現状態の遷移リストを取り出す、実装部
  * @detail 与えられたテーブルを行ごとに調べていく
  * @detail 部分特殊化、見つかった時
  */
  template <typename SrcState, typename... StateList, typename... Ts>
  struct table_search_impl<SrcState, row<SrcState, StateList...>, Ts...>
  {
    //遷移リストが見つかった
    static constexpr bool value = true;

    using type = row<SrcState, StateList...>;
  };

  /**
  * @brief 状態遷移テーブルから現状態の遷移リストを取り出す、実装部
  * @detail 与えられたテーブルを行ごとに調べていく
  * @detail 部分特殊化、探索中・・・
  */
  template <typename SrcState, typename Head, typename... StateList, typename... Ts>
  struct table_search_impl<SrcState, row<Head, StateList...>, Ts...> : table_search_impl<SrcState, Ts...> {};

  /**
  * @brief 状態遷移テーブルから現状態の遷移リストを取り出す
  */
  template <typename SrcState, typename... Ts>
  struct table_search;

  template <typename SrcState, typename... Ts>
  struct table_search<SrcState, table<Ts...>> : table_search_impl<SrcState, Ts...> {};

  /**
  * @brief 遷移リスト内に次の状態が含まれているかを調べる
  */
  template <typename NextState, typename Row>
  struct contains_list;

  template <typename SrcState, typename NextState, typename... Ts>
  struct contains_list<NextState, row<SrcState, Ts...>> : contains<NextState, Ts...> {};

  /**
  * @brief 状態遷移テーブルから状態遷移の妥当性をチェックする
  * @tparam SrcState 現状態
  * @tparam NextState 遷移先
  * @tparam Table 状態遷移テーブル
  */
  template <typename SrcState, typename NextState, typename Table>
  struct transition_check
  {
    using search = table_search<SrcState, Table>;
    using pick_row = typename search::type;

    //該当行が見つかっており、かつ行内にNextStateが見つかったらtrue
    static constexpr bool value = std::conjunction_v<search, contains_list<NextState, pick_row>>;
  };

  /**
    * @brief Stateの共通部分となるクラス
    * @tparam Table 状態遷移テーブル
    * @tparam States... 取りうる状態の型のリスト
    * @detail 継承して使用する
    */
  template <typename Table, typename... States>
  class state_base {

    std::variant<States...> m_state;

  protected:

    template <typename NextState, typename NowState, typename... Args>
    void transition(NowState, Args&&... args) {
      //状態遷移が妥当かを状態遷移表をもとにチェック
      static_assert(kusabira::sm::transition_check<NowState, NextState, Table>::value, "Invalid state transition.");

      m_state.template emplace<NextState>(std::forward<Args>(args)...);
    }

    template <typename F>
    decltype(auto) visit(F &&func) {
      return std::visit(std::forward<F>(func), m_state);
    }

    state_base() = default;
    ~state_base() = default;
  };

} // namespace kusabira::sm

namespace kusabira::PP
{

  /**
  * @brief プリプロセッシングトークンを分割する状態機械の状態型定義
  */
  namespace states {
    struct init {};

    /**
    * @brief 文字列リテラルの終了のためなど、この状態は何を読んでもfalseを返しリセットする
    */
    struct end_seq {};

    struct white_space_seq {};

    struct identifier_seq {};

    /**
    * @brief 文字・文字列リテラルっぽい文字
    */
    struct maybe_str_literal {};

    /**
    * @brief UTF-8文字・文字列リテラルっぽい文字
    */
    struct maybe_u8str_literal {};

    /**
    * @brief uR""の形の文字列リテラルかもしれない・・・
    */
    struct maybe_uR_prefix {};

    /**
    * @brief 生文字列リテラルっぽい文字
    */
    struct maybe_rawstr_literal{};

    /**
    * @brief 生文字列リテラル
    */
    struct raw_string_literal {};

    struct char_literal {};

    struct string_literal {};

    struct number_literal {};

    struct number_sign {};

    /**
    * @brief 文字列リテラル中のエスケープシーケンスを無視する
    */
    struct ignore_escape_seq {

      //文字と文字列、どちらを読み取り中なのか?
      //trueで文字列リテラル
      bool is_string_parsing;

      ignore_escape_seq(bool is_str_parsing)
       : is_string_parsing(is_str_parsing)
      {}
    };

    struct punct_seq {};


    using kusabira::sm::table;
    using kusabira::sm::row;

    using transition_table = table<
        row<init, white_space_seq, identifier_seq, maybe_str_literal, maybe_u8str_literal, maybe_rawstr_literal, raw_string_literal, string_literal, char_literal, number_literal, number_sign, punct_seq, end_seq>,
        row<end_seq, init>,
        row<white_space_seq, init>,
        row<identifier_seq, init>,
        row<maybe_str_literal, string_literal, char_literal, maybe_rawstr_literal, identifier_seq>,
        row<maybe_u8str_literal, string_literal, char_literal, maybe_str_literal, maybe_rawstr_literal, identifier_seq>,
        row<maybe_rawstr_literal, raw_string_literal, identifier_seq>,
        row<raw_string_literal, end_seq>,
        row<string_literal, ignore_escape_seq, end_seq>,
        row<char_literal, ignore_escape_seq, end_seq>,
        row<ignore_escape_seq, string_literal, char_literal>,
        row<number_literal, number_sign, init>,
        row<number_sign, number_literal, init>,
        row<punct_seq, init>>;
  } // namespace states

  /**
  * @brief プリプロセッシングトークンのカテゴリ
  * @detail トークナイズ後に識別を容易にするためのもの
  * @detail 規格書 5.4 Preprocessing tokens [lex.pptoken]のプリプロセッシングトークン分類とは一致していない（同じものを対象としてはいる）
  */
  enum class pp_token_category : unsigned int {
    Whitspaces,     //ホワイトスペースの列、もしくはコメント
    Identifier,     //識別子、文字列リテラルのプレフィックス、ユーザー定義リテラル、ヘッダ名、importを含む
    StringLiteral,  //文字・文字列リテラル、LRUu8等のプレフィックスは含まない
    NumberLiteral,  //数値リテラル
    OPorPunc,       //記号列、それが演算子として妥当であるかはチェックしていない
    OtherChar       //その他の非空白文字の一文字
  };


  fni is_nondigit(char8_t ch) -> bool {
    return std::isalpha(static_cast<unsigned char>(ch)) or ch == u8'_';
  }

  struct pp_tokenizer_sm
      : protected kusabira::sm::state_base<
            states::transition_table,
            states::init, states::end_seq,
            states::white_space_seq, states::identifier_seq,
            states::maybe_str_literal, states::maybe_rawstr_literal, states::maybe_u8str_literal,
            states::raw_string_literal,
            states::string_literal, states::char_literal, states::ignore_escape_seq,
            states::number_literal, states::number_sign, states::punct_seq>
  {

    pp_tokenizer_sm() = default;

    /**
    * @brief トークン終端を検出した時、次のトークン認識を開始する
    * @detail 初期状態に戻し、現在の入力文字をもとに次のトークン認識を開始する
    * @tparam NosWtate 今の状態型
    * @param state 現在の状態
    * @return false
    */
    template<typename NowState>
    fn restart(NowState state, char8_t c) -> bool {
      this->transition<states::init>(state);
      [[maybe_unused]] bool discard = this->read(c);
  
      return false;
    }

    fn read(char8_t c) -> bool {

      auto visitor = kusabira::sm::overloaded{
        //最初の文字による初期状態決定
        [this](states::init state, char8_t ch) -> bool {
          //Cの<ctype>関数群は文字をunsigned charとして扱うのでキャストしておく
          auto cv = static_cast<unsigned char>(ch);

          if (std::isspace(cv)) {
            //ホワイトスペース列読み込みモード
            this->transition<states::white_space_seq>(state);
          } else if (ch == u8'R') {
            //生文字列リテラルの可能性がある
            this->transition<states::maybe_rawstr_literal>(state);
          } else if (ch == u8'L' or ch == u8'R' or ch == u8'U' ) {
            //文字列リテラルの可能性がある
            this->transition<states::maybe_str_literal>(state);
          } else if (ch == u8'u') {
            //u・u8リテラルの可能性がある
            this->transition<states::maybe_u8str_literal>(state);
          } else if (ch == u8'"') {
            //文字列リテラル読み込み
            this->transition<states::string_literal>(state);
          } else if (ch == u8'\'') {
            //文字リテラル読み込み
            this->transition<states::char_literal>(state);
          } else if (is_nondigit(ch)) {
            //識別子読み込みモード、識別子の先頭は非数字でなければならない
            this->transition<states::identifier_seq>(state);
          } else if (std::isdigit(cv)) {
            //数値リテラル読み取り
            this->transition<states::number_literal>(state);
          } else if (std::ispunct(cv)) {
            //区切り文字（記号）列読み取りモード
            this->transition<states::punct_seq>(state);
          } else {
            //その他上記に当てはまらない非空白文字、1文字づつ分離
            this->transition<states::end_seq>(state);
          }
          return true;
        },
        //トークン終端を検出後に次のトークン読み込みを始める、文字列リテラル等の時
        [this](states::end_seq state, char8_t ch) -> bool {
          return this->restart(state, ch);
        },
        //ホワイトスペースシーケンス読み出し
        [this](states::white_space_seq state, char8_t ch) -> bool {
          if (std::isspace(static_cast<unsigned char>(ch))) {
            return true;
          } else {
            return this->restart(state, ch);
          }
        },
        //文字列リテラル判定
        [this](states::maybe_str_literal state, char8_t ch) -> bool {
          if (ch == u8'R') {
            //生文字列リテラルっぽい
            this->transition<states::maybe_rawstr_literal>(state);
          } else if (ch == u8'\'') {
            //文字リテラル確定
            this->transition<states::char_literal>(state);
          } else if (ch == u8'"') {
            //文字列リテラル確定
            this->transition<states::string_literal>(state);
          } else {
            //本当は不正確、例えば記号が来た時は明らかにエラー
            this->transition<states::identifier_seq>(state);
          }
          return true;
        },
        //u・u8リテラル判定
        [this](states::maybe_u8str_literal state, char8_t ch) -> bool {
          if (ch == u8'8') {
            //u8から始まる文字・文字列リテラルっぽい
            this->transition<states::maybe_str_literal>(state);
          } else if (ch == u8'\'') {
            //u''の形の文字リテラル確定
            this->transition<states::char_literal>(state);
          } else if (ch == u8'"') {
            //u""の形の文字列リテラル確定
            this->transition<states::string_literal>(state);
          } else if (ch == u8'R') {
            //uRの形の文字列リテラルっぽい
            this->transition<states::maybe_rawstr_literal>(state);
          } else {
            //本当は不正確、例えば記号が来た時は明らかにエラー
            this->transition<states::identifier_seq>(state);
          }
          return true;
        },
        //生文字列リテラル判定
        [this](states::maybe_rawstr_literal state, char8_t ch) -> bool {
          if (ch == u8'"') {
            //生文字列リテラル確定
            this->transition<states::raw_string_literal>(state);
          } else {
            //本当は不正確、例えば記号が来た時は明らかにエラー
            this->transition<states::identifier_seq>(state);
          }
          return true;
        },
        //生文字列リテラル読み込み
        [this](states::raw_string_literal state, char8_t ch) -> bool {
          if (ch == u8'"') {
            this->transition<states::end_seq>(state);
            return false;
          } else {
            return true;
          }
        },
        //文字列リテラル読み込み
        [this](states::string_literal state, char8_t ch) -> bool {
          if (ch == u8'\\') {
            //エスケープシーケンスを無視
            this->transition<states::ignore_escape_seq>(state, true);
          } else if (ch == u8'"') {
            //クオートを読んだら次で終了
            this->transition<states::end_seq>(state);
          }
          return true;
        },
        //文字リテラル読み込み
        [this](states::char_literal state, char8_t ch) -> bool {
          if (ch == u8'\\') {
            //エスケープシーケンスを無視
            this->transition<states::ignore_escape_seq>(state, false);
          } else if (ch == u8'\'') {
            //クオートを読んだら次で終了
            this->transition<states::end_seq>(state);
          }
          return true;
        },
        //エスケープシーケンスを飛ばして文字読み込み継続
        [this](states::ignore_escape_seq state, char8_t) -> bool {
          if (state.is_string_parsing == true) {
            this->transition<states::string_literal>(state);
          } else {
            this->transition<states::char_literal>(state);
          }
          return true;
        },
        //識別子読み出し
        [this](states::identifier_seq state, char8_t ch) -> bool {
          if (std::isalnum(static_cast<unsigned char>(ch)) or ch == u8'_') {
            return true;
          } else {
            return this->restart(state, ch);
          }
        },
        //数値リテラル読み出し
        [this](states::number_literal state, char8_t ch) -> bool {
          if (std::isdigit(static_cast<unsigned char>(ch))) {
            //数字
            return true;
          } else if (ch == u8'\'') {
            //区切り文字、2つ続かないはずだけどここではチェックしないことにする・・・
            return true;
          } else if (ch == u8'e' or ch == u8'E' or ch == u8'p' or ch == u8'P') {
            //浮動小数点リテラルの指数部、符号読み取りへ
            this->transition<states::number_sign>(state);
            return true;
          } else if(is_nondigit(ch) or ch == u8'.') {
            //16進文字や小数点、サフィックス、ユーザー定義リテラル
            return true;
          } else {
            return this->restart(state, ch);
          }
        },
        //浮動小数点数の指数部の符号読み出し
        [this](states::number_sign state, char8_t ch) -> bool {
          if (ch == u8'-' or ch == u8'+') {
            this->transition<states::number_literal>(state);
            return true;
          } else {
            return this->restart(state, ch);
          }
        },
        //記号列の読み出し
        [this](states::punct_seq state, char8_t ch) -> bool {
          if (std::ispunct(static_cast<unsigned char>(ch))) {
            return true;
          } else {
            return this->restart(state, ch);
          }
        },
        //知らない人対策
        [](auto&&, auto) { return false;}
      };

      return this->visit([&visitor, c](auto state) {
        return visitor(state, c);
      });
    }

    fn should_line_continue() -> bool {
      auto visitor = kusabira::sm::overloaded{
        [](auto &&) -> bool { return false; }
      };

      return this->visit(visitor);
    }
  };

} // namespace kusabira::PP
