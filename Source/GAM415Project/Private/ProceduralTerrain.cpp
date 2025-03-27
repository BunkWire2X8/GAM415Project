#include "ProceduralTerrain.h"
#include "Kismet/GameplayStatics.h"

AProceduralTerrain::AProceduralTerrain()
{
    // Create a basic scene component as the root.
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // Create a single procedural mesh component and attach it to the root.
    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
    ProceduralMesh->SetupAttachment(RootComponent);

    PrimaryActorTick.bCanEverTick = false; // This actor does not need ticking
}

void AProceduralTerrain::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    GenerateTerrain();
}

// Generates the terrain by creating mesh sections for each chunk.
void AProceduralTerrain::GenerateTerrain()
{
    // Clear existing mesh sections and chunk data.
    ClearChunks();

    // Calculate the world size of a single chunk based on grid spacing.
    const float ChunkWorldSize = (ChunkSize - 1) * Scale;

    // Determine how many chunks are needed along X and Y.
    const FIntPoint NumChunks(FMath::CeilToInt(XSize / ChunkWorldSize),
                              FMath::CeilToInt(YSize / ChunkWorldSize));

    // Compute total world dimensions and half sizes.
    const FVector2D TotalWorldSize = NumChunks * ChunkWorldSize;
    const FVector2D HalfWorldSize = TotalWorldSize / 2;

    // Reserve memory for chunk data.
    Chunks.Reserve(NumChunks.X * NumChunks.Y);

    int32 SectionIndex = 0;
    // Loop through each grid coordinate and generate a chunk section.
    for (int32 x = 0; x < NumChunks.X; x++)
    {
        for (int32 y = 0; y < NumChunks.Y; y++)
        {
            FChunkData NewChunkData;
            NewChunkData.SectionIndex = SectionIndex;

            // Calculate the chunk's center position in actor-local space.
            const FVector ChunkCenter = CalculateChunkCenter(x, y, HalfWorldSize.X, HalfWorldSize.Y, ChunkWorldSize);

            // Containers for mesh data.
            TArray<FVector> Vertices;
            TArray<FVector2D> UVs;
            TArray<int32> Triangles;

            // Generate vertices, UVs, and triangles.
            // The vertices will be offset by ChunkCenter so that each chunk is in its own location.
            GenerateMeshData(ChunkCenter, Vertices, UVs, Triangles);

            // Calculate normals for proper lighting.
            TArray<FVector> Normals;
            CalculateNormals(Vertices, Triangles, Normals);

            // Create the mesh section for this chunk.
            ProceduralMesh->CreateMeshSection_LinearColor(SectionIndex, Vertices, Triangles, Normals, UVs, 
                                                          TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);

            // Set the material if provided.
            if (TerrainMaterial)
            {
                ProceduralMesh->SetMaterial(SectionIndex, TerrainMaterial);
            }

            // Save the generated chunk data for later use (e.g., for modifying terrain)
            const float HalfChunkSize = ChunkWorldSize * 0.5f;
            NewChunkData.MinBounds = FVector2D(ChunkCenter.X - HalfChunkSize, ChunkCenter.Y - HalfChunkSize);
            NewChunkData.MaxBounds = FVector2D(ChunkCenter.X + HalfChunkSize, ChunkCenter.Y + HalfChunkSize);
            NewChunkData.Vertices = Vertices;
            NewChunkData.Triangles = Triangles;
            Chunks.Add(NewChunkData);

            SectionIndex++;
        }
    }
}

// Clears all mesh sections and resets chunk data.
void AProceduralTerrain::ClearChunks()
{
    if (ProceduralMesh)
    {
        ProceduralMesh->ClearAllMeshSections();
    }
    Chunks.Empty();
}

