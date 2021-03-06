#pragma once

#include <variant>
#include <cctype>

#include "common.hpp"
#include "op_and_punc_table.hpp"

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
  };

} // namespace kusabira::sm


namespace kusabira::PP
{

  /**
  * @brief プリプロセッシングトークンを分割する状態機械の状態型定義
  */
  namespace states {
    namespace detail {

      /**
      * @brief 生文字列リテラルを読む
      * @detail 生文字列リテラルのデリミタと範囲を決定するための特殊処理、プッシュダウンオートマトン風
      */
      struct rawstr_literal_accepter {

        //std::pmr::u8string m_stack;
        char8_t m_stack[18] = {u8')'};
        bool m_is_delimiter = true;
        std::uint8_t m_index = 0;
        std::uint8_t m_length = 1;

        rawstr_literal_accepter() = default;

        //バグの元なので削除
        rawstr_literal_accepter(const rawstr_literal_accepter &) = delete;
        rawstr_literal_accepter(rawstr_literal_accepter &&) = default;

        /**
        * @brief 生文字列リテラルのデリミタを保存する
        * @param ch 1文字の入力
        * @return 受理状態、デリミタ関連のエラーを報告しうる
        */
        fn delimiter_push(char8_t ch) -> kusabira::PP::pp_token_category {
          //デリミタ読み取り中、最大16文字
          if (ch == u8'(') {
            m_is_delimiter = false;
            //)delimiter" の形にしておく
            m_stack[m_length] = u8'"';
          } else {
            //delimiterに現れてはいけない文字（閉じかっこ、バックスラッシュ、ホワイトスペース系）が現れたらエラー
            if (ch == u8')' || ch == u8'\\' || std::isspace(static_cast<unsigned char>(ch)))
              return { kusabira::PP::pp_token_category::RawStrLiteralDelimiterInvalid };
            //デリミタの長さが16文字を超えたらエラー
            if (17 <= m_length) return { kusabira::PP::pp_token_category::RawStrLiteralDelimiterOver16Chars };
            m_stack[m_length] = ch;
          }
          ++m_length;
          return { kusabira::PP::pp_token_category::Unaccepted };
        }

        /**
        * @brief 生文字列リテラル本体を読む、デリミタをチェックする
        * @param ch 1文字の入力
        * @return 生文字列リテラルとして受理したかどうか、エラーにはならない
        */
        fn delimiter_check(char8_t ch) -> kusabira::PP::pp_token_category {
          //生文字列本体読み取り中
          if (m_stack[m_index] == ch) {
            //デリミタっぽいものを検出している時
            ++m_index;
            if (m_index == m_length) return { kusabira::PP::pp_token_category::raw_string_literal };
          } else {
            m_index = 0;
          }
          return { kusabira::PP::pp_token_category::Unaccepted };
        }

        /**
        * @brief 生文字列リテラルを読む
        * @param ch 1文字の入力
        * @return 受理状態
        */
        fn operator()(char8_t ch) -> kusabira::PP::pp_token_category {
          if (m_is_delimiter) {
            return this->delimiter_push(ch);
          } else {
            return this->delimiter_check(ch);
          }
        }

      };
    } // namespace detail

    /**
    * @brief 初期状態、トークン最初の文字を受け取る
    */
    struct init {};

    /**
    * @brief 文字列リテラルの終了のためなど、この状態は何を読んでもリセットする
    */
    struct end_seq {
      pp_token_category category;

      end_seq(pp_token_category prev_status = {}) : category{prev_status}
      {}
    };

    struct white_space_seq {
      static constexpr pp_token_category Category = pp_token_category::whitespaces;
    };

    struct maybe_comment {
      static constexpr pp_token_category Category = pp_token_category::op_or_punc;
    };

    struct line_comment {
      static constexpr pp_token_category Category = pp_token_category::line_comment;
    };

    struct block_comment {
      static constexpr pp_token_category Category = pp_token_category::block_comment;
    };

