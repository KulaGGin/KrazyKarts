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
    AActor::SetReplicateMovement(false);

	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>("MovementComponent");
	MovementReplicator = CreateDefaultSubobject<UGoKartMovementReplicator>("MovementReplicator");
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

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

void AGoKart::MoveForward(float Value) {
	if(!MovementComponent) return;
	MovementComponent->SetThrottle(Value);
	
}

void AGoKart::MoveRight(float Value) {
	if(!MovementComponent) return;
	MovementComponent->SetSteeringThrow(Value);
}



