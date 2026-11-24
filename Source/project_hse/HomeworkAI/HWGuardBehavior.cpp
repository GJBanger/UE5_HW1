#include "HWGuardBehavior.h"

#include "HWCharacter.h"
#include "HWGuard.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"

using namespace HWGuardKeys;

UHWGuardSenseService::UHWGuardSenseService()
{
	NodeName = TEXT("Observe player and sprint noise");
	Interval = 0.15f;
	RandomDeviation = 0.f;
	bCallTickOnSearchStart = true;
}

void UHWGuardSenseService::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	AAIController* Controller = OwnerComp.GetAIOwner();
	UBlackboardComponent* Memory = OwnerComp.GetBlackboardComponent();
	const AHWGuard* Guard = Controller ? Cast<AHWGuard>(Controller->GetPawn()) : nullptr;
	if (!Guard || !Memory)
	{
		return;
	}

	const APlayerController* PlayerController = Guard->GetWorld()->GetFirstPlayerController();
	AHWCharacter* Player = PlayerController ? Cast<AHWCharacter>(PlayerController->GetPawn()) : nullptr;
	const FVector Offset = Player ? Player->GetActorLocation() - Guard->GetActorLocation() : FVector::ZeroVector;
	const bool bWasChasing = Memory->GetValueAsEnum(State) == static_cast<uint8>(EHWGuardState::Chase);
	const bool bInSightCone = Offset.SizeSquared2D() < FMath::Square(200.f) ||
		FVector::DotProduct(Guard->GetActorForwardVector(), Offset.GetSafeNormal2D()) >= 0.5f;
	const bool bVisible = Player && Offset.SizeSquared() <= FMath::Square(Guard->SightRadius) &&
		(bWasChasing || bInSightCone) && Controller->LineOfSightTo(Player);

	if (bVisible)
	{
		Memory->SetValueAsVector(LastSeen, Player->GetActorLocation());
		Memory->SetValueAsEnum(State, static_cast<uint8>(EHWGuardState::Chase));
		Memory->SetValueAsObject(Target, Player);
		Memory->ClearValue(InvestigateGoal);
		return;
	}

	if (bWasChasing && FAISystem::IsValidLocation(Memory->GetValueAsVector(LastSeen)))
	{
		Memory->SetValueAsVector(InvestigateGoal, Memory->GetValueAsVector(LastSeen));
	}
	else if (Player && Player->bIsSprinting && Player->GetCharacterMovement()->IsMovingOnGround() &&
		Player->GetVelocity().SizeSquared2D() > FMath::Square(100.f) &&
		Offset.SizeSquared() <= FMath::Square(Guard->SprintHearingRadius) &&
		!FAISystem::IsValidLocation(Memory->GetValueAsVector(InvestigateGoal)))
	{
		Memory->SetValueAsVector(InvestigateGoal, Player->GetActorLocation());
	}

	const EHWGuardState NextState = FAISystem::IsValidLocation(Memory->GetValueAsVector(InvestigateGoal))
		? EHWGuardState::Investigate : EHWGuardState::Patrol;
	Memory->SetValueAsEnum(State, static_cast<uint8>(NextState));
	Memory->ClearValue(Target);
}

UHWGuardPatrolGoalTask::UHWGuardPatrolGoalTask()
{
	NodeName = TEXT("Choose reachable point around home");
}

EBTNodeResult::Type UHWGuardPatrolGoalTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	const AHWGuard* Guard = Controller ? Cast<AHWGuard>(Controller->GetPawn()) : nullptr;
	UBlackboardComponent* Memory = OwnerComp.GetBlackboardComponent();
	const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerComp.GetWorld());
	if (!Guard || !Memory || !Navigation)
	{
		return EBTNodeResult::Failed;
	}

	const FVector Origin = Memory->GetValueAsVector(Home);
	FNavLocation NavOrigin;
	FNavLocation Goal;
	if (!Navigation->ProjectPointToNavigation(Origin, NavOrigin, FVector(150, 150, 250)) ||
		!Navigation->GetRandomReachablePointInRadius(NavOrigin.Location, Guard->PatrolRadius, Goal) ||
		FVector::DistSquared2D(Origin, Goal.Location) > FMath::Square(Guard->PatrolRadius))
	{
		return EBTNodeResult::Failed;
	}

	Memory->SetValueAsVector(PatrolGoal, Goal.Location);
	return EBTNodeResult::Succeeded;
}

