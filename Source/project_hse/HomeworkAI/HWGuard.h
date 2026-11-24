#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "HWGuard.generated.h"

class UBehaviorTree;

UCLASS()
class PROJECT_HSE_API AHWGuardController : public AAIController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
};

UCLASS()
class PROJECT_HSE_API AHWGuard : public ACharacter
{
	GENERATED_BODY()

public:
	AHWGuard();

	UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "100"))
	float PatrolRadius = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "100"))
	float SightRadius = 1500.f;

	UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "100"))
	float SprintHearingRadius = 1100.f;

	UPROPERTY(EditAnywhere, Category = "Guard")
	TObjectPtr<UBehaviorTree> BehaviorTree;
};
