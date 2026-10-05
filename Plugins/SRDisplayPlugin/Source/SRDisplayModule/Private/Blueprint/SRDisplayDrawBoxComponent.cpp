/*
 * Copyright 2019,2020,2021,2022,2023,2024 Sony Corporation
 */
#include "Blueprint/SRDisplayDrawBoxComponent.h"
#include "XRDisplaySystem.h"

#include <Engine/CollisionProfile.h>
#include <PrimitiveSceneProxy.h>
#include "SceneManagement.h"

#include "misc_util_wrapper.h"
#include <SRDisplayModule/Public/SRDisplayLinkedUtil.h>
#include <SRDisplayModule/Classes/Blueprint/SRDisplayProjectSettings.h>


/** Represents a draw frustum to the scene manager. */
class FSRDisplayDrawBoxSceneProxy final : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FSRDisplayDrawBoxSceneProxy(const USRDisplayDrawBoxComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		, BoxColor(InComponent->BoxColor)
		, DisplayColor(InComponent->DisplayColor)
		, LineThickness(InComponent->LineThickness)
		, TargetScale(InComponent->TargetScale)
		, DisplayWidth(InComponent->DisplayWidth)
		, DisplayHeight(InComponent->DisplayHeight)
		, DisplayTilt(InComponent->DisplayTilt)
		, EnableVerticalDisplayMode(InComponent->EnableVerticalDisplayMode)
		, EnableScalingMode(InComponent->EnableScalingMode)
	{
		bWillEverBeLit = false;
	}

	// FPrimitiveSceneProxy interface.
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_DrawFrustumSceneProxy_DrawDynamicElements);

		FVector BoxVerts[8];
		FVector DisplayVerts[4];
		const USRDisplayProjectSettings* settings = GetDefault<USRDisplayProjectSettings>();

		if (!((EnableVerticalDisplayMode && settings->LinkedMode == ELinkedMode::HORIZONTAL_MODE) || settings->LinkedMode == ELinkedMode::VERTICAL_MODE))
		{
			const float SRDisplayTile = FMath::DegreesToRadians(DisplayTilt);
			const float SRDisplayDepthByTilt = DisplayHeight * FMath::Sin(SRDisplayTile);
			const float SRDisplayHeightByTilt = DisplayHeight * FMath::Cos(SRDisplayTile);

			BoxVerts[0] = FVector(0, -DisplayWidth / 2, 0);
			BoxVerts[1] = FVector(SRDisplayDepthByTilt, -DisplayWidth / 2, 0);
			BoxVerts[2] = FVector(SRDisplayDepthByTilt, DisplayWidth / 2, 0);
			BoxVerts[3] = FVector(0, DisplayWidth / 2, 0);
			BoxVerts[4] = FVector(0, -DisplayWidth / 2, SRDisplayHeightByTilt);
			BoxVerts[5] = FVector(SRDisplayDepthByTilt, -DisplayWidth / 2, SRDisplayHeightByTilt);
			BoxVerts[6] = FVector(SRDisplayDepthByTilt, DisplayWidth / 2, SRDisplayHeightByTilt);
			BoxVerts[7] = FVector(0, DisplayWidth / 2, SRDisplayHeightByTilt);

			DisplayVerts[0] = FVector(0, -DisplayWidth / 2, 0);
			DisplayVerts[1] = FVector(SRDisplayDepthByTilt, -DisplayWidth / 2, SRDisplayHeightByTilt);
			DisplayVerts[2] = FVector(SRDisplayDepthByTilt, DisplayWidth / 2, SRDisplayHeightByTilt);
			DisplayVerts[3] = FVector(0, DisplayWidth / 2, 0);

			if (EnableVerticalDisplayMode)
			{
				for (int i = 0; i < 8; i++)
				{
					BoxVerts[i] = FRotator(45, 0, 0).RotateVector(BoxVerts[i]);
				}
				for (int i = 0; i < 4; i++)
				{
					DisplayVerts[i] = FRotator(45, 0, 0).RotateVector(DisplayVerts[i]);
				}
			}
		}
		else
		{
			const float DisplayForwardDepth = 16.8f * GetScalingModeScale();
			const float DisplayBackwardsDepth = 28.7f * GetScalingModeScale();

			BoxVerts[0] = FVector(-DisplayForwardDepth, -DisplayWidth / 2, 0);
			BoxVerts[1] = FVector(DisplayBackwardsDepth, -DisplayWidth / 2, 0);
			BoxVerts[2] = FVector(DisplayBackwardsDepth, DisplayWidth / 2, 0);
			BoxVerts[3] = FVector(-DisplayForwardDepth, DisplayWidth / 2, 0);
			BoxVerts[4] = FVector(-DisplayForwardDepth, -DisplayWidth / 2, DisplayHeight);
			BoxVerts[5] = FVector(DisplayBackwardsDepth, -DisplayWidth / 2, DisplayHeight);
			BoxVerts[6] = FVector(DisplayBackwardsDepth, DisplayWidth / 2, DisplayHeight);
			BoxVerts[7] = FVector(-DisplayForwardDepth, DisplayWidth / 2, DisplayHeight);

			DisplayVerts[0] = FVector(0, -DisplayWidth / 2, 0);
			DisplayVerts[1] = FVector(0, -DisplayWidth / 2, DisplayHeight);
			DisplayVerts[2] = FVector(0, DisplayWidth / 2, DisplayHeight);
			DisplayVerts[3] = FVector(0, DisplayWidth / 2, 0);
		}

		for (int i = 0; i < 8; i++)
		{
			BoxVerts[i] = GetLocalToWorld().TransformPosition(BoxVerts[i] * TargetScale);
		}
		for (int i = 0; i < 4; i++)
		{
			DisplayVerts[i] = GetLocalToWorld().TransformPosition(DisplayVerts[i] * TargetScale);
		}

		DrawSRDisplayMangerBox(BoxVerts, DisplayVerts, Views, ViewFamily, VisibilityMap, Collector);

		if (settings->LinkedMode == ELinkedMode::VERTICAL_MODE ||
			settings->LinkedMode == ELinkedMode::HORIZONTAL_MODE)
		{
			for (int BoxCount = 1; BoxCount < settings->NumSRDisplay; BoxCount++)
			{
				FVector tmpBoxVerts[8];
				for (int i = 0; i < 8; i++)
				{
					tmpBoxVerts[i] = BoxVerts[i] + GetLocalToWorld().TransformVector(srdisplay::util::GetSRDidplsyManagerPostion(DisplayHeight, DisplayWidth, settings->LinkedMode, BoxCount) * TargetScale);
				}
				FVector tmpDisplayVerts[4];
				for (int i = 0; i < 4; i++)
				{
					tmpDisplayVerts[i] = DisplayVerts[i] + GetLocalToWorld().TransformVector(srdisplay::util::GetSRDidplsyManagerPostion(DisplayHeight, DisplayWidth, settings->LinkedMode, BoxCount) * TargetScale);
				}
				DrawSRDisplayMangerBox(tmpBoxVerts, tmpDisplayVerts, Views, ViewFamily, VisibilityMap, Collector);
			}
		}
	}