    struct maybe_end_block_comment {
      static constexpr pp_token_category Category = pp_token_category::block_comment;
    };

    struct identifier_seq {
      static constexpr pp_token_category Category = pp_token_category::identifier;
    };

    /**
    * @brief 文字・文字列リテラルっぽい文字
    */
    struct maybe_str_literal {
      static constexpr pp_token_category Category = pp_token_category::identifier;
    };

    /**
    * @brief UTF-8文字・文字列リテラルっぽい文字
    */
    struct maybe_u8str_literal {
      static constexpr pp_token_category Category = pp_token_category::identifier;
    };

    /**
    * @brief 生文字列リテラルっぽい文字
    */
    struct maybe_rawstr_literal {
      static constexpr pp_token_category Category = pp_token_category::identifier;
    };

    /**
    * @brief 生文字列リテラル
    */
    struct raw_string_literal {

      static constexpr pp_token_category Category = pp_token_category::raw_string_literal;

      raw_string_literal() = default;

      // 暗黙コピーはバグの元
      raw_string_literal(const raw_string_literal &) = delete;

      raw_string_literal(raw_string_literal&&) = default;
      raw_string_literal& operator=(raw_string_literal&&) = default;

      detail::rawstr_literal_accepter m_reader{};

      /**
      * @brief 生文字列リテラルを読む
      * @param ch 1文字の入力
      * @return falseで終端"を検出
      */
      fn input_char(char8_t ch) -> pp_token_category {
        return m_reader(ch);
      }
    };

    struct char_literal {
      static constexpr pp_token_category Category = pp_token_category::UnexpectedNewLine;
    };

    struct string_literal {
      static constexpr pp_token_category Category = pp_token_category::UnexpectedNewLine;
    };

    struct number_literal {
      static constexpr pp_token_category Category = pp_token_category::pp_number;
    };

    struct number_sign {
      static constexpr pp_token_category Category = pp_token_category::pp_number;
    };

    struct maybe_number_literal {
      static constexpr pp_token_category Category = pp_token_category::op_or_punc;
    };

    /**
    * @brief 文字列リテラル中のエスケープシーケンスを無視する
    */
    struct ignore_escape_seq {

      static constexpr pp_token_category Category = pp_token_category::UnexpectedNewLine;

      //文字と文字列、どちらを読み取り中なのか?
      //trueで文字列リテラル
      bool is_string_parsing;

      ignore_escape_seq(bool is_str_parsing)
       : is_string_parsing(is_str_parsing)
      {}
    };

    /**
    * @brief 演算子記号、区切り文字を認識する
    * @detail 有効でない記号列は認識されず、最小の演算子か記号に分割される
    */
    struct punct_seq {
      static constexpr pp_token_category Category = pp_token_category::op_or_punc;

      int m_index;

      punct_seq(int iddex = 0) : m_index{iddex}
      {}

      // 暗黙コピーはバグの元
      punct_seq(const punct_seq &) = delete;

      punct_seq(punct_seq&&) = default;
      punct_seq& operator=(punct_seq&&) = default;

      /**
      * @brief 演算子、区切り文字をチェックする
      * @param ch 1文字の入力
      * @return 
      */
      fn input_char(char8_t ch) -> int {
        //テーブルを参照して受理すべきかを決定
        m_index = kusabira::table::ref_symbol_table(ch, m_index);

        return m_index;
      }
    };

    using kusabira::sm::table;
    using kusabira::sm::row;

