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
    ~state_base() = default;
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
        fn delimiter_push(char8_t ch) -> kusabira::PP::pp_tokenize_result {
          //デリミタ読み取り中、最大16文字
          if (ch == u8'(') {
            m_is_delimiter = false;
            //)delimiter" の形にしておく
            m_stack[m_length] = u8'"';
          } else {
            //delimiterに現れてはいけない文字（閉じかっこ、バックスラッシュ、ホワイトスペース系）が現れたらエラー
            if (ch == u8')' || ch == u8'\\' || std::isspace(static_cast<unsigned char>(ch)))
              return { kusabira::PP::pp_tokenize_status::RawStrLiteralDelimiterInvalid };
            //デリミタの長さが16文字を超えたらエラー
            if (17 <= m_length) return { kusabira::PP::pp_tokenize_status::RawStrLiteralDelimiterOver16Chars };
            m_stack[m_length] = ch;
          }
          ++m_length;
          return { kusabira::PP::pp_tokenize_status::Unaccepted };
        }

        /**
        * @brief 生文字列リテラル本体を読む、デリミタをチェックする
        * @param ch 1文字の入力
        * @return 生文字列リテラルとして受理したかどうか、エラーにはならない
        */
        fn delimiter_check(char8_t ch) -> kusabira::PP::pp_tokenize_result {
          //生文字列本体読み取り中
          if (m_stack[m_index] == ch) {
            //デリミタっぽいものを検出している時
            ++m_index;
            if (m_index == m_length) return { kusabira::PP::pp_tokenize_status::RawStrLiteral };
          } else {
            m_index = 0;
          }
          return { kusabira::PP::pp_tokenize_status::Unaccepted };
        }

        /**
        * @brief 生文字列リテラルを読む
        * @param ch 1文字の入力
        * @return 受理状態
        */
        fn operator()(char8_t ch) -> kusabira::PP::pp_tokenize_result {
          if (m_is_delimiter) {
            return this->delimiter_push(ch);
          } else {
            return this->delimiter_check(ch);
          }
        }

      };

      /**
      * @brief 記号列の受理を判定する
      * @param ch 入力文字
      * @param row_index 参照するテーブル番号
      */
      ifn ref_table(char8_t ch, int row_index = 0) -> int {
        //制御文字は考慮しない
        std::uint8_t index = ch - 33;

        //Ascii範囲外の文字か制御文字
        if ((126 - 33) < index) return -1;

        //テーブルを参照して受理すべきかを決定
        return kusabira::table::symbol_table[row_index][index];
      }
    }

    /**
    * @brief 初期状態、トークン最初の文字を受け取る
    */
    struct init {};

    /**
    * @brief 文字列リテラルの終了のためなど、この状態は何を読んでもリセットする
    */
    struct end_seq {
      pp_tokenize_status category;

      end_seq(pp_tokenize_status prev_status = {}) : category{prev_status}
      {}
    };

    struct white_space_seq {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::Whitespaces;
    };

    struct maybe_comment {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::OPorPunc;
    };

    struct line_comment {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::LineComment;
    };

    struct block_comment {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::BlockComment;
    };

    struct maybe_end_block_comment {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::Unaccepted;
    };

    struct identifier_seq {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::Identifier;
    };

    /**
    * @brief 文字・文字列リテラルっぽい文字
    */
    struct maybe_str_literal {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::Identifier;
    };

    /**
    * @brief UTF-8文字・文字列リテラルっぽい文字
    */
    struct maybe_u8str_literal {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::Identifier;
    };

    /**
    * @brief 生文字列リテラルっぽい文字
    */
    struct maybe_rawstr_literal {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::Identifier;
    };

    /**
    * @brief 生文字列リテラル
    */
    struct raw_string_literal {

      static constexpr pp_tokenize_status Category = pp_tokenize_status::RawStrLiteral;

      detail::rawstr_literal_accepter m_reader{};

      /**
      * @brief 生文字列リテラルを読む
      * @param ch 1文字の入力
      * @return falseで終端"を検出
      */
      fn input_char(char8_t ch) -> pp_tokenize_result {
        return m_reader(ch);
      }
    };

    struct char_literal {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::UnexpectedNewLine;
    };

    struct string_literal {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::UnexpectedNewLine;
    };

    struct number_literal {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::NumberLiteral;
    };

    struct number_sign {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::NumberLiteral;
    };

    /**
    * @brief 文字列リテラル中のエスケープシーケンスを無視する
    */
    struct ignore_escape_seq {

      static constexpr pp_tokenize_status Category = pp_tokenize_status::UnexpectedNewLine;

      //文字と文字列、どちらを読み取り中なのか?
      //trueで文字列リテラル
      bool is_string_parsing;

      ignore_escape_seq(bool is_str_parsing)
       : is_string_parsing(is_str_parsing)
      {}
    };

    struct punct_seq {
      static constexpr pp_tokenize_status Category = pp_tokenize_status::OPorPunc;

      int m_index;

      punct_seq(int iddex = 0) : m_index{iddex}
      {}

      /**
      * @brief 演算子、区切り文字をチェックする
      * @param ch 1文字の入力
      * @return falseで終端"を検出
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
    if (std::isxdigit(static_cast<unsigned char>(ch)))
    {
      //16進の範囲で使われうる英数字
      return true;
    } else if (ch == u8'x' or ch == u8'\'' or ch == u8'.') {
      //16進記号のxかドットか区切り文字、2つ続かないはずだけどここではチェックしないことにする・・・
      return true;
    } else {
      //サフィックスやユーザー定義リテラルは分割する
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
            states::number_literal, states::number_sign, states::punct_seq>
  {

    pp_tokenizer_sm() = default;

    /**
    * @brief トークン終端を検出した時、次のトークン認識を開始する
    * @detail 初期状態に戻し、現在の入力文字をもとに次のトークン認識を開始する
    * @tparam NosWtate 今の状態型
    * @param state 現在の状態
    * @param status 返すべき受理状態
    * @return status
    */
    template<typename NowState>
    fn restart(NowState state, char8_t c, pp_tokenize_status status) -> pp_tokenize_result {
      this->transition<states::init>(state);
      [[maybe_unused]] auto discard = this->input_char(c);

      return { status };
    }

    /**
    * @brief 文字を入力する
    * @detail 状態機械により1文字ごとにトークンを判定してゆく
    * @param c 入力文字
    * @return 受理状態か否かを表すステータス値
    */
    fn input_char(char8_t c) -> pp_tokenize_result {

      auto visitor = kusabira::sm::overloaded{
          //最初の文字による初期状態決定
          [this](states::init state, char8_t ch) -> pp_tokenize_result {
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
            } else if (int res = kusabira::table::ref_symbol_table(cv); 0 <= res) {
              //区切り文字（記号）列読み取りモード
              if (res == 0) {
                //1文字記号の入力
                this->transition<states::end_seq>(state, pp_tokenize_status::OPorPunc);
              } else {
                //記号列の読み取り
                this->transition<states::punct_seq>(state);
              }
            } else {
              //その他上記に当てはまらない非空白文字、1文字づつ分離
              this->transition<states::end_seq>(state, pp_tokenize_status::OtherChar);
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //トークン終端を検出後に次のトークン読み込みを始める、文字列リテラル等の時
          [this](states::end_seq state, char8_t ch) -> pp_tokenize_result {
            return this->restart(state, ch, state.category);
          },
          //ホワイトスペースシーケンス読み出し
          [this](states::white_space_seq state, char8_t ch) -> pp_tokenize_result {
            if (std::isspace(static_cast<unsigned char>(ch))) {
              return { pp_tokenize_status::Unaccepted };
            } else {
              return this->restart(state, ch, pp_tokenize_status::Whitespaces);
            }
          },
          //コメント判定
          [this](states::maybe_comment state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'/') {
              //行コメント
              this->transition<states::line_comment>(state);
            } else if (ch == u8'*') {
              //ブロックコメント
              this->transition<states::block_comment>(state);
            } else if (ch == u8'=') {
              // /=演算子の受理
              this->transition<states::end_seq>(state, pp_tokenize_status::OPorPunc);
            } else {
              //他の文字が出てきたら単体の/演算子だったという事・・・
              return this->restart(state, ch, pp_tokenize_status::OPorPunc);
            }
            return {pp_tokenize_status::Unaccepted};
          },
          //行コメント読み取り
          [this](states::line_comment, char8_t) -> pp_tokenize_result {
            //改行が現れるまではそのまま・・・
            return {pp_tokenize_status::Unaccepted};
          },
          //ブロックコメント読み取り
          [this](states::block_comment state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'*')
            {
              //終わりかもしれない・・・
              this->transition<states::maybe_end_block_comment>(state);
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //ブロックコメントの終端を判定
          [this](states::maybe_end_block_comment state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'/') {
              //ブロック閉じを検出
              this->transition<states::init>(state);
              return { pp_tokenize_status::BlockComment };
            } else {
              //閉じなかったらブロックコメント読み取りへ戻る
              this->transition<states::block_comment>(state);
              return { pp_tokenize_status::Unaccepted };
            }
          },
          //文字列リテラル判定
          [this](states::maybe_str_literal state, char8_t ch) -> pp_tokenize_result {
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
              return this->restart(state, ch, pp_tokenize_status::Identifier);
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //u・u8リテラル判定
          [this](states::maybe_u8str_literal state, char8_t ch) -> pp_tokenize_result {
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
              return this->restart(state, ch, pp_tokenize_status::Identifier);
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //生文字列リテラル判定
          [this](states::maybe_rawstr_literal state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'"') {
              //生文字列リテラル確定
              this->transition<states::raw_string_literal>(state);
            } else if (is_identifier(ch)) {
              //識別子らしい
              this->transition<states::identifier_seq>(state);
            } else {
              //それ以外なら1文字の識別子ということ
              return this->restart(state, ch, pp_tokenize_status::Identifier);
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //生文字列リテラル読み込み
          [this](states::raw_string_literal &state, char8_t ch) -> pp_tokenize_result {
            auto status = state.input_char(ch);
            if (status == pp_tokenize_status::RawStrLiteral) {
              //受理完了
              this->transition<states::end_seq>(state, pp_tokenize_status::RawStrLiteral);
            } else if (status < pp_tokenize_status::Unaccepted) {
              //不正な入力、エラー
              return status;
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //文字列リテラル読み込み
          [this](states::string_literal state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'\\') {
              //エスケープシーケンスを無視
              this->transition<states::ignore_escape_seq>(state, true);
            } else if (ch == u8'"') {
              //クオートを読んだら次で終了
              this->transition<states::end_seq>(state, pp_tokenize_status::StringLiteral);
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //文字リテラル読み込み
          [this](states::char_literal state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'\\') {
              //エスケープシーケンスを無視
              this->transition<states::ignore_escape_seq>(state, false);
            } else if (ch == u8'\'') {
              //クオートを読んだら次で終了
              this->transition<states::end_seq>(state, pp_tokenize_status::StringLiteral);
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //エスケープシーケンスを飛ばして文字読み込み継続
          [this](states::ignore_escape_seq &state, char8_t) -> pp_tokenize_result {
            if (state.is_string_parsing == true) {
              this->transition<states::string_literal>(state);
            } else {
              this->transition<states::char_literal>(state);
            }
            return { pp_tokenize_status::Unaccepted };
          },
          //識別子読み出し
          [this](states::identifier_seq state, char8_t ch) -> pp_tokenize_result {
            if (std::isalnum(static_cast<unsigned char>(ch)) or ch == u8'_') {
              return { pp_tokenize_status::Unaccepted };
            } else {
              return this->restart(state, ch, pp_tokenize_status::Identifier);
            }
          },
          //数値リテラル読み出し
          [this](states::number_literal state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'e' or ch == u8'E' or ch == u8'p' or ch == u8'P') {
              //浮動小数点リテラルの指数部、符号読み取りへ
              this->transition<states::number_sign>(state);
              return { pp_tokenize_status::Unaccepted };
            } else if(is_number_literal(ch)) {
              return { pp_tokenize_status::Unaccepted };
            } else {
              return this->restart(state, ch, pp_tokenize_status::NumberLiteral);
            }
          },
          //浮動小数点数の指数部の符号読み出し
          [this](states::number_sign state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'-' or ch == u8'+') {
              this->transition<states::number_literal>(state);
              return { pp_tokenize_status::Unaccepted };
            } else if (is_number_literal(ch)) {
              //符号は省略可
              return { pp_tokenize_status::Unaccepted };
            } else {
              //指数部に符号や数字以外のものが出てきた
              return this->restart(state, ch, pp_tokenize_status::NumberLiteral);
            }
          },
          //記号列の読み出し
          [this](states::punct_seq state, char8_t ch) -> pp_tokenize_result {
            if (ch == u8'/') {
              // /が2文字目以降にくる記号列は無い
              return this->restart(state, ch, pp_tokenize_status::OPorPunc);
            }
            
            if (std::ispunct(static_cast<unsigned char>(ch)) == false) {
              return this->restart(state, ch, pp_tokenize_status::OPorPunc);
            }
            return { pp_tokenize_status::Unaccepted };
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
    fn input_newline() -> pp_tokenize_result {

      auto visitor = kusabira::sm::overloaded{
        //init -> initはテーブルに無い
        [](states::init) -> pp_tokenize_result { return {pp_tokenize_status::Unaccepted}; },
        //1つ前の状態からもらったカテゴリを返す
        [this](states::end_seq state) -> pp_tokenize_result {
          this->transition<states::init>(state);
          return { state.category };
        },
        //ブロックコメント中の改行は無条件で継続
        [](states::block_comment) -> pp_tokenize_result { return {pp_tokenize_status::Unaccepted}; },
        [this](states::maybe_end_block_comment state) -> pp_tokenize_result {
          this->transition<states::block_comment>(state);
          return { pp_tokenize_status::Unaccepted };
        },
        //生文字列リテラルは複数行にわたりうる
        [](states::raw_string_literal& state) -> pp_tokenize_result {
          auto status = state.input_char(u8'\n');
          if (pp_tokenize_status::Unaccepted < status) {
            //受理状態にはならないはず・・・
            return { pp_tokenize_status::FailedRawStrLiteralRead };
          } else if (status < pp_tokenize_status::Unaccepted) {
            //不正な入力、エラー、おそらくデリミタ読み取りの途中
            return status;
          }
          return { pp_tokenize_status::Unaccepted };
        },
        //生文字列リテラルとコメントブロック以外は改行でトークン分割する
        [this](auto&& state) -> pp_tokenize_result {
          using state_t = std::remove_cvref_t<decltype(state)>;

          this->transition<states::init>(state);
          //状態によってはエラーを報告する
          return { state_t::Category };
        }
      };

      return this->visit(std::move(visitor));
    }

  };

} // namespace kusabira::PP
