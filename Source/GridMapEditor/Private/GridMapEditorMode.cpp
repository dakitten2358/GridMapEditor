// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GridMapEditorMode.h"
#include "DrawDebugHelpers.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Framework/Commands/UICommandList.h"
#include "GridMapEditCommands.h"
#include "GridMapEditorModeToolkit.h"
#include "GridMapStaticMeshActor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TileSet.h"
#include "Toolkits/ToolkitManager.h"

static FName GridMapBrushHighlightColorParamName("HighlightColor");

const FEditorModeID FGridMapEditorMode::EM_GridMapEditorModeId = TEXT("EM_GridMapEditorMode");

FGridMapEditorMode::FGridMapEditorMode()
	: FEdMode()
{
	BrushDefaultHighlightColor = FColor(127, 127, 255, 255);
	BrushWarningHighlightColor = FColor::Red;

	// Load resources and construct brush component
	UStaticMesh* StaticMesh = nullptr;
	if (!IsRunningCommandlet())
	{
		UMaterial* BrushMaterial = LoadObject<UMaterial>(nullptr, TEXT("/GridMapEditor/M_TileBrushCube.M_TileBrushCube"), nullptr, LOAD_None, nullptr);
		BrushMID = UMaterialInstanceDynamic::Create(BrushMaterial, GetTransientPackage());
		check(BrushMID != nullptr);
		FLinearColor DefaultColor;
		BrushMID->GetVectorParameterDefaultValue(GridMapBrushHighlightColorParamName, DefaultColor);
		BrushDefaultHighlightColor = DefaultColor.ToFColor(false);
		StaticMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/GridMapEditor/SM_TileBrushCube.SM_TileBrushCube"), nullptr, LOAD_None, nullptr);
	}

	BrushCurrentHighlightColor = BrushDefaultHighlightColor;
	TileBrushComponent = NewObject<UStaticMeshComponent>(GetTransientPackage(), TEXT("TileBrushComponent"));
	TileBrushComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	TileBrushComponent->SetCollisionObjectType(ECC_WorldDynamic);
	TileBrushComponent->SetStaticMesh(StaticMesh);
	TileBrushComponent->SetMaterial(0, BrushMID);
	TileBrushComponent->SetAbsolute(true, true, true);
	TileBrushComponent->CastShadow = false;

	bBrushTraceValid = false;
	BrushLocation = FVector::ZeroVector;

	// Setup and bind commands
	UICommandList = MakeShareable(new FUICommandList);
	BindCommandList();
}

FGridMapEditorMode::~FGridMapEditorMode()
{

}

void FGridMapEditorMode::BindCommandList()
{
	const FGridMapEditCommands& Commands = FGridMapEditCommands::Get();

	UICommandList->MapAction(
		Commands.SetPaintTiles,
		FExecuteAction::CreateRaw(this, &FGridMapEditorMode::OnSetPaintTiles),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([=]
		{
			return UISettings.GetPaintToolSelected();
		}));

	UICommandList->MapAction(
		Commands.SetSelectTiles,
		FExecuteAction::CreateRaw(this, &FGridMapEditorMode::OnSetSelectTiles),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([=]
		{
			return UISettings.GetSelectToolSelected();
		}));

	UICommandList->MapAction(
		Commands.SetTileSettings,
		FExecuteAction::CreateRaw(this, &FGridMapEditorMode::OnSetTileSettings),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([=]
		{
			return UISettings.GetSettingsToolSelected();
		}));
}

void FGridMapEditorMode::Enter()
{
	FEdMode::Enter();

	// Clear any selections
	const bool bNoteSelectionChange(false);
	const bool bDeslectBSPSurfs(true);
	GEditor->SelectNone(bNoteSelectionChange, bDeslectBSPSurfs);

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FGridMapEditorModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}

	const bool bNewVisibility(true);
	TileBrushComponent->SetVisibility(bNewVisibility);
}

