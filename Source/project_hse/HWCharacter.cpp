#include "HWCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

AHWCharacter::AHWCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AHWCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	Stamina = MaxStamina;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AHWCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSprinting)
	{
		Stamina = FMath::Max(0.f, Stamina - StaminaDrainRate * DeltaTime);
		if (Stamina <= 0.f)
		{
			UpdateSprintState();
		}
	}
	else
	{
		Stamina = FMath::Min(MaxStamina, Stamina + StaminaRegenRate * DeltaTime);
	}
}

void AHWCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (LookAction)
		{
			UE_LOG(LogTemp, Warning, TEXT("LookAction УСПЕШНО ЗАГРУЖЕН"));
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHWCharacter::Look);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ОШИБКА: LookAction РАВЕН NULL!"));
		}
		EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AHWCharacter::StartSprint);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHWCharacter::StopSprint);
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AHWCharacter::Interact);
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHWCharacter::Move);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHWCharacter::Look);
		}
	}
}

void AHWCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(ForwardDirection, MovementVector.X);
		AddMovementInput(RightDirection, MovementVector.Y);
	}
}

void AHWCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AHWCharacter::StartSprint()
{
	bSprintKeyHeld = true;
	UpdateSprintState();
}

void AHWCharacter::StopSprint()
{
	bSprintKeyHeld = false;
	UpdateSprintState();
}

void AHWCharacter::UpdateSprintState()
{
	const bool bNewSprinting = bSprintKeyHeld && Stamina > 0.f;

	if (bNewSprinting != bIsSprinting)
	{
		bIsSprinting = bNewSprinting;
		GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

void AHWCharacter::Interact()
{
	OnInteract.Broadcast();
}