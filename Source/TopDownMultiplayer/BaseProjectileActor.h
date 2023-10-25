// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseProjectileActor.generated.h"

UCLASS()
class TOPDOWNMULTIPLAYER_API ABaseProjectileActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseProjectileActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//Overriding default unreal destroyed function
	virtual void Destroyed() override;

	UFUNCTION(Category = "Projectile")
	void OnProjectileImpact(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* Othercomp, FVector NormalImpulse, const FHitResult& Hit);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* SphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere, Category = "Components")
	class UParticleSystem* ExplosionParticle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<class UDamageType> DamageType;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	float Damage;

	UPROPERTY(EditAnywhere, Category = "Damage")
	float damageNumber = 35.f;
};