void FGridMapEditorMode::Exit()
{
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	// Remove the brush
	TileBrushComponent->UnregisterComponent();

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

void FGridMapEditorMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	if (!IsEditingEnabled())
		return;

	if (bBrushTraceValid)
	{
		FTransform BrushTransform = FTransform(FQuat::Identity, BrushLocation, FVector::OneVector);
		TileBrushComponent->SetRelativeTransform(BrushTransform);

		// warning color if we're erasing tiles
		FColor BrushHighlightColor = BrushDefaultHighlightColor;
		if (UISettings.GetPaintMode() == EGridMapPaintMode::Erase)
		{
			BrushHighlightColor = BrushWarningHighlightColor;
		}
		
		// adjust color if needed
		if (BrushCurrentHighlightColor != BrushHighlightColor)
		{
			BrushCurrentHighlightColor = BrushHighlightColor;
			BrushMID->SetVectorParameterValue(GridMapBrushHighlightColorParamName, BrushHighlightColor);
		}		

		if (!TileBrushComponent->IsRegistered())
		{
			TileBrushComponent->RegisterComponentWithWorld(ViewportClient->GetWorld());
		}
	}
	else
	{
		if (TileBrushComponent->IsRegistered())
		{
			TileBrushComponent->UnregisterComponent();
		}
	}

}

bool FGridMapEditorMode::UsesToolkits() const
{
	return true;
}

void FGridMapEditorMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdMode::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(TileBrushComponent);
}

bool FGridMapEditorMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	return FEdMode::StartTracking(InViewportClient, InViewport);
}

bool FGridMapEditorMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	return FEdMode::EndTracking(InViewportClient, InViewport);
}

bool FGridMapEditorMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY)
{
	if (IsEditingEnabled())
	{
		// Compute a world space ray from the screen space mouse coordinates
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			ViewportClient->Viewport,
			ViewportClient->GetScene(),
			ViewportClient->EngineShowFlags)
			.SetRealtimeUpdate(ViewportClient->IsRealtime()));

		FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
		FViewportCursorLocation MouseViewportRay(View, ViewportClient, MouseX, MouseY);
		BrushTraceDirection = MouseViewportRay.GetDirection();

		FVector BrushTraceStart = MouseViewportRay.GetOrigin();
		if (ViewportClient->IsOrtho())
		{
			BrushTraceStart += -WORLD_MAX * BrushTraceDirection;
		}

		GridMapBrushTrace(ViewportClient, BrushTraceStart, BrushTraceDirection);
	}
	return false;
}

bool FGridMapEditorMode::CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY)
{
	//Compute a world space ray from the screen space mouse coordinates
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		InViewportClient->Viewport,
		InViewportClient->GetScene(),
		InViewportClient->EngineShowFlags)
		.SetRealtimeUpdate(InViewportClient->IsRealtime()));

	FSceneView* View = InViewportClient->CalcSceneView(&ViewFamily);
	FViewportCursorLocation MouseViewportRay(View, InViewportClient, InMouseX, InMouseY);
	BrushTraceDirection = MouseViewportRay.GetDirection();

	FVector BrushTraceStart = MouseViewportRay.GetOrigin();
	if (InViewportClient->IsOrtho())
	{
		BrushTraceStart += -WORLD_MAX * BrushTraceDirection;
	}

	GridMapBrushTrace(InViewportClient, BrushTraceStart, BrushTraceDirection);

	PaintTile();
	return true;
}