private:
	void DrawSRDisplayMangerBox(FVector* BoxVerts, FVector* DisplayVerts, const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				const FSceneView* View = Views[ViewIndex];

				const uint8 DepthPriorityGroup = GetDepthPriorityGroup(View);
				PDI->DrawLine(BoxVerts[0], BoxVerts[1], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[1], BoxVerts[2], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[2], BoxVerts[3], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[3], BoxVerts[0], BoxColor, DepthPriorityGroup, LineThickness);

				PDI->DrawLine(BoxVerts[0], BoxVerts[4], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[1], BoxVerts[5], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[2], BoxVerts[6], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[3], BoxVerts[7], BoxColor, DepthPriorityGroup, LineThickness);

				PDI->DrawLine(BoxVerts[4], BoxVerts[5], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[5], BoxVerts[6], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[6], BoxVerts[7], BoxColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(BoxVerts[7], BoxVerts[4], BoxColor, DepthPriorityGroup, LineThickness);

				//display surface
				PDI->DrawLine(DisplayVerts[0], DisplayVerts[1], DisplayColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(DisplayVerts[1], DisplayVerts[2], DisplayColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(DisplayVerts[2], DisplayVerts[3], DisplayColor, DepthPriorityGroup, LineThickness);
				PDI->DrawLine(DisplayVerts[3], DisplayVerts[0], DisplayColor, DepthPriorityGroup, LineThickness);
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && true;
		Result.bDynamicRelevance = true;
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		return Result;
	}

	float GetScalingModeScale() const
	{
		if (EnableScalingMode)
		{
			xr_display::FXRDisplaySystem* XRDisplaySystem = static_cast<xr_display::FXRDisplaySystem*>(GEngine->XRSystem.Get());
			if (!XRDisplaySystem) {
				return 1.f;
			}
			int32_t Size = 0;
			if (!sony::oz::xr_runtime::GetPanelSpecOfSupportedDevices(XRDisplaySystem->GetXRDisplay()->GetPlatformId(), nullptr, &Size) || Size < 2) {
				return 1.f;
			}
			auto PanelSpecSize = MakeUnique<sony::oz::xr_runtime::supported_panel_spec[]>(Size);
			if (!sony::oz::xr_runtime::GetPanelSpecOfSupportedDevices(XRDisplaySystem->GetXRDisplay()->GetPlatformId(), PanelSpecSize.Get(), &Size)) {
				return 1.f;
			}
			return PanelSpecSize[0].width / PanelSpecSize[1].width;
		}
		return 1.f;
	}

	virtual uint32 GetMemoryFootprint(void) const override { return (sizeof(*this) + GetAllocatedSize()); }
	uint32 GetAllocatedSize(void) const { return (FPrimitiveSceneProxy::GetAllocatedSize()); }

private:
	FColor BoxColor;
	FColor DisplayColor;
	float LineThickness;
	float TargetScale;
	float DisplayWidth;
	float DisplayHeight;
	float DisplayTilt;
	bool EnableVerticalDisplayMode;
	bool EnableScalingMode;
};

USRDisplayDrawBoxComponent::USRDisplayDrawBoxComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set Defaults
	BoxColor = FColor::Blue;
	DisplayColor = FColor::White;
	LineThickness = 1.f;
	TargetScale = 1.f;

	bUseEditorCompositing = true;
	bHiddenInGame = true;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);

	BoxSize = DrawBoxSize::ELF_SR1_Size;
	EnableVerticalDisplayMode = false;
	EnableScalingMode = false;
}

