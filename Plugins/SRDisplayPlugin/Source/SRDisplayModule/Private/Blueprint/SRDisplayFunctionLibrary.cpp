/**
 * Copyright 2019,2020,2021,2022,2023 Sony Corporation
 */
#include "Blueprint/SRDisplayFunctionLibrary.h"
#include "SRDisplaySystem.h"

USRDisplayFunctionLibrary::USRDisplayFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

static const FName XRDisplaySystemName(TEXT("Sony XR Display"));
xr_display::FXRDisplaySystem* USRDisplayFunctionLibrary::GetXRDisplaySystem()
{
	if (GEngine && GEngine->XRSystem.IsValid()) {
		if (GEngine->XRSystem->GetSystemName() == XRDisplaySystemName) {
			return static_cast<xr_display::FXRDisplaySystem*>(GEngine->XRSystem.Get());
		}
	}
	return nullptr;
}

srdisplay_module::FSRDisplaySystem* USRDisplayFunctionLibrary::GetSRDisplaySystem()
{
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem) {
		return static_cast<srdisplay_module::FSRDisplaySystem*>(XRDisplaySystem->GetXRDisplay());
	}
	return nullptr;
}

bool USRDisplayFunctionLibrary::IsSRDisplayConnected()
{
#if PLATFORM_WINDOWS
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem) {
		return XRDisplaySystem->IsSRDisplayConnected();
	}
#endif // PLATFORM_WINDOWS
	return false;
}

bool USRDisplayFunctionLibrary::GetConnectedSRDisplayName(EDeviceName& DeviceName)
{
#if PLATFORM_WINDOWS
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem)
	{
		if(!XRDisplaySystem->IsSRDisplayConnected())
		{
			DeviceName = EDeviceName::NONE;
			return true;
		}
	}
	srdisplay_module::FSRDisplaySystem* SRDisplaySystem = GetSRDisplaySystem();
	if (SRDisplaySystem)
	{
		if (SRDisplaySystem->GetConnectedDevice() == sony::oz::xr_runtime::SupportDevice::ELF_SR2)
		{
			DeviceName = EDeviceName::SR2_SIZE;
		}
		else
		{
			DeviceName = EDeviceName::SR1_SIZE;
		}
		return true;
	}
#endif // PLATFORM_WINDOWS
	return false;
}

bool USRDisplayFunctionLibrary::GetMousePosition(float& LocationX, float& LocationY, const bool bLocalPosition)
{
#if PLATFORM_WINDOWS
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem)
	{
		return XRDisplaySystem->GetMousePosition(LocationX, LocationY, bLocalPosition);
	}
#endif // PLATFORM_WINDOWS

	LocationX = 0.f;
	LocationY = 0.f;
	return false;
}

bool USRDisplayFunctionLibrary::GetSRDTopLeftCornerWorldPosition(FVector2D& SRDTopLeftCornerWorldPosition)
{
#if PLATFORM_WINDOWS
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem) {
		return XRDisplaySystem->GetSRDTopLeftCornerWorldPosition(SRDTopLeftCornerWorldPosition);
	}
#endif // PLATFORM_WINDOWS

	return false;
}

bool USRDisplayFunctionLibrary::DeprojectScreenToWorld(APlayerController const* Player, const FVector2D& ScreenPosition, FVector& WorldPosition, FVector& WorldDirection, FVector& CameraPosition)
{
#if PLATFORM_WINDOWS
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem)
	{
		return XRDisplaySystem->DeprojectScreenToWorld(Player, ScreenPosition, WorldPosition, WorldDirection, CameraPosition);
	}
#endif // PLATFORM_WINDOWS

	// something went wrong, zero things and return false
	WorldPosition = FVector::ZeroVector;
	WorldDirection = FVector::ZeroVector;
	return false;
}

bool USRDisplayFunctionLibrary::ConvertMouseLocationToWorldSpace(APlayerController const* Player, FVector& WorldPosition, FVector& WorldDirection, FVector& CameraPosition)
{
#if PLATFORM_WINDOWS
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem)
	{
		float LocationX = 0.f;
		float LocationY = 0.f;
		if (XRDisplaySystem->GetMousePosition(LocationX, LocationY))
		{
			FVector2D ScreenPosition(LocationX, LocationY);
			return XRDisplaySystem->DeprojectScreenToWorld(Player, ScreenPosition, WorldPosition, WorldDirection, CameraPosition);
		}
	}
