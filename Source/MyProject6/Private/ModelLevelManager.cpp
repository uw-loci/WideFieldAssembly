#include "ModelLevelManager.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "PartInfo.h"
#include "Components/ActorComponent.h"
AModelLevelManager::AModelLevelManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AModelLevelManager::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("ModelLevelManager BeginPlay: PlayerController is null")
        );

        return;
    }

    EnableInput(PC);

    if (!InputComponent)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("ModelLevelManager BeginPlay: InputComponent is null")
        );

        return;
    }

    InputComponent->BindKey(
        EKeys::L,
        IE_Pressed,
        this,
        &AModelLevelManager::ToggleLaser
    );

    FInputKeyBinding& LeftMouseBinding =
        InputComponent->BindKey(
            EKeys::LeftMouseButton,
            IE_Pressed,
            this,
            &AModelLevelManager::ShowPartInfo
        );

    LeftMouseBinding.bConsumeInput = false;

    InputComponent->BindKey(
        EKeys::Escape,
        IE_Pressed,
        this,
        &AModelLevelManager::TogglePauseMenu
    );

    InputComponent->BindKey(
        EKeys::M,
        IE_Pressed,
        this,
        &AModelLevelManager::TogglePauseMenu
    );

    if (TipsWidgetClass)
    {
        TipsWidget =
            CreateWidget<UUserWidget>(
                GetWorld(),
                TipsWidgetClass
            );

        if (TipsWidget)
        {
            TipsWidget->AddToViewport();
        }
    }

    FTimerHandle InitialLaserTimer;

    GetWorldTimerManager().SetTimer(
        InitialLaserTimer,
        this,
        &AModelLevelManager::HideAllLasers,
        0.2f,
        false
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "ModelLevelManager BeginPlay completed. "
            "LaserClass=%s"
        ),
        LaserClass
        ? *LaserClass->GetName()
        : TEXT("NULL")
    );
}

void AModelLevelManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TraceMouse();
}

void AModelLevelManager::TraceMouse()
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC) return;

    float MouseX = 0.f;
    float MouseY = 0.f;

    if (!PC->GetMousePosition(MouseX, MouseY)) return;

    FVector WorldLocation;
    FVector WorldDirection;

    PC->DeprojectScreenPositionToWorld(
        MouseX,
        MouseY,
        WorldLocation,
        WorldDirection
    );

    FVector Start = WorldLocation;
    FVector End = Start + (WorldDirection * 2000.f);

    FHitResult Hit;

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility
    );

    AActor* NewHitActor = bHit ? Hit.GetActor() : nullptr;

    if (NewHitActor != LastHoverActor)
    {
        if (LastHoverActor)
        {
            UnhoverActor(LastHoverActor);
        }

        if (NewHitActor)
        {
            HoverActor(NewHitActor);
            UE_LOG(LogTemp, Warning, TEXT("Hover Actor: %s"), *NewHitActor->GetName());
        }

        LastHoverActor = NewHitActor;
    }

    HitActor = NewHitActor;
}

void AModelLevelManager::HoverActor(AActor* NewActor)
{
    if (!NewActor || !HoverOverlayMaterial) return;

    UStaticMeshComponent* MeshComp =
        NewActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return;

    MeshComp->bDisallowNanite = true;
    MeshComp->MarkRenderStateDirty();

    MeshComp->SetOverlayMaterial(HoverOverlayMaterial);
}

void AModelLevelManager::UnhoverActor(AActor* OldActor)
{
    if (!OldActor) return;

    UStaticMeshComponent* MeshComp =
        OldActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return;

    MeshComp->SetOverlayMaterial(nullptr);

    MeshComp->bDisallowNanite = false;
    MeshComp->MarkRenderStateDirty();
}

void AModelLevelManager::HideAllLasers()
{

    const int32 DeactivateCalls =
        CallLaserEmitterFunction(
            TEXT("DeactiveLaser")
        );


    ShutdownRemainingLasers();

    bLaserPressed = false;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "HideAllLasers completed: "
            "DeactiveCalls=%d"
        ),
        DeactivateCalls
    );
}
void AModelLevelManager::ToggleLaser()
{
    if (bLaserPressed)
    {

        HideAllLasers();
        return;
    }

    const int32 ActivateCalls =
        CallLaserEmitterFunction(
            TEXT("ActivateLaser")
        );

    TArray<AActor*> Lasers;

    if (LaserClass)
    {
        UGameplayStatics::GetAllActorsOfClass(
            this,
            LaserClass,
            Lasers
        );
    }

    for (AActor* Laser : Lasers)
    {
        if (!IsValid(Laser))
        {
            continue;
        }

        Laser->SetActorHiddenInGame(false);
        Laser->SetActorTickEnabled(true);
    }

    bLaserPressed = true;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "ToggleLaser ON: "
            "ActivateCalls=%d LasersAfterActivate=%d"
        ),
        ActivateCalls,
        Lasers.Num()
    );
}