UHWGuardMoveTask::UHWGuardMoveTask()
{
	bNotifyTaskFinished = true;
}

void UHWGuardMoveTask::SetGoal(FName Key)
{
	BlackboardKey.SelectedKeyName = Key;
	NodeName = FString::Printf(TEXT("Move To: %s"), *Key.ToString());
	bObserveBlackboardValue = Key == Target;
	bTrackMovingGoal = Key == Target;
	bAllowPartialPath = Key != PatrolGoal;
	AcceptableRadius = Key == Target ? 85.f : 45.f;
}

void UHWGuardMoveTask::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
	if ((TaskResult == EBTNodeResult::Succeeded || TaskResult == EBTNodeResult::Failed) &&
		GetSelectedBlackboardKey() == InvestigateGoal)
	{
		UBlackboardComponent* Memory = OwnerComp.GetBlackboardComponent();
		if (!Memory->GetValueAsObject(Target))
		{
			Memory->SetValueAsEnum(State, static_cast<uint8>(EHWGuardState::Patrol));
		}
		Memory->ClearValue(InvestigateGoal);
	}
}

UHWGuardHasGoal::UHWGuardHasGoal()
{
	FlowAbortMode = EBTFlowAbortMode::Both;
	NotifyObserver = EBTBlackboardRestart::ResultChange;
	OperationType = EBasicKeyOperation::Set;
#if WITH_EDITORONLY_DATA
	BasicOperation = EBasicKeyOperation::Set;
#endif
}

void UHWGuardHasGoal::SetGoal(FName Key)
{
	BlackboardKey.SelectedKeyName = Key;
	NodeName = FString::Printf(TEXT("%s is set"), *Key.ToString());
#if WITH_EDITOR
	BuildDescription();
#endif
}

void InitializeHWGuardTree(UBehaviorTree& Tree, UBlackboardData& Blackboard)
{
	Tree.BlackboardAsset = &Blackboard;
	for (const FName& Key : { Home, PatrolGoal, InvestigateGoal, LastSeen })
	{
		Blackboard.UpdatePersistentKey<UBlackboardKeyType_Vector>(Key);
	}
	if (UBlackboardKeyType_Object* Key = Blackboard.UpdatePersistentKey<UBlackboardKeyType_Object>(Target))
	{
		Key->BaseClass = AHWCharacter::StaticClass();
	}
	if (UBlackboardKeyType_Enum* Key = Blackboard.UpdatePersistentKey<UBlackboardKeyType_Enum>(State))
	{
		Key->EnumType = StaticEnum<EHWGuardState>();
	}

	UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(&Tree);
	Root->NodeName = TEXT("Guard behavior");
	Tree.RootNode = Root;
	Root->Services.Add(NewObject<UHWGuardSenseService>(&Tree));

	for (const FName& Key : { Target, InvestigateGoal, PatrolGoal })
	{
		UBTComposite_Sequence* Branch = NewObject<UBTComposite_Sequence>(&Tree);
		Branch->NodeName = Key == Target ? TEXT("Chase") : Key == InvestigateGoal ? TEXT("Investigate") : TEXT("Patrol");
		FBTCompositeChild& RootChild = Root->Children.AddDefaulted_GetRef();
		RootChild.ChildComposite = Branch;
		if (Key != PatrolGoal)
		{
			UHWGuardHasGoal* Condition = NewObject<UHWGuardHasGoal>(&Tree);
			Condition->SetGoal(Key);
			RootChild.Decorators.Add(Condition);
		}
		else
		{
			// Wait before choosing a point so missing or still-building nav data can recover.
			UBTTask_Wait* Wait = NewObject<UBTTask_Wait>(&Tree);
			Wait->WaitTime = 0.6f;
			Branch->Children.AddDefaulted_GetRef().ChildTask = Wait;
			Branch->Children.AddDefaulted_GetRef().ChildTask = NewObject<UHWGuardPatrolGoalTask>(&Tree);
		}
		UHWGuardMoveTask* Move = NewObject<UHWGuardMoveTask>(&Tree);
		Move->SetGoal(Key);
		Branch->Children.AddDefaulted_GetRef().ChildTask = Move;
	}
}