static const float defaultDisplayWidth_cm = 59.6736f;
static const float defaultDisplayHeight_cm = 33.5664f;
static const float defaultDisplayTilt_rad = 0.785398f;

FPrimitiveSceneProxy* USRDisplayDrawBoxComponent::CreateSceneProxy()
{
	UpdateDisplaySpec();
	return new FSRDisplayDrawBoxSceneProxy(this);
}

FBoxSphereBounds USRDisplayDrawBoxComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	UpdateDisplaySpec();
	FVector origin(FVector(0, -DisplayWidth / 2, 0) * TargetScale);
	FVector extent(FVector(DisplayHeight * FMath::Sin(DisplayTilt), DisplayWidth / 2, DisplayHeight * FMath::Cos(DisplayTilt)) * TargetScale);
	float radious = (extent - origin).Size();
	return FBoxSphereBounds(LocalToWorld.TransformPosition(origin), extent, radious);
}

void USRDisplayDrawBoxComponent::UpdateDisplaySpec() const
{
	constexpr float METER_TO_CENTIMETER = 100.f;
	DisplayWidth = defaultDisplayWidth_cm;
	DisplayHeight = defaultDisplayHeight_cm;
	DisplayTilt = FMath::RadiansToDegrees(defaultDisplayTilt_rad);

	xr_display::FXRDisplaySystem* XRDisplaySystem = static_cast<xr_display::FXRDisplaySystem*>(GEngine->XRSystem.Get());
	if (!XRDisplaySystem) {
		return;
	}

	int32_t Size = 0;
	if (!sony::oz::xr_runtime::GetPanelSpecOfSupportedDevices(XRDisplaySystem->GetXRDisplay()->GetPlatformId(), nullptr, &Size) || Size < 1) {
		return;
	}
	auto PanelSpecSize = MakeUnique<sony::oz::xr_runtime::supported_panel_spec[]>(Size);
	if (!sony::oz::xr_runtime::GetPanelSpecOfSupportedDevices(XRDisplaySystem->GetXRDisplay()->GetPlatformId(), PanelSpecSize.Get(), &Size)) {
		return;
	}

	sony::oz::xr_runtime::supported_panel_spec spec = PanelSpecSize[static_cast<int>(BoxSize)];
	DisplayWidth = spec.width * METER_TO_CENTIMETER;
	DisplayHeight = spec.height * METER_TO_CENTIMETER;
	DisplayTilt = FMath::RadiansToDegrees(spec.angle);
}
