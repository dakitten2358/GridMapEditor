// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "GridMapEditorMode.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Framework/Commands/UICommandList.h"
#include "GridMapEditCommands.h"
#include "GridMapEditorModeToolkit.h"
#include "GridMapHelper.h"
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
	if (bIsPainting && bBrushTraceValid && UISettings.GetPaintMode() == EGridMapPaintMode::Erase)
	{
		// and we're on top of something?
		if (BrushTraceHitActor.IsValid())
		{
			GetWorld()->DestroyActor(BrushTraceHitActor.Get());
		}
		return;
	}

	// if we're painting, 
	if (bIsPainting && bBrushTraceValid && UISettings.GetCurrentTileSet().IsValid())
	{
		UGridMapTileSet* TileSet = UISettings.GetCurrentTileSet().Get();

		// empty spot?
		if (!BrushTraceHitActor.IsValid())
		{
			// figure out the bitmask for this tile
			int32 SnapWidth = GEditor->GetGridSize();
			uint32 Adjacency = GetTileAdjacencyBitmask(GetWorld(), BrushLocation, SnapWidth, TileSet->Name);
			const FGridMapTileList* TileList = TileSet->FindTilesForAdjacency(Adjacency);
			if (TileList)
			{
				TAssetPtr<UStaticMesh> StaticMeshAsset = TileList->GetRandomTile();

				FActorSpawnParameters SpawnParameters;
				if (UISettings.GetHideOwnedActors())
				{
					SpawnParameters.bHideFromSceneOutliner = true;
				}
				AStaticMeshActor* MeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnParameters);

				FGridMapHelper::SetIsOwnedByGridMap(MeshActor);
				FGridMapHelper::SetTileset(MeshActor, TileSet->Name);

				// Rename the display name of the new actor in the editor to reflect the mesh that is being created from.
				FActorLabelUtilities::SetActorLabelUnique(MeshActor, TEXT("ActorLabelUnique"));

				UStaticMesh* StaticMesh = Cast<UStaticMesh>(StaticMeshAsset.LoadSynchronous());
				MeshActor->GetStaticMeshComponent()->SetStaticMesh(StaticMesh);
				MeshActor->ReregisterAllComponents();

				FTransform BrushTransform = FTransform(TileList->Rotation.Quaternion(), BrushLocation, FVector::OneVector);
				MeshActor->SetActorTransform(BrushTransform);

				// update adjacent tiles
				UpdateAdjacentTiles(GetWorld(), MeshActor, TileSet->Name);

				// Switch to another mesh from the list
				//ResetBrushMesh();
			}
		}
		// we're about to replace an actor
		else
		{

		}
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

			// did we hit something we own?
			const FVector TraceStart(InRayOrigin);
			const FVector TraceEnd(InRayOrigin + InRayDirection * HALF_WORLD_MAX);

			FHitResult Hit;
			if (GridMapStaticMeshTrace(TraceStart, TraceEnd, Hit))
			{
				if (Hit.Actor.IsValid() && Hit.Actor->IsA<AStaticMeshActor>() && FGridMapHelper::IsOwnedByGridMap(Hit.Actor.Get()))
				{
					BrushTraceHitActor = Hit.Actor;
				}
			}
		}
	}
}

bool FGridMapEditorMode::GridMapStaticMeshTrace(const FVector& TraceStart, const FVector& TraceEnd, FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	
	static FName NAME_GridMapBrush = FName(TEXT("GridMapBrush"));
	FCollisionQueryParams queryParams(NAME_GridMapBrush);
	return World->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECollisionChannel::ECC_WorldStatic, queryParams);
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

