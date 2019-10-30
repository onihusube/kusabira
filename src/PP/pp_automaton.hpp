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

    template <typename NextState, typename NowState>
    void transition(NowState) {
      static_assert(kusabira::sm::transition_check<NowState, NextState, Table>::value, "Invalid state transition.");

      m_state.template emplace<NextState>();
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
  namespace states {
    struct init {};

    /**
    * @brief 文字列リテラルの終了のためなど、この状態は何を読んでもfalseを返しリセットする
    */
    struct end_seq {};

    struct white_space_seq {};

    struct identifier_seq {};

    struct char_literal {};

    struct string_literal {};

    /**
    * @brief　生文字列リテラルの読み取り
    */
    struct raw_string_literal {};

    /**
    * @brief 文字列リテラル中のエスケープシーケンスを無視する
    */
    struct ignore_escape_seq {};

    struct maybe_string_literal {};

    struct maybe_u8string_literal {};

    struct punct_seq {};

    using kusabira::sm::table;
    using kusabira::sm::row;

    using transition_table = table<
        row<init, white_space_seq, identifier_seq, string_literal, char_literal, raw_string_literal, maybe_string_literal, maybe_u8string_literal, punct_seq>,
        row<end_seq, init>,
        row<white_space_seq, init>,
        row<identifier_seq, init>,
        row<string_literal, ignore_escape_seq, end_seq>,
        row<char_literal, ignore_escape_seq, end_seq>,
        row<ignore_escape_seq, string_literal, char_literal>,
        //row<raw_string_literal, init>,
        //row<maybe_string_literal, string_literal, identifier_seq>,
        //row<maybe_u8string_literal, string_literal, raw_string_literal, identifier_seq>,
        row<punct_seq, init>>;
  } // namespace states


  fni is_identifier_char(char8_t ch) -> bool {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == u8'_';
  }

  struct pp_tokenizer_sm
      : protected kusabira::sm::state_base<
            states::transition_table,
            states::init, states::end_seq,
            states::white_space_seq, states::identifier_seq,
            states::string_literal, states::char_literal, states::raw_string_literal, states::maybe_string_literal, states::ignore_escape_seq,
            states::punct_seq>
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
          } else if(ch == u8'U' || ch == u8'L') {
            //文字列リテラルと思しき列の読み込み
            this->transition<states::maybe_string_literal>(state);
          } else if(ch == u8'R') {
            //生文字列リテラルのような列の読み込み
            this->transition<states::maybe_string_literal>(state);
          } else if(ch == u8'u') {
            //u8文字列リテラルっぽい列の読み込み
            this->transition<states::maybe_string_literal>(state);
          } else if (ch == u8'"') {
            //文字列リテラル読み込み
            this->transition<states::string_literal>(state);
          } else if (ch == u8'\'') {
            //文字リテラル読み込み
            this->transition<states::char_literal>(state);
          } else if (std::isalpha(cv) || ch == u8'_') {
            //識別子読み込みモード、識別子の先頭は非数字でなければならない
            this->transition<states::identifier_seq>(state);
          } else if (std::ispunct(cv)) {
            //区切り文字（記号）列読み取りモード
            this->transition<states::punct_seq>(state);
          } else {
            return false;
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
        //文字列リテラル読み込み
        [this](states::string_literal state, char8_t ch) -> bool {
          if (ch == u8'\\') {
            //エスケープシーケンスを無視
            this->transition<states::ignore_escape_seq>(state);
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
            this->transition<states::ignore_escape_seq>(state);
          } else if (ch == u8'\'') {
            //クオートを読んだら次で終了
            this->transition<states::end_seq>(state);
          }
          return true;
        },
        //エスケープシーケンスを飛ばして文字読み込み継続
        [this](states::ignore_escape_seq state, char8_t) -> bool {
          this->transition<states::string_literal>(state);
          return true;
        },
        //識別子読み出し
        [this](states::identifier_seq state, char8_t ch) -> bool {
          if (std::isalnum(static_cast<unsigned char>(ch)) || ch == u8'_') {
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
        [](states::raw_string_literal) -> bool { return true; },
        [](auto &&) -> bool { return false; }
      };

      return this->visit(visitor);
    }
  };

} // namespace kusabira::PP
