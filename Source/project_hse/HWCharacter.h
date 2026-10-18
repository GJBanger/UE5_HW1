#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "HWCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractSignature);

UCLASS()
class PROJECT_HSE_API AHWCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AHWCharacter();
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractSignature OnInteract;
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;
	UPROPERTY(BlueprintReadOnly, Category = "Stamina")
	float Stamina = 100.f;
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float MaxStamina = 100.f;
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float StaminaDrainRate = 20.f;
	UPROPERTY(EditAnywhere, Category = "Stamina")
	float StaminaRegenRate = 10.f;

protected:
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* DefaultMappingContext;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkSpeed = 400.f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float SprintSpeed = 800.f;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

private:
	void StartSprint();
	void StopSprint();
	void Interact();
	bool bSprintKeyHeld = false;
	void UpdateSprintState();
};