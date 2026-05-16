/*---------------------------------------------------------------------------------------------------------



---------------------------------------------------------------------------------------------------------*/
#load "bld/ext/clang.csx"
#load "bld/csx/build.flags.csx"
#load "bld/csx/build.resources.csx"

#r "../bld/bin/mkb.dll"
using Kltv.Kombine.Api;
using Kltv.Kombine.Types;
using static Kltv.Kombine.Api.Statics;
using static Kltv.Kombine.Api.Tool;

// Application Friendly Name
KValue appname = "RetroDev (GUI)";
// Path where the test sources are located
KValue srcpath = "gui/";
// Name of the binary for the output
KValue binname = "retro.dev.gui";

// No dependencies to fetch application itself
int dependencies(string[] args){
	return 0;
}

//
// Build application action
//
int build(string[] args){
	Msg.Print($"Building {appname} application");
	Msg.BeginIndent();

	// Build the application
	// ------------------------------------------------------------------------
	// Output paths
	KValue OutputBin = KValue.Import("OutputBin");
	KValue OutputLib = KValue.Import("OutputLib");
	KValue OutputTmp = KValue.Import("OutputTmp");
	// Disable some warnings
	//
	KList CompilerFlags = new KList { "-Wall -Wextra -Wno-unused-parameter" };
	// The list of defines to use
	//
	//
	KList Defines = new KList();
	// ImGui user config header name to search for
	Defines += "IMGUI_USER_CONFIG=\\\"retrodev.imconfig.h\\\"";
	// Include directories
	KList Includes = new KList();
	// Include our own library
	//
	Includes += Share.Registry("retrodev", "incpath");
	// ImGui for the UI
	//
	Includes += Share.Registry("imgui", "incpath");
	Includes += Share.Registry("imgui", "incbackendspath");
	Includes += Share.Registry("imgui", "incconfigpath");
	Includes += Share.Registry("imgui", "incextensionspath");
	// SDL for image manipulation
	//
	Includes += Share.Registry("sdl", "incpath");
	Includes += Share.Registry("sdl.img", "incpath");
	// Freetype for font rendering
	Includes += Share.Registry("freetype", "incpath");
	// CTRE include
	Includes += Share.Registry("ctre", "incpath");
	// And our own source directory
	Includes += srcpath;
	// Library directories
	KList LibraryDirs = new KList();
	LibraryDirs += Share.Registry("retrodev", "libpath");
	LibraryDirs += Share.Registry("sdl","libpath");
	LibraryDirs += Share.Registry("sdl.img","libpath");
	LibraryDirs += Share.Registry("freetype", "libpath");
	LibraryDirs += Share.Registry("ascript", "libpath");
	LibraryDirs += Share.Registry("rasm", "libpath");
	// Libraries
	KList Libraries = new KList();
	Libraries += Share.Registry("retrodev", "libname");
	Libraries += Share.Registry("sdl","libname");
	Libraries += Share.Registry("sdl.img","libname");
	Libraries += Share.Registry("freetype", "libname");
	Libraries += Share.Registry("ascript", "libname");
	Libraries += Share.Registry("rasm", "libname");
	// Windows SDK libraries
	if (Host.IsWindows()){
		Libraries += "dwmapi.lib";
		Libraries += "user32.lib";
		Libraries += "kernel32.lib";
		Libraries += "gdi32.lib";
		Libraries += "winmm.lib";
		Libraries += "setupapi.lib";
		Libraries += "imm32.lib";
		Libraries += "shell32.lib";
		Libraries += "ole32.lib";
		Libraries += "advapi32.lib";
		Libraries += "version.lib";
		Libraries += "oleaut32.lib";
		Libraries += "Shcore.lib";
		Libraries += "uxtheme.lib";
		Libraries += "Ws2_32.lib";
	}
	if (Host.IsLinux()) {
		Libraries += "m";
		Libraries += "dl";
		Libraries += "pthread";
		Libraries += "rt";
	}
	// Create an instance of the clang tool.
	Clang clang = new Clang();
	//
	//
	//
	// On Windows and release build we want to build a windows application without console,
	// so we need to pass the subsystem option to the linker
	//
	if (Host.IsWindows()) {
		// In Windows build we should specify the subsystem console / windows
		if (BuildFlags.Flags.BuildMode == "release") {
			clang.Options.SwitchesLD += "-Wl,/subsystem:windows";
			// In Windows we pass the option to the linker to embed the manifest
			//
			// Manifest is required by some CEF features, if a valid manifest is not embedded things like 3D
			// acceleration will not work as intended
			//
			clang.Options.SwitchesLD += "-Wl,/manifest:embed";
			clang.Options.SwitchesLD += "-Wl,/manifestinput:gui/os/win/retrodev.manifest";
		}
	}
	// Output name folder
	//
	OutputBin += binname + "/";
	OutputLib += binname + "/";
	OutputTmp += binname + "/";
	//
	// Generate resources
	// ------------------------------------------------------------------------
	Msg.Print("Generating resources");
	BuildResources resources = new BuildResources();
	resources.ResourcePath = srcpath + "res/";
	resources.ResourceGlob = "**/*.*";
	resources.OutputPath = OutputTmp;
	resources.ObjExtension = clang.Options.ObjectExtension;
	resources.CppFileName = srcpath+"gen/resources.cpp";
	resources.OutputFileName = "compiled.resources";
	resources.GenResources();
	Msg.PrintTask("Generating resources: "); Msg.PrintTaskSuccess(" done");
	//
	// Create the list of sources to be compiled
	// We pass the relative path from the current folder
	//
	KList src = CreateSourceList(srcpath);
	// Set the output paths
	clang.Options.SwitchesCC += CompilerFlags;
	clang.Options.SwitchesCXX += CompilerFlags;
	clang.Options.IncludeDirs += Includes;
	clang.Options.LibraryDirs += LibraryDirs;
	clang.Options.Libraries += Libraries;
	clang.Options.Defines += Defines;

	KList LinkerFlags = new KList();
	clang.Options.SwitchesLD += LinkerFlags;
	// Generate the list of object files to be used as output
	KList objs = src.WithExtension(clang.Options.ObjectExtension).WithPrefix(OutputTmp);
	// We set a delegate to modify parameters on each file build
	clang.ProcessFile += CustomParameters;
	// And compile the sources
	clang.Compile(src, objs);
	// Use the librarian to generate a static library
	KValue resourcesObj = OutputTmp + "compiled.resources" + clang.Options.ObjectExtension;
	if (Host.IsWindows() && Files.Exists(resourcesObj))
		objs += resourcesObj;
	clang.Linker(objs, OutputBin + binname + clang.Options.BinaryExtension);
	// ------------------------------------------------------------------------
	Msg.PrintTask("Building binary: " + binname + clang.Options.BinaryExtension);
	Msg.PrintTaskSuccess(" done");

	Msg.Print("---------------------------------------------------------------------------------------");
	Msg.EndIndent();
	return 0;
}

