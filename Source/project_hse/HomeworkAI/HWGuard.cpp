#include "HWGuard.h"

#include "HWGuardBehavior.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AHWGuard::AHWGuard()
{
	AIControllerClass = AHWGuardController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 260.f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MeshAsset.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -88), FRotator(0, -90, 0));
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimClass(
		TEXT("/Game/HW2/ABP_HWGuard"));
	if (AnimClass.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimClass.Class);
	}
}

void AHWGuardController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	const AHWGuard* Guard = Cast<AHWGuard>(InPawn);
	if (!Guard)
	{
		return;
	}

	UBehaviorTree* Tree = Guard->BehaviorTree;
	if (!Tree)
	{
		Tree = NewObject<UBehaviorTree>(this);
		InitializeHWGuardTree(*Tree, *NewObject<UBlackboardData>(Tree));
	}

	UBlackboardComponent* Memory = nullptr;
	if (UseBlackboard(Tree->BlackboardAsset, Memory))
	{
		Memory->ClearValue(HWGuardKeys::Target);
		Memory->ClearValue(HWGuardKeys::InvestigateGoal);
		Memory->ClearValue(HWGuardKeys::LastSeen);
		Memory->ClearValue(HWGuardKeys::PatrolGoal);
		Memory->SetValueAsVector(HWGuardKeys::Home, InPawn->GetActorLocation());
		Memory->SetValueAsEnum(HWGuardKeys::State, static_cast<uint8>(EHWGuardState::Patrol));
		RunBehaviorTree(Tree);
	}
}

void AHWGuardController::OnUnPossess()
{
	if (UBehaviorTreeComponent* Tree = Cast<UBehaviorTreeComponent>(GetBrainComponent()))
	{
		Tree->StopTree(EBTStopMode::Forced);
	}
	Super::OnUnPossess();
}