    /**
    * @brief 状態遷移テーブル
    * @detail 各行の先頭が始まりの状態、その後はそこからの遷移先の候補、そこに無い遷移先は妥当でない
    */
    using transition_table = table
      <
        row<init, white_space_seq, maybe_comment, line_comment, block_comment, maybe_end_block_comment, identifier_seq, maybe_str_literal, maybe_u8str_literal, maybe_rawstr_literal, raw_string_literal, string_literal, char_literal, maybe_number_literal, number_literal, number_sign, punct_seq, end_seq > ,
        row<end_seq, init>,
        row<white_space_seq, init>,
        row<maybe_comment, line_comment, block_comment, punct_seq, end_seq, init>,
        row<line_comment, init>,
        row<block_comment, maybe_end_block_comment, init>,
        row<maybe_end_block_comment, block_comment, end_seq, init>,
        row<identifier_seq, init>,
        row<maybe_str_literal, string_literal, char_literal, maybe_rawstr_literal, identifier_seq, init>,
        row<maybe_u8str_literal, string_literal, char_literal, maybe_str_literal, maybe_rawstr_literal, identifier_seq, init>,
        row<maybe_rawstr_literal, raw_string_literal, identifier_seq, init>,
        row<raw_string_literal, end_seq>,
        row<string_literal, ignore_escape_seq, end_seq, init>,
        row<char_literal, ignore_escape_seq, end_seq, init>,
        row<ignore_escape_seq, string_literal, char_literal, init>,
        row<maybe_number_literal, number_literal, punct_seq, end_seq, init>,
        row<number_literal, number_sign, init>,
        row<number_sign, number_literal, init>,
        row<punct_seq, maybe_comment, end_seq, init>
      >;

  } // namespace states


  /**
  * @brief 識別子として受理可能かを調べる
  * @return 受理可能ならtrue
  */
  ifn is_identifier(char8_t ch) -> bool {
    return std::isalnum(static_cast<unsigned char>(ch)) or ch == u8'_';
  }

