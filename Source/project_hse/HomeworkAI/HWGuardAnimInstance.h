#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "HWGuardAnimInstance.generated.h"

UCLASS(Blueprintable)
class PROJECT_HSE_API UHWGuardAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// The shared Blend Space uses 0, 300 and 600 for idle, walk and jog.
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float BlendSpeed = 0.f;
};
