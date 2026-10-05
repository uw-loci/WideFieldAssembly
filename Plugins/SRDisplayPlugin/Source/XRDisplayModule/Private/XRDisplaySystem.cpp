/*
 * Copyright 2019,2020,2021,2022,2023 Sony Corporation
 */

#include "XRDisplaySystem.h"

#include "XRDisplayTextResource.h"
#include "RenderTargetManager.h"

#include "Window/XRDisplayWindow.h"

#include <Slate/SceneViewport.h>
#include <Modules/ModuleManager.h>
#include "Engine/GameEngine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/WorldSettings.h"
#include "TextureResource.h"

#if PLATFORM_WINDOWS
#include <Windows/AllowWindowsPlatformTypes.h>
#include <Windows/HideWindowsPlatformTypes.h>

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
#include "ID3D12DynamicRHI.h"
#else
#include "D3D12RHIPrivate.h"
#endif // ENGINE_MAJOR_VERSION == 5
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
#include "GameFramework/PlayerController.h"
#else
#include "Engine/LocalPlayer.h"
#endif

#include "RenderGraphBuilder.h"

DEFINE_LOG_CATEGORY(LogXRDisplay);

using FXrRuntimePose = SonyOzPoseId;
using FHalfAngleList = SonyOzProjection;

BEGIN_SHADER_PARAMETER_STRUCT(FCopyToStagingParameters, )
	RDG_TEXTURE_ACCESS(Input, ERHIAccess::CopySrc)
	RDG_TEXTURE_ACCESS(Output, ERHIAccess::CopyDest)
END_SHADER_PARAMETER_STRUCT()

namespace xr_display
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	FTextureRHIRef RenderTexture = nullptr;
#else
	FTexture2DRHIRef RenderTexture = nullptr;
#endif

	static const float METER_TO_CENTIMETER = 100.f;

	const float FXRDisplaySystem::DEFAULT_VIEW_SPACE_SCALE = 1.f;
	const float FXRDisplaySystem::DEFAULT_FAR_CLIP = 100000000.f;

	IXRDisplayInterface* FXRDisplaySystem::XRDisplay = nullptr;

	FXRDisplaySystem::FXRDisplaySystem(const FAutoRegister& AutoRegister)
		: FHeadMountedDisplayBase(nullptr)
		, FSceneViewExtensionBase(AutoRegister)
	{
		LeftViewProjectionMatrix = GetDefaultProjectionMatrix();
		RightViewProjectionMatrix = GetDefaultProjectionMatrix();
	}

	FXRDisplaySystem::~FXRDisplaySystem()
	{
		if (RenderTargetManager != nullptr)
		{
			delete RenderTargetManager;
			RenderTargetManager = nullptr;
		}

		FinalizeXrRuntime();
	}

	void FXRDisplaySystem::SetXRDisplay(IXRDisplayInterface* InXRDisplay)
	{
		if (XRDisplay == nullptr)
		{
			XRDisplay = InXRDisplay;
		}
	}

	FVector2D FXRDisplaySystem::GetScreenPosition()
	{
		return FXRDisplayWindow::GetScreenPosition();
	}

	IXRDisplayInterface* FXRDisplaySystem::GetXRDisplay()
	{
		return XRDisplay;
	}

	bool FXRDisplaySystem::InitializeXrRuntime()
	{
		if (!IsProjectGraphicsAPISupported())
		{
			return false;
		}

		{
			LeftViewProjectionMatrix = GetDefaultProjectionMatrix();
			RightViewProjectionMatrix = GetDefaultProjectionMatrix();
		}

		return true;
	}

	bool FXRDisplaySystem::FinalizeXrRuntime()
	{
		StopXrRuntime();

		return true;
	}

	FString FXRDisplaySystem::GetVersionString() const
	{
		// FIXME: Get version string from M/W.
		if (XRDisplay)
		{
			return XRDisplay->GetVersionString();
		}
		return FString();
	}

#if ENGINE_MAJOR_VERSION == 5 || ENGINE_MINOR_VERSION >= 26
	int32 FXRDisplaySystem::GetXRSystemFlags() const
	{
		return EXRSystemFlags::IsHeadMounted;
	}