  /**
  * @brief 数値リテラル（pp-number）として受け入れ可能かを調べる
  * @detail 指数部の符号指定はここには含まれていない
  * @return 受理可能ならtrue
  */
  ifn is_number_literal(char8_t ch) -> bool {
    if (std::isxdigit(static_cast<unsigned char>(ch))) {
      //16進の範囲で使われうる英数字
      return true;
    } else if (ch == u8'x' or ch == u8'\'' or ch == u8'.' or ch == u8'l'  or ch == u8'L' or ch == u8'u'  or ch == u8'U') {
      //16進記号のxかドットか区切り文字、2つ続かないはずだけどここではチェックしないことにする・・・
      //longリテラルサフィックス、unsignedリテラルサフィックス最後以外に来てたらエラーだけど・・・
      return true;
    } else {
      //ユーザー定義リテラルは分割する
      return false;
    }
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
            states::maybe_str_literal, states::maybe_rawstr_literal, states::maybe_u8str_literal, states::raw_string_literal,
            states::string_literal, states::char_literal, states::ignore_escape_seq,
            states::maybe_number_literal, states::number_literal, states::number_sign, states::punct_seq>
  {

    pp_tokenizer_sm() = default;

    /**
    * @brief 現在のトークン読み取りを完了し、初期状態へ戻る
    * @detail 初期状態に戻し、現在の入力文字をもとに次のトークン認識を開始する
    * @tparam NosWtate 今の状態型
    * @param state 現在の状態
    * @param status 返すべき受理状態
    * @return status
    */
    template<typename NowState>
    fn restart(NowState&& state, pp_token_category status) -> pp_token_category {
      this->transition<states::init>(state);
      //[[maybe_unused]] auto discard = this->input_char(c);

      return { status };
    }

    /**
    * @brief 文字を入力する
    * @detail 状態機械により1文字ごとにトークンを判定してゆく
    * @detail トークンによってはそのトークンの範囲内で終了判定ができるものと、1文字余分に読まないとわからないものがあるが
    * @detail 全て1文字余分に読んだ上で受理を返す
    * @detail 余分に読んだ1文字は破棄し、次のトークン判定はその文字から読み直す
    * @param c 入力文字
    * @return 受理状態か否かを表すステータス値
    */
    fn input_char(char8_t c) -> pp_token_category {

      auto visitor = kusabira::sm::overloaded{
          //最初の文字による初期状態決定
          [this](states::init state, char8_t ch) -> pp_token_category {
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
            } else if (std::isalpha(cv) or ch == u8'_') {
              //識別子読み込みモード、識別子の先頭は非数字でなければならない
              this->transition<states::identifier_seq>(state);
            } else if (std::isdigit(cv)) {
              //数値リテラル読み取り、必ず数字で始まる
              this->transition<states::number_literal>(state);
            } else if (ch == u8'.') {
              //.から始まるトークン列、浮動小数点リテラルか記号列
              this->transition<states::maybe_number_literal>(state);
            } else if (int res = kusabira::table::ref_symbol_table(cv); 0 <= res) {
              //区切り文字（記号）列読み取りモード
              if (res == 0) {
                //1文字記号の入力
                this->transition<states::end_seq>(state, pp_token_category::op_or_punc);
              } else {
                //記号列の読み取り
                this->transition<states::punct_seq>(state, res);
              }
            } else {
              //その他上記に当てはまらない非空白文字、1文字づつ分離
              this->transition<states::end_seq>(state, pp_token_category::other_character);
            }
            return { pp_token_category::Unaccepted };
          },
          //トークン終端を検出後に次の1文字をもって終了する
          [this](states::end_seq state, char8_t) -> pp_token_category {
            return this->restart(state, state.category);
          },
          //ホワイトスペースシーケンス読み出し
          [this](states::white_space_seq state, char8_t ch) -> pp_token_category {
            if (std::isspace(static_cast<unsigned char>(ch))) {
              return { pp_token_category::Unaccepted };
            } else {
              //ホワイトスペース以外出現で終了
              return this->restart(state, pp_token_category::whitespaces);
            }
          },
          //コメント判定
          [this](states::maybe_comment state, char8_t ch) -> pp_token_category {
            if (ch == u8'/') {
              //行コメント
              this->transition<states::line_comment>(state);
            } else if (ch == u8'*') {
              //ブロックコメント
              this->transition<states::block_comment>(state);
            } else if (ch == u8'=') {
              // /=演算子の受理
              this->transition<states::end_seq>(state, pp_token_category::op_or_punc);
            } else {
              //他の文字が出てきたら単体の/演算子だったという事・・・
              return this->restart(state, pp_token_category::op_or_punc);
            }
            return {pp_token_category::Unaccepted};
          },
          //行コメント読み取り
          [this](states::line_comment, char8_t) -> pp_token_category {
            //改行が現れるまではそのまま・・・
            return {pp_token_category::Unaccepted};
          },
          //ブロックコメント読み取り
          [this](states::block_comment state, char8_t ch) -> pp_token_category {
            if (ch == u8'*')
            {
              //終わりかもしれない・・・
              this->transition<states::maybe_end_block_comment>(state);
            }
            return { pp_token_category::Unaccepted };
          },
          //ブロックコメントの終端を判定
          [this](states::maybe_end_block_comment state, char8_t ch) -> pp_token_category {
            if (ch == u8'/') {
              //ブロック閉じを検出
              this->transition<states::end_seq>(state, pp_token_category::block_comment);
              return {pp_token_category::Unaccepted};
            } else {
              //閉じなかったらブロックコメント読み取りへ戻る
              this->transition<states::block_comment>(state);
              return { pp_token_category::Unaccepted };
            }
          },
          //文字列リテラル判定
          [this](states::maybe_str_literal state, char8_t ch) -> pp_token_category {
            if (ch == u8'R') {
              //生文字列リテラルっぽい
              this->transition<states::maybe_rawstr_literal>(state);
            } else if (ch == u8'\'') {
              //文字リテラル確定
              this->transition<states::char_literal>(state);
            } else if (ch == u8'"') {
              //文字列リテラル確定
              this->transition<states::string_literal>(state);
            } else if (is_identifier(ch)) {
              //識別子らしい
              this->transition<states::identifier_seq>(state);
            } else {
              //それ以外なら1文字の識別子ということ
              return this->restart(state, pp_token_category::identifier);
            }
            return { pp_token_category::Unaccepted };
          },
          //u・u8リテラル判定
          [this](states::maybe_u8str_literal state, char8_t ch) -> pp_token_category {
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
            } else if (is_identifier(ch)) {
              //識別子らしい
              this->transition<states::identifier_seq>(state);
            } else {
              //それ以外なら1文字の識別子ということ
              return this->restart(state, pp_token_category::identifier);
            }
            return { pp_token_category::Unaccepted };
          },
          //生文字列リテラル判定
          [this](states::maybe_rawstr_literal state, char8_t ch) -> pp_token_category {
            if (ch == u8'"') {
              //生文字列リテラル確定
              this->transition<states::raw_string_literal>(state);
            } else if (is_identifier(ch)) {
              //識別子らしい
              this->transition<states::identifier_seq>(state);
            } else {
              //それ以外なら1文字の識別子ということ
              return this->restart(state, pp_token_category::identifier);
            }
            return { pp_token_category::Unaccepted };
          },
          //生文字列リテラル読み込み
          [this](states::raw_string_literal& state, char8_t ch) -> pp_token_category {
            auto status = state.input_char(ch);
            if (status == pp_token_category::raw_string_literal) {
              //受理完了
              this->transition<states::end_seq>(state, pp_token_category::raw_string_literal);
            } else if (status < pp_token_category::Unaccepted) {
              //不正な入力、エラー
              return status;
            }
            return { pp_token_category::Unaccepted };
          },
          //文字列リテラル読み込み
          [this](states::string_literal state, char8_t ch) -> pp_token_category {
            if (ch == u8'\\') {
              //エスケープシーケンスを無視
              this->transition<states::ignore_escape_seq>(state, true);
            } else if (ch == u8'"') {
              //クオートを読んだら次で終了
              this->transition<states::end_seq>(state, pp_token_category::string_literal);
            }
            return { pp_token_category::Unaccepted };
          },
          //文字リテラル読み込み
          [this](states::char_literal state, char8_t ch) -> pp_token_category {
            if (ch == u8'\\') {
              //エスケープシーケンスを無視
              this->transition<states::ignore_escape_seq>(state, false);
            } else if (ch == u8'\'') {
              //クオートを読んだら次で終了
              this->transition<states::end_seq>(state, pp_token_category::string_literal);
            }
            return { pp_token_category::Unaccepted };
          },
          //エスケープシーケンスを飛ばして文字読み込み継続
          [this](states::ignore_escape_seq &state, char8_t) -> pp_token_category {
            if (state.is_string_parsing == true) {
              this->transition<states::string_literal>(state);
            } else {
              this->transition<states::char_literal>(state);
            }
            return { pp_token_category::Unaccepted };
          },
          //識別子読み出し
          [this](states::identifier_seq state, char8_t ch) -> pp_token_category {
            if (std::isalnum(static_cast<unsigned char>(ch)) or ch == u8'_') {
              return { pp_token_category::Unaccepted };
            } else {
              //識別子以外のものが出たら終了
              return this->restart(state, pp_token_category::identifier);
            }
          },
          //数値リテラル読み出し
          [this](states::number_literal state, char8_t ch) -> pp_token_category {
            if (ch == u8'e' or ch == u8'E' or ch == u8'p' or ch == u8'P') {
              //浮動小数点リテラルの指数部、符号読み取りへ
              this->transition<states::number_sign>(state);
              return { pp_token_category::Unaccepted };
            } else if(is_number_literal(ch)) {
              return { pp_token_category::Unaccepted };
            } else {
              //数字リテラル以外が出たら終了
              return this->restart(state, pp_token_category::pp_number);
            }
          },
          //.から始まるトークン読み出し
          [this](states::maybe_number_literal state, char8_t ch) -> pp_token_category {
            if (ch == u8'.') {
              //ドットが二つ続いた場合は記号列とする
              constexpr int res = kusabira::table::ref_symbol_table(u8'.', kusabira::table::ref_symbol_table(u8'.'));
              this->transition<states::punct_seq>(state, res);
              return { pp_token_category::Unaccepted };
            } else if (std::isdigit(static_cast<unsigned char>(ch))) {
              //浮動小数点リテラル
              this->transition<states::number_literal>(state);
              return { pp_token_category::Unaccepted };
            } else if (ch == u8'*') {
              //記号列、.*演算子、次の文字入力をもって確定する
              this->transition<states::end_seq>(state, pp_token_category::op_or_punc);
              return {pp_token_category::Unaccepted};
            } else {
              //ドット一つの読み取り
              return this->restart(state, pp_token_category::op_or_punc);
            }
          },
          //浮動小数点数の指数部の符号読み出し
          [this](states::number_sign state, char8_t ch) -> pp_token_category {
            if (ch == u8'-' or ch == u8'+') {
              this->transition<states::number_literal>(state);
              return { pp_token_category::Unaccepted };
            } else if (is_number_literal(ch)) {
              //符号は省略可
              return { pp_token_category::Unaccepted };
            } else {
              //指数部に符号や数字以外のものが出てきた
              return this->restart(state, pp_token_category::pp_number);
            }
          },
          //記号列の読み出し
          [this](states::punct_seq& state, char8_t ch) -> pp_token_category {
            if (ch == u8'/') {
              // /が2文字目以降にくる記号列は無い
              return this->restart(state, pp_token_category::op_or_punc);
            }

            //正しい記号列だけを認識する
            int res = state.input_char(ch);
            if (res == 0) {
              //受理、ほとんどの記号列は2文字
              this->transition<states::end_seq>(state, pp_token_category::op_or_punc);
            } else if (res < 0) {
              //適正な記号以外が出たら終了
              return this->restart(state, pp_token_category::op_or_punc);
            }
            return { pp_token_category::Unaccepted };
          }
      };

      return this->visit([&visitor, c](auto& state) {
        return visitor(state, c);
      });
    }