// Calculates the center position of a chunk in actor-local space based on grid coordinates.
// Offsets the chunk by half its size so that the grid is centered.
FVector AProceduralTerrain::CalculateChunkCenter(int32 ChunkX, int32 ChunkY, float HalfWorldSizeX, float HalfWorldSizeY, float ChunkWorldSize) const
{
    return FVector(-HalfWorldSizeX + (ChunkX * ChunkWorldSize) + (ChunkWorldSize * 0.5f),
                   -HalfWorldSizeY + (ChunkY * ChunkWorldSize) + (ChunkWorldSize * 0.5f),
                   0);
}

// Generates geometry for a single chunk: vertices, UVs, and triangles.
void AProceduralTerrain::GenerateMeshData(const FVector& ChunkCenter, TArray<FVector>& Vertices, TArray<FVector2D>& UVs, TArray<int32>& Triangles) const
{
    const int32 TotalVertices = ChunkSize * ChunkSize;
    Vertices.Reserve(TotalVertices);
    UVs.Reserve(TotalVertices);

    const int32 TotalQuads = (ChunkSize - 1) * (ChunkSize - 1);
    Triangles.Reserve(TotalQuads * 6);

    // Full size of the chunk in world units.
    const float FullChunkSize = (ChunkSize - 1) * Scale;
    
    // Local offset to center the grid on (0,0).
    const FVector GridOffset = FVector(FullChunkSize * 0.5f, FullChunkSize * 0.5f, 0);

    // Compute inverse chunk size for UV mapping
    const float InvChunkSize = 1.0f / (ChunkSize - 1);

    // Loop through the grid.
    for (int32 x = 0; x < ChunkSize; x++)
    {
        // Compute the grid X position.
        const float GridX = x * Scale;

        // Compute local vertex X position relative to the chunk center.
        const float LocalPosX = GridX - GridOffset.X;

        // Compute final vertex X position in actor-local space.
        const float FinalPosX = LocalPosX + ChunkCenter.X;

        // Compute world X position by adding the actor's X location.
        const float WorldPosX = FinalPosX + GetActorLocation().X;

        // Compute the UV's X value
        const float UVValX = x * InvChunkSize;

        for (int32 y = 0; y < ChunkSize; y++)
        {
            // Compute the grid Y position.
            const float GridY = y * Scale;

            // Compute local vertex X position relative to the chunk center.
            const float LocalPosY = GridY - GridOffset.Y;

            // Compute final vertex Y position in actor-local space.
            const float FinalPosY = LocalPosY + ChunkCenter.Y;

            // Compute world Y position by adding the actor's Y location.
            const float WorldPosY = FinalPosY + GetActorLocation().Y;

            // Compute the UV's Y value
            const float UVValY = y * InvChunkSize;

            // Use world space to compute height using noise.
            const float FinalPosZ = GetHeightAtWorldPosition(WorldPosX, WorldPosY);

            Vertices.Add(FVector(FinalPosX, FinalPosY, FinalPosZ));
            UVs.Add(FVector2D(UVValX, UVValY));

            // Generate triangles (except on the boundary).
            if (x < ChunkSize - 1 && y < ChunkSize - 1)
            {
                // Calculate vertex index in grid
                const int32 idx = x * ChunkSize + y;

                // Define two triangles per quad

                // First triangle
                Triangles.Add(idx);
                Triangles.Add(idx + 1);
                Triangles.Add(idx + ChunkSize + 1);

                // Second triangle
                Triangles.Add(idx + ChunkSize + 1);
                Triangles.Add(idx + ChunkSize);
                Triangles.Add(idx);
            }
        }
    }
}

// Uses Perlin noise to compute the height at a given world coordinate.
float AProceduralTerrain::GetHeightAtWorldPosition(float WorldX, float WorldY) const
{
    return FMath::PerlinNoise2D(FVector2D(WorldX, WorldY) * NoiseScale) * HeightScale;
}