#endif // ENGINE_MAJOR_VERSION == 5 || ENGINE_MINOR_VERSION >= 26

	bool FXRDisplaySystem::EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type)
	{
		if (Type == EXRTrackedDeviceType::Any || Type == EXRTrackedDeviceType::HeadMountedDisplay)
		{
			OutDevices.Add(IXRTrackingSystem::HMDDeviceId);
			return true;
		}

		return false;
	}

	bool FXRDisplaySystem::GetCurrentPose(int32 DeviceId, FQuat& OutOrientation, FVector& OutPosition)
	{
		FScopeLock SLGetCurrentPose(&CSGetCurrentPose);

		OutOrientation = FQuat::Identity;
		OutPosition = FVector::ZeroVector;

		if (DeviceId != IXRTrackingSystem::HMDDeviceId)
		{
			return false;
		}

		if (IsInRenderingThread())
		{
			return false;
		}

		if (XrRuntimeWrapper.IsValid() == false)
		{
			return false;
		}

		const SonyOzPosef& Pose = XrRuntimeWrapper->GetCachedPose(FXrRuntimePose::HEAD);
		ConvertPoseFromXrRuntimeToGameWorld(Pose, OutOrientation, OutPosition);

		return true;
	}

	void FXRDisplaySystem::ConvertPoseFromXrRuntimeToGameWorld(SonyOzPosef SrcPose, FQuat& DstOrientation, FVector& DstPosition) const
	{
		FTransform Transform = FTransform(
			FQuat(SrcPose.orientation.x, SrcPose.orientation.y, SrcPose.orientation.z, SrcPose.orientation.w),
			FVector(SrcPose.position.x, SrcPose.position.y, SrcPose.position.z),
			FVector(1.f, 1.f, 1.f));

		if (XRDisplay)
		{
			XRDisplay->ConvertPoseFromXrRuntimeToGameWorld(Transform, DstOrientation, DstPosition);
		}
	}

	void FXRDisplaySystem::ShowXrSystemErrorDialog()
	{
		enum SystemErrorCode {
			kSuccess = 0,            ///< API request succeeded.
			kNoAvailableDevice = 1,  ///< There is no available device.
			kDeviceLost = 2,         ///< The device lost.
			kDeviceBusy = 3,         ///< The device is already uses by another application.
			kInvalidData = 4,        ///< Invalid API argument.
			kNoData = 5,             ///< The requested data doesn't exist.
			kOperationFailed = 6,    ///< Some operation has failed.
			kUsbNotConnected = 7,    ///< USB cable is disconnected.
			kCameraWithUsb20 = 8,    ///< Camera is connected with USB2.0
			kNoUsbOrNoPower = 9,
			kAnotherApplicationRunning = -10002
		};
		if (XrRuntimeWrapper)
		{
			SonyOzXrSystemError Error;
			XrRuntimeWrapper->GetXrSystemError(&Error);
			if (Error.result != SonyOzXrSystemErrorResult::Error)
			{
				return;
			}
			std::wstring error_message = SRD_ERROR_MESSAGE_C016 + L"\n" + SRD_ERROR_MESSAGE_C017;
			switch (Error.code)
			{
				case kSuccess:
					return;
				case kNoAvailableDevice:
					error_message = SRD_ERROR_MESSAGE_C020 + L"\n" + SRD_ERROR_MESSAGE_C017;
					break;
				case kDeviceLost:
					error_message = SRD_ERROR_MESSAGE_C021 + L"\n" + SRD_ERROR_MESSAGE_C017;
					break;
				case kDeviceBusy:
					error_message = L"kDeviceBusy";
					break;
				case kInvalidData:
					error_message = L"kInvalidData";
					break;
				case kNoData:
					error_message = L"kNoData";
					break;
				case kOperationFailed:
					error_message = L"kOperationFailed";
					break;
				case kUsbNotConnected:
					error_message = SRD_ERROR_MESSAGE_C019 + L"\n" + SRD_ERROR_MESSAGE_C017;
					break;
				case kCameraWithUsb20:
					error_message = SRD_ERROR_MESSAGE_C030 + L"\n" + SRD_ERROR_MESSAGE_C017;
					break;
				case kNoUsbOrNoPower:
					error_message = SRD_ERROR_MESSAGE_C019_2 + L"\n" + SRD_ERROR_MESSAGE_C017;
					break;
				case kAnotherApplicationRunning:
					error_message = SRD_ERROR_MESSAGE_C023 + L"\n" + SRD_ERROR_MESSAGE_C017;
					break;
				default:
					break;
			}
			std::wstring dialog_title = L"SRDisplay";
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, error_message.c_str(), dialog_title.c_str());
		}
	}

	bool FXRDisplaySystem::IsHeadTrackingAllowed() const
	{
		return bIsSessionBegun;
	}

	void FXRDisplaySystem::OnBeginPlay(FWorldContext& InWorldContext)
	{
		// initialize parameter
		bQuitApplication = false;

		if (!IsProjectPlayModeSupported())
		{
			bQuitApplication = true;
			return;
		}

		if (XRDisplay)
		{
			XRDisplay->OnBeginPlay(InWorldContext);
		}

		if (!StartXrRuntime())
		{
			bQuitApplication = true;
			return;
		}

		if (!InitializeXrRuntime())
		{
			bQuitApplication = true;
			return;
		}
		if (isWithoutXRDisplayMode)
		{
			CurrentHeadPose = XRDisplay->GetStubHeadPose();
		}
		EnableStereo(true);
	}

	void FXRDisplaySystem::OnEndPlay(FWorldContext& InWorldContext)
	{
		EnableStereo(false);

		if (XRDisplay)
		{
			XRDisplay->OnEndPlay(InWorldContext);
		}

		RenderTexture.SafeRelease();

		StopXrRuntime();
	}

	bool FXRDisplaySystem::StartXrRuntime()
	{
		// If this application hasn't begun XR runtime session yet, begin it.
		if (!bIsSessionBegun)
		{
			if (XRDisplay == nullptr)
			{
				return false;
			}

			XrRuntimeWrapper = MakeUnique<FXrRuntimeWrapper>();
			
			SonyOzPlatformId PlatformId = XRDisplay->GetPlatformId();

			if (!HandleXRRuntimeResult(XrRuntimeWrapper->InitializeRuntime(PlatformId))) {
				return false;
			}

			auto logCallback = [](const char* MessagePointer, SonyOzLogSettings_LogLevels RuntimeLogLevel)
			{
				FString Message(UTF8_TO_TCHAR(MessagePointer));
				switch (RuntimeLogLevel)
				{
				case SonyOzLogSettings_LogLevels::TRACE:
					UE_LOG(LogXRDisplay, VeryVerbose, TEXT("%s"), *Message);
					break;
				case SonyOzLogSettings_LogLevels::DEBUG:
					UE_LOG(LogXRDisplay, Log, TEXT("%s"), *Message);
					break;
				case SonyOzLogSettings_LogLevels::INFO:
					UE_LOG(LogXRDisplay, Display, TEXT("%s"), *Message);
					break;
				case SonyOzLogSettings_LogLevels::WARN:
					UE_LOG(LogXRDisplay, Warning, TEXT("%s"), *Message);
					break;
				case SonyOzLogSettings_LogLevels::ERR:
					UE_LOG(LogXRDisplay, Error, TEXT("%s"), *Message);
					break;
				case SonyOzLogSettings_LogLevels::CRITICAL:
					UE_LOG(LogXRDisplay, Fatal, TEXT("%s"), *Message);
					break;
				default:
					break;
				}
			};

			SonyOzResult Result = SonyOzResult::SUCCESS;
#if WITH_EDITOR
			if (!BeginSingleSession(PlatformId, logCallback, Result))
			{
				return false;
			}
#else
			if (isWithoutXRDisplayMode)
			{
				if (!BeginSingleSession(PlatformId, logCallback, Result))
				{
					return false;
				}
			}
			else
			{
				if (!BeginMultiSession(PlatformId, logCallback, Result))
				{
					return false;
				}
			}
#endif

			bIsSessionBegun = true;
			if (Result != SonyOzResult::SUCCESS)
			{
				if (Result == SonyOzResult::ERROR_SESSION_NOT_RUNNING)
				{
					ShowXrSystemErrorDialog();
				}
				return false;
			}

			Result = XrRuntimeWrapper->GetDisplaySpec(&DisplaySpec);
			if (Result != SonyOzResult::SUCCESS)
			{
				return false;
			}

			XrDisplaySpec ConvertedDisplaySpec = {
				{DisplaySpec.display_size.width_m, DisplaySpec.display_size.height_m},
				{DisplaySpec.display_resolution.width, DisplaySpec.display_resolution.height, DisplaySpec.display_resolution.area},
				DisplaySpec.display_tilt_rad
			};
			XRDisplay->SetDisplaySpec(ConvertedDisplaySpec);

			XRDisplay->SetSessionHandle(XrRuntimeWrapper->GetSessionHandle());

			bIsSessionBegun = true;

			XrRuntimeWrapper->SetLensShiftEnabled(true);
			XrRuntimeWrapper->GetLensShiftEnabled(&isLensShiftEnabled);
			XrRuntimeWrapper->GetPerformancePriorityEnabled(&isPerformancePriorityEnabled);
		}

		return true;
	}

	bool FXRDisplaySystem::BeginSingleSession(SonyOzPlatformId PlatformId, SonyOzLogSettings_LogCallback callback, SonyOzResult& Result)
	{
		uint64_t Size = 0;
		if (!HandleXRRuntimeResult(XrRuntimeWrapper->GetDeviceNum(PlatformId, &Size))) {
			return false;
		}
		auto DeviceList = MakeUnique<SonyOzDeviceInfo[]>(Size);
		if (!HandleXRRuntimeResult(XrRuntimeWrapper->EnumerateDevices(PlatformId, Size, DeviceList.Get()))
			|| DeviceList.IsValid() == false) {
			return false;
		}
		if (!isWithoutXRDisplayMode && Size <= 1) {
			HandleXRRuntimeResult(SonyOzResult::ERROR_DEVICE_NOT_FOUND);
			return false;
		}
		SonyOzDeviceInfo* DeviceInfo = XRDisplay->GetSelectDeviceInfo(DeviceList.Get(), Size, 0);
		if (!DeviceInfo) {
			return false;
		}
		Result = XrRuntimeWrapper->InitializeSession(PlatformId, DeviceInfo, callback);

		return true;
	}

	bool FXRDisplaySystem::BeginMultiSession(SonyOzPlatformId PlatformId, SonyOzLogSettings_LogCallback callback, SonyOzResult& Result)
	{
		auto ShowDialog = [](std::wstring error_message)
		{
			std::wstring dialog_title = L"SRDisplay";
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, error_message.c_str(), dialog_title.c_str());
		};
		auto ShowErrorDialog = [this, ShowDialog](SonyOzResult Result)
		{
			if (Result == SonyOzResult::ERROR_RUNTIME_UNSUPPORTED)
			{
				ShowDialog(SRD_ERROR_MESSAGE_C036_2_4_0);
			}
			else
			{
				HandleXRRuntimeResult(Result);
			}
		};

		uint64_t Size = 0;
		Result = XrRuntimeWrapper->GetMultiDisplayDeviceNum(PlatformId, &Size);
		if (Result != SonyOzResult::SUCCESS)
		{
			if (LinkedMode == SonyOzMultiDisplayMode::Single)
			{
				return BeginSingleSession(PlatformId, callback, Result);
			}
			ShowErrorDialog(Result);
			return false;
		}
		auto DeviceList = MakeUnique<SonyOzMultiDisplayDeviceInfo[]>(Size);
		Result = XrRuntimeWrapper->EnumerateMultiDisplayDevices(PlatformId, Size, DeviceList.Get());
		if (Result != SonyOzResult::SUCCESS || DeviceList.IsValid() == false)
		{
			if (LinkedMode == SonyOzMultiDisplayMode::Single)
			{
				return BeginSingleSession(PlatformId, callback, Result);
			}
			ShowErrorDialog(Result);
			return false;
		}
		if (Size <= 1)
		{
			if (LinkedMode == SonyOzMultiDisplayMode::Single)
			{
				return BeginSingleSession(PlatformId, callback, Result);
			}
			ShowErrorDialog(SonyOzResult::ERROR_DEVICE_NOT_FOUND);
			return false;
		}

		int num = Size - 1;
		auto LinkedDeviceList = MakeUnique<SonyOzDeviceInfo[]>(num);
		for (int i = 0; i < num; i++)
		{
			LinkedDeviceList[i] = DeviceList[i].device_info;
		}

		SonyOzMultiDisplayMode mode = DeviceList[0].multi_display_mode;
		if (!(LinkedMode == SonyOzMultiDisplayMode::Single && mode == SonyOzMultiDisplayMode::Clone))
		{
			if (mode != LinkedMode)
			{
				auto MultiDisplayModeToWString = [](SonyOzMultiDisplayMode mode) -> std::wstring {
					switch (mode)
					{
					case SonyOzMultiDisplayMode::Single: return L"Single";
					case SonyOzMultiDisplayMode::Vertical: return L"Vertical";
					case SonyOzMultiDisplayMode::Horizontal: return L"Horizontal";
					case SonyOzMultiDisplayMode::Grid: return L"Grid";
					case SonyOzMultiDisplayMode::Clone: return L"Duplicated";
					default: return L"Single";
					}
				};
				std::wstring message = L"The Multi Display Mode set in the project settings (" +
					MultiDisplayModeToWString(LinkedMode) + L") does not match that of the Spatial Reality Display Settings(" +
					MultiDisplayModeToWString(mode) + L").";
				ShowDialog(message);
			}
		}

		if (LinkedMode != SonyOzMultiDisplayMode::Single || (LinkedMode == SonyOzMultiDisplayMode::Single && mode == SonyOzMultiDisplayMode::Clone))
		{
			if (num < SettingSRDisplayNum)
			{
				std::wstring message = L"The number of Spatial Reality Displays detected (" +
					std::to_wstring(num) + L") is less than the number set in the project settings (" + std::to_wstring(SettingSRDisplayNum) + L").";
				ShowDialog(message);
			}
			for (int i = 0; i < num; i++)
			{
				SonyOzDeviceInfo* DeviceInfo = XRDisplay->GetSelectDeviceInfo(LinkedDeviceList.Get(), num, i, mode == SonyOzMultiDisplayMode::Clone);
				Result = XrRuntimeWrapper->InitializeSession(PlatformId, DeviceInfo, callback);
				if (Result == SonyOzResult::SUCCESS) {
					SessionIndex = i;
					break;
				}
			}
			return true;
		}
		else
		{
			return BeginSingleSession(PlatformId, callback, Result);
		}
	}

	bool FXRDisplaySystem::StopXrRuntime()
	{
		if (XRDisplay)
		{
			XRDisplay->RemoveSessionHandle();
		}

		if (XrRuntimeWrapper)
		{
			XrRuntimeWrapper->FinalizeSession();
			XrRuntimeWrapper->FinalizeRuntime();
			XrRuntimeWrapper.Reset();
		}

		bIsSessionBegun = false;

		return true;
	}

	bool FXRDisplaySystem::OnStartGameFrame(FWorldContext& WorldContext)
	{
		check(WorldContext.World());

		if (bQuitApplication)
		{
			if (GWorld->GetWorld()->GetFirstPlayerController())
			{
				GWorld->GetWorld()->GetFirstPlayerController()->ConsoleCommand("quit");
			}
			return false;
		}

		if (!bIsSessionBegun)
		{
			return false;
		}

		if (XrRuntimeWrapper.IsValid() == false)
		{
			return false;
		}

		EnableStereo(true);

		if (XrRuntimeWrapper) {
			SonyOzSessionState state;
			XrRuntimeWrapper->GetState(&state);
			if (state == SonyOzSessionState::STOPPING) {
				bQuitApplication = true;
				ShowXrSystemErrorDialog();
			}
			else if (state != SonyOzSessionState::RUNNING) {
				return false;
			}
		}

		if (XRDisplay)
		{
			XRDisplay->OnStartGameFrame(WorldContext);
			DisplayCorners = XRDisplay->GetDisplayCornners();
		}

		XrRuntimeWrapper->UpdatePoses();

		if (isWithoutXRDisplayMode)
		{
			if (FXRDisplayWindow::GetPlayWindow() && FXRDisplayWindow::GetPlayWindow()->GetMouseRightButtonPressed())
			{
				float LocationX = 0.f;
				float LocationY = 0.f;
				if (GetMousePosition(LocationX, LocationY))
				{
					if (!isMousePressed)
					{
						PrevMousePos = FVector2D(LocationX, LocationY);
						isMousePressed = true;
					}
					float DiffX = (LocationX - PrevMousePos.X) / 1000;
					float DiffY = (LocationY - PrevMousePos.Y) / 1000;

					constexpr float minX = -0.2f;
					constexpr float maxX = 0.2f;
					if (minX < CurrentHeadPose.position.x + DiffX && CurrentHeadPose.position.x + DiffX < maxX)
					{
						CurrentHeadPose = SonyOzPosef(CurrentHeadPose.orientation,
																					SonyOzVector3f(CurrentHeadPose.position + SonyOzVector3f(DiffX, 0, 0)));
					}
					constexpr float minY = 0.1f;
					constexpr float maxY = 0.4f;
					if (minY < CurrentHeadPose.position.y + DiffY && CurrentHeadPose.position.y + DiffY < maxY)
					{
						CurrentHeadPose = SonyOzPosef(CurrentHeadPose.orientation,
																					SonyOzVector3f(CurrentHeadPose.position + SonyOzVector3f(0, DiffY, 0)));
					}
					XRDisplay->SetStubHeadPose(CurrentHeadPose);
					PrevMousePos = FVector2D(LocationX, LocationY);
				}
			}
			else
			{
				isMousePressed = false;
			}
		}

		UpdateProjectionMatrix(); // Update Projection matrix. Calculation will occur only if it is necessary.

		// Update Tracking-World transformation matrix
		RefreshTrackingToWorldTransform(WorldContext);

		return true;
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	void FXRDisplaySystem::OnBeginRendering_RenderThread(FRDGBuilder& GraphBuilder, FSceneViewFamily& ViewFamily)
	{
		FHeadMountedDisplayBase::OnBeginRendering_RenderThread(GraphBuilder, ViewFamily);
		if (XRDisplay)
		{
			XRDisplay->OnBeginRendering_RenderThread(GraphBuilder.RHICmdList, ViewFamily);
		}
	}
#else
	void FXRDisplaySystem::OnBeginRendering_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily)
	{
		FHeadMountedDisplayBase::OnBeginRendering_RenderThread(RHICmdList, ViewFamily);
		if (XRDisplay)
		{
			XRDisplay->OnBeginRendering_RenderThread(RHICmdList, ViewFamily);
		}
	}
