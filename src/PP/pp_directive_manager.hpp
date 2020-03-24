#include "../common.hpp"

namespace kusabira::PP {

  struct pp_directive_manager {

    template<typename TokensIterator, typename TokensSentinel>
    fn include(TokensIterator it, TokensSentinel end) -> std::pmr::list<pp_token> {
      //未実装
      assert(false);
      return {};
    }


    /**
    * @brief #errorディレクティブを実行する
    * @param it プリプロセッシングトークン列の先頭イテレータ
    * @param end プリプロセッシングトークン列の終端イテレータ
    */
    void error(std::u8string_view) const {
      //エラーメッセージを出力
      //std::cout << err_message;
    }
  };  
    
} // namespace kusabira::PP