// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Common : ModuleRules
{
	public Common(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.Add("Core");

		PCHUsage = PCHUsageMode.NoPCHs;

		// Only compile tests when building the editor
		if (Target.Type == TargetType.Editor)
		{
			PrivateIncludePaths.Add("Common/Tests");
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}