void FGridMapEditorMode::PaintTile()
{
	// are we erasing?
	if (bIsPainting && bBrushTraceValid && UISettings.GetPaintMode() == EGridMapPaintMode::Erase && UISettings.GetCurrentTileSet().IsValid())
	{
		// and we're on top of something?
		if (BrushTraceHitActor.IsValid() && Cast<AGridMapStaticMeshActor>(BrushTraceHitActor.Get()))
		{
			EraseTile(Cast<AGridMapStaticMeshActor>(BrushTraceHitActor.Get()));
			BrushTraceHitActor.Reset();
		}
		return;
	}

	// if we're painting, 
	if (bIsPainting && bBrushTraceValid && UISettings.GetCurrentTileSet().IsValid())
	{
		UGridMapTileSet* TileSet = UISettings.GetCurrentTileSet().Get();

		// we're about to replace an actor
		if (BrushTraceHitActor.IsValid())
		{
			EraseTile(Cast<AGridMapStaticMeshActor>(BrushTraceHitActor.Get()));
			BrushTraceHitActor.Reset();
		}
		
		// figure out the bitmask for this tile
		uint32 Adjacency = GetTileAdjacencyBitmask(GetWorld(), BrushLocation, TileSet->Name);
		const FGridMapTileList* TileList = TileSet->FindTilesForAdjacency(Adjacency);
		if (TileList)
		{
			TAssetPtr<UStaticMesh> StaticMeshAsset = TileList->GetRandomTile();

			FActorSpawnParameters SpawnParameters;
			if (UISettings.GetHideOwnedActors())
			{
				SpawnParameters.bHideFromSceneOutliner = true;
			}
			AGridMapStaticMeshActor* MeshActor = GetWorld()->SpawnActor<AGridMapStaticMeshActor>(AGridMapStaticMeshActor::StaticClass(), SpawnParameters);
			MeshActor->TileSet = TileSet;

			// Rename the display name of the new actor in the editor to reflect the mesh that is being created from.
			FActorLabelUtilities::SetActorLabelUnique(MeshActor, TEXT("ActorLabelUnique"));

			UStaticMesh* StaticMesh = Cast<UStaticMesh>(StaticMeshAsset.LoadSynchronous());
			MeshActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
			MeshActor->ReregisterAllComponents();

			FTransform BrushTransform = FTransform(TileList->Rotation.Quaternion(), BrushLocation, FVector::OneVector);
			MeshActor->SetActorTransform(BrushTransform);

			// update adjacent tiles
			TArray<FAdjacentTile> AdjacentTiles;
			if (GetAdjacentTiles(GetWorld(), BrushLocation, AdjacentTiles))
			{
				UpdateAdjacentTiles(GetWorld(), AdjacentTiles);
			}
		}
	}
}

void FGridMapEditorMode::EraseTile(AGridMapStaticMeshActor* TileToErase)
{
	TArray<FAdjacentTile> AdjacentTiles;
	bool hasAdjacentTiles = GetAdjacentTiles(GetWorld(), TileToErase->GetActorLocation(), AdjacentTiles);
	
	GetWorld()->DestroyActor(TileToErase);

	if (hasAdjacentTiles)
	{
		UpdateAdjacentTiles(GetWorld(), AdjacentTiles);
	}
}

bool FGridMapEditorMode::InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent)
{
	bool bHandled = false;

	const bool bEventIsLeftButtonDown = (InKey == EKeys::LeftMouseButton && InEvent != IE_Released);
	const bool bKeystateIsLeftButtonDown = InViewport->KeyState(EKeys::LeftMouseButton);
	const bool bWantsToErase = InViewport->KeyState(EKeys::LeftControl) || InViewport->KeyState(EKeys::RightControl);

	const bool bIsLeftButtonDown =  bEventIsLeftButtonDown || bKeystateIsLeftButtonDown;
	if (InViewportClient->EngineShowFlags.ModeWidgets)
	{
		const bool bUserWantsPaint = bIsLeftButtonDown;
		bIsPainting = bUserWantsPaint;

		// check paint mode?
		if (bWantsToErase)
		{
			UISettings.SetPaintMode(EGridMapPaintMode::Erase);
		}
		else
		{
			UISettings.SetPaintMode(EGridMapPaintMode::Paint);
		}

		if (bUserWantsPaint)
		{
			bHandled = true;
			PaintTile();
		}
	}

	if (!bHandled)
	{
		bHandled = FEdMode::InputKey(InViewportClient, InViewport, InKey, InEvent);
	}

	return bHandled;
}

bool FGridMapEditorMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click)
{
	return true;
}

