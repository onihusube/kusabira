#pragma once

#include <list>
#include <forward_list>
#include <utility>
#include <cassert>

#include "../common.hpp"
#include "file_reader.hpp"
#include "pp_automaton.hpp"
#include "pp_tokenizer.hpp"
#include "vocabulary/whimsy_str_view.hpp"

namespace kusabira::PP {
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
    other_character,
    
    //生文字列リテラル識別のため・・・
    raw_string_literal,
    user_defined_raw_string_literal,
  };

  struct pp_token {

    pp_token(pp_token_category cat)
        : category{cat}
        , lextokens{std::pmr::polymorphic_allocator<lex_token>(&kusabira::def_mr)}
    {}

    pp_token(pp_token_category cat, lex_token &&ltoken)
        : category{cat}
        , token{ltoken.token}
        , lextokens{std::pmr::polymorphic_allocator<lex_token>(&kusabira::def_mr)}
    {
      lextokens.emplace_front(std::move(ltoken));
    }

    //プリプロセッシングトークン種別
    pp_token_category category;
    //プリプロセッシングトークン文字列
    kusabira::vocabulary::whimsy_str_view<> token;
    //構成する字句トークン列
    std::pmr::forward_list<lex_token> lextokens;
  };

  enum class pp_parse_status : std::uint8_t {
    UnknownTokens,
    Complete,
    FollowingSharpToken,
    EndOfFile
  };

  enum class pp_parse_context : std::int8_t {
    UnknownError = std::numeric_limits<std::int8_t>::min(),
    FailedRawStrLiteralRead,            //生文字列リテラルの読み取りに失敗した、バグの可能性が高い
    RawStrLiteralDelimiterOver16Chars,  //生文字列リテラルデリミタの長さが16文字を超えた
    RawStrLiteralDelimiterInvalid,      //生文字列リテラルデリミタに現れてはいけない文字が現れた
    UnexpectedNewLine,                  //予期しない改行入力があった

    GroupPart = 0,      // #の後で有効なトークンが現れなかった
    IfSection,          // #ifセクションの途中で読み取り終了してしまった？
    IfGroup_Mistake,    // #ifから始まるifdef,ifndefではない間違ったトークン
    IfGroup_Invalid,    // 1つの定数式・識別子の前後、改行までの間に不正なトークンが現れている

    ElseGroup,          // 改行の前に不正なトークンが現れている
    EndifLine_Mistake,  // #endifがくるべき所に別のものが来ている
    EndifLine_Invalid,  // #endif ~ 改行までの間に不正なトークンが現れている
    TextLine            // 改行が現れる前にファイル終端に達した？バグっぽい
  };

  struct pp_err_info {

    template<typename Token = lex_token>
    pp_err_info(Token&& lextoken, pp_parse_context err_from)
      : token(std::forward<Token>(lextoken))
      , context(err_from)
    {}

    lex_token token;
    pp_parse_context context;
  };

  using parse_status = tl::expected<pp_parse_status, pp_err_info>;

  /**
  * @brief パース中のエラーオブジェクトを作成する
  * @return エラー状態のparse_status
  */
  template<typename TokenIterator>
  ifn make_error(TokenIterator& it, pp_parse_context context) -> parse_status {
    return parse_status{tl::unexpect, std::move(*it), context};
  }



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
#define SKIP_WHITESPACE(iterator, sentinel)     \
  if (not skip_whitespaces(iterator, sentinel)) \
    return {tl::in_place, pp_parse_status::EndOfFile}

//トークナイザのエラーチェック
#define TOKNIZE_ERR_CHECK(iterator)                                               \
  if (auto kind = (*iterator).kind.status; kind < pp_tokenize_status::Unaccepted) \
  return make_error(iterator, pp_parse_context{static_cast<std::underlying_type_t<decltype(kind)>>(kind)})

