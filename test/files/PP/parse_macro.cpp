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