#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "HWGuardBehavior.generated.h"

UENUM()
enum class EHWGuardState : uint8
{
	Patrol,
	Investigate,
	Chase
};

namespace HWGuardKeys
{
	inline const FName Home = TEXT("Home");
	inline const FName PatrolGoal = TEXT("PatrolGoal");
	inline const FName InvestigateGoal = TEXT("InvestigateGoal");
	inline const FName LastSeen = TEXT("LastSeen");
	inline const FName Target = TEXT("Target");
	inline const FName State = TEXT("State");
}

UCLASS()
class UHWGuardSenseService : public UBTService
{
	GENERATED_BODY()

public:
	UHWGuardSenseService();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS()
class UHWGuardPatrolGoalTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UHWGuardPatrolGoalTask();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class UHWGuardMoveTask : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	UHWGuardMoveTask();
	void SetGoal(FName Key);

protected:
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		EBTNodeResult::Type TaskResult) override;
};

UCLASS()
class UHWGuardHasGoal : public UBTDecorator_Blackboard
{
	GENERATED_BODY()

public:
	UHWGuardHasGoal();
	void SetGoal(FName Key);
};

void InitializeHWGuardTree(UBehaviorTree& Tree, UBlackboardData& Blackboard);
