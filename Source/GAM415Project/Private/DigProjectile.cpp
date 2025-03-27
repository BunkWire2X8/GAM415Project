#include "DigProjectile.h"
#include "ProceduralTerrain.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"

ADigProjectile::ADigProjectile()
{
    PrimaryActorTick.bCanEverTick = false; // This actor does not need ticking
}

void ADigProjectile::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    // Store the transform used when constructing the actor for later use.
    SpawnTransform = Transform;
}

void ADigProjectile::BeginPlay()
{
    Super::BeginPlay();

    // Extract the location from the stored spawn transform.
    FVector SpawnLocation = SpawnTransform.GetLocation();
    // Get the forward direction from the stored rotation.
    FVector SpawnDirection = SpawnTransform.GetRotation().GetForwardVector();

    // Fire a line trace from the spawn location in the forward direction.
    Fire(SpawnLocation, SpawnDirection);
    // Immediately destroy the actor after firing.
    Destroy();
}

// Performs a line trace to detect collisions and modify terrain if applicable.
void ADigProjectile::Fire(const FVector& StartLocation, const FVector& Direction)
{
    // Calculate the end point of the trace based on the maximum distance.
    const FVector EndLocation = StartLocation + Direction * MaxDistance;
    FHitResult HitResult;
    FCollisionQueryParams CollisionParams;

    // Ensure the trace ignores the projectile itself.
    CollisionParams.AddIgnoredActor(this);
    bool bBlockingHitIsFound = false;

    // Perform the line trace along the specified channel (static world objects).
    if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation,
                                             ECC_WorldStatic, CollisionParams))
    {
        bBlockingHitIsFound = true;
        // Check if the hit actor is of type AProceduralTerrain.
        if (AProceduralTerrain* Terrain = Cast<AProceduralTerrain>(HitResult.GetActor()))
        {
            // Modify the terrain at the hit location using specified radius and strength.
            Terrain->ModifyTerrainAtLocation(HitResult.Location, DigRadius, DigStrength);
            // Draw a green debug sphere to visualize the radius.
            DrawDebugSphere(GetWorld(), HitResult.Location, DigRadius, 12, FColor::Green, false, 1.0f);
        }
    }

    // Determine the endpoint for the debug line: either the hit location or the full trace length.
    FVector LineStop = bBlockingHitIsFound ? HitResult.Location : EndLocation;
    // Draw a red debug line representing the trace.
    DrawDebugLine(GetWorld(), StartLocation, LineStop, FColor::Red, false, 1.0f, 0, 2.0f);
}
