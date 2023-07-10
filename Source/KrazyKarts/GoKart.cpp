// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "DrawDebugHelpers.h"
#include "Components/InputComponent.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();
	if(HasAuthority())
		NetUpdateFrequency = 1;
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);
}

void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKart, ReplicatedTransform);
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * DeltaTime;

	ApplyRotation(DeltaTime);

	UpdateLocationFromVelocity(DeltaTime);

	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);

	if(HasAuthority()) {
		ReplicatedTransform = GetActorTransform();
	}
}

FVector AGoKart::GetAirResistance() {
	FVector Resistance = - Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;

	return Resistance;
}

FVector AGoKart::GetRollingResistance() {
	float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
	float NormalForce = Mass * AccelerationDueToGravity;
	FVector Resistance = -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;

	return Resistance;
}

void AGoKart::ApplyRotation(float DeltaTime) {
	float RotationAngle = FMath::DegreesToRadians(MaxDegreesPerSecond * DeltaTime * SteeringThrow);
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	FHitResult HitResult;
	AddActorWorldRotation(RotationDelta, true, &HitResult);
	if(HitResult.IsValidBlockingHit())
		SteeringThrow = 0;

	auto ActorRotation = GetActorRotation().Vector();
	Velocity = Velocity.Size() * ActorRotation;
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime) {
	FVector Translation = Velocity * DeltaTime * 100;
	FHitResult HitResult{};
	AddActorWorldOffset(Translation, true, &HitResult);
	if(HitResult.IsValidBlockingHit())
		Velocity = {};
}

void AGoKart::MoveForward(float Value) {
	Local_MoveForward(Value);
	Server_MoveForward(Value);
}

void AGoKart::MoveRight(float Value) {
	Local_MoveRight(Value);
	Server_MoveRight(Value);
}

void AGoKart::Local_MoveForward(float Value) {
	Throttle = Value;
}

void AGoKart::Local_MoveRight(float Value) {
	SteeringThrow = Value;
}

void AGoKart::OnRep_ReplicatedFTransform() {
	SetActorTransform(ReplicatedTransform);
}

void AGoKart::Server_MoveForward_Implementation(float Value) {
	Local_MoveForward(Value);
}

bool AGoKart::Server_MoveForward_Validate(float Value) {
	return FMath::Abs(Value) <= 1.f;
}

void AGoKart::Server_MoveRight_Implementation(float Value) {
	Local_MoveRight(Value);
}

bool AGoKart::Server_MoveRight_Validate(float Value) {
	return FMath::Abs(Value) <= 1.f;
}