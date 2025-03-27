#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DigProjectile.generated.h"

// Actor class that fires a trace that can "dig" through an AProceduralTerrain instance.
UCLASS()
class GAM415PROJECT_API ADigProjectile : public AActor
{
    GENERATED_BODY()

public:
    ADigProjectile();

    virtual void OnConstruction(const FTransform& Transform) override;

    // The radius around the hit point where terrain will be modified.
    UPROPERTY(EditAnywhere, Category = "Digging")
    float DigRadius = 200.0f;

    // The strength with which the terrain is modified (e.g., how much material is removed).
    UPROPERTY(EditAnywhere, Category = "Digging")
    float DigStrength = 125.0f;

    // Maximum distance the projectile's trace will cover.
    UPROPERTY(EditAnywhere, Category = "Digging")
    float MaxDistance = 10000.0f;

    // "Fires" the projectile by doing a line trace from a start location in a specified direction.
    void Fire(const FVector& StartLocation, const FVector& Direction);

protected:
    virtual void BeginPlay() override;

    // Stores the transform (position, rotation, scale) where the actor was spawned.
    FTransform SpawnTransform = FTransform::Identity;
};
