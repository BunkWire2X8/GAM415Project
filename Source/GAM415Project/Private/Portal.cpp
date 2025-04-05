#include "Portal.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

APortal::APortal()
{
    PrimaryActorTick.bCanEverTick = true;

    // Root component for hierarchy organization
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Portal visualization setup
    PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
    PortalMesh->SetupAttachment(RootComponent);
    PortalMesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    PortalMesh->CastShadow = false;

    // Scene capture setup
    SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
    SceneCapture->SetupAttachment(RootComponent);
}

void APortal::BeginPlay()
{
    Super::BeginPlay();

    InitializePortalMaterial();
    PortalMesh->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);
    InitializeSceneCapture();
}

// Frame update: Maintains portal view rendering
void APortal::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (LinkedPortal.IsValid())
    {
        UpdateRenderTargetSize();
        UpdateSceneCapture();
    }
}

/// | Teleportation System Implementation | ///

// Handles overlap events with portal surface
void APortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
                             bool bFromSweep, const FHitResult& SweepResult)
{
    if (!LinkedPortal.IsValid()
     || !OtherActor->IsA(APawn::StaticClass())
     || IgnoredActors.Contains(OtherActor))
        return;

    APawn* Pawn = Cast<APawn>(OtherActor);
    if (Pawn && Pawn->GetController() && IsFrontFacing(SweepResult.ImpactNormal))
        HandleTeleportation(Pawn);
}

// Determines if impact occurred on front-facing side of portal
bool APortal::IsFrontFacing(const FVector& ImpactNormal) const
{
    // Threshold for front-facing detection (70% alignment)
    return FVector::DotProduct(ImpactNormal, GetActorForwardVector()) > 0.7f;
}

// Main teleportation sequence controller
void APortal::HandleTeleportation(APawn* Pawn)
{
    if (!LinkedPortal.IsValid()) return;

    // Add to teleport cooldown list
    IgnoredActors.Add(Pawn);
    LinkedPortal->IgnoredActors.Add(Pawn);
    
    // Cooldown timer setup
    FTimerDelegate TimerDelegate;
    TimerDelegate.BindLambda([this, Pawn]()
    {
        IgnoredActors.Remove(Pawn);
        if (LinkedPortal.IsValid())
            LinkedPortal->IgnoredActors.Remove(Pawn);
    });
    GetWorld()->GetTimerManager().SetTimer(IgnoreTimerHandle, TimerDelegate, 0.2f, false);

    // Calculate transformed properties
    const FVector NewLocation = CalculateTeleportLocation(Pawn);
    const FRotator NewRotation = CalculateTeleportRotation(Pawn);
    const FRotator NewControlRotation = CalculateControllerRotation(Pawn->GetController());
    const FVector NewVelocity = CalculateTeleportVelocity(GetTeleportVelocity(Pawn));

    // Execute teleportation
    Pawn->TeleportTo(NewLocation, NewRotation);

    // Update controller orientation
    if (AController* Controller = Pawn->GetController())
        Controller->SetControlRotation(NewControlRotation);

    ApplyTeleportVelocity(Pawn, NewVelocity);
}

