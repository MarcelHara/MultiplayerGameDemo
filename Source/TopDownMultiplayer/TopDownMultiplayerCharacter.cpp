// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownMultiplayerCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "BaseProjectileActor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ATopDownMultiplayerCharacter::ATopDownMultiplayerCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	//Health
	MaxHealth = 100;
	CurrentHealth = MaxHealth;

	//FireRate
	FireRate = 0.25f;
	bIsFiringWeapon = false;
}

void ATopDownMultiplayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void ATopDownMultiplayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	World = GetWorld();
	
	if (ProjectileClass == nullptr) return;
}

void ATopDownMultiplayerCharacter::SetCurrentHealth(float healthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
		OnHealthUpdated();
	}
}

float ATopDownMultiplayerCharacter::TakeDamage(float DamageTaken, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(damageApplied);
	return damageApplied;
}


void ATopDownMultiplayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//This is how we add the variable to be replicated:

	//Replicating CurrentHealth var
	DOREPLIFETIME(ATopDownMultiplayerCharacter, CurrentHealth);
}

void ATopDownMultiplayerCharacter::OnHealthUpdated()
{
	//Client func
	if (IsLocallyControlled())
	{
		FString healthMsg = FString::Printf(TEXT("You now have %d health remaining"), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMsg);

		if (CurrentHealth <= 0)
		{
			FString deathMsg = FString::Printf(TEXT("You have died"));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMsg);
		}
	}

	//Server func
	//You have different Roles you can choose such as autonimousProxy
	if (GetLocalRole() == ROLE_Authority)
	{
		FString healthMessage = FString::Printf(TEXT("%s now has %d health remaining."), *GetFName().ToString(), CurrentHealth);//Gets the name of this object. (player name)
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
	}
}

void ATopDownMultiplayerCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdated();
}

void ATopDownMultiplayerCharacter::StartFire()
{
	UE_LOG(LogTemp, Display, TEXT("StartFire Called"));
	UE_LOG(LogTemp, Display, TEXT("BisFiring:   %hhd"), bIsFiringWeapon);
	if (!bIsFiringWeapon)
	{
		bIsFiringWeapon = true;
		World = GetWorld();
		World->GetTimerManager().SetTimer(FiringTimer, this, &ATopDownMultiplayerCharacter::StopFire, FireRate, false);
		HandleFire();
	}
}

void ATopDownMultiplayerCharacter::StartDash()
{
	if(!bIsDashing)
	{
		bIsDashing = true;
		World->GetTimerManager().SetTimer(DashTimer, this, &ATopDownMultiplayerCharacter::StopDash, 2.5f, false);
		UE_LOG(LogTemp, Display, TEXT("Start Dash executed"));
		HandleDash();
	}
}

void ATopDownMultiplayerCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void ATopDownMultiplayerCharacter::StopDash()
{
	bIsDashing = false;
}

void ATopDownMultiplayerCharacter::HandleDash_Implementation()
{
	FString dashMsg = FString::Printf(TEXT("Player Has Dashed"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, dashMsg);
	
	UCharacterMovementComponent* CharMovementComp = Cast<UCharacterMovementComponent>(this->GetCharacterMovement());
	FVector impulse = this->GetActorForwardVector();
	CharMovementComp->Launch(impulse * 2500);
}

//Only called on Server- Reliable RPC
//_Implementation is unreals way of saying RPC
void ATopDownMultiplayerCharacter::HandleFire_Implementation()
{
	FVector spawnLocation = GetActorLocation() + (GetActorRotation().Vector() * 100.0f) + (GetActorUpVector() * 50.0f);
	FRotator spawnRotation = GetActorRotation();

	FActorSpawnParameters spawnParameters;
	//Getting whoever called this
	spawnParameters.Instigator = GetInstigator();
	//Whoever spawned it IE the owner so the player that pressed the button
	spawnParameters.Owner = this;
	
	ABaseProjectileActor* spawnedProjectile = GetWorld()->SpawnActor<ABaseProjectileActor>(ProjectileClass, spawnLocation, spawnRotation, spawnParameters);
}