void AModelLevelManager::ShowPartInfo()
{
    if (!HitActor)
    {
        ClosePartInfo();
        return;
    }

    if (!PartInfoWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PartInfoWidgetClass is null"));
        return;
    }

    UStaticMeshComponent* MeshComp =
        HitActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp || !MeshComp->GetStaticMesh())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("No StaticMesh found on Actor: %s"),
            *HitActor->GetName());
        return;
    }

    FString MeshName =
        MeshComp->GetStaticMesh()->GetName();

    FPartInfoData PartData;

    if (PartDatabase &&
        PartDatabase->FindPartInfo(
            MeshName,
            PartData))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Found Part: %s"),
            *PartData.PartName);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("No PartData found for Mesh: %s"),
            *MeshName);

        return;
    }

    if (CurrentPartInfoWidget)
    {
        CurrentPartInfoWidget->RemoveFromParent();
        CurrentPartInfoWidget = nullptr;
    }

    UPartInfo* Widget =
        CreateWidget<UPartInfo>(
            GetWorld(),
            PartInfoWidgetClass
        );

    if (!Widget)
    {
        return;
    }

    Widget->PartName = PartData.PartName;
    Widget->PartDescription = PartData.PartDescription;
    Widget->PartURL = PartData.PartURL;
    Widget->PartImage = PartData.PartImage;

    Widget->AddToViewport();

    CurrentPartInfoWidget = Widget;

    UE_LOG(LogTemp, Warning,
        TEXT("Create widget for Mesh: %s"),
        *MeshName);
}

void AModelLevelManager::TogglePauseMenu()
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC) return;
    if (!PauseWidget)
    {
        PauseWidget =
            CreateWidget<UUserWidget>(
                GetWorld(),
                PauseWidgetClass
            );

        if (!PauseWidget) return;

        PauseWidget->AddToViewport();

        UGameplayStatics::SetGamePaused(
            GetWorld(),
            true
        );

        PC->SetShowMouseCursor(true);

        PC->SetInputMode(FInputModeUIOnly());
    }
    else
    {
        PauseWidget->RemoveFromParent();

        PauseWidget = nullptr;

        UGameplayStatics::SetGamePaused(
            GetWorld(),
            false
        );

        PC->SetInputMode(FInputModeGameOnly());

        PC->SetShowMouseCursor(false);
    }
}
void AModelLevelManager::ClosePartInfo()
{
    if (CurrentPartInfoWidget)
    {
        CurrentPartInfoWidget->RemoveFromParent();
        CurrentPartInfoWidget = nullptr;
    }
}
int32 AModelLevelManager::CallLaserEmitterFunction(
    FName FunctionName
)
{
    if (!LaserEmitterClass)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT(
                "CallLaserEmitterFunction failed: "
                "LaserEmitterClass is null"
            )
        );

        return 0;
    }

    TArray<AActor*> Emitters;

    UGameplayStatics::GetAllActorsOfClass(
        this,
        LaserEmitterClass,
        Emitters
    );

    int32 FunctionCallCount = 0;

    for (AActor* EmitterActor : Emitters)
    {
        if (!IsValid(EmitterActor))
        {
            continue;
        }

        if (
            UFunction* ActorFunction =
            EmitterActor->FindFunction(FunctionName)
            )
        {
            EmitterActor->ProcessEvent(
                ActorFunction,
                nullptr
            );

            ++FunctionCallCount;
        }

        TInlineComponentArray<UActorComponent*> Components;

        EmitterActor->GetComponents(Components);

        for (UActorComponent* Component : Components)
        {
            if (!IsValid(Component))
            {
                continue;
            }

            UFunction* ComponentFunction =
                Component->FindFunction(FunctionName);

            if (!ComponentFunction)
            {
                continue;
            }

            Component->ProcessEvent(
                ComponentFunction,
                nullptr
            );

            ++FunctionCallCount;

            UE_LOG(
                LogTemp,
                Warning,
                TEXT(
                    "Called %s on Component=%s Owner=%s"
                ),
                *FunctionName.ToString(),
                *Component->GetName(),
                *EmitterActor->GetName()
            );
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "CallLaserEmitterFunction: "
            "Function=%s EmitterActors=%d FunctionCalls=%d"
        ),
        *FunctionName.ToString(),
        Emitters.Num(),
        FunctionCallCount
    );

    return FunctionCallCount;
}

void AModelLevelManager::ShutdownRemainingLasers()
{
    if (!LaserClass)
    {
        return;
    }

    TArray<AActor*> Lasers;

    UGameplayStatics::GetAllActorsOfClass(
        this,
        LaserClass,
        Lasers
    );

    for (AActor* Laser : Lasers)
    {
        if (!IsValid(Laser))
        {
            continue;
        }

        if (
            UFunction* ShutdownFunction =
            Laser->FindFunction(TEXT("ShutdownLaser"))
            )
        {
            Laser->ProcessEvent(
                ShutdownFunction,
                nullptr
            );
        }

        if (IsValid(Laser))
        {
            Laser->Destroy();
        }
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "ShutdownRemainingLasers: InitialCount=%d"
        ),
        Lasers.Num()
    );
}