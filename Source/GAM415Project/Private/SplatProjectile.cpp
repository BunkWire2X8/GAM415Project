#include "SplatProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

ASplatProjectile::ASplatProjectile()
{
    PrimaryActorTick.bCanEverTick = false;

    // Collision setup (root component)
    CollisionSphere = CreateDefaultSubobject<USphereComponent>("CollisionSphere");
    RootComponent = CollisionSphere;
    CollisionSphere->InitSphereRadius(10.0f);
    CollisionSphere->SetCollisionProfileName("Projectile");

    // Mesh setup (child of collision sphere)
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("ProjectileMesh");
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Mesh doesn't need collision

    // Set collision for hit events
    SetActorEnableCollision(true);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionSphere->SetCollisionObjectType(ECC_GameTraceChannel1); // Projectile Channel
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    CollisionSphere->SetNotifyRigidBodyCollision(true);
    
    // Add projectile movement component
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
    ProjectileMovement->UpdatedComponent = RootComponent;
    ProjectileMovement->InitialSpeed = 3000.f;
    ProjectileMovement->MaxSpeed = 3000.f;
    ProjectileMovement->bShouldBounce = false;
}

void ASplatProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Create dynamic material instance
    if (UMaterialInterface* BaseMaterial = ProjectileMesh->GetMaterial(0))
    {
        ProjectileMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        ProjectileMesh->SetMaterial(0, ProjectileMaterial);

        // Generate random color from HSV8 for a random vibrant color
        ProjectileColor = FLinearColor::MakeRandomColor();
        ProjectileColor.A = 1.0f;

        // Set color material parameter
        ProjectileMaterial->SetVectorParameterValue("Color", ProjectileColor);
    }

    // Assign hit event
    CollisionSphere->OnComponentHit.AddDynamic(this, &ASplatProjectile::OnHit);
}

void ASplatProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
                             UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                             const FHitResult& Hit)
{
    if (OtherComp)
    {
        // If the collided object is a static mesh, apply the decal
        if (OtherComp->GetCollisionObjectType() == ECC_WorldStatic)
        {
            SpawnSplatDecal(Hit);
        }
        // If it's instead a physics object, just apply force to it.
        else if (OtherComp->IsSimulatingPhysics())
        {
            ApplyForce(OtherComp);
        }
    }

    SpawnEffect(Hit);
    Destroy();
}

void ASplatProjectile::ApplyForce(UPrimitiveComponent* OtherComp)
{
    OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
}

void ASplatProjectile::SpawnEffect(const FHitResult& Hit)
{
    if (!NiagaraSplatEffect) return;

    UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(),
        NiagaraSplatEffect,
        Hit.ImpactPoint,
        Hit.ImpactNormal.Rotation(),
        FVector::OneVector,
        true // Auto destroy
    );

    if (!NiagaraComp) return;

    // Set color parameter
    NiagaraComp->SetVariableLinearColor("Color", ProjectileColor);
}

void ASplatProjectile::SpawnSplatDecal(const FHitResult& Hit)
{
    if (!DecalMaterial || SplatTextures.IsEmpty()) return;

    // Spawn decal at the impact location
    UDecalComponent* Decal = UGameplayStatics::SpawnDecalAtLocation(
        GetWorld(),
        DecalMaterial,
        FVector(DecalSize),
        Hit.ImpactPoint,
        Hit.ImpactNormal.Rotation(),
        DecalLifetime
    );

    if (!Decal) return;

    // For fade effect in decal material
    if (DecalLifetime != 0)
        Decal->SetFadeOut(DecalLifetime - DecalFadeOutLength, DecalFadeOutLength);

    // Get random splat texture
    UTexture2D* RandomTexture = SplatTextures[FMath::RandRange(0, SplatTextures.Num() - 1)];

    // Configure decal material
    UMaterialInstanceDynamic* DecalMaterialInstance = Decal->CreateDynamicMaterialInstance();
    DecalMaterialInstance->SetTextureParameterValue("Texture", RandomTexture);
    DecalMaterialInstance->SetVectorParameterValue("Color", ProjectileColor);
}