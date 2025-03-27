#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralTerrain.generated.h"

// Structure to store data for each terrain chunk section.
USTRUCT()
struct FChunkData
{
    GENERATED_BODY()

    // Section index in the procedural mesh component
    UPROPERTY()
    int32 SectionIndex = -1;

    // Vertex positions for the mesh section (local to the chunk center)
    UPROPERTY()
    TArray<FVector> Vertices;

    // Triangle indices defining mesh faces for this section
    UPROPERTY()
    TArray<int32> Triangles;

    // 2D bounds of the chunk (min corner in world coordinates)
    UPROPERTY()
    FVector2D MinBounds;

    // 2D bounds of the chunk (max corner in world coordinates)
    UPROPERTY()
    FVector2D MaxBounds;
};

// Actor class responsible for generating and managing procedural terrain using one procedural mesh component
UCLASS()
class GAM415PROJECT_API AProceduralTerrain : public AActor
{
    GENERATED_BODY()

public:
    AProceduralTerrain();

    // Called when properties are changed in editor or actor is spawned.
    virtual void OnConstruction(const FTransform& Transform) override;

    // Terrain parameters that define overall size and appearance.
    UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "1.0"))
    float XSize = 10000.0f;

    UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "1.0"))
    float YSize = 10000.0f;

    // Scale determines the spacing between vertices.
    UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "1.0"))
    float Scale = 100.0f;

    // Multiplier for the height obtained from noise.
    UPROPERTY(EditAnywhere, Category = "Terrain")
    float HeightScale = 500.0f;

    // Determines the frequency of the noise function.
    UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "0.0001"))
    float NoiseScale = 0.0005f;

    // Parameters for chunking the terrain.
    UPROPERTY(EditAnywhere, Category = "Chunking", meta = (ClampMin = "8"))
    int32 ChunkSize = 32;

    // Material used to render the terrain.
    UPROPERTY(EditAnywhere, Category = "Material")
    UMaterialInterface* TerrainMaterial;

    // Function callable to modify the terrain (e.g., for digging).
    UFUNCTION(BlueprintCallable, Category = "Terrain")
    void ModifyTerrainAtLocation(const FVector& DigLocation, float DigRadius = 200.0f, float DigStrength = 125.0f);

protected:
    // Generates the entire terrain. Called from OnConstruction.
    void GenerateTerrain();

    // Clears all current terrain chunk sections from the procedural mesh component.
    void ClearChunks();

private:
    // Single procedural mesh component used to hold all chunk sections.
    UPROPERTY()
    UProceduralMeshComponent* ProceduralMesh;

    // Array storing data for each generated chunk section.
    UPROPERTY()
    TArray<FChunkData> Chunks;

    // Helper function to determine height using Perlin noise.
    float GetHeightAtWorldPosition(float WorldX, float WorldY) const;

    // Helper function to calculate normals for proper lighting.
    void CalculateNormals(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, TArray<FVector>& Normals) const;

    // Calculates the chunk’s center position in world space based on grid coordinates.
    FVector CalculateChunkCenter(int32 ChunkX, int32 ChunkY, float HalfWorldSizeX, float HalfWorldSizeY, float ChunkWorldSize) const;

    // Generates mesh data (vertices, UVs, triangles) for a terrain chunk section.
    // The vertices are centered relative to the chunk center.
    void GenerateMeshData(const FVector& ChunkCenter, TArray<FVector>& Vertices, TArray<FVector2D>& UVs, TArray<int32>& Triangles) const;
};
