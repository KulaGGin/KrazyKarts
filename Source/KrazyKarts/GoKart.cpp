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

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>("MovementComponent");
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

	if(!MovementComponent) return;

	if(GetLocalRole() == ROLE_AutonomousProxy) {
		auto Move = MovementComponent->CreateMove(DeltaTime);
		MovementComponent->SimulateMove(Move);
		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move);
	}

	// We are the server and in control of the pawn
	if(GetLocalRole() == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy) {
		auto Move = MovementComponent->CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if(GetLocalRole() == ROLE_SimulatedProxy) {
		MovementComponent->SimulateMove(ServerState.LastMove);
	}

	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(GetLocalRole()), this, FColor::White, DeltaTime);
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
	if(!MovementComponent) return;

	SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);

	for(const FGoKartMove& Move : UnacknowledgedMoves) {
		MovementComponent->SimulateMove(Move);
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

void AGoKart::MoveForward(float Value) {
	if(!MovementComponent) return;
	MovementComponent->SetThrottle(Value);
	
}

void AGoKart::MoveRight(float Value) {
	if(!MovementComponent) return;
	MovementComponent->SetSteeringThrow(Value);
}

void AGoKart::Server_SendMove_Implementation(FGoKartMove Move) {
	if(!MovementComponent) return;
	MovementComponent->SimulateMove(Move);

	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

bool AGoKart::Server_SendMove_Validate(FGoKartMove Move) {
	return true;
}

