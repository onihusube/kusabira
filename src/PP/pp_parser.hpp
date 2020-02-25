#pragma once

#include <list>
#include <forward_list>

#include "../common.hpp"
#include "file_reader.hpp"
#include "pp_automaton.hpp"
#include "pp_tokenizer.hpp"

namespace kusabira::PP
{
  enum class pp_token_category : std::uint8_t {
    comment,
    whitespaces,

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
    op_or_punc,
    other_character
  };

  struct pp_token {
    //プリプロセッシングトークン種別
    pp_token_category category;
    //構成する字句トークン列
    std::pmr::forward_list<lex_token> tokens;
  };

  enum class pp_parse_status : std::int8_t {
    UnknownTokens,
    Complete,
    FollowingSharpToken,
  };

  enum class pp_parse_context : std::int8_t {
    GroupPart
  };

  struct pp_err_info {
    lex_token token;
    pp_parse_context context;
  };

  using parse_status = tl::expected<pp_parse_status, pp_err_info>;

  /*
  struct parse_status {
    pp_parse_status status = pp_parse_status::Complete;

    explicit operator bool() const noexcept {
      return status == pp_parse_status::Complete;
    }

    ffn operator==(parse_status self, const pp_parse_status ext_status) noexcept -> bool {
      return self.status == ext_status;
    }
  };*/

  /**
  * @brief ホワイトスペース列とブロックコメントを読み飛ばす
  * @return 有効なトークンを指していれば（終端に達していなければ）true
  */
  template <typename TokenIterator, typename Sentinel>
  ifn skip_whitespaces(TokenIterator& it, Sentinel end) -> bool {

    auto check_status = [](auto kind) -> bool {
      //ホワイトスペース列とブロックコメントはスルー
      return kind == pp_tokenize_status::Whitespaces or kind == pp_tokenize_status::BlockComment;
    };

    do {
      ++it;
    } while (it != end and check_status((*it).kind));

    return it != end;
  }

//ホワイトスペース読み飛ばしのテンプレ
#define SKIP_WHITESPACE(iterator, sentinel) if (not skip_whitespaces(iterator, sentinel)) return {}


  /**
  * @brief トークン列をパースしてプリプロセッシングディレクティブの検出とCPPの実行を行う
  * @detail 再帰下降構文解析によりパースする
  * @detail パースしながらプリプロセスを実行し、成果物はプリプロセッシングトークン列
  */
  template <typename T = void>
  struct ll_paser {
    using Tokenizer = kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm>;
    using iterator = decltype(begin(std::declval<Tokenizer &>()));
    using sentinel = decltype(end(std::declval<Tokenizer &>()));

    std::pmr::list<pp_token> m_token_list{};

    fn start(Tokenizer &pp_tokenizer) -> parse_status {
      auto it = begin(pp_tokenizer);
      auto se = end(pp_tokenizer);

      //空のファイル判定
      if (it == se) return {};
      if (auto kind = (*it).kind; kind == pp_tokenize_status::Whitespaces or kind == pp_tokenize_status::BlockComment) {
        //空に等しいファイルだった
        SKIP_WHITESPACE(it, se);
      }

      //モジュールファイルを識別
      if (auto& top_token = *it; top_token.kind == pp_tokenize_status::Identifier) {
        using namespace std::string_view_literals;

        if (auto str = top_token.token; str == u8"module" or str == u8"export") {
          //モジュールファイルとして読むため、終了後はそのまま終わり
          return this->module_file(it, se);
        }
      }

      //通常のファイル
      return this->group(it, se);
    }

    fn module_file(iterator &iter, sentinel end) -> parse_status {
      //モジュール宣言とかを生成する
      return {};
    }

    fn group(iterator& it, sentinel end) -> parse_status {
      parse_status status{true};

      while (it != end and status) {
        status = group_part(it, end);
      }

      return status;
    }