void FGridMapEditorMode::GridMapBrushTrace(FEditorViewportClient* ViewportClient, const FVector& InRayOrigin, const FVector& InRayDirection)
{
	bBrushTraceValid = false;
	BrushTraceHitActor.Reset();

	if (ViewportClient == nullptr || (!ViewportClient->IsMovingCamera() && ViewportClient->IsVisible()))
	{
		if (UISettings.GetPaintToolSelected())
		{
			const FPlane GroundPlane(FVector(0, 0, UISettings.GetPaintHeight()), FVector::UpVector);

			FVector IntersectionLocation = FMath::RayPlaneIntersection(InRayOrigin, InRayDirection, GroundPlane);
			BrushLocation = SnapLocation(IntersectionLocation);
			bBrushTraceValid = true;

			TArray<AGridMapStaticMeshActor*> HitTiles;
			TilesAt(GetWorld(), BrushLocation, HitTiles);
			if (HitTiles.Num() > 0)
			{
				BrushTraceHitActor = HitTiles[0];
			}
		}
	}
}

FVector FGridMapEditorMode::SnapLocation(const FVector& InLocation)
{
	int32 SnapWidth = GEditor->GetGridSize();
	if (SnapWidth <= 0) {
		return InLocation;
	}
	float X = InLocation.X / SnapWidth;
	float Y = InLocation.Y / SnapWidth;
	float Z = InLocation.Z / SnapWidth;

	X = FMath::RoundToInt(X) * SnapWidth;
	Y = FMath::RoundToInt(Y) * SnapWidth;
	Z = FMath::RoundToInt(Z) * SnapWidth;
	return FVector(X, Y, Z);
}

bool FGridMapEditorMode::Select(AActor* InActor, bool bInSelected)
{
	// return true if you filter that selection
	// however - return false if we are trying to deselect so that it will infact do the deselection
	if (bInSelected == false)
	{
		return false;
	}
	return true;
}

bool FGridMapEditorMode::IsSelectionAllowed(AActor* InActor, bool bInSelection) const
{
	return false;
}

void FGridMapEditorMode::ClearAllToolSelection()
{
	UISettings.SetPaintToolSelected(false);
	UISettings.SetSelectToolSelected(false);
	UISettings.SetSettingsToolSelected(false);
}

void FGridMapEditorMode::OnSetPaintTiles()
{
	ClearAllToolSelection();
	UISettings.SetPaintToolSelected(true);
	//HandleToolChanged();
}

void FGridMapEditorMode::OnSetSelectTiles()
{
	ClearAllToolSelection();
	UISettings.SetSelectToolSelected(true);
}

void FGridMapEditorMode::OnSetTileSettings()
{
	ClearAllToolSelection();
	UISettings.SetSettingsToolSelected(true);
}

EGridMapEditingState FGridMapEditorMode::GetEditingState() const
{
	UWorld* World = GetWorld();

	if (GEditor->bIsSimulatingInEditor)
	{
		return EGridMapEditingState::SIEWorld;
	}
	else if (GEditor->PlayWorld != NULL)
	{
		return EGridMapEditingState::PIEWorld;
	}
	else if (World == nullptr)
	{
		return EGridMapEditingState::Unknown;
	}

	return EGridMapEditingState::Enabled;
}

uint32 FGridMapEditorMode::GetTileAdjacencyBitmask(UWorld* World, const FVector& Origin, FName TileSetName) const
{
	uint32 bitmask = 0;

	TArray<FAdjacentTile> AdjacentTiles;
	if (GetAdjacentTiles(World, Origin, AdjacentTiles))
	{
		for (const FAdjacentTile& AdjacentTile : AdjacentTiles)
		{
			if (AdjacentTile.Key->TileSet->Name == TileSetName)
				bitmask |= AdjacentTile.Value;
		}
	}

	return bitmask;
}

bool FGridMapEditorMode::TilesAt(UWorld* World, const FVector& Origin, TArray<AGridMapStaticMeshActor*>& OutTiles) const
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	TArray<AActor*> ActorsToIgnore;
	TArray<AActor*> OutActors;

	if (UKismetSystemLibrary::SphereOverlapActors(World, Origin, GetTileCheckRadius(), ObjectTypes, AGridMapStaticMeshActor::StaticClass(), ActorsToIgnore, OutActors))
	{
		OutTiles.Reserve(OutActors.Num());
		for (AActor* OutActor : OutActors)
		{
			if (Cast<AGridMapStaticMeshActor>(OutActor))
				OutTiles.Add(Cast<AGridMapStaticMeshActor>(OutActor));
		}
	}

	return OutTiles.Num() > 0;
}