#endif // PLATFORM_WINDOWS

	// something went wrong, zero things and return false
	WorldPosition = FVector::ZeroVector;
	WorldDirection = FVector::ZeroVector;
	return false;
}

bool USRDisplayFunctionLibrary::GetEyeLocationAndRotation(FVector& LeftLocation, FRotator& LeftRotation, FVector& RightLocation, FRotator& RightRotation)
{
#if PLATFORM_WINDOWS
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem) {
		return XRDisplaySystem->GetEyeLocationAndRotation(LeftLocation, LeftRotation, RightLocation, RightRotation);
	}
#endif // PLATFORM_WINDOWS

	// something went wrong, zero things and return false
	LeftLocation = RightLocation = FVector::ZeroVector;
	LeftRotation = RightRotation = FRotator::ZeroRotator;
	return false;
}

bool USRDisplayFunctionLibrary::GetPanelSize(EDeviceName DeviceName, float& Width, float& Height, float& Tilt)
{
#if PLATFORM_WINDOWS
	if (DeviceName == EDeviceName::NONE) {
		return false;
	}
	srdisplay_module::FSRDisplaySystem* SRDisplaySystem = GetSRDisplaySystem();
	if (!SRDisplaySystem) {
		return false;
	}
	int32_t Size = 0;
	if (!sony::oz::xr_runtime::GetPanelSpecOfSupportedDevices(SRDisplaySystem->GetPlatformId(), nullptr, &Size) || Size < 1) {
		return false;
	}
	auto PanelSpecSizeList = MakeUnique<sony::oz::xr_runtime::supported_panel_spec[]>(Size);
	if (sony::oz::xr_runtime::GetPanelSpecOfSupportedDevices(SRDisplaySystem->GetPlatformId(), PanelSpecSizeList.Get(), &Size)) {
		constexpr int METER_TO_CENTIMETER = 100;
		sony::oz::xr_runtime::supported_panel_spec spec = PanelSpecSizeList[static_cast<int>(DeviceName) - 1];
		Width = spec.width * METER_TO_CENTIMETER;
		Height = spec.height * METER_TO_CENTIMETER;
		Tilt = spec.angle;
		return true;
	}
#endif // PLATFORM_WINDOWS
	return false;
}

bool USRDisplayFunctionLibrary::SetHeadTrackingPaused(bool paused)
{
#if PLATFORM_WINDOWS
	srdisplay_module::FSRDisplaySystem* SRDisplaySystem = GetSRDisplaySystem();
	if (SRDisplaySystem)
	{
		return SRDisplaySystem->SetHeadTrackingPaused(paused);
	}
#endif // PLATFORM_WINDOWS
	return false;
}

bool USRDisplayFunctionLibrary::IsSRDisplayDetectedFace()
{
#if PLATFORM_WINDOWS
	xr_display::FXRDisplaySystem* XRDisplaySystem = GetXRDisplaySystem();
	if (XRDisplaySystem)
	{
		return XRDisplaySystem->IsXRDisplayDetectedFace();
	}
#endif // PLATFORM_WINDOWS
	return false;
}

bool USRDisplayFunctionLibrary::GetRuntimeVersion(FString& version) {
	srdisplay_module::FSRDisplaySystem* SRDisplaySystem = GetSRDisplaySystem();
	if (SRDisplaySystem) {
		return SRDisplaySystem->GetRuntimeVersion(version);
	}
	return false;
}

bool USRDisplayFunctionLibrary::GetDisplayVersion(FString& version) {
	srdisplay_module::FSRDisplaySystem* SRDisplaySystem = GetSRDisplaySystem();
	if (SRDisplaySystem) {
		return SRDisplaySystem->GetDisplayVersion(version);
	}
	return false;
}

bool USRDisplayFunctionLibrary::GetRuntimeVersionInfo(FSRDisplayVersionInfo& info)
{
	srdisplay_module::FSRDisplaySystem* SRDisplaySystem = GetSRDisplaySystem();
	if (SRDisplaySystem) {
		return SRDisplaySystem->GetRuntimeVersionInfo(info.MajorNumber, info.MinorNumber, info.BuildNumber, info.RevisionNumber);
	}
	return false;
}
