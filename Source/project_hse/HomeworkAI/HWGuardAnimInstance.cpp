#include "HWGuardAnimInstance.h"

#include "GameFramework/Pawn.h"

void UHWGuardAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* Pawn = TryGetPawnOwner();
	const float GroundSpeed = Pawn ? Pawn->GetVelocity().Size2D() : 0.f;
	constexpr float WalkSpeed = 150.f;
	constexpr float RunSpeed = 260.f;

	BlendSpeed = GroundSpeed <= WalkSpeed
		? FMath::GetMappedRangeValueClamped(FVector2f(0.f, WalkSpeed), FVector2f(0.f, 300.f), GroundSpeed)
		: FMath::GetMappedRangeValueClamped(FVector2f(WalkSpeed, RunSpeed), FVector2f(300.f, 600.f), GroundSpeed);
}
