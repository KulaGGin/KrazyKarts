// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartMovementReplicator.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UGoKartMovementReplicator::UGoKartMovementReplicator() {
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}

// Called when the game starts
void UGoKartMovementReplicator::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}

// Called every frame
void UGoKartMovementReplicator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if(!MovementComponent) return;

	auto LastMove = MovementComponent->GetLastMove();
	if(GetOwnerRole() == ROLE_AutonomousProxy) {
		UnacknowledgedMoves.Add(LastMove);
		Server_SendMove(LastMove);
	}

	// We are the server and in control of the pawn
	if(GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy) {
		UpdateServerState(LastMove);
	}

	if(GetOwnerRole() == ROLE_SimulatedProxy) {
		MovementComponent->SimulateMove(ServerState.LastMove);
	}

	// ...
}

void UGoKartMovementReplicator::UpdateServerState(const FGoKartMove& Move) {
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}


void UGoKartMovementReplicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartMovementReplicator, ServerState);
}

void UGoKartMovementReplicator::ClearAcknowledgedMoves(FGoKartMove& LastAcknowledgedMove) {
	TArray<FGoKartMove> NewMoves;

	for(const FGoKartMove& Move : UnacknowledgedMoves) {
		if(Move.Time > LastAcknowledgedMove.Time)
			NewMoves.Add(Move);
	}

	UnacknowledgedMoves = NewMoves;
}



void UGoKartMovementReplicator::Server_SendMove_Implementation(FGoKartMove Move) {
	if(!MovementComponent) return;
	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);
}

bool UGoKartMovementReplicator::Server_SendMove_Validate(FGoKartMove Move) {
	return true;
}

void UGoKartMovementReplicator::OnRep_ServerState() {
	if(!MovementComponent) return;

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);

	for(const FGoKartMove& Move : UnacknowledgedMoves) {
		MovementComponent->SimulateMove(Move);
	}
}