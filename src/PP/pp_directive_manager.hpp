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
    template<typename TokensIterator, typename TokensSentinel>
    fn error(TokensIterator it, TokensSentinel end) -> std::pmr::u8string {
      std::pmr::u8string err_massage{ u8_pmralloc{&def_mr };
      
      //本当は素の行にある文字列をそのまま出力する方が正しそう・・・
      for (; it != end; ++it) {
        //スペース区切りでパース済みpp-tokenの文字列を追加していく
        err_massage.append((*it).token);
        err_massage.append(u8" ");
      }

      return err_massage;
    }

  };
    
} // namespace kusabira::PP