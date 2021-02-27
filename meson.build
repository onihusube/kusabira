project('kusabira', 'cpp', default_options : ['warning_level=3', 'cpp_std=c++17', 'cpp_debugstl=true'], meson_version : '>=0.50.0')

#依存プロジェクト
doctest_proj = subproject('doctest')
doctest_dep = doctest_proj.get_variable('doctest_dep')
tlexpected_proj = subproject('tlexpected')
tlexpected_dep = tlexpected_proj.get_variable('tlexpected_dep')

#コンパイルオプション調整
cppcompiler = meson.get_compiler('cpp').get_argument_syntax()

if cppcompiler == 'msvc'
    options = ['/std:c++latest', '/source-charset:utf-8', '/Zc:__cplusplus']
elif cppcompiler == 'gcc'
    options = ['-std=c++2a']
else
    options = []
endif

#VSプロジェクトに編集しうるファイルを追加する
files = ['src/common.hpp', 'src/PP/file_reader.hpp', 'src/PP/pp_tokenizer.hpp', 'test/PP/pp_filereader_test.hpp',
         'test/PP/pp_tokenizer_test.hpp', 'src/PP/pp_automaton.hpp', 'test/PP/pp_automaton_test.hpp',
         'src/PP/op_and_punc_table.hpp', 'src/PP/pp_parser.hpp', 'src/vocabulary/whimsy_str_view.hpp',
         'test/vocabulary/whimsy_str_view_test.hpp', 'test/PP/pp_paser_test.hpp',
         'src/PP/pp_directive_manager.hpp', 'src/report_output.hpp', 'test/common_test.hpp',
         'test/report_output_test.hpp', 'test/PP/pp_directive_manager_test.hpp',
         'src/vocabulary/scope.hpp', 'test/vocabulary/scope_test.hpp', 'src/vocabulary/concat.hpp', 'test/vocabulary/concat_test.hpp',
         'src/PP/macro_manager.hpp', 'test/PP/unified_macro_test.hpp', 'test/PP/pp_directive_test.hpp',
         'src/PP/pp_constexpr.hpp', 'test/PP/pp_constexpr_test.hpp']

include_dir = include_directories('src', 'subprojects/doctest', 'subprojects/tlexpected/include')

exe = executable('kusabira_test', 'test/kusabira_test.cpp', include_directories : include_dir, extra_files : files, cpp_args : options, dependencies : [doctest_dep, tlexpected_dep])

#テストの設定
test('kusabira test', exe)