#include "../common.hpp"
#include "../report_output.hpp"

namespace kusabira::PP {

  struct pp_directive_manager {

    fs::path m_filename{};

    template<typename TokensIterator, typename TokensSentinel>
    fn include(TokensIterator& it, TokensSentinel end) -> std::pmr::list<pp_token> {
      //未実装
      assert(false);
      return {};
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

      //ホワイトスペースを飛ばす
      do {
        ++it;
      } while (it != end and (*it).kind == pp_tokenize_status::Whitespaces);

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
  };  
    
} // namespace kusabira::PP