#endif

	float FXRDisplaySystem::GetWorldToMetersScale() const
	{
		return GWorld ? GWorld->GetWorldSettings()->WorldToMeters : 100.f;
	}

	bool FXRDisplaySystem::IsHMDConnected()
	{
		return true;
	}

	bool FXRDisplaySystem::GetHMDMonitorInfo(MonitorInfo& MonitorDesc)
	{
		static const FString MONITOR_NAME = TEXT("Oz XR Display");
		static const size_t MONITOR_ID = 0;
		static const int DESKTOP_X = 0;
		static const int DESKTOP_Y = 1;
		int resolution_div = 1;

		if (bQuitApplication)
		{
			return false;
		}

		if (isPerformancePriorityEnabled)
			resolution_div = 2;

		MonitorDesc.MonitorName = MONITOR_NAME;
		MonitorDesc.MonitorId = MONITOR_ID;
		MonitorDesc.DesktopX = DESKTOP_X;
		MonitorDesc.DesktopY = DESKTOP_Y;
		MonitorDesc.ResolutionX = DisplaySpec.display_resolution.width * 2 / resolution_div;
		MonitorDesc.ResolutionY = DisplaySpec.display_resolution.height / resolution_div;
		return true;
	}

	FIntPoint FXRDisplaySystem::GetIdealRenderTargetSize() const
	{
		HWND WindowHandle = (HWND)GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
		RECT WindowRect;
		GetWindowRect(WindowHandle, &WindowRect);

		return FIntPoint(WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top);
	}

	bool FXRDisplaySystem::IsStereoEnabled() const
	{
		return bStereoEnabled;
	}

	bool FXRDisplaySystem::EnableStereo(bool stereo)
	{
		if (stereo)
		{
			FSceneViewport* SceneViewport = nullptr;
			if (!bStereoEnabled && IsStereoRenderingAllowed(&SceneViewport))
			{
				bStereoEnabled = true;
				CreateWindowOnXRDisplay();
			}
		}
		else
		{
			DestroyWindowOnXRDisplay();
			bStereoEnabled = false;
		}

		return bStereoEnabled;
	}

