/*
 * Copyright 2019,2020,2021,2022,2023 Sony Corporation
 */

#pragma once

// <-- Engine Header -->
#include <CoreMinimal.h>
#include <Widgets/SWindow.h>
#include "Runtime/Launch/Resources/Version.h"

class UTextureRenderTarget2D;
class FRHICommandListImmediate;
class FViewport;

namespace xr_display
{
	class SXRDisplaySceneViewport;

	class FXRDisplayWindow
	{
	public:

		static void Initialize(FVector2D Resolution, FIntRect ScreenPosition);
		static void Finalize();
		static FXRDisplayWindow* GetPlayWindow();
		static FVector2D ScreenPos_;
		static FVector2D GetScreenPosition() { return ScreenPos_; }

	public:

		FXRDisplayWindow(FVector2D Resolution);
		~FXRDisplayWindow();

		TSharedPtr<SWindow> GetWindow();
		UTextureRenderTarget2D* GetRenderTarget();

		void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		void RenderTexture_RenderThread(FRDGBuilder& GraphBuilder);
		void DrawXRDisplayRendering(FRDGBuilder& GraphBuilder, SWindow& window, FRDGTexture* Texture);
#endif

		bool GetMouseRightButtonPressed() const;
		bool GetMousePosition(FVector2D& MousePosition, const bool bLocalPosition = true) const;
		bool GetSRDTopLeftCornerWorldPosition(FVector2D& SRDTopLeftCornerWorldPosition);

		FViewport* GetViewport();

	private:

		void OnWindowClosed(const TSharedRef<SWindow>& InWindow);

	private:

		static TSharedPtr<FXRDisplayWindow> PlayWindow;

		TSharedPtr<SWindow> Window;
		FOnWindowClosed InDelegate;
		TSharedPtr<SXRDisplaySceneViewport> PlaySceneViewport;
	};

} // namespace xr_display
