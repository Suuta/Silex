
#pragma once

#include "Core/CoreType.h"


namespace Silex
{
#if 0
    //==============================================================================================
    // godot エンジン風  レンダーオブジェクトの抽象化
    //==============================================================================================
#define SL_DECLARE_HANDLE(name)                                                                    \
    struct name##Handle : public Handle                                                            \
    {                                                                                              \
        explicit operator bool() const                    { return handle != 0; }                  \
        bool operator< (const name##Handle&  other) const { return handle <  other.handle; }       \
        bool operator==(const name##Handle&  other) const { return handle == other.handle; }       \
        bool operator!=(const name##Handle&  other) const { return handle != other.handle; }       \
        name##Handle& operator=(name##Handle other)       { handle = other.handle; return *this; } \
                                                                                                   \
        explicit name##Handle(uint64 handle) : Handle(handle)         {}                           \
        explicit name##Handle(void* handle)  : Handle((uint64)handle) {}                           \
                                                                                                   \
        name##Handle(const name##Handle& other) : Handle(other.handle) {}                          \
        name##Handle() = default;                                                                  \
    };                                                                                             \
    using name = name##Handle;
#endif
    
    struct Handle
    {
        uint64 handle = 0;

        Handle()          = default;
        virtual ~Handle() = default;

        Handle(uint64 handle) : handle(handle) {}
        Handle(void* handle) : handle((uint64)handle) {}
    };

    using SwapChain     = Handle;
    using CommandPool   = Handle;
    using CommandBuffer = Handle;
    using Buffer        = Handle;
    using Image         = Handle;



    enum class Result : uint16
    {
        OK   = 0,
        Fail = 1,
    };

    enum class RenderDeviceAPI : uint8
    {
        None   = 0,
        Vulkan = 1,
        D3D12  = 2,
    };

    enum class RenderDeviceFeature : uint16
    {
        None = 0,
    };

    struct RenderDeviceDesc
    {
        RenderDeviceAPI     api;
        RenderDeviceFeature feature;
        bool                vsyncEnable = false;
    };

    class RenderDevice
    {
    public:

        virtual Result CreateSwapChain(SwapChain** swapchain) = 0;
        virtual void   DestroySwapChain(SwapChain* swapChain) = 0;
    };


    struct VKSwapChain : public SwapChain
    {
    };

    struct D3D12SwapChain : public SwapChain
    {
    };


    class VulkanDevice : public RenderDevice
    {
    public:

        Result CreateSwapChain(SwapChain** swapchain) override
        {
            VKSwapChain* sc = new VKSwapChain();
            sc->handle = (uint64)sc;
            *swapchain = sc;

            return Result::OK;
        }

        void DestroySwapChain(SwapChain* swapChain) override
        {
            VKSwapChain* sc = (VKSwapChain*)swapChain->handle;
            delete sc;
        }
    };

    class D3D12Device : public RenderDevice
    {
    public:

        Result CreateSwapChain(SwapChain** swapchain) override
        {
            D3D12SwapChain* sc = new D3D12SwapChain();
            sc->handle = (uint64)sc;
            *swapchain = sc;

            return Result::OK;
        }

        void DestroySwapChain(SwapChain* swapChain) override
        {
            D3D12SwapChain* sc = (D3D12SwapChain*)swapChain->handle;
            delete sc;
        }
    };

    inline Result CreateRenderDevice(const RenderDeviceDesc& desc, RenderDevice** device)
    {
        switch (desc.api)
        {
            case RenderDeviceAPI::None:   *device = nullptr;            return Result::Fail;
            case RenderDeviceAPI::Vulkan: *device = new VulkanDevice(); return Result::OK;
            case RenderDeviceAPI::D3D12:  *device = new D3D12Device();  return Result::OK;
        }

        return Result::Fail;
    }

    inline void DestroyRenderDevice(RenderDevice* device)
    {
        delete device;
    }

    inline void TEST_HANDLE_H()
    {
        Result result = Result::Fail;
        RenderDevice* device    = nullptr;
        SwapChain*    swapchain = nullptr;

        RenderDeviceDesc desc = {};
        desc.vsyncEnable = false;
        desc.api         = RenderDeviceAPI::Vulkan;

        result = CreateRenderDevice(desc, &device);
        result = device->CreateSwapChain(&swapchain);

        device->DestroySwapChain(swapchain);

        DestroyRenderDevice(device);
    }
}
