// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PartDatabase.h"
#include "ModelLevelManager.generated.h"

class UUserWidget;

UCLASS()
class MYPROJECT6_API AModelLevelManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AModelLevelManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	UPROPERTY()
	AActor* HitActor;

	UPROPERTY()
	AActor* LastHoverActor;

	void TraceMouse();
	void HoverActor(AActor* NewActor);
	void UnhoverActor(AActor* OldActor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
	UMaterialInterface* HoverOverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
	TSubclassOf<AActor> LaserClass;

	UPROPERTY(BlueprintReadWrite, Category = "Laser")
	bool bLaserPressed = true;

	void HideAllLasers();

	void ToggleLaser();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PartInfoWidgetClass;

	UFUNCTION()
	void ShowPartInfo();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PauseWidgetClass;

	UPROPERTY()
	UUserWidget* PauseWidget;

	void TogglePauseMenu();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parts")
	UPartDatabase* PartDatabase;

	UPROPERTY()
	UUserWidget* CurrentPartInfoWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> TipsWidgetClass;

	UPROPERTY()
	UUserWidget* TipsWidget;

	UFUNCTION(BlueprintCallable)
	void ClosePartInfo();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
	TSubclassOf<AActor> LaserEmitterClass;

private:
	int32 CallLaserEmitterFunction(FName FunctionName);

	void ShutdownRemainingLasers();
};
