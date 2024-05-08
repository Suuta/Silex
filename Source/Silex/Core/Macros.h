
#pragma once

#define SL_PLATFORM_OPENGL              1
#define SL_PLATFORM_VULKAN              0 // Vulkan は未実装
#define SL_ENABLE_TRACK_HEAP_ALLOCATION 0
#define SL_ENABLE_TRACK_CRT_MEMORY_LEAK 1


#if (SL_PLATFORM_OPENGL + SL_PLATFORM_VULKAN != 1)
#error "描画APIは1つのみ指定可能です"
#endif


// __VA_ARGS__ 再帰展開を正しく展開させるため?
#define SL_EXPAND(x) x

// コンパイラ警告の抑制や、三項演算子で実行しないステートのダミー処理?
#define SL_NO_USE(x) (void)(x)


#if SL_PLATFORM_WINDOWS

    #define SL_DEBUG_BREAK() __debugbreak();
    #define SL_INLINE        __inline
    #define SL_FORCEINLINE   __forceinline

    //==============================
    // Windows メモリリークチェッカー
    //==============================
    #if SL_ENABLE_TRACK_CRT_MEMORY_LEAK
        #define SL_TRACK_CRT_MEMORY_LEAK() _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #else
        #define SL_TRACK_CRT_MEMORY_LEAK()
    #endif

#endif

#include "Core/Assert.h"


//======================================================================================
// ■Class ランタイム型情報
// -------------------------------------------------------------------------------------
// ClassID:   整数型 識別子
// ClassName: 文字列 識別子
// HashID:    ハッシュ値
// Super:     親クラス型
//======================================================================================

#define SL_DECLARE_CLASS(T, TSuper)\
public:\
    virtual const char* GetRuntimeClassName() const override { return staticClassName; }\
    virtual      uint64 GetRuntimeHashID()    const override { return staticHashID;    }\
public:\
    static inline const char*  staticClassName = #T;\
    static inline const uint64 staticHashID    = GlobalClassDataBase::Register<T>(#T);\
public:\
    using Super = TSuper;


//================================================================================================================
// グローバル new / delete 演算子オーバーライト
//================================================================================================================
// new    演算子: operator new() + T()     // new    に追加の引数を渡すと対応する operator new が呼ばれる
// delete 演算子: ~T() + operator delete() // delete は追加の引数を渡せない
//
// *** 追加の引数を渡す場合は、デストラクタ + operator delete(ptr, ...) の呼び出しが必要 ***
// std::destruct_at(ptr);     // ≒ ptr->~T(); デストラクタ
// operator delete(ptr, ...); // ≒ free(ptr); 解放
//================================================================================================================

//---------------------------------------------------------------------------
// プールアロケータ実装に伴い未使用となっている (追跡するメモリーなし)
//---------------------------------------------------------------------------
//#define SL_NEW          new
//#define SL_NEW_ARRAY    new
//#define SL_DELETE       delete
//#define SL_DELETE_ARRAY delete[]
//
//#define SL_PLACEMENT_NEW(location) new(location)
//---------------------------------------------------------------------------

struct SLEmpty {};

inline void* operator new  (size_t, void* where, SLEmpty) noexcept { return where; }
inline void* operator new[](size_t, void* where, SLEmpty) noexcept { return where; }
inline void  operator delete  (void*, void*, SLEmpty) noexcept { return; }
inline void  operator delete[](void*, void*, SLEmpty) noexcept { return; }


//=============================================
// stb_image.h 利用のためのマクロ
//=============================================
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif


#define FOR(num) for(uint64 i = 0; i < num; i++)