/*
 * Copyright 2022,2024 Sony Corporation
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "SRDisplayProjectSettings.generated.h"

UENUM()
enum class ELinkedMode
{
	SINGLE_MODE UMETA(DisplayName = "Single Display/Duplicated Output"),
	VERTICAL_MODE UMETA(DisplayName = "Vertical Multi Display"),
	HORIZONTAL_MODE UMETA(DisplayName = "Horizontal Multi Display")
};

UCLASS(config = Game, defaultconfig)
class SRDISPLAYMODULE_API USRDisplayProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Config, Category = "Spatial Reality Display",
		meta = (ToolTip = "Check this if you want to run your app with no Spatial Reality Display."))
	bool RunWithoutSRDisplayMode = false;

	UPROPERTY(EditAnywhere, Config, Category = "MultiDisplaySettings", meta = (DisplayName = "Multi Display Mode", EditCondition = "RunWithoutSRDisplayMode == false",
		ToolTip = "Select whether this application supports multiple Spatial Reality Displays."))
	ELinkedMode LinkedMode = ELinkedMode::SINGLE_MODE;

	UPROPERTY(EditAnywhere, Config, Category = "MultiDisplaySettings", meta = (DisplayName = "Number Of Displays", ClampMin = "2", ClampMax = "3",
		EditCondition = "RunWithoutSRDisplayMode == false && LinkedMode == ELinkedMode::VERTICAL_MODE || LinkedMode == ELinkedMode::HORIZONTAL_MODE",
		ToolTip = "Define the number of Spatial Reality Displays this application is supposed to use."))
	int NumSRDisplay = 2;

	UPROPERTY(EditAnywhere, Config, Category = "MultiDisplaySettings", meta = (DisplayName = "Position Change Time", ClampMin = "1", ClampMax = "15", Delta = 0.01, Multiple = 0.01,
		EditCondition = "RunWithoutSRDisplayMode == false && LinkedMode == ELinkedMode::VERTICAL_MODE || LinkedMode == ELinkedMode::HORIZONTAL_MODE",
		ToolTip = "Set the interval (in seconds) at which the SRDisplayManager switches between display positions in Editor Play mode."))
	float PositionSwitchInterval = 3;

public:
	virtual FName GetContainerName() const override {
		return Super::GetContainerName();
	}
	virtual FName GetCategoryName() const override {
		return "Plugins";
	}
	virtual FName GetSectionName() const override {
		return Super::GetSectionName();
	}

#if WITH_EDITOR
	virtual FText GetSectionText() const override {
		return FText::FromString(TEXT("Spatial Reality Display"));
	}

	virtual FText GetSectionDescription() const override {
		return FText::FromString(TEXT("Set up Spatial Reality Display Plugin"));
	}

	void PostEditChangeProperty(struct FPropertyChangedEvent& e) override {
		FName MemberPropertyName = (e.Property != NULL) ? e.MemberProperty->GetFName() : NAME_None;
		if (MemberPropertyName == "LinkedMode")
		{
			switch (LinkedMode)
			{
				case ELinkedMode::SINGLE_MODE:
					NumSRDisplay = 1;
					break;
				case ELinkedMode::VERTICAL_MODE:
					if (NumSRDisplay < 2 || 4 < NumSRDisplay)
					{
						NumSRDisplay = 2;
					}
					break;
				case ELinkedMode::HORIZONTAL_MODE:
					if (NumSRDisplay < 2 || 3 < NumSRDisplay)
					{
						NumSRDisplay = 2;
					}
					break;
				default:
					break;
			}
			SaveConfig(CPF_Config, *GetDefaultConfigFilename());
		}
		else if (MemberPropertyName == "NumSRDisplay")
		{
			if (LinkedMode == ELinkedMode::HORIZONTAL_MODE) {
				if (NumSRDisplay > 3) {
					NumSRDisplay = 3;
					SaveConfig(CPF_Config, *GetDefaultConfigFilename());
				}
			}
		}
		else if (MemberPropertyName == "PositionSwitchInterval")
		{
			PositionSwitchInterval = FMath::RoundToFloat(PositionSwitchInterval * 100.f) / 100.f;
		}
		Super::PostEditChangeProperty(e);
	}

#endif
};