#if ENGINE_MAJOR_VERSION == 5
	void FXRDisplaySystem::AdjustViewRect(const int32 ViewIndex, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
	{
		static const EStereoscopicEye RIGHT_EYE = EStereoscopicEye::eSSE_RIGHT_EYE;
		int32 StereoPass = ViewIndex;
#else // ENGINE_MAJOR_VERSION == 5
	void FXRDisplaySystem::AdjustViewRect(enum EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
	{
		static const EStereoscopicPass RIGHT_EYE = EStereoscopicPass::eSSP_RIGHT_EYE;
#endif // ENGINE_MAJOR_VERSION == 5
		SizeX = SizeX / 2;

		if (StereoPass == RIGHT_EYE)
		{
			X += SizeX;
		}
	}

#if ENGINE_MAJOR_VERSION == 5
	void FXRDisplaySystem::CalculateStereoViewOffset(
		const int32 ViewIndex, FRotator& ViewRotation, const float WorldToMeters, FVector& ViewLocation)
	{
		static const EStereoscopicEye LEFT_EYE = EStereoscopicEye::eSSE_LEFT_EYE;
		int32 StereoPassType = ViewIndex;

		if (StereoPassType == EStereoscopicEye::eSSE_MONOSCOPIC ||
			StereoPassType == EStereoscopicEye::eSSE_LEFT_EYE_SIDE ||
			StereoPassType == EStereoscopicEye::eSSE_RIGHT_EYE_SIDE) {
			return;
		}
#else // ENGINE_MAJOR_VERSION == 5
	void FXRDisplaySystem::CalculateStereoViewOffset(
		const enum EStereoscopicPass StereoPassType, FRotator& ViewRotation,
		const float WorldToMeters, FVector& ViewLocation)
	{
		static const EStereoscopicPass LEFT_EYE = EStereoscopicPass::eSSP_LEFT_EYE;

		if (StereoPassType == eSSP_FULL ||
			StereoPassType == eSSP_LEFT_EYE_SIDE ||
			StereoPassType == eSSP_RIGHT_EYE_SIDE) {
			return;
		}
#endif // ENGINE_MAJOR_VERSION == 5

		if (XrRuntimeWrapper.IsValid() == false)
		{
			return;
		}

		const SonyOzPosef& LeftEyePose = XrRuntimeWrapper->GetCachedPose(FXrRuntimePose::LEFT_EYE);
		const SonyOzPosef& RightEyePose = XrRuntimeWrapper->GetCachedPose(FXrRuntimePose::RIGHT_EYE);
		SonyOzPosef Pose = StereoPassType == LEFT_EYE ? LeftEyePose : RightEyePose;

		FQuat ViewOrientation;
		ConvertPoseFromXrRuntimeToGameWorld(Pose, ViewOrientation, ViewLocation);
		ViewRotation = ViewOrientation.Rotator();
	}

#if ENGINE_MAJOR_VERSION == 5
	FMatrix FXRDisplaySystem::GetStereoProjectionMatrix(const int32 ViewIndex) const
	{
		static const EStereoscopicEye LEFT_EYE = EStereoscopicEye::eSSE_LEFT_EYE;
		int32 StereoPassType = ViewIndex;
#else // ENGINE_MAJOR_VERSION == 5
	FMatrix FXRDisplaySystem::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const
	{
		static const EStereoscopicPass LEFT_EYE = EStereoscopicPass::eSSP_LEFT_EYE;
#endif // ENGINE_MAJOR_VERSION == 5
		if (StereoPassType == LEFT_EYE)
		{
			return LeftViewProjectionMatrix;
		}
		else
		{
			return RightViewProjectionMatrix;
		}
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	void FXRDisplaySystem::CompositeRenderTexture(FRHICommandListImmediate& RHICmdList, FRHITexture* BackBuffer, FRHITexture* SrcTexture, FVector2D WindowSize) const
	{
		if (!FXRDisplayWindow::GetPlayWindow())
		{
			return;
		}

		uint32 SourceWidth = SrcTexture->GetSizeX();
		uint32 SourceHeight = SrcTexture->GetSizeY();

		EPixelFormat PixelFormat = BackBuffer->GetFormat();
		DXGI_FORMAT Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		switch (PixelFormat) {
		case EPixelFormat::PF_B8G8R8A8:
			Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			break;
		default:
			Format = DXGI_FORMAT_R10G10B10A2_UNORM;
			break;
		}

		if (!isPerformancePriorityEnabled && RenderTexture == nullptr) {
			const FRHITextureCreateDesc StagingDesc =
				FRHITextureCreateDesc::Create2D(TEXT("DisplayProjectorTempTexture"), BackBuffer->GetSizeX(), BackBuffer->GetSizeY(), BackBuffer->GetFormat())
				.SetFlags(TexCreate_None | TexCreate_ShaderResource | TexCreate_RenderTargetable);
			RenderTexture = RHICreateTexture(StagingDesc);
		}

		if (XRDisplay)
		{
			if (!isPerformancePriorityEnabled)
			{
				XRDisplay->RenderTexture_RenderThread(RHICmdList, RenderTexture, SrcTexture, WindowSize, isLensShiftEnabled);
			}
			else
			{
				XRDisplay->BroadcastSrdRenderTextureCompletedDelegate(RHICmdList, SrcTexture);
			}
		}

		// Composite stereo images
		{
			FRHIRenderPassInfo RPInfoTemp(
				FXRDisplayWindow::GetPlayWindow()->GetRenderTarget()->GetRenderTargetResource()->GetRenderTargetTexture(),
				ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(RPInfoTemp, TEXT("FSRDisplaySystem_RenderTexture_RenderThread"));
			{
				// RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);  // TODO: When it turns out to be necessary, release comment out
				if (isPerformancePriorityEnabled)
				{
					CompositeStereoImages(RHICmdList,
						FXRDisplayWindow::GetPlayWindow()->GetRenderTarget()->GetRenderTargetResource()->GetRenderTargetTexture(),
						SrcTexture, SourceWidth, SourceHeight, Format);
				}
				else
				{
					CompositeStereoImages(RHICmdList,
						FXRDisplayWindow::GetPlayWindow()->GetRenderTarget()->GetRenderTargetResource()->GetRenderTargetTexture(),
						RenderTexture, SourceWidth, SourceHeight, Format);
				}
			}
			RHICmdList.EndRenderPass();
		}
	}

#if ENGINE_MINOR_VERSION >= 6
	void FXRDisplaySystem::RenderTexture_RenderThread(FRDGBuilder& GraphBuilder, FRDGTextureRef BackBuffer, FRDGTextureRef SrcTexture, FVector2f WindowSize) const
	{
		if (GetCurrentGraphicsAPI().Find("D3D11RHI") != INDEX_NONE)
		{
			FCopyToStagingParameters* PassParameters = GraphBuilder.AllocParameters<FCopyToStagingParameters>();
			PassParameters->Input = SrcTexture;
			PassParameters->Output = BackBuffer;
			GraphBuilder.AddPass(RDG_EVENT_NAME("SRDisplay Plugin"), PassParameters, ERDGPassFlags::Copy,
				[BackBuffer, SrcTexture, WindowSize, this](FRHICommandListImmediate& RHICmdList)
				{
					CompositeRenderTexture(RHICmdList, BackBuffer->GetRHI(), SrcTexture->GetRHI(), (FVector2D)WindowSize.RoundToVector());
				});
		}
		else
		{
			CompositeRenderTexture(GraphBuilder.RHICmdList, BackBuffer->GetRHI(), SrcTexture->GetRHI(), (FVector2D)WindowSize.RoundToVector());
		}

	}
#else
	void FXRDisplaySystem::RenderTexture_RenderThread(class FRHICommandListImmediate& RHICmdList, class FRHITexture* BackBuffer, class FRHITexture* SrcTexture, FVector2D WindowSize) const
	{
		CompositeRenderTexture(RHICmdList, BackBuffer, SrcTexture, WindowSize);
#if ENGINE_MINOR_VERSION < 5
		FXRDisplayWindow::GetPlayWindow()->RenderTexture_RenderThread(RHICmdList);
#endif
	}
#endif
#else
	void FXRDisplaySystem::RenderTexture_RenderThread(class FRHICommandListImmediate& RHICmdList, class FRHITexture2D* BackBuffer, class FRHITexture2D* SrcTexture, FVector2D WindowSize) const
	{
		if (!FXRDisplayWindow::GetPlayWindow())
		{
			return;
		}

		uint32 SourceWidth = SrcTexture->GetSizeX();
		uint32 SourceHeight = SrcTexture->GetSizeY();

		EPixelFormat PixelFormat = BackBuffer->GetFormat();
		DXGI_FORMAT Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		switch (PixelFormat) {
		case EPixelFormat::PF_B8G8R8A8:
			Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			break;
		default:
			Format = DXGI_FORMAT_R10G10B10A2_UNORM;
			break;
		}

		if (!isPerformancePriorityEnabled && RenderTexture == nullptr) {

#if ENGINE_MAJOR_VERSION == 5
			FRHIResourceCreateInfo TmpInfo(TEXT("DisplayProjectorTempTexture"));
#else
			FRHIResourceCreateInfo TmpInfo;
#endif
			RenderTexture = RHICreateTexture2D(
				BackBuffer->GetSizeX(), BackBuffer->GetSizeY(), BackBuffer->GetFormat(), 1, 1,
				TexCreate_None | TexCreate_ShaderResource | TexCreate_RenderTargetable, TmpInfo);
		}

		if (XRDisplay)
		{
			if (!isPerformancePriorityEnabled)
			{
				XRDisplay->RenderTexture_RenderThread(RHICmdList, RenderTexture, SrcTexture, WindowSize, isLensShiftEnabled);
			}
			else
			{
				XRDisplay->BroadcastSrdRenderTextureCompletedDelegate(RHICmdList, SrcTexture);
			}
		}

		// Composite stereo images
		{
			FRHIRenderPassInfo RPInfoTemp(
				FXRDisplayWindow::GetPlayWindow()->GetRenderTarget()->GetRenderTargetResource()->GetRenderTargetTexture(),
				ERenderTargetActions::Load_Store);
			RHICmdList.BeginRenderPass(RPInfoTemp, TEXT("FSRDisplaySystem_RenderTexture_RenderThread"));
			{
				// RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThreadFlushResources);  // TODO: When it turns out to be necessary, release comment out
				if (isPerformancePriorityEnabled)
				{
					CompositeStereoImages(RHICmdList,
								FXRDisplayWindow::GetPlayWindow()->GetRenderTarget()->GetRenderTargetResource()->GetRenderTargetTexture(),
								SrcTexture, SourceWidth, SourceHeight, Format);
				}
				else
				{
					CompositeStereoImages(RHICmdList,
								FXRDisplayWindow::GetPlayWindow()->GetRenderTarget()->GetRenderTargetResource()->GetRenderTargetTexture(),
								RenderTexture, SourceWidth, SourceHeight, Format);
				}
			}
			RHICmdList.EndRenderPass();

			FXRDisplayWindow::GetPlayWindow()->RenderTexture_RenderThread(RHICmdList);
		}
	}
#endif

	IStereoRenderTargetManager* FXRDisplaySystem::GetRenderTargetManager()
	{
		if (RenderTargetManager == nullptr)
		{
			RenderTargetManager = new FRenderTargetManager();
		}

		return RenderTargetManager;
	}

	void FXRDisplaySystem::SetupViewFamily(FSceneViewFamily& InViewFamily)
	{
		InViewFamily.EngineShowFlags.MotionBlur = 0;
		InViewFamily.EngineShowFlags.HMDDistortion = false;
		InViewFamily.EngineShowFlags.ScreenPercentage = 1.f;
		InViewFamily.EngineShowFlags.StereoRendering = IsStereoEnabled();
	}

	void FXRDisplaySystem::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
	{
		InView.BaseHmdOrientation = FQuat(FRotator(0.f, 0.f, 0.f));
		InView.BaseHmdLocation = FVector(0.f);
	}

	void FXRDisplaySystem::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
	{
	}

#if ENGINE_MAJOR_VERSION == 4 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6)
	void FXRDisplaySystem::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily)
	{
	}

	void FXRDisplaySystem::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
	{
	}
#endif

	bool FXRDisplaySystem::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
	{
		return GEngine && GEngine->IsStereoscopic3D(Context.Viewport);
	}
	/***********************************************************************************/
	/* SR Display APIs.                                                                */
	/***********************************************************************************/
	bool FXRDisplaySystem::GetMousePosition(float& LocationX, float& LocationY, const bool bLocalPosition)
	{
		bool bGotMousePosition = false;

		if (FXRDisplayWindow::GetPlayWindow())
		{
			FVector2D MousePosition;

			bGotMousePosition = FXRDisplayWindow::GetPlayWindow()->GetMousePosition(MousePosition, bLocalPosition);

			if (bGotMousePosition)
			{
				LocationX = MousePosition.X;
				LocationY = MousePosition.Y;
			}
		}

		return bGotMousePosition;
	}

	bool FXRDisplaySystem::GetSRDTopLeftCornerWorldPosition(FVector2D& SRDTopLeftCornerWorldPosition)
	{
		if (FXRDisplayWindow::GetPlayWindow())
		{
			return FXRDisplayWindow::GetPlayWindow()->GetSRDTopLeftCornerWorldPosition(SRDTopLeftCornerWorldPosition);
		}
		return false;
	}

	bool FXRDisplaySystem::DeprojectScreenToWorld(APlayerController const* Player, const FVector2D& ScreenPosition, FVector& WorldPosition, FVector& WorldDirection, FVector& CameraPosition)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		FSceneViewProjectionData ProjectionData;
		if (CalculateProjectionData(EStereoscopicEye::eSSE_LEFT_EYE, ProjectionData))
		{
			if (XRDisplay)
			{
				return XRDisplay->DeprojectScreenToWorld(ProjectionData, ScreenPosition, WorldPosition, WorldDirection, CameraPosition);
			}
		}
#else
		ULocalPlayer* const LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
		if (LocalPlayer && FXRDisplayWindow::GetPlayWindow() && FXRDisplayWindow::GetPlayWindow()->GetViewport())
		{
			// get the projection data
			FSceneViewProjectionData ProjectionData;
#if ENGINE_MAJOR_VERSION == 5
			if (LocalPlayer->GetProjectionData(FXRDisplayWindow::GetPlayWindow()->GetViewport(),  /*out*/ ProjectionData, EStereoscopicEye::eSSE_LEFT_EYE))
#else // ENGINE_MAJOR_VERSION == 5
			if (LocalPlayer->GetProjectionData(FXRDisplayWindow::GetPlayWindow()->GetViewport(), eSSP_LEFT_EYE, /*out*/ ProjectionData))
#endif // ENGINE_MAJOR_VERSION == 5
			{
				if (XRDisplay)
				{
					return XRDisplay->DeprojectScreenToWorld(ProjectionData, ScreenPosition, WorldPosition, WorldDirection, CameraPosition);
				}
			}
		}
#endif

		// something went wrong, zero things and return false
		WorldPosition = FVector::ZeroVector;
		WorldDirection = FVector::ZeroVector;
		return false;
	}

	void FXRDisplaySystem::SetSRRenderingActive(bool isActive)
	{
		XrRuntimeWrapper->EnableStereo(isActive);
	}

	bool FXRDisplaySystem::IsSRDisplayConnected()
	{
		SonyOzPlatformId PlatformId = XRDisplay->GetPlatformId();
		uint64_t Size = 0;
		if (XrRuntimeWrapper)
		{
			if (XrRuntimeWrapper->GetDeviceNum(PlatformId, &Size) != SonyOzResult::SUCCESS || Size <= 1)
			{
				return false;
			}
		}
		return true;
	}

	bool FXRDisplaySystem::GetEyeLocationAndRotation(FVector& LeftLocation, FRotator& LeftRotation, FVector& RightLocation, FRotator& RightRotation)
	{
		if (XrRuntimeWrapper.IsValid() == false)
		{
			return false;
		}
		FQuat Orientation;
		const SonyOzPosef& LeftEyePose = XrRuntimeWrapper->GetCachedPose(FXrRuntimePose::LEFT_EYE);
		ConvertPoseFromXrRuntimeToGameWorld(LeftEyePose, Orientation, LeftLocation);
		LeftRotation = Orientation.Rotator();
		const SonyOzPosef& RightEyePose = XrRuntimeWrapper->GetCachedPose(FXrRuntimePose::RIGHT_EYE);
		ConvertPoseFromXrRuntimeToGameWorld(RightEyePose, Orientation, RightLocation);
		RightRotation = Orientation.Rotator();

		return true;
	}

	bool FXRDisplaySystem::IsXRDisplayDetectedFace()
	{
		if(XrRuntimeWrapper){
			return XrRuntimeWrapper->IsDetectedFace();
		}
		return false;
	}

	/***********************************************************************************/
	/* Private subroutines.                                                            */
	/***********************************************************************************/
	bool FXRDisplaySystem::IsStereoRenderingAllowed(FSceneViewport** OutSceneViewport)
	{
		if (!GIsEditor)
		{
			UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
			if (GameEngine != nullptr && GameEngine->SceneViewport.IsValid())
			{
				*OutSceneViewport = GameEngine->SceneViewport.Get();
				return GameEngine->SceneViewport.Get()->IsStereoRenderingAllowed();
			}
		}
#if WITH_EDITOR
		else
		{
			// Return true if automation test
			if (GIsAutomationTesting)
			{
				*OutSceneViewport = nullptr;
				return true;
			}

			UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
			FSceneViewport* PIEViewport = (FSceneViewport*)EditorEngine->GetPIEViewport();
			if (PIEViewport != nullptr)
			{
				*OutSceneViewport = PIEViewport;
				return PIEViewport->IsStereoRenderingAllowed();
			}
			else
			{
				// Check to see if the active editor viewport is drawing in stereo mode
				// @todo VR Editor: Should work with even non-active viewport!
				FSceneViewport* EditorViewport = (FSceneViewport*)EditorEngine->GetActiveViewport();
				if (EditorViewport != nullptr)
				{
					*OutSceneViewport = EditorViewport;
					return EditorViewport->IsStereoRenderingAllowed();
				}
			}
		}
#endif
		*OutSceneViewport = nullptr;
		return false;
	}

	bool FXRDisplaySystem::IsProjectPlayModeSupported()
	{
#if WITH_EDITOR
		if (GWorld->GetWorld()->IsPlayInMobilePreview())
		{
			if (bQuitApplication)
			{
				return false;
			}

			std::wstring error_message = SRD_ERROR_MESSAGE_UE022 + L"\n" + SRD_ERROR_MESSAGE_C017;
			UE_LOG(LogXRDisplay, Error, TEXT("%s"), error_message.c_str());
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, error_message.c_str(), *(this->GetSystemName().ToString()));
			return false;
		}
#endif
		return true;
	}

	bool FXRDisplaySystem::IsProjectGraphicsAPISupported()
	{
		FString RHIModuleName = GetSelectedDynamicRHIModuleName();
		if (RHIModuleName.Find("D3D11RHI") != INDEX_NONE)
		{
			return true;
		}
		if (RHIModuleName.Find("D3D12RHI") != INDEX_NONE)
		{
			return true;
		}

		if (bQuitApplication)
		{
			return false;
		}

		std::wstring error_message = SRD_ERROR_MESSAGE_UE023 + L"\n" + SRD_ERROR_MESSAGE_C017;
		UE_LOG(LogXRDisplay, Error, TEXT("%s"), error_message.c_str());
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, error_message.c_str(), *(this->GetSystemName().ToString()));

		UE_LOG(LogXRDisplay, Error, TEXT("RHIModule used in this module is %s"), *RHIModuleName);
		return false;
	}

	FString FXRDisplaySystem::GetCurrentGraphicsAPI() const
	{
		return GetSelectedDynamicRHIModuleName();
	}

	void FXRDisplaySystem::UpdateProjectionMatrix()
	{
		if (XrRuntimeWrapper.IsValid() == false)
		{
			return;
		}

		const float DEFAULT_NEAR_Z = 0.15f;
		const float DEFAULT_FAR_Z = 1000000.f;

		auto GetFrustum = [](auto Left, auto Right, auto Bottom, auto Top, auto nearZ, auto farZ) {
			float M00 = 2.f * nearZ / (Left - Right);
			float M11 = 2.f * nearZ / (Top - Bottom);
			float M20 = (Left + Right) / (Left - Right);
			float M21 = (Top + Bottom) / (Top - Bottom);
			float M22 = farZ / (nearZ - farZ);
			float M23 = -1.f;
			float M32 = nearZ * farZ / (nearZ - farZ);

			FMatrix ReverseZ(
				FPlane(1.f, 0.f, 0.f, 0.f),
				FPlane(0.f, 1.f, 0.f, 0.f),
				FPlane(0.f, 0.f, -1.f, 0.f),
				FPlane(0.f, 0.f, 1.f, 1.f));

			const float ToCenti = 100.f;
			// const FPlane RightHandedSystem(M20, M21, M22, M23);
			const FPlane ToLeftHandedSystem(-M20, -M21, -M22, -M23);

			return FMatrix(
				FPlane(M00, 0.f, 0.f, 0.f),
				FPlane(0.f, M11, 0.f, 0.f),
				ToLeftHandedSystem,
				FPlane(0.f, 0.f, M32 * ToCenti, 0.f)
			) * ReverseZ;
		};

		float Near = DEFAULT_NEAR_Z;
		float Far = DEFAULT_FAR_Z;
		if (XRDisplay)
		{
			Near = XRDisplay->GetNearClip();
			Far = XRDisplay->GetFarClip();
		}
		{
			const FHalfAngleList HalfAngleList = XrRuntimeWrapper->GetHalfAngles(FXrRuntimePose::LEFT_EYE);

			const float Left = Near * tanf(HalfAngleList.half_angles_left);
			const float Right = Near * tanf(HalfAngleList.half_angles_right);
			const float Top = Near * tanf(HalfAngleList.half_angles_top);
			const float Bottom = Near * tanf(HalfAngleList.half_angles_bottom);
			// Swap Right and Left, because XrRuntimeWrapper::GetHalfAngles is unity vector(-x, y, z)
			LeftViewProjectionMatrix = GetFrustum(Right, Left, Bottom, Top, Near, Far);
		}
		{
			const FHalfAngleList HalfAngleList = XrRuntimeWrapper->GetHalfAngles(FXrRuntimePose::RIGHT_EYE);
			const float Left = Near * tanf(HalfAngleList.half_angles_left);
			const float Right = Near * tanf(HalfAngleList.half_angles_right);
			const float Top = Near * tanf(HalfAngleList.half_angles_top);
			const float Bottom = Near * tanf(HalfAngleList.half_angles_bottom);
			RightViewProjectionMatrix = GetFrustum(Right, Left, Bottom, Top, Near, Far);
		}

		if (XRDisplay)
		{
			XRDisplay->UpdateProjectionMatrix(LeftViewProjectionMatrix, RightViewProjectionMatrix);
		}
	}

	FMatrix FXRDisplaySystem::GetSpatialClippedProjectionMatrix(FVector ClipPlanePositionInWorldCoord, FVector ClipPlaneNormalVecInWorldCoord, FMatrix ViewMatrix, FMatrix ProjectionMatrix)
	{
		// Caluculate clipping plane(view)
		FVector NormalInCameraCoord = ViewMatrix.TransformVector(ClipPlaneNormalVecInWorldCoord);
		NormalInCameraCoord.Normalize();
		FVector PosInCameraCoord = ViewMatrix.TransformPosition(ClipPlanePositionInWorldCoord);
		FVector4 ClipPlane = FVector4(NormalInCameraCoord.X, NormalInCameraCoord.Y, NormalInCameraCoord.Z,
			-FVector::DotProduct(PosInCameraCoord, NormalInCameraCoord));

		return GetNearClipPlaneConvertedProjectionMatrix(ProjectionMatrix, ClipPlane);
	}

	float sgn(float a)
	{
		if (a > 0.f) { return (1.f); }
		if (a < 0.f) { return (-1.f); }
		return (0.f);
	}

	FMatrix FXRDisplaySystem::GetNearClipPlaneConvertedProjectionMatrix(FMatrix SrcProjMat, FVector4 PlaneInViewSpace)
	{
		FMatrix ZReverseMatrix;
		ZReverseMatrix.SetIdentity();
		ZReverseMatrix.M[2][2] = -1.f;
		ZReverseMatrix.M[3][2] = 1.f;

		FMatrix Test = ZReverseMatrix * ZReverseMatrix;
		FMatrix UnZReversedProjMat = SrcProjMat * ZReverseMatrix;

		FVector4 CornerPlane(
			(sgn(PlaneInViewSpace.X) - UnZReversedProjMat.M[2][0]) / UnZReversedProjMat.M[0][0],
			(sgn(PlaneInViewSpace.Y) - UnZReversedProjMat.M[2][1]) / UnZReversedProjMat.M[1][1],
			1.f,
			(1.f - UnZReversedProjMat.M[2][2]) / UnZReversedProjMat.M[3][2]
		);

		FVector4 ProjPlane(PlaneInViewSpace * (1.f / Dot4(PlaneInViewSpace, CornerPlane)));

		UnZReversedProjMat.M[0][2] = ProjPlane.X;
		UnZReversedProjMat.M[1][2] = ProjPlane.Y;
		UnZReversedProjMat.M[2][2] = ProjPlane.Z;
		UnZReversedProjMat.M[3][2] = ProjPlane.W;

		return UnZReversedProjMat * ZReverseMatrix;
	}

	FMatrix FXRDisplaySystem::GetDefaultProjectionMatrix() const
	{
		// XR tracking session has not established yet. Use defalut projection matrix.
		const float DEFAULT_NEAR_Z = 15.f;
		const float DEFAULT_FAR_Z = 1000000.f;
		const float DEFAULT_FOV = 30.f;

		// Convert from degrees to radian.
		const float HalfFov = DEFAULT_FOV / 180.f * 3.14f;
		const int ViewportWidth = DisplaySpec.display_resolution.width * 2;
		const int ViewportHeight = DisplaySpec.display_resolution.height;

		// Translate into DirectX coordinate system (Z-forward).
		const float M0_0 = ViewportHeight / tan(HalfFov / 2.f) / (ViewportWidth / 2.f);
		const float M1_1 = 1.f / tan(HalfFov / 2.f);
		const float M2_2 = DEFAULT_NEAR_Z / (DEFAULT_NEAR_Z - DEFAULT_FAR_Z);
		const float M2_3 = 1.f;
		const float M3_2 = (-DEFAULT_NEAR_Z * DEFAULT_FAR_Z) / (DEFAULT_NEAR_Z - DEFAULT_FAR_Z);

		const FMatrix DefaultProjectionMatrix(
			FPlane(M0_0, 0.f, 0.f, 0.f),
			FPlane(0.f, M1_1, 0.f, 0.f),
			FPlane(0.f, 0.f, M2_2, M2_3),
			FPlane(0.f, 0.f, M3_2, 0.f)
		);

		return DefaultProjectionMatrix;
	}

	bool FXRDisplaySystem::GetViewMatrix(FMatrix& LeftViewMatrix, FMatrix& RightViewMatrix)
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		FSceneViewProjectionData LeftProjectionData;
		FSceneViewProjectionData RightProjectionData;
		if (CalculateProjectionData(EStereoscopicEye::eSSE_LEFT_EYE, LeftProjectionData)
			&& CalculateProjectionData(EStereoscopicEye::eSSE_RIGHT_EYE, RightProjectionData))
		{
			LeftViewMatrix = FTranslationMatrix(-LeftProjectionData.ViewOrigin) * LeftProjectionData.ViewRotationMatrix;
			RightViewMatrix = FTranslationMatrix(-RightProjectionData.ViewOrigin) * RightProjectionData.ViewRotationMatrix;

			return true;
		}
#else
		if (GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWorld())
		{
			APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GEngine->GameViewport->GetWorld(), 0);
			ULocalPlayer* const LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
			if (LocalPlayer && FXRDisplayWindow::GetPlayWindow() && FXRDisplayWindow::GetPlayWindow()->GetViewport())
			{
				FSceneViewProjectionData LeftProjectionData;
				FSceneViewProjectionData RightProjectionData;
#if ENGINE_MAJOR_VERSION == 5
				if (LocalPlayer->GetProjectionData(FXRDisplayWindow::GetPlayWindow()->GetViewport(), /*out*/ LeftProjectionData, EStereoscopicEye::eSSE_LEFT_EYE)
					&& LocalPlayer->GetProjectionData(FXRDisplayWindow::GetPlayWindow()->GetViewport(), /*out*/ RightProjectionData, EStereoscopicEye::eSSE_RIGHT_EYE))
#else // ENGINE_MAJOR_VERSION == 5
				if (LocalPlayer->GetProjectionData(FXRDisplayWindow::GetPlayWindow()->GetViewport(), eSSP_LEFT_EYE, /*out*/ LeftProjectionData)
					&& LocalPlayer->GetProjectionData(FXRDisplayWindow::GetPlayWindow()->GetViewport(), eSSP_RIGHT_EYE, /*out*/ RightProjectionData))
#endif // ENGINE_MAJOR_VERSION == 5
				{
					LeftViewMatrix = FTranslationMatrix(-LeftProjectionData.ViewOrigin) * LeftProjectionData.ViewRotationMatrix;
					RightViewMatrix = FTranslationMatrix(-RightProjectionData.ViewOrigin) * RightProjectionData.ViewRotationMatrix;

					return true;
				}
			}
		}
#endif
		return false;
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	bool FXRDisplaySystem::CalculateProjectionData(int32 StereoViewIndex, FSceneViewProjectionData& ProjectionData)
	{
		int32 X, Y;
		uint32 SizeX, SizeY;
		AdjustViewRect(StereoViewIndex, X, Y, SizeX, SizeY);

		FRotator rotator;
		FVector location;
		if (GEngine && GEngine->GameViewport && GEngine->GameViewport->GetWorld() && GEngine->GameViewport->GetWorld()->GetWorldSettings())
		{
			CalculateStereoViewOffset(StereoViewIndex, rotator, GEngine->GameViewport->GetWorld()->GetWorldSettings()->WorldToMeters, location);
		}
		else
		{
			return false;
		}

		ProjectionData.ViewOrigin = location;
		ProjectionData.ViewRotationMatrix = FInverseRotationMatrix(rotator) * FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1));

		ProjectionData.ProjectionMatrix = GetStereoProjectionMatrix(StereoViewIndex);
		ProjectionData.SetViewRectangle(FIntRect(X, Y, X + SizeX, Y + SizeY));

		return true;
	}
