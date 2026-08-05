// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MyProject6Target : TargetRules
{
	public MyProject6Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
<<<<<<< Updated upstream
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange( new string[] { "MyProject6" } );
=======
        DefaultBuildSettings = BuildSettingsVersion.V7;

        ExtraModuleNames.AddRange( new string[] { "MyProject6" } );
>>>>>>> Stashed changes
	}
}