    fn group_part(iterator& it, sentinel end) -> parse_status {
      using namespace std::string_view_literals;

      //ホワイトスペース列を読み飛ばす
      if (auto kind = (*it).kind; kind == pp_tokenize_status::Whitespaces or kind == pp_tokenize_status::BlockComment) {
        SKIP_WHITESPACE(it, end);
      }

      if (auto& token = *it; token.kind == pp_tokenize_status::OPorPunc) {
        //先頭#から始まればプリプロセッシングディレクティブ
        if (token.token == u8"#") {
          //ホワイトスペース列を読み飛ばす
          SKIP_WHITESPACE(it, end);

          //なにかしらの識別子が来てるはず
          if (auto& next_token = *it; next_token.kind != pp_tokenize_status::Identifier) {
            if (next_token.kind == pp_tokenize_status::NewLine) {
              //空のプリプロセッシングディレクティブ
              return {};
            }
            //エラーでは？？
            return {pp_parse_status::UnknownTokens};
          }
          if (next_token.token.starts_with(u8"if")) {
            //if-sectionへ
            return this->if_section(it, end);
          } else {
            //control-lineへ
            return this->control_line(it, end);
            //conditionally-supported-directiveはどうしよう・・
          }
        } else {
          //text-lineへ
          return this->text_line(it, end);
        }
      }
    }

    fn control_line(iterator& it, sentinel end) -> parse_status {
      return {};
    }

    fn if_section(iterator& it, sentinel end) -> parse_status {
      //#を読んだ上でここに来ているかをチェック
      auto chack_status = [&status]() noexcept -> bool { return status == pp_parse_status::FollowingSharpToken; };

      //ifを処理
      auto status = this->if_group(it, end);

      //正常にここに戻った場合はすでに#を読んでいるはず
      if (chack_status() and token == u8"elif") {
        //#elif
        status = this->elif_groups(it, end);
      }
      if (chack_status() and token == u8"else") {
        //#else
        status = this->else_group(it, end);
      } 
      if (chack_status()) {
        //endif以外にありえない
        return this->endif_line(it, end);
      } else {
        //対応するセクションが無い、あるいは普通にエラー
        return status;
      }
    }

    fn if_group(iterator& it, sentinel end) -> parse_status {
      if (auto token = (*it).token; token.length() == 3) {
        //#if

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        //定数式の処理

      } else if (token == u8"ifdef" or token == u8"ifndef") {
        //#ifdef #ifndef

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        if (auto kind = (*it).kind; kind != pp_tokenize_status::Identifier) {
          //識別子以外が出てきたらエラー
          return { pp_parse_status::UnknownTokens };
        }
        //識別子を#define名としてチェックする

      } else {
        //エラー？
        return {pp_parse_status::UnknownTokens};
      }

      //ホワイトスペース列を読み飛ばす
      SKIP_WHITESPACE(it, end);

      if (auto kind = (*it).kind; kind != pp_tokenize_status::NewLine) {
        //改行文字以外が出てきたらエラー
        return { pp_parse_status::UnknownTokens };
      }

      return this->group(it, end);
    }

    fn elif_groups(iterator& it, sentinel end) -> parse_status {
      parse_status status{};

      while (it != end and status) {
        status = elif_group(it, end);
      }

      return status;
    }

    fn elif_group(iterator& it, sentinel end) -> parse_status {
      //#elifを処理
      return this->group(it, end);
    }

    fn else_group(iterator& it, sentinel end) -> parse_status {
      //#elseを処理
      return this->group(it, end);
    }

    fn endif_line(iterator& it, sentinel end) -> parse_status {
      //#endifを処理
      if (auto token = (*it).token; token == u8"endif") {
        //ホワイトスペース列を読み飛ばす
        do {
          ++it;
        } while ((*it).kind == pp_tokenize_status::Whitespaces);
        //改行が現れなければならない
        if (it != end and (*it).kind == pp_tokenize_status::NewLine) {
          //正常終了
          return {};
        }
        //先に終端に到達したか他のトークンが出てきた
      }
      //endifじゃないなど、エラー
      return {};
    }

    fn text_line(iterator& it, sentinel end) -> parse_status {
      //要するに普通のトークン列
      auto status = this->pp_tokens(it, end);
      if ((*it).kind != pp_tokenize_status::NewLine) {
        //エラー
        return {pp_parse_status::UnknownTokens};
      }
      //1行分プリプロセッシングトークン列読み出し
      status = this->pp_tokens(it, end);
      //改行されて終了
      if (status and (*it).kind == pp_tokenize_status::NewLine) {
        //正常にtext-lineを読み込んだ
        return {};
      }
      //何かおかしい
      return {};
    }

    fn pp_tokens(iterator& it, sentinel end) -> parse_status {
      //プリプロセッシングトークン列を読み出す
      return {};
    }
  };

} // namespace kusabira::PP