// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MyProject6EditorTarget : TargetRules
{
	public MyProject6EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

        DefaultBuildSettings = BuildSettingsVersion.V7;

        ExtraModuleNames.AddRange( new string[] { "MyProject6" } );

	}
}