void FGridMapEditorMode::UpdateAdjacentTiles(UWorld* World, const TArray<FAdjacentTile>& RootActors)
{
	TArray<FAdjacentTile> AdjacentTiles;
	TArray<AGridMapStaticMeshActor*> ProcessedActors;
	TQueue<AGridMapStaticMeshActor*> QueuedActors;

	for (const FAdjacentTile& RootTile : RootActors)
	{
		QueuedActors.Enqueue(RootTile.Key);
	}
	
	while (!QueuedActors.IsEmpty())
	{
		AGridMapStaticMeshActor* CurrentActor = nullptr;
		QueuedActors.Dequeue(CurrentActor);

		// have we already been processed?
		if (ProcessedActors.Contains(CurrentActor))
			continue;

		UGridMapTileSet* TileSet = CurrentActor->TileSet;
		uint32 Adjacency = GetTileAdjacencyBitmask(GetWorld(), CurrentActor->GetActorLocation(), TileSet->Name);
		const FGridMapTileList* TileList = TileSet->FindTilesForAdjacency(Adjacency);
		if (TileList == nullptr)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 4.0f, FColor::Red, TEXT("Failed to find tile!"), true, FVector2D::UnitVector);
			continue;
		}
		UStaticMesh* ExpectedStaticMesh = TileList->GetRandomTile().LoadSynchronous();
		UStaticMesh* CurrentStaticMesh = CurrentActor->GetStaticMeshComponent()->GetStaticMesh();

		// it hasn't changed
		if (CurrentStaticMesh == ExpectedStaticMesh)
			continue;

		if (UISettings.GetDebugDrawTiles())
		{
			DrawDebugPoint(GetWorld(), CurrentActor->GetActorLocation(), 10.f, FColor::Yellow, false, 5.0f, 255);
		}

		// it has changed, so let's update it
		CurrentActor->GetStaticMeshComponent()->SetStaticMesh(ExpectedStaticMesh);
		CurrentActor->SetActorRotation(TileList->Rotation);
		ProcessedActors.Add(CurrentActor);

		// find any neighbors that haven't been updated, and queue them for an update
		if (GetAdjacentTiles(World, CurrentActor->GetActorLocation(), AdjacentTiles))
		{
			for (const FAdjacentTile& AdjacentTile : AdjacentTiles)
			{
				if (AdjacentTile.Key && !ProcessedActors.Contains(AdjacentTile.Key))
				{
					QueuedActors.Enqueue(AdjacentTile.Key);
				}
			}
		}
	}
}

bool FGridMapEditorMode::GetAdjacentTiles(UWorld* World, const FVector& Origin, TArray<TPair<AGridMapStaticMeshActor*, uint32>>& OutAdjacentTiles) const
{
	static const int TileCount = 4;

	static FVector Offsets[TileCount]{
		FVector(0, -GetTileSize(), 0),	// top-center
		FVector(-GetTileSize(), 0, 0),	// center-left
		FVector(GetTileSize(), 0, 0),	// center-right
		FVector(0, GetTileSize(), 0),	// botton-center
	};

	static uint32 Bits[TileCount]{
		1 << 0,
		1 << 1,
		1 << 2,
		1 << 3,
	};

	TArray<AGridMapStaticMeshActor*> Tiles;
	for (int i = 0; i < TileCount; ++i)
	{
		if (TilesAt(World, Origin + Offsets[i], Tiles))
		{
			for (AGridMapStaticMeshActor* Tile : Tiles)
			{
				OutAdjacentTiles.Add(TPair<AGridMapStaticMeshActor*, uint32>(Tile, Bits[i]));
			}
			Tiles.Empty();
		}
	}

	return OutAdjacentTiles.Num() > 0;
}

int32 FGridMapEditorMode::GetTileSize() const
{
	return GEditor->GetGridSize();
}

float FGridMapEditorMode::GetTileCheckRadius() const
{
	return GetTileSize() / 3.0f;
}