// Retrieves current velocity from appropriate component
FVector APortal::GetTeleportVelocity(const APawn* Pawn) const
{
    if (const UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
        return Movement->Velocity;

    if (UPrimitiveComponent* Physics = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
        return Physics->IsSimulatingPhysics() ? Physics->GetPhysicsLinearVelocity() : FVector::ZeroVector;

    return FVector::ZeroVector;
}

// Applies transformed velocity to target component
void APortal::ApplyTeleportVelocity(APawn* Pawn, const FVector& NewVelocity) const
{
    if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
        Movement->Velocity = NewVelocity;
    else if (UPrimitiveComponent* Physics = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
        if (Physics->IsSimulatingPhysics()) 
            Physics->SetPhysicsLinearVelocity(NewVelocity);
}

/// | Spatial Transformation Calculations | ///

FVector APortal::CalculateTeleportLocation(const AActor* TeleportingActor) const
{
    return TransformPositionBetweenPortals(GetActorTransform(), 
                                           LinkedPortal->GetActorTransform(),
                                           TeleportingActor->GetActorLocation());
}

FRotator APortal::CalculateTeleportRotation(const AActor* TeleportingActor) const
{
    return TransformRotationBetweenPortals(GetActorTransform(),
                                           LinkedPortal->GetActorTransform(),
                                           TeleportingActor->GetActorRotation());
}

FRotator APortal::CalculateControllerRotation(const AController* Controller) const
{
    return TransformRotationBetweenPortals(GetActorTransform(),
                                           LinkedPortal->GetActorTransform(),
                                           Controller->GetControlRotation());
}

FVector APortal::CalculateTeleportVelocity(const FVector& OldVelocity) const
{
    return TransformVelocityBetweenPortals(GetActorTransform(),
                                           LinkedPortal->GetActorTransform(),
                                           OldVelocity);
}

/// | Core Transformation Mathematics | ///

// Transforms position between portal spaces with coordinate system inversion
FVector APortal::TransformPositionBetweenPortals(const FTransform& Source, 
                                                 const FTransform& Target, 
                                                 const FVector& Position)
{
    // Convert to local space, mirror across Y axis, then convert to target space
    const FVector Local = Source.InverseTransformPosition(Position);
    return Target.TransformPosition(FVector(Local.X, -Local.Y, Local.Z));
}

// Transforms rotation between portal spaces with proper axis alignment
FRotator APortal::TransformRotationBetweenPortals(const FTransform& Source,
                                                  const FTransform& Target,
                                                  const FRotator& Rotation)
{
    // Convert to quaternions for proper rotation composition
    const FQuat Local = Source.GetRotation().Inverse() * FQuat(Rotation);
    const FQuat Mirrored = FQuat(FVector::UpVector, PI) * Local; // 180 degree Yaw rotation
    return (Target.GetRotation() * Mirrored).Rotator();
}

// Transforms velocity vector between portal spaces
FVector APortal::TransformVelocityBetweenPortals(const FTransform& Source,
                                                 const FTransform& Target,
                                                 const FVector& Velocity)
{
    // Convert velocity to local space, mirror direction, then convert to target space
    const FVector Local = Source.InverseTransformVector(Velocity);
    const FVector Mirrored = FQuat(FVector::UpVector, PI).RotateVector(Local);
    return Target.TransformVector(Mirrored);
}

/// | Scene Capture System Implementation | ///

// Initializes scene capture components and render target
void APortal::InitializeSceneCapture()
{
    if (!SceneCapture || !LinkedPortal.IsValid()) return;

    // Create dynamic render target
    SceneCapture->TextureTarget = NewObject<UTextureRenderTarget2D>();
    UpdateRenderTargetSize();
    ConfigureClipPlane();

    // Apply render texture to portal material
    if (PortalMaterialInstance)
        PortalMaterialInstance->SetTextureParameterValue("RenderTexture", SceneCapture->TextureTarget);

    // Hide portal actors from own capture
    SceneCapture->HiddenActors.Add(this);
    SceneCapture->HiddenActors.Add(LinkedPortal.Get());
}

// Adjusts render target resolution to match viewport
void APortal::UpdateRenderTargetSize()
{
    const FVector2D ViewportSize = GetViewportSize();
    UTextureRenderTarget2D* Target = SceneCapture->TextureTarget;

    if (Target && (Target->SizeX != ViewportSize.X || Target->SizeY != ViewportSize.Y))
    {
        Target->InitAutoFormat(ViewportSize.X, ViewportSize.Y);
        Target->UpdateResource();
    }
}

// Configures clipping plane to prevent seeing behind portal
void APortal::ConfigureClipPlane()
{
    if (!bShouldClipPlane) return;

    SceneCapture->bEnableClipPlane = true;
    SceneCapture->ClipPlaneBase = LinkedPortal->GetActorLocation();
    SceneCapture->ClipPlaneNormal = -LinkedPortal->GetActorForwardVector();
}

// Updates scene capture to match player perspective through portal
void APortal::UpdateSceneCapture()
{
    if (APlayerCameraManager* Camera = GetWorld()->GetFirstPlayerController()->PlayerCameraManager)
    {
        UpdateSceneCaptureTransform(Camera->GetCameraLocation(),
                                    Camera->GetCameraRotation(),
                                    Camera->GetFOVAngle());
    }
}

// Transforms camera perspective through portal connection
void APortal::UpdateSceneCaptureTransform(const FVector& CameraLocation,
                                          const FRotator& CameraRotation,
                                          const float CameraFOV)
{
    const FTransform Source = GetActorTransform();
    const FTransform Target = LinkedPortal->GetActorTransform();

    // Transform camera position through portal
    const FVector LocalCamera = Source.InverseTransformPosition(CameraLocation);
    FVector TransformedLocation = Target.TransformPosition(FVector(-LocalCamera.X,
                                                                   -LocalCamera.Y,
                                                                    LocalCamera.Z));

    // Calculate an FOV-adjusted vertical offset to prevent visual glitches when viewed from afar
    FVector2D ViewportSize = GetViewportSize();
    const float Distance = FVector::Distance(CameraLocation, GetActorLocation());
    const float PitchRadians = FMath::DegreesToRadians(CameraRotation.Pitch);
    const float AspectRatio = ViewportSize.X / FMath::Max(ViewportSize.Y, 1.0f);
    const float FOVRadians = FMath::DegreesToRadians(CameraFOV);

    const float VerticalOffset = (-Distance * FMath::Tan(PitchRadians)) /
                                 (AspectRatio * FMath::Tan(FOVRadians * 0.5f)) * 0.5f;

    TransformedLocation += LinkedPortal->GetActorUpVector() * VerticalOffset;

    // Apply transformed rotation
    const FRotator TransformedRotation = TransformRotationBetweenPortals(Source, Target, CameraRotation);
    SceneCapture->SetWorldLocationAndRotation(TransformedLocation, TransformedRotation);
}

// Gets current viewport resolution
FVector2D APortal::GetViewportSize() const
{
    FVector2D Size(1920, 1080);
    if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
    return Size;
}

/// | Material System | ///

// Initializes dynamic material instance for portal surface
void APortal::InitializePortalMaterial()
{
    if (UMaterialInterface* Material = PortalMesh->GetMaterial(0))
        PortalMaterialInstance = PortalMesh->CreateAndSetMaterialInstanceDynamic(0);
}

/// | Portal Linking System | ///

// Establishes bidirectional portal connection
void APortal::SetLinkedPortal(APortal* NewPortal)
{
    LinkedPortal = NewPortal;
    if (NewPortal && NewPortal->LinkedPortal != this) {
        NewPortal->SetLinkedPortal(this);
        InitializeSceneCapture();
    }
}