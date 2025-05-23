// Copyright 2024 Diversion Company, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Diversion : ModuleRules
{
	public Diversion(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        IWYUSupport = IWYUSupport.Full; 

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Slate",
				"SlateCore",
				"InputCore",
                "UnrealEd",
                "Projects",
                "SourceControl",
                "AgentAPI",
                "CoreAPI",
                "HTTP",
                "Json",
                "DesktopWidgets",
                "CoreUObject",
                "Engine",
                "Common"
			}
        );

        UnsafeTypeCastWarningLevel = WarningLevel.Error;
        
        // Only compile tests when building the editor
        if (Target.Type == TargetType.Editor)
        {
	        PrivateIncludePaths.Add("Diversion/Tests");
	        PrivateDependencyModuleNames.Add("UnrealEd");
        }
    }
}
