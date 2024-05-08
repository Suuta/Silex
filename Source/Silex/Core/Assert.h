#pragma once

//======================================================================================
// アサーション
//======================================================================================

#ifdef SL_RELEASE
// TODO: MessageBox関数を呼び出す処理を追加
#define SL_ENABLE_ASSERTS 0
#else
#define SL_ENABLE_ASSERTS 1
#endif


// マクロの引数の数でオーバーロード
// https://qiita.com/tyanmahou/items/bb45c0ad63b9df4abaf1

#if SL_ENABLE_ASSERTS

    // ARG3(_1, _2, _3) の第一引数が可変長引数（__VA_ARGS__）になっており
    // 引数が1個ならば、ARG3(_1,     PRINT(), BREAK())に展開され、第3引数の BREAK() を呼ぶ
    // 引数が2個ならば、ARG3(_1, _2, PRINT(), BREAK())に展開され、第3引数の PRINT() を呼ぶ
    // SL_EXPAND() は再帰展開が上手く行われないための対応
    #define ARG3(_1, _2, _3, ...) _3
    #define BREAK(expr, ...) { if(!(expr)) {                            SL_DEBUG_BREAK(); } }
    #define PRINT(expr, ...) { if(!(expr)) { SL_LOG_FATAL(__VA_ARGS__); SL_DEBUG_BREAK(); } }

    // 第一引数に真偽値、第2引数にオプションでデバッグ出力する文字列を指定可能
    #define SL_ASSERT(...) SL_EXPAND(ARG3(__VA_ARGS__, PRINT(__VA_ARGS__), BREAK(__VA_ARGS__)))

#else

    //TODO: MSG(expr, "", __FILE__, __LINE__); のように、アサートのLog関数部分を MessageBox関数に置き換える
    #define SL_ASSERT(...)

#endif
