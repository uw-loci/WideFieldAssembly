/*
 * Copyright 2019,2020,2021,2022,2023,2024 Sony Corporation
 */

#pragma once

#include "IXRDisplayInterface.h"
#include "Blueprint/SRDisplayManager.h"

#include <CoreMinimal.h>
#include <HeadMountedDisplayBase.h>
#include <SceneViewExtension.h>

#include "misc_util_wrapper.h"
#include <memory>

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
#include "Containers/DynamicRHIResourceArray.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogSRDisplay, Log, All);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSRDRenderTextureCompletedDelegate, FRHICommandListImmediate&, FTextureRHIRef);
#else
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSRDRenderTextureCompletedDelegate, FRHICommandListImmediate&, FTexture2DRHIRef);
#endif

namespace srdisplay_module
{
	class SRDISPLAYMODULE_API FSRDisplaySystem : public xr_display::IXRDisplayInterface
	{

	public:

		FSRDisplaySystem();
		~FSRDisplaySystem();

		// Begin IXRSystemIdentifier
		virtual FName GetSystemName() const override
		{
			static const FName Name(TEXT("SRDisplay"));
			return Name;
		}
		// End IXRSystemIdentifier

		// Begin IXRTrackingSystem
		virtual FString GetVersionString() const override;
		virtual void OnBeginPlay(FWorldContext& InWorldContext) override;
		virtual void OnEndPlay(FWorldContext& InWorldContext) override;
		virtual bool OnStartGameFrame(FWorldContext& WorldContext) override;
		virtual void OnBeginRendering_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily) override;
		// End IXRTrackingSystem

		// Begin IStereoRendering
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		virtual void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture* BackBuffer, FRHITexture* SrcTexture, FVector2D WindowSize, bool skipHomography) const override;
		void HomographyTransform(FRHICommandListImmediate& RHICmdList, FRHITexture* BackBuffer, FRHITexture* SrcTexture, FVector2D WindowSize) const;
		void LowPassFilter(FRHICommandListImmediate& RHICmdList, FRHITexture* BackBuffer, FRHITexture* SrcTexture, FVector2D WindowSize) const;
#else
		virtual void RenderTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture2D* BackBuffer, FRHITexture2D* SrcTexture, FVector2D WindowSize, bool skipHomography) const override;
		void HomographyTransform(FRHICommandListImmediate& RHICmdList, FRHITexture2D* BackBuffer, FRHITexture2D* SrcTexture, FVector2D WindowSize) const;
		void LowPassFilter(FRHICommandListImmediate& RHICmdList, FRHITexture2D* BackBuffer, FRHITexture2D* SrcTexture, FVector2D WindowSize) const;
#endif
		// End IStereoRendering

		virtual const char* GetPlatformId() override;
		virtual SonyOzDeviceInfo* GetSelectDeviceInfo(SonyOzDeviceInfo* DeviceList, const uint64_t Size, int index = 0, bool isDuplicatedOutput = false) override;

		virtual void ConvertPoseFromXrRuntimeToGameWorld(FTransform Transform, FQuat& DstOrientation, FVector& DstPosition) const override;

		// Get projection matrix and clipping distance, and calculate stereo projection matrix.
		// Please do SetDisplaySpec before execution.
		virtual void UpdateProjectionMatrix(FMatrix& LeftViewProjectionMatrix, FMatrix& RightViewProjectionMatrix) override;

		virtual void SetDisplaySpec(xr_display::XrDisplaySpec InDisplaySpec) override;

		virtual bool DeprojectScreenToWorld(FSceneViewProjectionData ProjectionData, const FVector2D& ScreenPosition, FVector& WorldPosition, FVector& WorldDirection, FVector& CameraPosition) override;

		virtual float GetNearClip() override;
		virtual float GetFarClip() override;

		virtual SonyOzPosef GetStubHeadPose() override;
		virtual void SetStubHeadPose(const SonyOzPosef pose) override;

		virtual xr_display::FDisplayCorners GetDisplayCornners() override;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		virtual void BroadcastSrdRenderTextureCompletedDelegate(FRHICommandListImmediate& RHICmdList, FRHITexture* SrcTexture) override;