    /**
    * @brief 改行文字を入力する
    * @return 受理状態か否かを表すステータス値
    */
    fn input_newline() -> pp_token_category {

      auto visitor = kusabira::sm::overloaded{
        //init -> initはテーブルに無い
        [](states::init) -> pp_token_category { return pp_token_category::empty; },
        //1つ前の状態からもらったカテゴリを返す
        [this](states::end_seq state) -> pp_token_category {
          this->transition<states::init>(state);
          return state.category;
        },
        //ブロックコメント中の改行は無条件で継続
        [](states::block_comment) -> pp_token_category { return pp_token_category::block_comment; },
        [this](states::maybe_end_block_comment state) -> pp_token_category {
          this->transition<states::block_comment>(state);
          return pp_token_category::block_comment;
        },
        //生文字列リテラルは複数行にわたりうる
        [](states::raw_string_literal& state) -> pp_token_category {
          auto status = state.input_char(u8'\n');
          if (pp_token_category::Unaccepted < status) {
            //受理状態にはならないはず・・・
            return pp_token_category::FailedRawStrLiteralRead;
          } else if (status < pp_token_category::Unaccepted) {
            //不正な入力、エラー、おそらくデリミタ読み取りの途中
            return status;
          }
          return pp_token_category::during_raw_string_literal;
        },
        //生文字列リテラルとコメントブロック以外は改行でトークン分割する
        [this](auto&& state) -> pp_token_category {
          using state_t = std::remove_cvref_t<decltype(state)>;

          this->transition<states::init>(state);
          //状態によってはエラーを報告する
          return state_t::Category;
        }
      };

      return this->visit(std::move(visitor));
    }

  };

} // namespace kusabira::PP
