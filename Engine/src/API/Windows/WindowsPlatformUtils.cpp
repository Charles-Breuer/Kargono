#include "kgpch.h"

#include "Kargono/Utils/PlatformUtils.h"
#include "Kargono/Core/Application.h"

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>


namespace Kargono::Utility
{

	float Time::GetTime()
	{
		return (float)glfwGetTime();
	}
#if KG_EXPORT == 0
	void OSCommands::OpenFileExplorer(const std::filesystem::path& path)
	{
		KG_CORE_ASSERT(std::filesystem::is_directory(path), "Invalid path provided, needs to be a directory!");
		// TODO: Add More Input Validation for system call.
		std::string outputString = "explorer " + path.string();
		system(outputString.c_str());
	}

	void OSCommands::OpenScriptProject(const std::filesystem::path& path)
	{
		// TODO: Add More Input Validation for system call.
		if (!std::filesystem::exists(path))
		{
			KG_CORE_ERROR("Invalid path provided to OpenScriptProject");
			return;
		}
		std::string outputString = "start " + path.string();
		system(outputString.c_str());
	}
#endif

	std::filesystem::path FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::GetCurrentApp().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return std::string();

	}

	std::filesystem::path FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::GetCurrentApp().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		// Sets the default extension by extracting it from the filter
		ofn.lpstrDefExt = strchr(filter, '\0') + 1;

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return std::string();
	}

}
