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

	DOREPLIFETIME(AGoKart, ServerState);
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(GetLocalRole() == ROLE_AutonomousProxy) {
		auto Move = CreateMove(DeltaTime);
		SimulateMove(Move);
		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move);
	}

	// We are the server and in control of the pawn
	if(GetLocalRole() == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy) {
		auto Move = CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if(GetLocalRole() == ROLE_SimulatedProxy) {
		SimulateMove(ServerState.LastMove);
	}

	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
}

void AGoKart::SimulateMove(const FGoKartMove& Move) {
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;
	Force += GetAirResistance();
	Force += GetRollingResistance();
	FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove AGoKart::CreateMove(float DeltaTime) {
	FGoKartMove Move{};
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->TimeSeconds;

	return Move;
}

FString AGoKart::GetEnumText(ENetRole NetRole) {
	switch(NetRole) {
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy: 
		return "SimulatedProxy";
	case ROLE_AutonomousProxy: 
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	default:
		return "Error";;
	}
}

void AGoKart::OnRep_ServerState() {
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgedMoves(ServerState.LastMove);

	for(const FGoKartMove& Move : UnacknowledgedMoves) {
		SimulateMove(Move);
	}
}

void AGoKart::ClearAcknowledgedMoves(FGoKartMove& LastAcknowledgedMove) {
	TArray<FGoKartMove> NewMoves;

	for(const FGoKartMove& Move: UnacknowledgedMoves) {
		if(Move.Time > LastAcknowledgedMove.Time)
			NewMoves.Add(Move);
	}

	UnacknowledgedMoves = NewMoves;
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

void AGoKart::ApplyRotation(float DeltaTime, float SteeringThrowParameter) {
	float RotationAngle = FMath::DegreesToRadians(MaxDegreesPerSecond * DeltaTime * SteeringThrowParameter);
	FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	FHitResult HitResult;
	AddActorWorldRotation(RotationDelta, true, &HitResult);
	if(HitResult.IsValidBlockingHit())
		SteeringThrowParameter = 0;

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
	Throttle = Value;
	
}

void AGoKart::MoveRight(float Value) {
	SteeringThrow = Value;
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move) {
	SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move) {
	return true;
}