#else
		virtual void BroadcastSrdRenderTextureCompletedDelegate(FRHICommandListImmediate& RHICmdList, FRHITexture2D* SrcTexture) override;
#endif

	public:
		static const float DEFAULT_SRD_VIEW_SPACE_SCALE;
		static const float DEFAULT_SRD_FAR_CLIP;
		static const float METER_TO_CENTIMETER;

		void GetDisplayLength(float& Width, float& DepthByTilt, float& HeightByTilt) const;

	private:
		ASRDisplayManager* GetSRDisplayManagerActor();
		void UpdateSRDisplayManagerState();
		void UpdateSRDisplayPosition();
		bool GetViewMatrix(FMatrix& LeftViewMatrix, FMatrix& RightViewMatrix);
		FMatrix GetSpatialClippedProjectionMatrix(FVector ClipPlanePositionInWorldCoord, FVector ClipPlaneNormalVecInWorldCoord, FMatrix ViewMatrix, FMatrix ProjectionMatrix);
		FMatrix GetNearClipPlaneConvertedProjectionMatrix(FMatrix SrcProjMat, FVector4 PlaneInViewSpace);
		bool GetDisplayCornersPosition(FVector4& LeftBottomPosition, FVector4& LeftTopPosition, FVector4& RightBottomPosition, FVector4& RightTopPosition) const;

	private:
		IRendererModule* RendererModule;
		bool preShowCameraWindow = false;
		bool preIsSRRenderingActive = true;
		sony::oz::xr_runtime::SupportDevice ConnectedDevice = sony::oz::xr_runtime::SupportDevice::ELF_SR1;
		bool preEnableVerticalDisplayMode = false;
		int preRotationPitchDegree = 0;
		FVector preSRDisplayMangerPostion;

		// Actor "SRDisplayManager" and it's params.
		ASRDisplayManager* SRDisplayManager = nullptr;
		float SRDisplayViewSpaceScale = DEFAULT_SRD_VIEW_SPACE_SCALE;
		FTransform SRDisplayTransform = FTransform::Identity;
		float SRDisplayFarClip = DEFAULT_SRD_FAR_CLIP;
		bool SpatialClipping = false;
		bool ShowMouseCursorOnSRDisplay = false;

		// SR Display's specs.
		xr_display::XrDisplaySpec DisplaySpec;
		FVector2D DisplayResolution;
		xr_display::FDisplayCorners DisplayCorners;

		FMatrix LeftViewProjectionMatrix_RenderingThread;
		FMatrix RightViewProjectionMatrix_RenderingThread;

#if ENGINE_MAJOR_VERSION == 4
		FVector2D cursorHotSpot;
#else
		FVector2f cursorHotSpot;
#endif
		std::unique_ptr<uint8_t[]> CursorImageData;

		TResourceArray<uint32_t> structuredBufferResourceArray;
		int32_t structuredBufferSize;
		int32_t cursorImage_w;
		int32_t cursorImage_h;

		double preFrameStartTime;
		double deltaSwitchTime = 0;
		int PositionSwithchCount = 0;
		FVector initSRDisplayMangerPosition;

	// SRD API's
	public:
		bool SetHeadTrackingPaused(bool paused);
		sony::oz::xr_runtime::SupportDevice GetConnectedDevice();
		bool GetRuntimeVersion(FString& version);
		bool GetDisplayVersion(FString& version);
		bool GetRuntimeVersionInfo(int& major, int& minor, int& build, int& revision);
	private:
		bool OriginalShowMouseCursor = false;
		bool IsPreMouseInScreen = false;
		bool ShowMouseCursor = false;

	// RenderTexture_RenderThread Delegate
	private:
		FOnSRDRenderTextureCompletedDelegate OnSrdRenderTextureCompletedDelegate;
	public:
		FOnSRDRenderTextureCompletedDelegate& GetSRDRenderTextureCompletedDelegate();
	}; // class FSRDisplaySystem

} // namespace srdisplay_module
