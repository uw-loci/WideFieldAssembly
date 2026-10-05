/*
 * Copyright 2019,2020,2021,2022,2023 Sony Corporation
 */

#pragma once

// <-- Engine Header -->
#include <CoreMinimal.h>
#include <UnrealClient.h>

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
#include "UnrealClient.h"
#include "ViewportClient.h"
#endif

class UTextureRenderTarget2D;

namespace xr_display
{
	class FXRDisplayViewportClient : public FViewportClient
	{
	public:

		FXRDisplayViewportClient(float SizeX, float SizeY);

		void Shutdown();

		void Tick();

		virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
		virtual bool InputAxis(const FInputKeyEventArgs& Args) override;
#elif ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		virtual bool InputAxis(FViewport* InViewport, FInputDeviceId InputDevice, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false) override;
#else
		virtual bool InputAxis(FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples = 1, bool bGamepad = false) override;
#endif
		virtual bool InputChar(FViewport* InViewport, int32 ControllerId, TCHAR Character) override;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
		virtual bool InputTouch(FViewport* InViewport, const FInputDeviceId DeviceId, uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, float Force, uint32 TouchpadIndex, const uint64 Timestamp) override;
		virtual bool InputMotion(FViewport* InViewport, const FInputDeviceId DeviceId, const FVector& Tilt, const FVector& RotationRate, const FVector& Gravity, const FVector& Acceleration, const uint64 Timestamp) override;
#else
		virtual bool InputTouch(FViewport* InViewport, int32 ControllerId, uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, float Force, FDateTime DeviceTimestamp, uint32 TouchpadIndex) override;
		virtual bool InputMotion(FViewport* INViewport, int32 ControllerId, const FVector& Tilt, const FVector& RotationRate, const FVector& Gravity, const FVector& Acceleration) override;
#endif

		virtual void MouseEnter(FViewport* InViewport, int32 x, int32 y) override;
		virtual void MouseLeave(FViewport* InViewport) override;
		virtual EMouseCaptureMode GetMouseCaptureMode() const override
		{
			return MouseCaptureMode;
		}

		UTextureRenderTarget2D* GetRenderTarget();

		// Draw canvas in RenderThread
		void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		void RenderTexture_RenderThread(FRDGBuilder& GraphBuilder);
#endif

		bool GetMouseRightButtonPressed() const;
		bool GetMousePosition(FVector2D& MousePosition, const bool bLocalPosition = true) const;

	private:

		void ShowMouseCursor(bool bShow);

	public:

		FViewport* Viewport;

	private:

		UTextureRenderTarget2D* RenderTarget;

		bool bShowMouseCursor;
		bool bMouseRightButtonPressed = false;

		// Used to prevent the cursor from being hidden when it is leave from the Window
		bool bLeaveMouseCursor;
		EMouseCaptureMode MouseCaptureMode;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		FCanvas* RenderCanvas;
#endif
	};

} // namespace xr_display
