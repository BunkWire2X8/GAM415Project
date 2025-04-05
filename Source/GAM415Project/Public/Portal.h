#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

class UStaticMeshComponent;
class USceneCaptureComponent2D;
class UMaterialInstanceDynamic;

UCLASS()
class GAM415PROJECT_API APortal : public AActor
{
    GENERATED_BODY()

public:
    APortal();

    /// | Portal Components | ///
    
    // Visual representation of the portal surface
    UPROPERTY(EditAnywhere, Category = "Portal")
    TObjectPtr<UStaticMeshComponent> PortalMesh;

    // Component that captures the scene for the portal's view
    UPROPERTY(EditAnywhere, Category = "Portal")
    TObjectPtr<USceneCaptureComponent2D> SceneCapture;

    /// | Portal Properties | ///
    
    // The paired portal that this portal connects to
    UPROPERTY(EditInstanceOnly, Category = "Portal")
    TSoftObjectPtr<APortal> LinkedPortal;

    // Enables view clipping through the portal plane to prevent seeing behind
    UPROPERTY(EditInstanceOnly, Category = "Portal")
    bool bShouldClipPlane = true;

    /// | Portal Functionality | ///
    
    // Sets the linked portal and establishes bidirectional relationship
    UFUNCTION(BlueprintCallable, Category = "Portal")
    void SetLinkedPortal(APortal* NewPortal);

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

private:
    /// | Dynamic material instance for the portal surface | ///
    UPROPERTY()
    UMaterialInstanceDynamic* PortalMaterialInstance;

    /// | Teleportation System | ///
    
    // Handles overlap events and initiates teleportation
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                        bool bFromSweep, const FHitResult& SweepResult);

    // Main teleportation logic controller
    void HandleTeleportation(APawn* Pawn);
    
    // Calculates new position after teleportation
    FVector CalculateTeleportLocation(const AActor* TeleportingActor) const;
    
    // Calculates new rotation after teleportation
    FRotator CalculateTeleportRotation(const AActor* TeleportingActor) const;
    
    // Adjusts controller rotation to maintain view consistency
    FRotator CalculateControllerRotation(const AController* Controller) const;
    
    // Transforms velocity through portal connection
    FVector CalculateTeleportVelocity(const FVector& OldVelocity) const;
    
    // Gets current velocity from movement component or physics
    FVector GetTeleportVelocity(const APawn* Pawn) const;
    
    // Determines if actor approached from front-facing direction
    bool IsFrontFacing(const FVector& ImpactNormal) const;
    
    // Applies transformed velocity to pawn
    void ApplyTeleportVelocity(APawn* Pawn, const FVector& NewVelocity) const;

    /// | Scene Capture System | ///
    
    // Initializes scene capture components and render target
    void InitializeSceneCapture();
    
    // Updates capture transform to match player perspective
    void UpdateSceneCapture();
    
    // Configures clipping plane for portal view
    void ConfigureClipPlane();
    
    // Adjusts render target to match viewport resolution
    void UpdateRenderTargetSize();
    
    // Positions scene capture to simulate linked portal view
    void UpdateSceneCaptureTransform(const FVector& CameraLocation,
                                     const FRotator& CameraRotation,
                                     float CameraFOV);
    
    // Gets current viewport resolution
    FVector2D GetViewportSize() const;
    
    // Initializes portal surface material
    void InitializePortalMaterial();

    /// | Spatial Transformation Helpers | ///
    
    // Transforms position between portal coordinate spaces
    static FVector TransformPositionBetweenPortals(const FTransform& Source,
                                                   const FTransform& Target,
                                                   const FVector& Position);
    
    // Transforms rotation between portal coordinate spaces
    static FRotator TransformRotationBetweenPortals(const FTransform& Source,
                                                    const FTransform& Target,
                                                    const FRotator& Rotation);
    
    // Transforms velocity vector between portal coordinate spaces
    static FVector TransformVelocityBetweenPortals(const FTransform& Source,
                                                   const FTransform& Target,
                                                   const FVector& Velocity);

    /// | Teleportation State Management | ///
    
    // Actors currently ignoring portal collisions
    TArray<TWeakObjectPtr<AActor>> IgnoredActors;
    
    // Timer handle for teleportation cooldown
    FTimerHandle IgnoreTimerHandle;
};