// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TopDownMultiplayerCharacter.generated.h"

UCLASS(Blueprintable)
class ATopDownMultiplayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATopDownMultiplayerCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;

	/** Returns TopDownCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	//Blueprint Pure means it only returns a value and you cannot set this in blueprints
	UFUNCTION(BlueprintPure, Category = "Health")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetCurrentHealth(float healthValue);

	//Unreals TakeDamage event
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	//Starting firing of projectile
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StartDash();

	bool bIsFiringWeapon;
	bool bIsDashing;

	UPROPERTY(EditAnywhere, Category = "Gameplay")
	TSubclassOf<class ABaseProjectileActor> ProjectileClass;
	
private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UWorld* World;

protected:
	
	//This will never change so dont give access to replicate
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	int MaxHealth;

	//This is how we choose we are replacating our currentHealth (sending to all clients etc)
	//When this variable gets changed it will notifty OnRep_CurrentHealth()func
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_CurrentHealth)
	int CurrentHealth;

	//Rep notify is triggered when a client successfully recieves the replicated data.
	
	//RepNotify for current health changes
	UFUNCTION()
	void OnRep_CurrentHealth();

	//Needed to replicate variables:
	//Unreal engines own function so we need to override it
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void OnHealthUpdated();

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	float FireRate;

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StopDash();

	//Server handling the spawning of projectiles
	//If a client tries to call this it will activate on the Host Instead of the client
	//Reliable puts this RPC into a queue to makesure it gets activated
	//If queue gets too full it can disconnect the user so make sure it gets deleted out of the queue when done
	UFUNCTION(Server, Reliable)
	void HandleFire();

	UFUNCTION(Server, Reliable)
	void HandleDash();

	//Timer to stop spamming of projectiles so server doesnt die
	FTimerHandle FiringTimer;
	FTimerHandle DashTimer;
};

