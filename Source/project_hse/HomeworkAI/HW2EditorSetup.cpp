#if WITH_EDITOR

#include "HWGuard.h"
#include "HWGuardBehavior.h"

#include "ActorFactories/ActorFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTreeGraph.h"
#include "Builders/CubeBuilder.h"
#include "Editor.h"
#include "Elements/PCGCreatePointsGrid.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "Elements/PCGTransformPoints.h"
#include "EngineUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MeshSelectors/PCGMeshSelectorWeighted.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGVolume.h"
#include "UObject/SavePackage.h"
#include "project_hse.h"

namespace
{
	constexpr TCHAR MapPath[] = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");
	constexpr TCHAR TreePath[] = TEXT("/Game/HW2/BT_HWGuard");
	constexpr TCHAR BlackboardPath[] = TEXT("/Game/HW2/BB_HWGuard");
	constexpr TCHAR GraphPath[] = TEXT("/Game/HW2/PCG_HWObstacles");

	bool SaveAsset(UObject* Asset)
	{
		UPackage* Package = Asset->GetPackage();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Package, Asset, *File, Args);
	}

	UBehaviorTree* GetOrCreateTree()
	{
		constexpr EObjectFlags AssetFlags = RF_Public | RF_Standalone | RF_Transactional;
		UBehaviorTree* Tree = nullptr;
		if (FPackageName::DoesPackageExist(TreePath))
		{
			Tree = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/HW2/BT_HWGuard.BT_HWGuard"));
			if (!Tree || Tree->RootNode)
			{
				return Tree;
			}
		}
		else
		{
			Tree = NewObject<UBehaviorTree>(CreatePackage(TreePath), TEXT("BT_HWGuard"), AssetFlags);
			FAssetRegistryModule::AssetCreated(Tree);
		}

		UBlackboardData* Blackboard = nullptr;
		if (FPackageName::DoesPackageExist(BlackboardPath))
		{
			Blackboard = LoadObject<UBlackboardData>(nullptr, TEXT("/Game/HW2/BB_HWGuard.BB_HWGuard"));
		}
		else
		{
			Blackboard = NewObject<UBlackboardData>(CreatePackage(BlackboardPath), TEXT("BB_HWGuard"), AssetFlags);
			FAssetRegistryModule::AssetCreated(Blackboard);
		}
		if (!Blackboard)
		{
			return nullptr;
		}
		InitializeHWGuardTree(*Tree, *Blackboard);
		if (Tree->BTGraph)
		{
			Tree->BTGraph->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors);
		}
		const TSubclassOf<UEdGraphSchema> SchemaClass = GetDefault<UBehaviorTreeGraph>()->Schema;
		UBehaviorTreeGraph* Graph = CastChecked<UBehaviorTreeGraph>(FBlueprintEditorUtils::CreateNewGraph(
			Tree, TEXT("Behavior Tree"), UBehaviorTreeGraph::StaticClass(), SchemaClass));
		Tree->BTGraph = Graph;
		Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
		// Adding graph subnodes must not rebuild the runtime tree before links reach the root.
		Graph->LockUpdates();
		Graph->OnCreated();
		Graph->UnlockUpdates();
		Graph->Initialize();
		return Tree->RootNode && SaveAsset(Blackboard) && SaveAsset(Tree) ? Tree : nullptr;
	}

	UPCGGraph* GetOrCreatePCG()
	{
		if (FPackageName::DoesPackageExist(GraphPath))
		{
			return LoadObject<UPCGGraph>(nullptr, TEXT("/Game/HW2/PCG_HWObstacles.PCG_HWObstacles"));
		}

		UPCGGraph* Graph = NewObject<UPCGGraph>(CreatePackage(GraphPath), TEXT("PCG_HWObstacles"),
			RF_Public | RF_Standalone | RF_Transactional);
		UPCGCreatePointsGridSettings* GridSettings = nullptr;
		UPCGNode* Grid = Graph->AddNodeOfType(GridSettings);
		GridSettings->CoordinateSpace = EPCGCoordinateSpace::OriginalComponent;
		GridSettings->GridExtents = FVector(450, 780, 0);
		GridSettings->CellSize = FVector(450, 520, 100);

		UPCGTransformPointsSettings* TransformSettings = nullptr;
		UPCGNode* Transform = Graph->AddNodeOfType(TransformSettings);
		TransformSettings->OffsetMin = FVector(-60, -85, 110);
		TransformSettings->OffsetMax = FVector(60, 85, 110);
		TransformSettings->RotationMin.Yaw = -35;
		TransformSettings->RotationMax.Yaw = 35;
		TransformSettings->bUniformScale = false;
		TransformSettings->ScaleMin = FVector(1.25, 1.1, 2.2);
		TransformSettings->ScaleMax = FVector(1.75, 1.55, 2.2);

		UPCGStaticMeshSpawnerSettings* SpawnSettings = nullptr;
		UPCGNode* Spawn = Graph->AddNodeOfType(SpawnSettings);
		SpawnSettings->SetMeshSelectorType(UPCGMeshSelectorWeighted::StaticClass());
		UPCGMeshSelectorWeighted* Selector = CastChecked<UPCGMeshSelectorWeighted>(SpawnSettings->MeshSelectorParameters);
		FPCGMeshSelectorWeightedEntry& Entry = Selector->MeshEntries.Emplace_GetRef(
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube"))), 1);
		Entry.Descriptor.BodyInstance.SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Entry.Descriptor.bCanEverAffectNavigation = true;
		Selector->RefreshDisplayNames();

		Graph->GetInputNode()->SetNodePosition(-280, 0);
		Grid->SetNodePosition(0, 0);
		Transform->SetNodePosition(270, 0);
		Spawn->SetNodePosition(540, 0);
		Graph->GetOutputNode()->SetNodePosition(810, 0);
		Graph->AddEdge(Grid, PCGPinConstants::DefaultOutputLabel, Transform, PCGPinConstants::DefaultInputLabel);
		Graph->AddEdge(Transform, PCGPinConstants::DefaultOutputLabel, Spawn, PCGPinConstants::DefaultInputLabel);
		FAssetRegistryModule::AssetCreated(Graph);
		return SaveAsset(Graph) ? Graph : nullptr;
	}

	template <typename T>
	T* FindOrPlace(UWorld& World, const TCHAR* Label, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		for (TActorIterator<T> Actor(&World); Actor; ++Actor)
		{
			if (Actor->GetActorLabel() == Label)
			{
				return *Actor;
			}
		}

		T* Actor = CastChecked<T>(GEditor->AddActor(World.PersistentLevel, T::StaticClass(),
			FTransform(Rotation, Location), true, RF_Transactional, false));
		Actor->SetActorLabel(Label);
		Actor->SetFolderPath(TEXT("HW2"));
		return Actor;
	}

	void BuildVolume(AVolume& Volume, const FVector& Size)
	{
		if (!Volume.Brush)
		{
			UCubeBuilder* Builder = NewObject<UCubeBuilder>();
			Builder->X = Size.X;
			Builder->Y = Size.Y;
			Builder->Z = Size.Z;
			UActorFactory::CreateBrushForVolumeActor(&Volume, Builder);
			Volume.PostEditChange();
		}
	}

	void SetupHW2()
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || World->GetPackage()->GetName() != MapPath)
		{
			UE_LOG(Logproject_hse, Error, TEXT("HW2.Setup requires Lvl_ThirdPerson in the editor."));
			return;
		}

		UBehaviorTree* Tree = GetOrCreateTree();
		UPCGGraph* Graph = GetOrCreatePCG();
		if (!Tree || !Graph)
		{
			UE_LOG(Logproject_hse, Error, TEXT("HW2.Setup could not save or load the HW2 assets."));
			return;
		}

		APCGVolume* PCG = FindOrPlace<APCGVolume>(*World, TEXT("HW2 Obstacles"), FVector(1160, 0, 0));
		BuildVolume(*PCG, FVector(1020, 1750, 600));
		PCG->PCGComponent->Modify();
		PCG->PCGComponent->SetIsPartitioned(false);
		PCG->PCGComponent->SetGraph(Graph);

		ANavMeshBoundsVolume* NavBounds = FindOrPlace<ANavMeshBoundsVolume>(*World, TEXT("HW2 Navigation"),
			FVector(0, 0, 200));
		BuildVolume(*NavBounds, FVector(3950, 3950, 1200));
		if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			Navigation->OnNavigationBoundsUpdated(NavBounds);
		}

		AHWGuard* Guard = FindOrPlace<AHWGuard>(*World, TEXT("HW2 Guard"), FVector(880, -970, 92), FRotator(0, 90, 0));
		Guard->Modify();
		Guard->BehaviorTree = Tree;
		World->MarkPackageDirty();
		UE_LOG(Logproject_hse, Display, TEXT("HW2.Setup complete."));
	}

	FAutoConsoleCommand HW2SetupCommand(TEXT("HW2.Setup"), TEXT("Create homework assets and place actors in Lvl_ThirdPerson."),
		FConsoleCommandDelegate::CreateStatic(&SetupHW2));
}

#endif