void FGridMapEditorMode::OnSetPaintTiles()
{
	//ClearAllToolSelection();
	UISettings.SetPaintToolSelected(true);
	//HandleToolChanged();
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

uint32 FGridMapEditorMode::GetTileAdjacencyBitmask(UWorld* World, const FVector& Origin, int32 Size, FName Tag) const
{
	uint32 bitmask = 0;

	static const int TileCount = 4;

	static FVector Offsets[TileCount] {
		FVector(0, -Size, 0),	// top-center
		FVector(-Size, 0, 0),	// center-left
		FVector(Size, 0, 0),	// center-right
		FVector(0, Size, 0),	// botton-center
	};

	static int32 Bits[TileCount]{
		1 << 0,
		1 << 1,
		1 << 2,
		1 << 3,
	};

	for (int i = 0; i < TileCount; ++i)
	{
		if (IsMatchingTilePresentAt(World, Origin + Offsets[i], Size / 3, Tag))
			bitmask |= Bits[i];
	}

	return bitmask;
}

bool FGridMapEditorMode::IsMatchingTilePresentAt(UWorld* World, const FVector& Origin, int32 Radius, FName Tag) const
{
	AStaticMeshActor* StaticMeshActor = MatchingTileAt(World, Origin, Radius, Tag);
	if (StaticMeshActor)
		return true;
	return false;
}

AStaticMeshActor* FGridMapEditorMode::MatchingTileAt(class UWorld* World, const FVector& Origin, int32 Radius, FName Tag) const
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	TArray<AActor*> ActorsToIgnore;
	TArray<AActor*> OutActors;

	if (UKismetSystemLibrary::SphereOverlapActors(World, Origin, Radius, ObjectTypes, AStaticMeshActor::StaticClass(), ActorsToIgnore, OutActors))
	{
		for (AActor* OutActor : OutActors)
		{
			if (OutActor->Tags.Contains(Tag))
				return Cast<AStaticMeshActor>(OutActor);
		}
	}

	return nullptr;
}

void FGridMapEditorMode::UpdateAdjacentTiles(UWorld* World, AStaticMeshActor* RootActor, const FName& Tag)
{
	UGridMapTileSet* TileSet = UISettings.GetCurrentTileSet().Get();
	int32 SnapWidth = GEditor->GetGridSize();

	static const int TileCount = 4;
	static FVector Offsets[TileCount]{
		FVector(0, -SnapWidth, 0),	// top-center
		FVector(-SnapWidth, 0, 0),	// center-left
		FVector(SnapWidth, 0, 0),	// center-right
		FVector(0, SnapWidth, 0),	// botton-center
	};	

	TArray<AStaticMeshActor*> ProcessedActors;
	TQueue<AStaticMeshActor*> QueuedActors;

	// Queue the neighbors of this tile
	for (int i = 0; i < TileCount; ++i)
	{
		AStaticMeshActor* AdjacentActor = MatchingTileAt(World, RootActor->GetActorLocation() + Offsets[i], SnapWidth / 3, Tag);
		if (AdjacentActor && !ProcessedActors.Contains(AdjacentActor))
		{
			QueuedActors.Enqueue(AdjacentActor);
		}
	}
	
	while (!QueuedActors.IsEmpty())
	{
		AStaticMeshActor* CurrentActor = nullptr;
		QueuedActors.Dequeue(CurrentActor);

		uint32 Adjacency = GetTileAdjacencyBitmask(GetWorld(), CurrentActor->GetActorLocation(), SnapWidth, TileSet->Name);
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

		// it has changed, so let's update it
		CurrentActor->GetStaticMeshComponent()->SetStaticMesh(ExpectedStaticMesh);
		CurrentActor->SetActorRotation(TileList->Rotation);
		ProcessedActors.Add(CurrentActor);

		// find any neighbors that haven't been updated, and queue them for an update
		for (int i = 0; i < TileCount; ++i)
		{
			AStaticMeshActor* AdjacentActor = MatchingTileAt(World, CurrentActor->GetActorLocation() + Offsets[i], SnapWidth / 3, Tag);
			if (AdjacentActor && !ProcessedActors.Contains(AdjacentActor))
			{
				QueuedActors.Enqueue(AdjacentActor);
			}
		}
	}
}