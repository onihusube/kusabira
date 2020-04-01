#include "../common.hpp"
#include "../report_output.hpp"

namespace kusabira::PP {

  struct pp_directive_manager {

    std::size_t m_line{};
    fs::path m_filename{};
    std::optional<fs::path> m_replace_filename{};

    void newline() {
      ++m_line;
    }

    template<typename TokensIterator, typename TokensSentinel>
    fn include(TokensIterator& it, TokensSentinel end) -> std::pmr::list<pp_token> {
      //未実装
      assert(false);
      return {};
    }

    /**
    * @brief #lineディレクティブを実行する
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    template<typename Reporter, typename TokensIterator, typename TokensSentinel>
    void line(TokensIterator& it, TokensSentinel end) const {
      //事前条件
      assert(it != end);
      assert((*it).kind == pp_tokenize_status::Identifier);
      assert((*it).token == u8"line");

      this->skip_whitespace(it, end);

      if ((*it).kind == pp_tokenize_status::NumberLiteral) {
        //現在行番号の変更

        this->skip_whitespace(it, end);

        if ((*it).kind == pp_tokenize_status::NewLine) return;
        
        if ((*it).kind == pp_tokenize_status::StringLiteral) {
          //ファイル名変更
          this->skip_whitespace(it, end);
          if ((*it).kind == pp_tokenize_status::NewLine) return;
          //ここにきたらどうしよう・・・
        }
      }

      //マクロを展開した上で#lineディレクティブを実行する
      //展開後に#lineにならなければ未定義動作、エラーにしようかなあ
      
    }


    /**
    * @brief #errorディレクティブを実行する
    * @param reporter レポート出力オブジェクトへの参照
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    template<typename Reporter, typename TokensIterator, typename TokensSentinel>
    void error(Reporter& reporter, TokensIterator& it, TokensSentinel end) const {
      assert(it != end);

      auto err_token = std::move(*it);
      const auto &line_str = err_token.get_line_string();

      this->skip_whitespace(it, end);

      if ((*it).kind != pp_tokenize_status::NewLine) {
        //行は1から、列は0から・・・
        const auto [row, col] = (*it).get_phline_pos();

        assert(col < line_str.length());

        reporter.print_report({line_str.data() + col + 1, line_str.length() - (col + 1)}, m_filename, err_token);
      } else {
        //出力するものがない・・・
        reporter.print_report({}, m_filename, *it);
      }
    }

    /**
    * @brief #pragmaディレクティブを実行する
    * @param reporter レポート出力オブジェクトへの参照
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    template<typename Reporter, typename TokensIterator, typename TokensSentinel>
    void pragma(Reporter&, TokensIterator& it, TokensSentinel end) const {
      assert(it != end);

      //改行まで飛ばす
      //あらゆるpragmaを無視、仮実装
      do {
        ++it;
      } while (it != end and (*it).kind != pp_tokenize_status::NewLine);
    }

    /**
    * @brief ホワイトスペースだけを飛ばす
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    template<typename TokensIterator, typename TokensSentinel>
    void skip_whitespace(TokensIterator& it, TokensSentinel end) {
      do {
        ++it;
      } while (it != end and (*it).kind == pp_tokenize_status::Whitespaces);
    }

  };  
    
} // namespace kusabira::PP