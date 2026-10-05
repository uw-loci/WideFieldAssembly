// Some copyright should be here...

using System.IO;
using System.Diagnostics;
using UnrealBuildTool;

public class SRDisplayModule : ModuleRules
{
	public SRDisplayModule(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

#if UE_5_5_OR_LATER
		CppStandard = CppStandardVersion.Cpp20;
#else
		CppStandard = CppStandardVersion.Cpp17;
#endif

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);

		string XrRuntimeRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "External/xr-runtime/"));
		PublicIncludePaths.Add(Path.Combine(XrRuntimeRoot, "include"));

		string XRDisplayModuleXrRuntimeRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../XRDisplayModule/External/xr-runtime/"));
		PublicIncludePaths.Add(Path.Combine(XRDisplayModuleXrRuntimeRoot, "include"));

		string EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"XRDisplayModule",
				Path.Combine(EngineDir, @"Source\Runtime\Renderer\Private"),
                Path.Combine(EngineDir, @"Source\Runtime\Renderer\Internal"),
            }
            );
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"DeveloperSettings"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"HeadMountedDisplay",
				"RHI",
				"Renderer",
				"RenderCore",
				"Projects",
				"XRDisplayModule",
			}
			);

		if (ReadOnlyBuildVersion.Current.MajorVersion == 5 && ReadOnlyBuildVersion.Current.MinorVersion >= 3)
		{
			PrivateDependencyModuleNames.Add("XRBase");
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
