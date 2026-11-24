// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class project_hse : ModuleRules
{
	public project_hse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.Add("NavigationSystem");

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BehaviorTreeEditor", "AIGraph", "AssetRegistry", "PCG" });
		}

		PublicIncludePaths.AddRange(new string[] {
			"project_hse",
			"project_hse/Variant_Platforming",
			"project_hse/Variant_Platforming/Animation",
			"project_hse/Variant_Combat",
			"project_hse/Variant_Combat/AI",
			"project_hse/Variant_Combat/Animation",
			"project_hse/Variant_Combat/Gameplay",
			"project_hse/Variant_Combat/Interfaces",
			"project_hse/Variant_Combat/UI",
			"project_hse/Variant_SideScrolling",
			"project_hse/Variant_SideScrolling/AI",
			"project_hse/Variant_SideScrolling/Gameplay",
			"project_hse/Variant_SideScrolling/Interfaces",
			"project_hse/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
