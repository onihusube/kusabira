#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_NO_TRY_CATCH_IN_ASSERTS
#include "doctest/doctest.h"

//テスト定義ヘッダをインクルード
#include "PP/pp_tokenizer_test.hpp"
#include "PP/pp_automaton_test.hpp"
#include "../src/PP/pp_parser.hpp"
#include "vocabulary/whimsy_str_view_test.hpp"
#include "PP/pp_paser_test.hpp"
#include "common_test.hpp"
#include "report_output_test.hpp"
#include "PP/pp_directive_manager_test.hpp"
#include "test/vocabulary/scope_test.hpp"
#include "test/vocabulary/concat_test.hpp"
#include "test/PP/unified_macro_test.hpp"
#include "test/PP/pp_directive_test.hpp"
#include "test/PP/pp_constexpr_test.hpp"