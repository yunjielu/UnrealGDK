// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using UnrealBuildTool;

public class SpatialGDKEditorCommandlet : ModuleRules
{
	public SpatialGDKEditorCommandlet(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bFasterWithoutUnity = true;

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
				"SpatialGDK",
				"SpatialGDKEditor",
				"UnrealEd",
			});

		PrivateIncludePaths.AddRange(
			new string[]
			{
				"SpatialGDKEditorCommandlet/Private",
				"SpatialGDKEditorCommandlet/Private/Commandlets"
			});
	}
}