#endif

	void FXRDisplaySystem::CreateWindowOnXRDisplay()
	{

		if (XrRuntimeWrapper)
		{
			const SonyOzRect& TargetMonitorRectangle =
				XrRuntimeWrapper->GetTargetMonitorRectangle();
			const FIntRect ScreenPosition(TargetMonitorRectangle.left, TargetMonitorRectangle.top,
				TargetMonitorRectangle.right, TargetMonitorRectangle.bottom);

			FXRDisplayWindow::Initialize(
				FVector2D(DisplaySpec.display_resolution.width, DisplaySpec.display_resolution.height),
				ScreenPosition);
		}
	}

	void FXRDisplaySystem::DestroyWindowOnXRDisplay()
	{
		FXRDisplayWindow::Finalize();
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	void FXRDisplaySystem::CompositeStereoImages(FRHICommandListImmediate& RHICmdList, FRHITexture* BackBuffer, FRHITexture* SrcSideBySide, uint32 SourceWidth, uint32 SourceHeight, DXGI_FORMAT Format) const
#else
	void FXRDisplaySystem::CompositeStereoImages(FRHICommandListImmediate& RHICmdList, FRHITexture2D* BackBuffer, FRHITexture2D* SrcSideBySide, uint32 SourceWidth, uint32 SourceHeight, DXGI_FORMAT Format) const
#endif
	{
		if (XrRuntimeWrapper.IsValid() == false)
		{
			return;
		}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
		XrRuntimeWrapper->SetColorSpace(1, 0, 2.2f);
#else
		XrRuntimeWrapper->SetColorSpace(1, 1, 2.2f);
#endif
		if (GetCurrentGraphicsAPI().Find("D3D12RHI") != INDEX_NONE) {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			ID3D12DynamicRHI* D3D12RHI = GetID3D12DynamicRHI();
			void* CommandQueue = D3D12RHI->RHIGetCommandQueue();
			if (CommandQueue == nullptr) {
				return;
			}
			ID3D12Device* Device = D3D12RHI->RHIGetDevice(0);
			const uint32 DeviceNodeMask = D3D12RHI->RHIGetDeviceNodeMask(0);
			XrRuntimeWrapper->SubmitD3d12(
				reinterpret_cast<ID3D12CommandQueue*>(CommandQueue),
				DeviceNodeMask,
				reinterpret_cast<ID3D12Resource*>(
					SrcSideBySide->GetNativeResource()),
				Format,
				reinterpret_cast<ID3D12Resource*>(
					BackBuffer->GetNativeResource())
				);
#else // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			FD3D12DynamicRHI* D3D12RHI = FD3D12DynamicRHI::GetD3DRHI();
			void* CommandQueue = D3D12RHI->RHIGetD3DCommandQueue();
			if (CommandQueue == nullptr) {
				return;
			}
			FD3D12Device* Device = D3D12RHI->GetAdapter().GetDevice(0);
			XrRuntimeWrapper->SubmitD3d12(
				reinterpret_cast<ID3D12CommandQueue*>(CommandQueue),
				Device->GetGPUMask().GetNative(),
				reinterpret_cast<ID3D12Resource*>(
					SrcSideBySide->GetNativeResource()),
				Format,
				reinterpret_cast<ID3D12Resource*>(
					BackBuffer->GetNativeResource()));
#endif // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
		}
		else
		{
			void* NativeDevice = RHIGetNativeDevice();
			if (NativeDevice == nullptr)
			{
				return;
			}
			XrRuntimeWrapper->SubmitD3d11(
				reinterpret_cast<ID3D11Device*>(NativeDevice),
				reinterpret_cast<ID3D11Texture2D*>(
					SrcSideBySide->GetNativeResource()),
				reinterpret_cast<ID3D11Texture2D*>(
					BackBuffer->GetNativeResource()));
		}
	}

	bool FXRDisplaySystem::HandleXRRuntimeResult(SonyOzResult Result)
	{
		if (Result != SonyOzResult::SUCCESS) {
#if WITH_EDITOR
			UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine);
			if (EditorEngine) {
				FSceneViewport* EditorViewport = (FSceneViewport*)EditorEngine->GetActiveViewport();
				if (EditorViewport == nullptr) {
					return false;
				}
			}
#endif
			std::wstring error_message = SRD_ERROR_MESSAGE_C016 + L"\n" + SRD_ERROR_MESSAGE_C017;
			switch (Result) {
			case SonyOzResult::ERROR_RUNTIME_NOT_FOUND:
				error_message = SRD_ERROR_MESSAGE_C022 + L"\n" + SRD_ERROR_MESSAGE_C017;
				break;
			case SonyOzResult::ERROR_VALIDATION_FAILURE:
				break;
			case SonyOzResult::ERROR_RUNTIME_FAILURE:
				error_message = SRD_ERROR_MESSAGE_C022 + L"\n" + SRD_ERROR_MESSAGE_C017;
				break;
			case SonyOzResult::ERROR_FUNCTION_UNSUPPORTED:
				error_message = L"Error:function unsupported";
				break;
			case SonyOzResult::ERROR_HANDLE_INVALID:
				error_message = L"Error:handle invalid";
				break;
			case SonyOzResult::ERROR_SESSION_CREATED:
				error_message = L"Error:session created";
				break;
			case SonyOzResult::ERROR_SESSION_READY:
				error_message = L"Error:session ready";
				break;
			case SonyOzResult::ERROR_SESSION_STARTING:
				error_message = L"Error:session starting";
				break;
			case SonyOzResult::ERROR_SESSION_RUNNING:
				error_message = L"Error:session running";
				break;
			case SonyOzResult::ERROR_SESSION_STOPPING:
				error_message = L"Error:session stopping";
				break;
			case SonyOzResult::ERROR_SESSION_NOT_CREATE:
				error_message = L"Error: session not created";
				break;
			case SonyOzResult::ERROR_SESSION_NOT_READY:
				error_message = L"Error: session not ready";
				break;
			case SonyOzResult::ERROR_SESSION_NOT_RUNNING:
				error_message = L"Error:session not running";
				break;
			case SonyOzResult::ERROR_SESSION_STILL_USED:
				error_message = L"Error:session still used";
				break;
			case SonyOzResult::ERROR_POSE_INVALID:
				error_message = L"Error:pose invalid";
				break;
			case SonyOzResult::ERROR_SET_DATA_FAILURE:
				error_message = L"Error:set data failure";
				break;
			case SonyOzResult::ERROR_FILE_ACCESS_ERROR:
				error_message = L"Error:file access error";
				break;
			case SonyOzResult::ERROR_DEVICE_NOT_FOUND:
				error_message = SRD_ERROR_MESSAGE_C018 + L"\n" + SRD_ERROR_MESSAGE_C017;
				break;
			case SonyOzResult::ERROR_RUNTIME_UNSUPPORTED:
				error_message = SRD_ERROR_MESSAGE_C036 + L"\n" + SRD_ERROR_MESSAGE_C017;
				break;
			default:
				break;
			}
			std::wstring dialog_title = L"SRDisplay";
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, error_message.c_str(), dialog_title.c_str());

			return false;
		}
		return true;

	}

} // namespace xr_display
