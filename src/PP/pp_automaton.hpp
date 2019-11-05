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
    void transition(NowState&&, Args&&... args) {
      //状態遷移が妥当かを状態遷移表をもとにチェック
      static_assert(kusabira::sm::transition_check<std::remove_cvref_t<NowState>, NextState, Table>::value, "Invalid state transition.");

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

    struct maybe_comment {};

    struct line_comment {};

    struct block_comment {};

    struct maybe_end_block_comment {};

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
    struct maybe_rawstr_literal {};

    /**
    * @brief 生文字列リテラル
    * @detail 生文字列リテラルのデリミタと範囲を決定するための特殊処理、プッシュダウンオートマトン風
    */
    struct raw_string_literal {

      std::pmr::u8string m_stack;
      bool m_is_delimiter = true;
      std::size_t m_index = 0;

      raw_string_literal()
        : m_stack{ 1, u8')', kusabira::u8_pmralloc{&kusabira::def_mr} }
      {}

      //バグの元なので削除
      raw_string_literal(const raw_string_literal&) = delete;
      raw_string_literal(raw_string_literal&&) = default;

      /**
      * @brief 生文字列リテラルのデリミタを保存する
      * @param ch 1文字の入力
      * @return falseで終端"を検出
      */
      fn delimiter_push(char8_t ch) -> bool {
        //デリミタ読み取り中、最大16文字らしい・・・・
        if (ch == u8'(') {
          m_is_delimiter = false;
          //)delimiter" の形にしておく
          m_stack.push_back(u8'"');
        } else {
          //delimiterに現れてはいけない文字（閉じかっこ、バックスラッシュ、ホワイトスペース系）が現れたらエラー
          //if (ch ==u8')' || ch == u8'\\' || std::isspace(static_cast<unsigned char>(ch))) return false;
          m_stack.push_back(ch);
        }
        return true;
      }

      /**
      * @brief 生文字列リテラル本体を読む、デリミタをチェックする
      * @param ch 1文字の入力
      * @return falseで終端"を検出
      */
      fn delimiter_check(char8_t ch) -> bool {
        //生文字列本体読み取り中
        if (m_stack[m_index] == ch) {
          //デリミタっぽいものを検出している時
          ++m_index;
          if (m_index == m_stack.length()) return false;
        } else {
          m_index = 0;
        }
        return true;
      }

      /**
      * @brief 生文字列リテラルを読む
      * @param ch 1文字の入力
      * @return falseで終端"を検出
      */
      fn input_char(char8_t ch) -> bool {
        if (m_is_delimiter) {
          return this->delimiter_push(ch);
        } else {
          return this->delimiter_check(ch);
        }
      }
    };

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

    /**
    * @brief 状態遷移テーブル
    * @detail 各行の先頭が始まりの状態、その後はそこからの遷移先の候補、そこに無い遷移先は妥当でない
    */
    using transition_table = table <
        row<init, white_space_seq, maybe_comment, line_comment, block_comment, maybe_end_block_comment, identifier_seq, maybe_str_literal, maybe_u8str_literal, maybe_rawstr_literal, raw_string_literal, string_literal, char_literal, number_literal, number_sign, punct_seq, end_seq > ,
        row<end_seq, init>,
        row<white_space_seq, init>,
        row<maybe_comment, line_comment, block_comment, punct_seq, init>,
        row<line_comment, init>,
        row<block_comment, maybe_end_block_comment, init>,
        row<maybe_end_block_comment, block_comment, init>,
        row<identifier_seq, init>,
        row<maybe_str_literal, string_literal, char_literal, maybe_rawstr_literal, identifier_seq, init>,
        row<maybe_u8str_literal, string_literal, char_literal, maybe_str_literal, maybe_rawstr_literal, identifier_seq, init>,
        row<maybe_rawstr_literal, raw_string_literal, identifier_seq, init>,
        row<raw_string_literal, end_seq>,
        row<string_literal, ignore_escape_seq, end_seq, init>,
        row<char_literal, ignore_escape_seq, end_seq, init>,
        row<ignore_escape_seq, string_literal, char_literal, init>,
        row<number_literal, number_sign, init>,
        row<number_sign, number_literal, init>,
        row<punct_seq, maybe_comment, init>>;
  } // namespace states

  /**
  * @brief プリプロセッシングトークンのカテゴリ
  * @detail トークナイズ後に識別を容易にするためのもの
  * @detail 規格書 5.4 Preprocessing tokens [lex.pptoken]のプリプロセッシングトークン分類とは一致していない（同じものを対象としてはいる）
  */
  enum class pp_token_category : unsigned int {
    Whitspaces,     //ホワイトスペースの列
    LineComment,    //行コメント
    BlockComment,   //コメントブロック、C形式コメント
    Identifier,     //識別子、文字列リテラルのプレフィックス、ユーザー定義リテラル、ヘッダ名、importを含む
    StringLiteral,  //文字・文字列リテラル、LRUu8等のプレフィックスを含むがユーザ定義リテラルは含まない
    RawStrLiteral,  //生文字列リテラルの途中
    RawStrLiteralEnd,//終端を伴う生文字列リテラル
    NumberLiteral,  //数値リテラル
    OPorPunc,       //記号列、それが演算子として妥当であるかはチェックしていない
    OtherChar       //その他の非空白文字の一文字
  };


  fni is_nondigit(char8_t ch) -> bool {
    return std::isalpha(static_cast<unsigned char>(ch)) or ch == u8'_';
  }

  /**
  * @brief 現在の読み取り文字を識別するオートマトン
  * @detail トークナイザ内で使用
  * @detail 1文字づつ入力し、状態を更新していく
  */
  struct pp_tokenizer_sm
      : protected kusabira::sm::state_base<
            states::transition_table,
            states::init, states::end_seq,
            states::maybe_comment, states::line_comment, states::block_comment, states::maybe_end_block_comment,
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
      [[maybe_unused]] bool discard = this->input_char(c);

      return false;
    }

    /**
    * @brief 文字を入力する
    * @detail 状態機械により1文字ごとにトークンを判定してゆく
    * @param c 入力文字
    * @return 受理状態に到達したらfalse
    */
    fn input_char(char8_t c) -> bool {

      auto visitor = kusabira::sm::overloaded{
        //最初の文字による初期状態決定
        [this](states::init state, char8_t ch) -> bool {
          //Cの<ctype>関数群は文字をunsigned charとして扱うのでキャストしておく
          auto cv = static_cast<unsigned char>(ch);

          if (std::isspace(cv)) {
            //ホワイトスペース列読み込みモード
            this->transition<states::white_space_seq>(state);
          } else if (ch == u8'/') {
            this->transition<states::maybe_comment>(state);
          } else if (ch == u8'R') {
            //生文字列リテラルの可能性がある
            this->transition<states::maybe_rawstr_literal>(state);
          } else if (ch == u8'L' or ch == u8'U' ) {
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
        [this](states::maybe_comment state, char8_t ch) -> bool {
          if (ch == u8'/') {
            //行コメント
            this->transition<states::line_comment>(state);
          } else if (ch == u8'*') {
            //ブロックコメント
            this->transition<states::block_comment>(state);
          } else if (std::ispunct(static_cast<unsigned char>(ch))) {
            //他の記号が出てきたら記号列として読み込み継続
            this->transition<states::punct_seq>(state);
          } else {
            //他の文字が出てきたら単体の/演算子だったという事・・・
            return false;
          }
          return true;
        },
        [this](states::line_comment, char8_t) -> bool {
          //改行が現れるまではそのまま・・・
          return true;
        },
        [this](states::block_comment state, char8_t ch) -> bool {
          if (ch == u8'*') {
            //終わりかもしれない・・・
            this->transition<states::maybe_end_block_comment>(state);
          }
          return true;
        },
        [this](states::maybe_end_block_comment state, char8_t ch) -> bool {
          if (ch == u8'/') {
            //ブロック閉じを検出
            this->transition<states::init>(state);
            return false;
          } else {
            //閉じなかったらブロックコメント読み取りへ戻る
            this->transition<states::block_comment>(state);
            return true;
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
            //uRの形の生文字列リテラルっぽい
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
        [this](states::raw_string_literal& state, char8_t ch) -> bool {
          if (state.input_char(ch) == false) {
            this->transition<states::end_seq>(state);
          }
          return true;
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
        [this](states::ignore_escape_seq& state, char8_t) -> bool {
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
            //指数部に符号以外のものが出てきたらエラーでは？
            return this->restart(state, ch);
          }
        },
        //記号列の読み出し
        [this](states::punct_seq state, char8_t ch) -> bool {
          if (ch == u8'/') {
            this->transition<states::maybe_comment>(state);
          } else if (std::ispunct(static_cast<unsigned char>(ch)) == false) {
            return this->restart(state, ch);
          }
          return true;
        },
        //[](auto&&, auto) { return false;}
      };

      return this->visit([&visitor, c](auto& state) {
        return visitor(state, c);
      });
    }

    /**
    * @brief 改行文字を入力する
    * @return 受理状態に到達したらfalse
    */
    fn input_newline() -> bool {

      auto visitor = kusabira::sm::overloaded{
        [](states::block_comment) -> bool { return true; },
        [](states::maybe_end_block_comment) -> bool { return true;},
        [](states::raw_string_literal&) -> bool { return true; },
        //init -> initはテーブルに無い
        [](states::init) -> bool { return false; },
        //生文字列リテラルとコメントブロック以外は改行でトークン分割する
        [this](auto&& state) -> bool {
          this->transition<states::init>(state);
          return false; 
        }
      };

      return this->visit(std::move(visitor));
    }
  };

} // namespace kusabira::PP
