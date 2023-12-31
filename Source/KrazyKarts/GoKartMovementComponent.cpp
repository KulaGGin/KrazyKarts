// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementComponent.h"

// Sets default values for this component's properties
UGoKartMovementComponent::UGoKartMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UGoKartMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGoKartMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(GetOwnerRole() == ROLE_AutonomousProxy || GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy) {
		LastMove = CreateMove(DeltaTime);
		SimulateMove(LastMove);
	}

	// ...
}

void UGoKartMovementComponent::SimulateMove(const FGoKartMove& Move) {
	FVector Force = GetOwner()->GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove UGoKartMovementComponent::CreateMove(float DeltaTime) {
	FGoKartMove Move{};
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->TimeSeconds;

	return Move;
}

FVector UGoKartMovementComponent::GetAirResistance() {
	FVector Resistance = -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;

	return Resistance;
}

FVector UGoKartMovementComponent::GetRollingResistance() {
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity;
	FVector Resistance = -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;

	return Resistance;
}

void UGoKartMovementComponent::ApplyRotation(float DeltaTime, float SteeringThrowParameter) {
	float RotationAngle = FMath::DegreesToRadians(MaxDegreesPerSecond * DeltaTime * SteeringThrowParameter);
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);

	FHitResult HitResult;
	GetOwner()->AddActorWorldRotation(RotationDelta, true, &HitResult);
	if(HitResult.IsValidBlockingHit())
		SteeringThrowParameter = 0;

	auto ActorRotation = GetOwner()->GetActorRotation().Vector();
	Velocity = Velocity.Size() * ActorRotation;
}

void UGoKartMovementComponent::UpdateLocationFromVelocity(float DeltaTime) {
	FVector Translation = Velocity * DeltaTime * 100;
	FHitResult HitResult{};
	GetOwner()->AddActorWorldOffset(Translation, true, &HitResult);
	if(HitResult.IsValidBlockingHit())
		Velocity = {};
}

void UGoKartMovementComponent::SetVelocity(FVector NewVelocity) {
	Velocity = NewVelocity;
}

FVector UGoKartMovementComponent::GetVelocity() {
	return Velocity;
}

void UGoKartMovementComponent::SetThrottle(float NewThrottle) {
	Throttle = NewThrottle;
}

float UGoKartMovementComponent::GetThrottle() {
	return Throttle;
}

void UGoKartMovementComponent::SetSteeringThrow(float NewSteeringThrow) {
	SteeringThrow = NewSteeringThrow;
}

float UGoKartMovementComponent::GetSteeringThrow() {
	return SteeringThrow;
}

FGoKartMove UGoKartMovementComponent::GetLastMove() {
	return LastMove;
}
