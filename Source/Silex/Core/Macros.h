
#pragma once

#define NEW_RENDERER                    1
#define SL_PLATFORM_OPENGL              1
#define SL_PLATFORM_VULKAN              0
#define SL_ENABLE_TRACK_HEAP_ALLOCATION 0
#define SL_ENABLE_ASSERTS               1

#if (SL_PLATFORM_OPENGL + SL_PLATFORM_VULKAN != 1)
#error "描画APIは1つのみ指定可能です"
#endif

// __VA_ARGS__ の再帰展開を正しく展開させるため?
#define SL_EXPAND(x) x

// コンパイラ警告の抑制や、三項演算子で実行しないステートのダミー処理?
#define SL_NO_USE(x) (void)(x)

// 引数の数によるオーバーロード解決
#define SL_ARG3(_1, _2, _3, ...) _3


//===========================================================================================================================
// 固有マクロ
//===========================================================================================================================
#ifdef _MSC_VER
    #define SL_DEBUG_BREAK() __debugbreak();
    #define SL_INLINE        __inline
    #define SL_FORCEINLINE   __forceinline
    #define SL_FUNCNAME      __FUNCTION__
    #define SL_FUNCSIG       __FUNCSIG__

    #ifdef CreateSemaphore
    #undef CreateSemaphore
    #endif
#endif

//===========================================================================================================================
// ログ
//===========================================================================================================================
#define SL_LOG_FATAL(...) Silex::Logger::Log(Silex::LogLevel::Fatal, std::format(__VA_ARGS__))
#define SL_LOG_ERROR(...) Silex::Logger::Log(Silex::LogLevel::Error, std::format(__VA_ARGS__))
#define SL_LOG_WARN(...)  Silex::Logger::Log(Silex::LogLevel::Warn,  std::format(__VA_ARGS__))
#define SL_LOG_INFO(...)  Silex::Logger::Log(Silex::LogLevel::Info,  std::format(__VA_ARGS__))
#define SL_LOG_TRACE(...) Silex::Logger::Log(Silex::LogLevel::Trace, std::format(__VA_ARGS__))
#define SL_LOG_DEBUG(...) Silex::Logger::Log(Silex::LogLevel::Debug, std::format(__VA_ARGS__))

#define SL_MESSAGE_INFO(...)  Silex::OS::Get()->Message(OS_MESSEGA_TYPE_INFO,  std::format(__VA_ARGS__))
#define SL_MESSAGE_ALERT(...) Silex::OS::Get()->Message(OS_MESSEGA_TYPE_ALERT, std::format(__VA_ARGS__))

//===========================================================================================================================
// エラーハンドリング
//===========================================================================================================================
#define SL_LOG_LOCATION(fn, file, line) Silex::Logger::Log(Silex::LogLevel::Error, std::format("{}, {}, {}", fn, file, line))
#define SL_CHECK_RETURN(expr, retval)   if (expr) { SL_LOG_LOCATION(__FUNCTION__, __FILE__, __LINE__); return retval; }

//===========================================================================================================================
// アサーション
//---------------------------------------------------------------------------------------------------------------------------
// マクロの引数の数でオーバーロード
// https://qiita.com/tyanmahou/items/bb45c0ad63b9df4abaf1
// 
// 第一引数に真偽値、第2引数にオプションでデバッグ出力する文字列を指定可能
// ARG3(_1, _2, _3) の第一引数が可変長引数（__VA_ARGS__）になっており
// 引数が1個ならば、ARG3(_1,     PRINT(), BREAK())に展開され、第3引数の BREAK() を呼ぶ
// 引数が2個ならば、ARG3(_1, _2, PRINT(), BREAK())に展開され、第3引数の PRINT() を呼ぶ
// SL_EXPAND() は再帰展開が上手く行われないための対応
//===========================================================================================================================

#if SL_ENABLE_ASSERTS
    #if SL_DEBUG
        #define SL_BREAK(expr, ...) { if(!(expr)) {                            SL_DEBUG_BREAK(); } }
        #define SL_PRINT(expr, ...) { if(!(expr)) { SL_LOG_FATAL(__VA_ARGS__); SL_DEBUG_BREAK(); } }
    #else
        #define SL_BREAK(expr, ...) { if(!(expr)) { SL_MESSAGE_ALERT(L"");            }}
        #define SL_PRINT(expr, ...) { if(!(expr)) { SL_MESSAGE_ALERT(L##__VA_ARGS__); }}
    #endif

    #define SL_ASSERT(...) SL_EXPAND(SL_ARG3(__VA_ARGS__, SL_PRINT(__VA_ARGS__), SL_BREAK(__VA_ARGS__)))
#endif

//===========================================================================================================================
// ■Class ランタイム型比較
//---------------------------------------------------------------------------------------------------------------------------
// ClassID:   整数型 識別子
// ClassName: 文字列 識別子
// HashID:    ハッシュ値
// Super:     親クラス型
//===========================================================================================================================

#define SL_CLASS(T, TSuper)\
public:\
    virtual const char* GetRuntimeClassName() const override { return staticClassName; }\
    virtual uint64      GetRuntimeHashID()    const override { return staticHashID;    }\
    static inline const char*  staticClassName = #T;\
    static inline const uint64 staticHashID    = GlobalClassDataBase::Register<T>(#T);\
    using Super = TSuper;

//============================================================================================================================
// グローバル new / delete 演算子オーバーライト
//----------------------------------------------------------------------------------------------------------------------------
// new    演算子: operator new() + T()     // new    に追加の引数を渡すと対応する operator new が呼ばれる
// delete 演算子: ~T() + operator delete() // delete は追加の引数を渡せない
//
// *** 追加の引数を渡す場合は、デストラクタ + operator delete(ptr, ...) の呼び出しが必要 ***
// std::destruct_at(ptr);     // ≒ ptr->~T(); デストラクタ
// operator delete(ptr, ...); // ≒ free(ptr); 解放
//============================================================================================================================

//----------------------------------------------------------------------------------------------------------------------------
// プールアロケータ実装に伴い未使用となっている (追跡するメモリーなし)
//----------------------------------------------------------------------------------------------------------------------------
//#define SL_NEW          new
//#define SL_NEW_ARRAY    new
//#define SL_DELETE       delete
//#define SL_DELETE_ARRAY delete[]
//
//#define SL_PLACEMENT_NEW(location) new(location)
//----------------------------------------------------------------------------------------------------------------------------

// struct SLEmpty {};
// inline void* operator new  (size_t, void* where, SLEmpty) noexcept { return where; }
// inline void* operator new[](size_t, void* where, SLEmpty) noexcept { return where; }
// inline void  operator delete  (void*, void*, SLEmpty) noexcept { return; }
// inline void  operator delete[](void*, void*, SLEmpty) noexcept { return; }
