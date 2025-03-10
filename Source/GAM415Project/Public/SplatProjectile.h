#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SplatProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UMaterialInterface;
class UDecalComponent;

UCLASS()
class GAM415PROJECT_API ASplatProjectile : public AActor
{
    GENERATED_BODY()

public:
    ASplatProjectile();

protected:
    virtual void BeginPlay() override;

    // Collision component (root)
    UPROPERTY(VisibleAnywhere, Category = "Components")
    USphereComponent* CollisionSphere;

    // Visual mesh component
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* ProjectileMesh;

    // Movement component
    UPROPERTY(VisibleAnywhere, Category = "Components")
    UProjectileMovementComponent* ProjectileMovement;

    // Material instance for dynamic color changes
    UPROPERTY()
    UMaterialInstanceDynamic* ProjectileMaterial;

    // Decal materials and textures
    UPROPERTY(EditDefaultsOnly, Category = "Decal")
    UMaterialInterface* DecalMaterial;

    // Decal size
    UPROPERTY(EditDefaultsOnly, Category = "Decal")
    float DecalSize = 64.0f;

    // Decal lifetime (seconds before expiration)
    UPROPERTY(EditDefaultsOnly, Category = "Decal")
    float DecalLifetime = 10.0f;

    // Decal fade out length, starting from the end of its lifetime
    UPROPERTY(EditDefaultsOnly, Category = "Decal")
    float DecalFadeOutLength = 1.0f;

    // Array of splat textures, preferably a diverse selection
    UPROPERTY(EditDefaultsOnly, Category = "Decal")
    TArray<UTexture2D*> SplatTextures;

    // Color value for both projectile and decal
    FLinearColor ProjectileColor;

    // Processes collision hit
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
               UPrimitiveComponent* OtherComp, FVector NormalImpulse,
               const FHitResult& Hit);

    // Spawns decal to represent the splat
    void SpawnSplatDecal(const FHitResult& Hit);

    // Applies force to physics object
    void ApplyForce(UPrimitiveComponent* OtherComp);
};