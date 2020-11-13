#define N 1
#   define    M    N
int vm = M;

#define LPAREN() (
#define G(Q) 42
#define F(R, X, ...)  __VA_OPT__(G R X) )
int x = F(LPAREN(), 0, <:-);
int x = F(LPAREN(), 0,);
int x = F(LPAREN(), 0,,);

#define debug(...) fprintf(stderr, __VA_ARGS__)
#define showlist(...) puts(#__VA_ARGS__)
#define report(test, ...) ((test) ? puts(#test) : printf(__VA_ARGS__))
debug("Flag");
debug("X = %d\n", x);
showlist(The first, second, and third items.);
report(x>y, "x is %d but y is %d", x, y);

#undef F  // ↓にわざと空白を入れてる
#undef G          

#define F(...)           f(0 __VA_OPT__(,) __VA_ARGS__)
#define G(X, ...)        f(0, X __VA_OPT__(,) __VA_ARGS__)
#define SDEF(sname, ...) S sname __VA_OPT__(= { __VA_ARGS__ })
#define EMP
#define EMP2 /*スペース入ってる空マクロ*/   // ラインコメント
#define FEMP() //↓空白なし空関数マクロ
#define FEMP2()

F(a, b, c)
F()
F(EMP)
F(FEMP())

G(a, b, c)
G(a, )
G(a)

SDEF(foo);
SDEF(bar, 1, 2);

#define H2(X, Y, ...) __VA_OPT__(X ## Y,) __VA_ARGS__

H2(a, b, c, d)  // ab, c, d

#define H3(X, ...) #__VA_OPT__(X##X X##X)

H3(, 0)   // "" （空の文字列トークン

#define H4(X, ...) __VA_OPT__(a X ## X) ## b

H4(, 1) // a b （識別子二つ

#define H5A(...) __VA_OPT__()/**/__VA_OPT__()
#define H5B(X) a ## X ## b
#define H5C(X) H5B(X)

H5C(H5A())  // ab

#undef debug
// #, ##のテスト
#define str(s)      # s
#define xstr(s)     str(s)
#define debug(s, t) printf("x" # s "= %d, x" # t "= %s", \
               x ## s, x ## t)
#define INCFILE(n)  vers ## n
#define glue(a, b)  a ## b
#define xglue(a, b) glue(a, b)
#define HIGHLOW     "hello"
#define LOW         LOW ", world"

debug(1, 2);    // printf("x" "1" "= %d, x" "2" "= %s", x1, x2);
fputs(str(strncmp("abc\0d", "abc", '\4')        // 呼び出し中のコメント、改行、消える
    == 0) str(: @\n), s);   // fputs("strncmp(\"abc\\0d\", \"abc\", '\\4') == 0" ": @\n", s);