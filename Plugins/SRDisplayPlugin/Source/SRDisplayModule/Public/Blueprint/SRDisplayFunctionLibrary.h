/**
 * Copyright 2019,2020,2021,2022,2023 Sony Corporation
 */
#pragma once

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "XRDisplaySystem.h"
#include <SRDisplayModule.h>

#include "SRDisplayFunctionLibrary.generated.h"

UENUM(BlueprintType)
enum class EDeviceName : uint8
{
	NONE UMETA(DisplayName = "None"),
	SR1_SIZE UMETA(DisplayName = "ELF-SR1"),
	SR2_SIZE UMETA(DisplayName = "ELF-SR2")
};

USTRUCT(BlueprintType)
struct FSRDisplayVersionInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SRDisplay")
	int MajorNumber = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SRDisplay")
	int MinorNumber = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SRDisplay")
	int BuildNumber = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SRDisplay")
	int RevisionNumber = 0;
};

/**
 * @brief TBD: Add brief description for this Blueprint Library Class.
 */
UCLASS()
class SRDISPLAYMODULE_API USRDisplayFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static xr_display::FXRDisplaySystem* GetXRDisplaySystem();

public:
	static srdisplay_module::FSRDisplaySystem* GetSRDisplaySystem();

public:

	USRDisplayFunctionLibrary(const FObjectInitializer& ObjectInitializer);

	/**
	 * @brief Returns if SRDisplay is connected.
	 */
	UFUNCTION(BlueprintCallable, Category = "SRDisplay")
	static bool IsSRDisplayConnected();

	/**
	 * @brief Returns the name of the connecting display when playing.
	 */
	UFUNCTION(BlueprintCallable, Category = "SRDisplay")
	static bool GetConnectedSRDisplayName(EDeviceName& DeviceName);

	/**
	 * @brief Retrieves the X and Y screen coordinates of the mouse cursor. Returns false if the mouse cursor is not over the window.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SRDisplay")
	static bool GetMousePosition(float& LocationX, float& LocationY, const bool bLocalPosition = true);

	/**
	 * @brief Get the top left coordinate of SRDisplay.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SRDisplay")
	static bool GetSRDTopLeftCornerWorldPosition(FVector2D& SRDTopLeftCornerWorldPosition);

	/**
	 * @brief Transforms the given 2D screen space coordinate into a 3D world-space point and direction (The reference camera position is also acquired).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SRDisplay")
	static bool DeprojectScreenToWorld(APlayerController const* Player, const FVector2D& ScreenPosition, FVector& WorldPosition, FVector& WorldDirection, FVector& CameraPosition);

	/**
	 * @brief Convert current mouse 2D position to World Space 3D position and direction. Returns false if unable to determine value (The reference camera position is also acquired).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SRDisplay", meta = (DisplayName = "ConvertMouseLocationToWorldSpace"))
	static bool ConvertMouseLocationToWorldSpace(APlayerController const* Player, FVector& WorldPosition, FVector& WorldDirection, FVector& CameraPosition);

	/**
	 * @brief Get current eye location and rotation. Returns false if face is not detected.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SRDisplay")
	static bool GetEyeLocationAndRotation(FVector& LeftLocation, FRotator& LeftRotation, FVector& RightLocation, FRotator& RightRotation);

	/**
	 * @brief Get Panel size of ELF-SR1 or ELF-SR2.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SRDisplay")
	static bool GetPanelSize(EDeviceName DeviceName, float& Width, float& Height, float& Tilt);

	/**
	 * @brief Pause/Resume face tracking.
	 */
	UFUNCTION(BlueprintCallable, Category = "SRDisplay")
	static bool SetHeadTrackingPaused(bool paused);
	/**
	 * @brief Returns if SRDisplay detects a face
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SRDisplay")
	static bool IsSRDisplayDetectedFace();

	/**
	* @brief Returns UEString of runtime version of SRDisplay 
	*/
	UFUNCTION(BlueprintCallable, Category = "SRDisplay")
	static bool GetRuntimeVersion(FString& version);

	/**
	* @brief Returns UEString of display version of SRDisplay
	*/
	UFUNCTION(BlueprintCallable, Category = "SRDisplay")
	static bool GetDisplayVersion(FString& version);

	/**
	* @brief Returns FSRDisplayVersionInfo struct of runtime version of SRDisplay
	*/
	UFUNCTION(BlueprintCallable, Category = "SRDisplay")
	static bool GetRuntimeVersionInfo(FSRDisplayVersionInfo& info);

};
