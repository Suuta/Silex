
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
	class RenderingAPI;
	class RenderingContext;
	using RenderingContextCreateFunction = RenderingContext* (*)();


    class RenderingContext : Object
    {
		SL_DECLARE_CLASS(RenderingContext, Object)

	public:

		static RenderingContext* Create()
		{
			return createFunction();
		}

		static void ResisterCreateFunction(RenderingContextCreateFunction func)
		{
			createFunction = func;
		}

	public:

		virtual Result Initialize(bool enableValidation) = 0;

		// API実装
		virtual RenderingAPI* CreateRendringAPI() = 0;
		virtual void DestroyRendringAPI(RenderingAPI* api) = 0;

		// プレゼント命令のサポート
		virtual bool DeviceCanPresent(Surface* surface) const = 0;

		// サーフェース
		virtual Surface* CreateSurface(void* platformData) = 0;
		virtual void DestroySurface(Surface* surface) = 0;

		// デバイス情報
		virtual const DeviceInfo& GetDeviceInfo() const = 0;

	private:

		static inline RenderingContextCreateFunction createFunction;
    };
}