// Calculates normals for proper lighting based on vertices and triangle indices.
void AProceduralTerrain::CalculateNormals(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, TArray<FVector>& Normals) const
{
    // Initialize normals array with zeros
    Normals.SetNumZeroed(Vertices.Num());

    // Iterate over triangles (each triangle uses 3 consecutive indices)
    for (int32 i = 0; i < Triangles.Num(); i += 3)
    {
        const int32 i0 = Triangles[i];
        const int32 i1 = Triangles[i + 1];
        const int32 i2 = Triangles[i + 2];

        // Compute two edges of the triangle
        const FVector Edge1 = Vertices[i1] - Vertices[i0];
        const FVector Edge2 = Vertices[i2] - Vertices[i0];
        // Calculate face normal (cross product) and normalize it
        const FVector FaceNormal = Edge1.Cross(Edge2).GetSafeNormal();

        // Add the face normal to each vertex normal
        Normals[i0] += FaceNormal;
        Normals[i1] += FaceNormal;
        Normals[i2] += FaceNormal;
    }

    // Normalize each vertex normal to ensure proper lighting calculations
    for (FVector& Normal : Normals)
    {
        Normal.Normalize();
    }
}

// Modifies the terrain at a specific location (e.g., "digging") by lowering vertex heights.
void AProceduralTerrain::ModifyTerrainAtLocation(const FVector& DigLocation, float DigRadius, float DigStrength)
{
    // Pre-calculate the squared radius
    const float DigRadiusSq = FMath::Square(DigRadius);

    // Iterate through all terrain chunks.
    // There are probably more optimized ways to check if the radius hits certain chunks,
    // but I've already worked on this system for a tad too long than I probably should have.
    for (FChunkData& Chunk : Chunks)
    {
        // AABB check: Expand chunk bounds by the dig radius and check if dig location is inside.
        if (DigLocation.X < Chunk.MinBounds.X - DigRadius ||
            DigLocation.X > Chunk.MaxBounds.X + DigRadius ||
            DigLocation.Y < Chunk.MinBounds.Y - DigRadius ||
            DigLocation.Y > Chunk.MaxBounds.Y + DigRadius)
        {
            continue;
        }

        // Precise distance check: find the closest point in the chunk's bounds to the dig location
        const float ClampedX = FMath::Clamp(DigLocation.X, Chunk.MinBounds.X, Chunk.MaxBounds.X);
        const float ClampedY = FMath::Clamp(DigLocation.Y, Chunk.MinBounds.Y, Chunk.MaxBounds.Y);
        const float DistanceSq = FVector::DistSquared2D(DigLocation, FVector(ClampedX, ClampedY, 0));

        if (DistanceSq > DigRadiusSq) continue;

        // Flag to indicate if any vertex has been modified (to update the mesh later)
        bool bModified = false;
        // Reconstruct the chunk center from its bounds.
        FVector ChunkCenter = FVector((Chunk.MinBounds.X + Chunk.MaxBounds.X) * 0.5f,
                                      (Chunk.MinBounds.Y + Chunk.MaxBounds.Y) * 0.5f, 0);

        // Iterate through all vertices in the chunk
        for (int32 i = 0; i < Chunk.Vertices.Num(); ++i)
        {
            // Compute the squared distance from the vertex to the dig location
            const float DistSq = FVector::DistSquared2D(Chunk.Vertices[i], DigLocation);

            // If within the dig radius, modify the vertex’s Z coordinate (height)
            if (DistSq <= DigRadiusSq)
            {
                const float Distance = FMath::Sqrt(DistSq);
                // Calculate influence: vertices closer to the center are modified more strongly
                const float Influence = FMath::Clamp(1.0f - (Distance / DigRadius), 0.0f, 1.0f);
                Chunk.Vertices[i].Z -= DigStrength * Influence;
                bModified = true;
            }
        }

        // If any vertices were modified, recalculate normals and update the mesh section.
        if (bModified)
        {
            TArray<FVector> Normals;
            CalculateNormals(Chunk.Vertices, Chunk.Triangles, Normals);
            // Update the mesh section the new vertex positions and normals
            ProceduralMesh->UpdateMeshSection_LinearColor(Chunk.SectionIndex,
                                                          Chunk.Vertices,
                                                          Normals, {}, {}, {});
        }
    }
}