//
// Clean artifacts
//
int clean(string[] args){
	// Output paths
	Msg.Print($"Cleaning {appname} ");
	KValue OutputBin = KValue.Import("OutputBin");
	KValue OutputLib = KValue.Import("OutputLib");
	KValue OutputTmp = KValue.Import("OutputTmp");
	KList folders = new KList();
	folders += OutputBin+binname+"/";
	folders += OutputLib+binname+"/";
	folders += OutputTmp+binname+"/";
	// We clean also the ext folder for obj files that are on the
	// ext but being built as part of the project (like imgui and extensions)
	folders += OutputTmp + "ext/";
	Msg.BeginIndent();
	foreach(KValue folder in folders){
		Msg.Print("Deleting: "+RealPath(folder));
	}
	Msg.EndIndent();
	// Clean the folders
	Folders.Delete(folders,true);
	// Clean generated resources cpp file
	Files.Delete(srcpath+"gen/resources.cpp");
	return 0;
}

// Create the source code list to be compiled
private KList CreateSourceList(KValue srcpath){
	KList src = new KList();
	Msg.Print("Creating source list for: "+RealPath(srcpath));
	src += Glob(srcpath+"**/*.cpp");
	src += Glob(srcpath+"**/*.c");
	//
	// In windows relase mode we add resources to create a win app not console one
	//
	if (Host.IsWindows()) {
		if (BuildFlags.Flags.BuildMode == "release")
			src += Glob(srcpath + "**/*.rc");
	}
	//
	// Include external sources we want to use (ImGui)
	//
	string imgui = Share.Registry("imgui", "srcpath");
	//
	// ImGui + SDL3 backend + Freetype extension sources
	//
	// Sources are provided always with relative path
	// Because we need later to compose the path in the output build artifacts
	// so, fetch a relative path to imgui sources from here
	//
	imgui = Path.GetRelativePath(CurrentScriptFolder, imgui);
	src += imgui + "imgui.cpp";
	src += imgui + "imgui_demo.cpp";
	src += imgui + "imgui_draw.cpp";
	src += imgui + "imgui_tables.cpp";
	src += imgui + "imgui_widgets.cpp";
	src += imgui + "backends/imgui_impl_sdl3.cpp";
	src += imgui + "backends/imgui_impl_sdlrenderer3.cpp";
	src += imgui + "misc/freetype/imgui_freetype.cpp";
	// ImGui extensions sources
	//
	string imguiExt = Share.Registry("imgui", "incextensionspath");
	imguiExt = Path.GetRelativePath(CurrentScriptFolder, imguiExt);
	src += Glob(imguiExt+"**/*.cpp");
	//
	// Remove platform dependent code
	//
	KList srcFiltered = new KList();
	foreach (KValue file in src) {
		string filex = file;
		// For Windows builds we skip everything on osx and lnx folders
		if (Host.IsWindows()) {
			if (filex.Contains("/osx/") || filex.Contains("/lnx/")) {
				continue;
			}
		}
		if (Host.IsLinux()) {
			if (filex.Contains("/win/") || filex.Contains("/osx/")) {
				continue;
			}
		}
		if (Host.IsMacOS()) {
			if (filex.Contains("/win/") || filex.Contains("/lnx/")) {
				continue;
			}
		}
		srcFiltered += file;
	}
	return srcFiltered;
}


//
//
public string CustomParameters(string file) {
	string addArgs = "";
	// Silence warings for some files
	//
	if (file == @"..\ext\imgui\misc/freetype/imgui_freetype.cpp" ||
		file == "../ext/imgui/misc/freetype/imgui_freetype.cpp") {
		addArgs = " -Wno-unused-function";
		Msg.Print("Applying: Custom warning removal for imgui freetype");
	}
	return addArgs;
}
