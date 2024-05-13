
#pragma once
#include "Core/Core.h"


namespace Silex
{
	// 各GPUのベンダーID
	enum DeviceVendor
	{
		DEVICE_VENDOR_UNKNOWN   = 0x0000,
		DEVICE_VENDOR_AMD       = 0x1002,
		DEVICE_VENDOR_IMGTEC    = 0x1010,
		DEVICE_VENDOR_APPLE     = 0x106B,
		DEVICE_VENDOR_NVIDIA    = 0x10DE,
		DEVICE_VENDOR_ARM       = 0x13B5,
		DEVICE_VENDOR_MICROSOFT = 0x1414,
		DEVICE_VENDOR_QUALCOMM  = 0x5143,
		DEVICE_VENDOR_INTEL     = 0x8086
	};

	enum DeviceType
	{
		DEVICE_TYPE_OTHER          = 0,
		DEVICE_TYPE_INTEGRATED_GPU = 1,
		DEVICE_TYPE_DISCRETE_GPU   = 2,
		DEVICE_TYPE_VIRTUAL_GPU    = 3,
		DEVICE_TYPE_CPU            = 4,
		DEVICE_TYPE_MAX            = 5,
	};

	struct DeviceInfo
	{
		std::string  name   = "Unknown";
		DeviceVendor vendor = DEVICE_VENDOR_UNKNOWN;
		DeviceType   type   = DEVICE_TYPE_OTHER;
	};


	using Surface = Handle;


    class RenderingContext
    {
	public:

		virtual void Initialize() = 0;

		// API実装
		//virtual RenderingAPI CreateRendringAPI()  = 0;
		//virtual void         DestroyRendringAPI() = 0;

		// サーフェース
		virtual Surface* CreateSurface(void* platformData) = 0;
		virtual void     DestroySurface()                  = 0;

		// デバイス情報
		virtual const DeviceInfo& GetDeviceInfo() const = 0;
    };
}