//イテレータを一つ進めるとともに終端チェック
#define EOF_CHECK(iterator, sentinel) ++it; if (iterator == sentinel) return {pp_parse_status::EndOfFile}


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

    using pptoken_conteiner = std::pmr::list<pp_token>;

    std::pmr::list<pp_token> m_token_list{std::pmr::polymorphic_allocator<pp_token>(&kusabira::def_mr)};

    fn start(Tokenizer& pp_tokenizer) -> parse_status {
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
      return {pp_parse_status::Complete};
    }

    fn group(iterator& it, sentinel end) -> parse_status {
      parse_status status{ tl::in_place, pp_parse_status::Complete };

      while (it != end and status == pp_parse_status::Complete) {
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
              return { tl::in_place, pp_parse_status::Complete };
            }
            //エラーでは？？
            return make_error(it, pp_parse_context::GroupPart);
          }

          // #elif等の出現を調べるもの
          auto check_elif_etc = [](auto token) -> bool {
            return token == u8"elif" or token == u8"else" or token == u8"endif";
          };

          // #に続く識別子、何かしらのプリプロセッシングディレクティブ
          if ((*it).token.starts_with(u8"if")) {
            // if-sectionへ
            return this->if_section(it, end);
          } else if (check_elif_etc((*it).token) == true) {
            // if-sectionの内部でif-group読取中のelif等の出現、group読取の終了
            return {tl::in_place, pp_parse_status::FollowingSharpToken};
          } else {
            // control-lineへ
            return this->control_line(it, end);
            // conditionally-supported-directiveはどうしよう・・
          }
        } else if((*it).token == u8"import" or (*it).token == u8"export") {
          // モジュールのインポート宣言
          return this->control_line(it, end);
        } else {
          // text-lineへ
          return this->text_line(it, end);
        }
      }
    }

    fn control_line(iterator& it, sentinel end) -> parse_status {
      return {};
    }

    fn if_section(iterator& it, sentinel end) -> parse_status {
      //ifを処理
      auto status = this->if_group(it, end);


      //#を読んだ上でここに来ているかをチェックするもの
      auto chack_status = [&status]() noexcept -> bool { return status == pp_parse_status::FollowingSharpToken; };

      //正常にここに戻った場合はすでに#を読んでいるはず
      if (chack_status() and (*it).token == u8"elif") {
        //#elif
        status = this->elif_groups(it, end);
      }
      if (chack_status() and (*it).token == u8"else") {
        //#else
        status = this->else_group(it, end);
      } 
      if (chack_status()) {
        //endif以外にありえない
        return this->endif_line(it, end);
      } else {
        //各セクションパース中のエラー、status == trueとなるときはどんな時だろう？
        return status ? make_error(it, pp_parse_context::IfSection) : status;
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
          return make_error(it, pp_parse_context::IfGroup_Invalid);
        }
        //識別子を#define名としてチェックする

      } else {
        // #ifから始まるがifdefでもifndefでもない何か
        return make_error(it, pp_parse_context::IfGroup_Mistake);
      }

      //ホワイトスペース列を読み飛ばす
      SKIP_WHITESPACE(it, end);

      if (auto kind = (*it).kind; kind != pp_tokenize_status::NewLine) {
        //改行文字以外が出てきたらエラー
        return make_error(it, pp_parse_context::IfGroup_Invalid);
      }

      return this->group(it, end);
    }

    fn elif_groups(iterator& it, sentinel end) -> parse_status {
      parse_status status{ tl::in_place, pp_parse_status::Complete };

      while (it != end and status == pp_parse_status::Complete) {
        status = elif_group(it, end);
      }

      return status;
    }

    fn elif_group(iterator& it, sentinel end) -> parse_status {
      //#elifを処理

      //定数式を処理

      //改行文字まで読みとばし


      return this->group(it, end);
    }

    fn else_group(iterator& it, sentinel end) -> parse_status {
      //#elseを処理
      
      //ホワイトスペース列を読み飛ばす
      SKIP_WHITESPACE(it, end);

      if (auto kind = (*it).kind; kind != pp_tokenize_status::NewLine) {
        //改行文字以外が出てきたらエラー
        return make_error(it, pp_parse_context::ElseGroup);
      }

      return this->group(it, end);
    }

    fn endif_line(iterator& it, sentinel end) -> parse_status {
      //#endifを処理
      if (auto token = (*it).token; token == u8"endif") {

        //ホワイトスペース列を読み飛ばす
        SKIP_WHITESPACE(it, end);

        //改行が現れなければならない
        if (it != end and (*it).kind == pp_tokenize_status::NewLine) {
          //正常終了
          return {pp_parse_status::Complete};
        } else {
          //他のトークンが出てきた
          return make_error(it, pp_parse_context::EndifLine_Invalid);
        }
      }
      //endifじゃない
      return make_error(it, pp_parse_context::EndifLine_Mistake);
    }

    fn text_line(iterator& it, sentinel end) -> parse_status {
      //1行分プリプロセッシングトークン列読み出し
      auto status = this->pp_tokens(it, end, this->m_token_list);

      if (status == false) return status;

      //改行されて終了
      if ((*it).kind == pp_tokenize_status::NewLine) {
        //正常にtext-lineを読み込んだ
        return {pp_parse_status::Complete};
      }

      //何かおかしい
      return make_error(it, pp_parse_context::TextLine);
    }

    fn pp_tokens(iterator& it, sentinel end, pptoken_conteiner& list) -> parse_status {
      //プリプロセッシングトークン列を読み出す
      auto kind = (*it).kind;
      while (kind != pp_tokenize_status::NewLine) {
        //トークナイザのエラーならそこで終わる
        if (kind < pp_tokenize_status::Unaccepted) break;

        //トークン読み出しとプリプロセッシングトークンの構成
        switch ((*it).kind.status) {
          case pp_tokenize_status::LineComment:  [[fallthrough]];
          case pp_tokenize_status::BlockComment: [[fallthrough]];
          case pp_tokenize_status::Whitespaces:  [[fallthrough]];
          case pp_tokenize_status::Empty:
            //これらのトークンは無視する
            break;
          case pp_tokenize_status::DuringRawStr:
            //生文字列リテラル全体を一つのトークンとして読み出す必要がある
            auto&& [tmp_pptoken, prev_token] = this->read_rawstring_tokens(it, end);
            
            TOKNIZE_ERR_CHECK(it);

            list.emplace_back(std::move(tmp_pptoken));

            EOF_CHECK(it, end);

            //次のトークンを調べてユーザー定義リテラルの有無を判断
            if (this->strliteral_classify(it, prev_token, list) == true) {
              //そのままおわる
              break;
            } else {
              //falseの時は次のトークンを通常の手順で処理して頂く
              kind = (*it).kind;
              continue;
            }
            break;
          case pp_tokenize_status::RawStrLiteral: [[fallthrough]];
          case pp_tokenize_status::StringLiteral:
            auto category = (kind == pp_tokenize_status::RawStrLiteral) ? pp_token_category::raw_string_literal : pp_token_category::string_literal;
            auto& prev = list.emplace_back(category, std::move(*it));

            EOF_CHECK(it, end);
            
            //次のトークンを調べてユーザー定義リテラルの有無を判断
            if (this->strliteral_classify(it, (*prev.lextokens.begin()).token, list) == true) {
              //そのままおわる
              break;
            } else {
              //falseの時は次のトークンを通常の手順で処理して頂く
              kind = (*it).kind;
              continue;
            }
          default:
          {
            //基本はトークン1つを読み込んでプリプロセッシングトークンを構成する
            auto category = this->tokenize_status_to_category(kind.status);
            list.emplace_back(category, std::move(*it));
          }
        }

        //トークンを進める
        EOF_CHECK(it, end);
        kind = (*it).kind;
      }

      TOKNIZE_ERR_CHECK(it);

      //改行で終了しているはず
      return {pp_parse_status::Complete};
    }

  private:

    fn tokenize_status_to_category(pp_tokenize_status status) -> pp_token_category {
      switch (status)
      {
      case kusabira::PP::pp_tokenize_status::Identifier:
        return pp_token_category::identifier;
      case kusabira::PP::pp_tokenize_status::NumberLiteral:
        return pp_token_category::pp_number;
      case kusabira::PP::pp_tokenize_status::OPorPunc:
        return pp_token_category::op_or_punc;
      case kusabira::PP::pp_tokenize_status::OtherChar:
        return pp_token_category::other_character;
      default:
        //ここに来たら上でバグってる
        assert(false);
        return pp_token_category{};
        break;
      }
    }

    fn strliteral_classify(iterator& it, std::u8string_view prevtoken , pptoken_conteiner& list) -> bool {
      //以前のトークンが文字列リテラルなのか文字リテラルなのか判断
      auto& prev_strtoken = list.back();
      
      //一つ前のトークンの最後の文字が'なら文字リテラル
      if (prevtoken.ends_with(u8'\'')) {
        prev_strtoken.category = pp_token_category::charcter_literal;
      }

      //イテレータは既に一つ進められているものとして扱う
      if ((*it).kind == pp_tokenize_status::Identifier) {
        using enum_int = std::underlying_type_t<decltype(prev_strtoken.category)>;

        //文字列のすぐあとが識別子ならユーザー定義リテラル
        //どちらにせよ1つ進めれば適切なカテゴリになる
        prev_strtoken.category = pp_token_category{ static_cast<enum_int>(prev_strtoken.category) + 1 };
        return true;
      } else {
        //そのほかのトークンはそのまま処理してもらう
        return false;
      }
    }

    fn read_rawstring_tokens(iterator& it, sentinel end) -> std::pair<pp_token, std::u8string_view> {
      pp_token token{pp_token_category::raw_string_literal};
      auto& list = token.lextokens;

      list.push_front(*it);
      auto pos = token.lextokens.begin();

      do {
        ++it;
        if ((*it).kind < pp_tokenize_status::Unaccepted) {
          //エラーかな
          return { std::move(token), u8"" };
        } else {
          //トークンをまとめて1つのPPトークンにする
          pos = list.emplace_after(pos, *it);
        }
      } while ((*it).kind != pp_tokenize_status::RawStrLiteral);

      return { std::move(token), (*pos).token };
    }
  };

} // namespace kusabira::PP