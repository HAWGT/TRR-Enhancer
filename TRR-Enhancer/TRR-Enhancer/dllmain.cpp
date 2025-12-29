#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Helpers.h"

const std::string ConfigFileName = "TRREnhancer.ini";
int bConsole = 0;
int Costume = 1;
int bOverrideOriginals = 1;
int bSavePhotoModeSettings = 1;
int bFreeMove = 0;

const std::string ExeName123 = "tomb123.exe";
const std::string ExeName456 = "tomb456.exe";

HMODULE tomb1DLL = nullptr;
HMODULE tomb2DLL = nullptr;
HMODULE tomb3DLL = nullptr;

HMODULE tomb4DLL = nullptr;
HMODULE tomb5DLL = nullptr;
HMODULE tomb6DLL = nullptr;

int* SelectedCostumePTR = nullptr;

typedef __int64(*PhotoModeTickTR123_t)(void);
PhotoModeTickTR123_t Orig_PhotoModeTickTR1;
PhotoModeTickTR123_t Orig_PhotoModeTickTR2;
PhotoModeTickTR123_t Orig_PhotoModeTickTR3;

__int64 hk_PhotoModeTickTR1();
__int64 hk_PhotoModeTickTR2();
__int64 hk_PhotoModeTickTR3();

void PatchModules123();
void PatchModules456();

void SaveConfig()
{
	std::ofstream ConfigFile;

	ConfigFile.open(ConfigFileName, std::ofstream::out | std::ofstream::trunc);

	ConfigFile << "Console " << bConsole << "\n";
	ConfigFile << "Costume " << Costume << "\n";
	ConfigFile << "OverrideOriginals " << bOverrideOriginals << "\n";
	ConfigFile << "SavePhotoModeSettings " << bSavePhotoModeSettings << "\n";
	ConfigFile << "FreeMove " << bFreeMove << "\n";

	ConfigFile.close();
}

void LoadConfig()
{
	std::ifstream config(ConfigFileName);

	auto CreateConsole = [](const char* name) {
		FILE* ConsoleIO;
		if (!AllocConsole())
			return;

		freopen_s(&ConsoleIO, "CONIN$", "r", stdin);
		freopen_s(&ConsoleIO, "CONOUT$", "w", stderr);
		freopen_s(&ConsoleIO, "CONOUT$", "w", stdout);

		SetConsoleTitleA(name);
		};

	if (config.is_open())
	{
		std::string param;
		unsigned int value = 0;

		while (config >> param >> value)
		{
			if (!param.compare("Console"))
			{
				if (value == 1)
				{
					CreateConsole("TRR Enhancer");
					bConsole = 1;
				}

				std::cout << "[*] TRR Enhancer" << "\n";
				std::cout << "[+] Settings Loaded: " << "\n";
				std::cout << param << " " << value << "\n";
			}
			if (!param.compare("Costume"))
			{
				Costume = value;
				std::cout << param << " " << Costume << "\n";
			}
			if (!param.compare("OverrideOriginals"))
			{
				bOverrideOriginals = value == 1;
				std::cout << param << " " << bOverrideOriginals << "\n";
			}
			if (!param.compare("SavePhotoModeSettings"))
			{
				bSavePhotoModeSettings = value == 1;
				std::cout << param << " " << bSavePhotoModeSettings << "\n";
			}
			if (!param.compare("FreeMove"))
			{
				bFreeMove = value == 1;
				std::cout << param << " " << bFreeMove << "\n";
			}
		}

		config.close();
	}
	else
	{
		SaveConfig();
	}


}

//Patches all costume load functions

bool PatchCostumeLoads123()
{
	//WB1
	BYTE CostumePatch1[] = { 0xBE, (BYTE)Costume, 0x00, 0x00, 0x00 };
	BYTE CostumePatch2[] = { 0xC7, 0x82, 0xF8, 0x02, 0x00, 0x00, (BYTE)Costume, 0x00, 0x00, 0x00 };

	BYTE* WB1CL1 = PatternScan("BE ? ? ? ? 74 ? 8B 82 F0 02 00 00", tomb1DLL);
	BYTE* WB1CL2 = PatternScan("C7 82 F8 02 00 00 ? ? ? ?", tomb1DLL);

	if (!WB1CL1 || !WB1CL2)
	{
		std::cout << "[!] WB1 costume load not found!" << "\n";
		return false;
	}

	Patch(CostumePatch1, WB1CL1, sizeof(CostumePatch1));
	Patch(CostumePatch2, WB1CL2, sizeof(CostumePatch2));

	//WB2
	BYTE CostumePatch3[] = { 0x41, 0xB9, (BYTE)Costume, 0x00, 0x00, 0x00 };

	BYTE* WB2CL = PatternScan("41 B9 ? ? ? ? 66 83 0D ? ? ? ? ?", tomb1DLL);

	if (!WB2CL)
	{
		std::cout << "[!] WB2 costume load not found!" << "\n";
		return false;
	}

	Patch(CostumePatch3, WB2CL, sizeof(CostumePatch3));

	//TR2 LOAD
	BYTE* TR2CL = PatternScan("C7 82 F8 02 00 00 ? ? ? ? 44 0F B7 0D ? ? ? ?", tomb2DLL);

	if (!TR2CL)
	{
		std::cout << "[!] TR2 costume load not found!" << "\n";
		return false;
	}

	BYTE CostumePatch4[] = { 0xC7, 0x82, 0xF8, 0x02, 0x00, 0x00, (BYTE)Costume, 0x00, 0x00, 0x00 };
	Patch(CostumePatch4, TR2CL, sizeof(CostumePatch4));

	//TR3 LOAD
	BYTE* TR3CL = PatternScan("C7 82 F8 02 00 00 ? ? ? ? 0F B7 15 ? ? ? ?", tomb3DLL);

	if (!TR3CL)
	{
		std::cout << "[!] TR3 costume load not found!" << "\n";
		return false;
	}

	Patch(CostumePatch4, TR3CL, sizeof(CostumePatch4));

	return true;
}

void PatchModules()
{
	LoadConfig();

	TCHAR ExeName[MAX_PATH + 1];
	GetModuleFileName(NULL, ExeName, MAX_PATH + 1);

	std::string ExeNameStr = ExeName;

	if (ExeNameStr.contains(ExeName123))
	{
		PatchModules123();
	}
	else if (ExeNameStr.contains(ExeName456))
	{
		PatchModules456();
	}
	else
	{
		std::cout << "[!] Couldn't find game executable!" << "\n";
	}
}

void PatchModules123()
{
	//Modules

	std::cout << "[?] Finding tomb1..." << "\n";

	while (!tomb1DLL)
	{
		tomb1DLL = GetModuleHandle("tomb1.dll");
	}

	std::cout << "[?] Finding tomb2..." << "\n";

	while (!tomb2DLL)
	{
		tomb2DLL = GetModuleHandle("tomb2.dll");
	}

	std::cout << "[?] Finding tomb3..." << "\n";

	while (!tomb3DLL)
	{
		tomb3DLL = GetModuleHandle("tomb3.dll");
	}

	//Find game ptr that stores the current costume

	std::uintptr_t TR1GamePTR_Addr = (std::uintptr_t)PatternScan("48 8B 0D ? ? ? ? FF C0", tomb1DLL);
	std::uintptr_t TR2GamePTR_Addr = (std::uintptr_t)PatternScan("48 8B 0D ? ? ? ? FF C0", tomb2DLL);
	std::uintptr_t TR3GamePTR_Addr = (std::uintptr_t)PatternScan("48 8B 0D ? ? ? ? FF C0", tomb3DLL);


	if (!TR1GamePTR_Addr || !TR2GamePTR_Addr || !TR3GamePTR_Addr)
	{
		std::cout << "[!] Couldn't find TRR Game PTR Address!" << "\n";
		return;
	}

	__int64 TRRGamePTR = *(__int64*)GetAddressFromInstruction(TR1GamePTR_Addr, 7);

	if (!TRRGamePTR)
	{
		TRRGamePTR = *(__int64*)GetAddressFromInstruction(TR2GamePTR_Addr, 7);

		if (!TRRGamePTR)
		{
			TRRGamePTR = *(__int64*)GetAddressFromInstruction(TR3GamePTR_Addr, 7);

			if (!TRRGamePTR)
			{
				std::cout << "[!] Couldn't find any TRR Game PTR!" << "\n";
				return;
			}
		}
	}

	SelectedCostumePTR = (int*)(TRRGamePTR + 0x2F8);
	*SelectedCostumePTR = Costume;

	//Keep photo mode costume

	BYTE* OnPhotoModeExitCostume_TR1 = PatternScan("0F 11 82 F8 02 00 00", tomb1DLL);
	BYTE* OnPhotoModeExitCostume_TR2 = PatternScan("0F 11 82 F8 02 00 00", tomb2DLL);
	BYTE* OnPhotoModeExitCostume_TR3 = PatternScan("0F 11 82 F8 02 00 00", tomb3DLL);

	BYTE OnPhotoModeExitCostumePatch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	if (!OnPhotoModeExitCostume_TR1 || !OnPhotoModeExitCostume_TR2 || !OnPhotoModeExitCostume_TR3)
	{
		std::cout << "[!] Couldn't find OnPhotoModeExit costume setter in module(s)!" << "\n";
		return;
	}

	Patch(OnPhotoModeExitCostumePatch, OnPhotoModeExitCostume_TR1, sizeof(OnPhotoModeExitCostumePatch));
	Patch(OnPhotoModeExitCostumePatch, OnPhotoModeExitCostume_TR2, sizeof(OnPhotoModeExitCostumePatch));
	Patch(OnPhotoModeExitCostumePatch, OnPhotoModeExitCostume_TR3, sizeof(OnPhotoModeExitCostumePatch));

	//Force selected costume everywhere

	if (bOverrideOriginals)
	{

		if (!PatchCostumeLoads123())
		{
			return;
		}

		Orig_PhotoModeTickTR1 = reinterpret_cast<PhotoModeTickTR123_t>(PatternScan("48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ? 4C 8B 0D ? ? ? ?", tomb1DLL));
		Orig_PhotoModeTickTR2 = reinterpret_cast<PhotoModeTickTR123_t>(PatternScan("48 89 5C 24 10 48 89 6C 24 18 48 89 7C 24 20 41 56 48 83 EC ? E8 ? ? ? ?", tomb2DLL));
		Orig_PhotoModeTickTR3 = reinterpret_cast<PhotoModeTickTR123_t>(PatternScan("48 89 5C 24 10 48 89 6C 24 18 48 89 7C 24 20 41 56 48 83 EC ? E8 ? ? ? ?", tomb3DLL));

		if (!Orig_PhotoModeTickTR1 || !Orig_PhotoModeTickTR2 || !Orig_PhotoModeTickTR3)
		{
			std::cout << "[!] Couldn't find photo mode tick(s)!" << "\n";
			return;
		}

		Orig_PhotoModeTickTR1 = reinterpret_cast<PhotoModeTickTR123_t>(TrampHook64((BYTE*)Orig_PhotoModeTickTR1, (BYTE*)hk_PhotoModeTickTR1, 12));
		Orig_PhotoModeTickTR2 = reinterpret_cast<PhotoModeTickTR123_t>(TrampHook64((BYTE*)Orig_PhotoModeTickTR2, (BYTE*)hk_PhotoModeTickTR2, 17));
		Orig_PhotoModeTickTR3 = reinterpret_cast<PhotoModeTickTR123_t>(TrampHook64((BYTE*)Orig_PhotoModeTickTR3, (BYTE*)hk_PhotoModeTickTR3, 17));

		//C7 82 F8 02 00 00 03 00 00 00

		BYTE* CostumeSwitchTableTR2 = PatternScan("77 ? 4C 8D 0D ? ? ? ? 43 8B 8C 99 DC 80 06 00", tomb2DLL);
		BYTE* CostumeSwitchTableTR3 = PatternScan("77 ? 4C 8D 0D ? ? ? ? 41 8B 8C 81 4C 59 0A 00", tomb3DLL);

		if (!CostumeSwitchTableTR2 || !CostumeSwitchTableTR3)
		{
			std::cout << "[!] Couldn't find costume switch table(s)!" << "\n";
			return;
		}

		BYTE SwitchTablePatch[] = { 0xEB };
		Patch(SwitchTablePatch, CostumeSwitchTableTR2, sizeof(SwitchTablePatch));
		Patch(SwitchTablePatch, CostumeSwitchTableTR3, sizeof(SwitchTablePatch));
	}

	//Save pose, face, roll and FOV

	if (bSavePhotoModeSettings)
	{
		BYTE* PoseFaceResetTR1 = PatternScan("0F 11 4A B0 0F 10 48 D0 0F 11 42 C0 0F 10 40 E0 0F 11 4A D0 0F 10 48 F0 0F 11 42 E0 0F 11 4A F0 48 83 E9 ? 75 ? 0F 10 00 48 8B 48 10", tomb1DLL);
		BYTE* PoseFaceResetTR2 = PatternScan("0F 11 4A B0 0F 10 48 D0 0F 11 42 C0 0F 10 40 E0 0F 11 4A D0 0F 10 48 F0 0F 11 42 E0 0F 11 4A F0 48 83 E9 ? 75 ? 0F 10 00 48 8B 48 10", tomb2DLL);
		BYTE* PoseFaceResetTR3 = PatternScan("0F 11 4A B0 0F 10 48 D0 0F 11 42 C0 0F 10 40 E0 0F 11 4A D0 0F 10 48 F0 0F 11 42 E0 0F 11 4A F0 48 83 E9 ? 75 ? 0F 10 00 48 8B 48 10", tomb3DLL);

		if (!PoseFaceResetTR1 || !PoseFaceResetTR2 || !PoseFaceResetTR3)
		{
			std::cout << "[!] Couldn't find pose and face reset(s)!" << "\n";
			return;
		}

		BYTE PoseFaceResetPatch[] = { 0x90, 0x90, 0x90, 0x90 };
		Patch(PoseFaceResetPatch, PoseFaceResetTR1, sizeof(PoseFaceResetPatch));
		Patch(PoseFaceResetPatch, PoseFaceResetTR2, sizeof(PoseFaceResetPatch));
		Patch(PoseFaceResetPatch, PoseFaceResetTR3, sizeof(PoseFaceResetPatch));

	}

	//Free move

	if (bFreeMove)
	{
		BYTE* ResetPositionTR1 = PatternScan("41 0F 11 40 58 41 89 40 68", tomb1DLL);
		BYTE* ResetPositionTR2 = PatternScan("41 0F 11 40 58 41 89 40 68", tomb2DLL);
		BYTE* ResetPositionTR3 = PatternScan("41 0F 11 40 58 41 89 40 68", tomb3DLL);

		if (!ResetPositionTR1 || !ResetPositionTR2 || !ResetPositionTR3)
		{
			std::cout << "[!] Couldn't find position reset(s)!" << "\n";
			return;
		}

		BYTE PositionResetPatch[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
		Patch(PositionResetPatch, ResetPositionTR1, sizeof(PositionResetPatch));
		Patch(PositionResetPatch, ResetPositionTR2, sizeof(PositionResetPatch));
		Patch(PositionResetPatch, ResetPositionTR3, sizeof(PositionResetPatch));
	}

	std::cout << "[-] Done!" << "\n";
}

void PatchModules456()
{
	//Modules

	std::cout << "[?] Finding tomb4..." << "\n";

	while (!tomb4DLL)
	{
		tomb4DLL = GetModuleHandle("tomb4.dll");
	}

	std::cout << "[?] Finding tomb5..." << "\n";

	while (!tomb5DLL)
	{
		tomb5DLL = GetModuleHandle("tomb5.dll");
	}

	std::cout << "[?] Finding tomb6..." << "\n";

	while (!tomb6DLL)
	{
		tomb6DLL = GetModuleHandle("tomb6.dll");
	}

	//Keep photo mode costume

	BYTE* OnPhotoModeExitCostume_TR4 = PatternScan("0F 11 83 F8 04 00 00", tomb4DLL);
	BYTE* OnPhotoModeExitCostume_TR5 = PatternScan("0F 11 83 F8 04 00 00", tomb5DLL);

	BYTE OnPhotoModeExitCostumePatch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	if (!OnPhotoModeExitCostume_TR4 || !OnPhotoModeExitCostume_TR5)
	{
		std::cout << "[!] Couldn't find OnPhotoModeExit costume setter in module(s)!" << "\n";
		return;
	}

	Patch(OnPhotoModeExitCostumePatch, OnPhotoModeExitCostume_TR4, sizeof(OnPhotoModeExitCostumePatch));
	Patch(OnPhotoModeExitCostumePatch, OnPhotoModeExitCostume_TR5, sizeof(OnPhotoModeExitCostumePatch));

	//Free move

	if (bFreeMove)
	{
		BYTE* ResetPositionTR4 = PatternScan("0F 11 47 60", tomb4DLL);
		BYTE* ResetPositionTR5 = PatternScan("0F 11 47 60 8B 05 ? ? ? ?", tomb5DLL);
		BYTE* ResetPositionsTR6_1 = PatternScan("F3 0F 11 40 40 F3 0F 10 0D ? ? ? ? F3 0F 11 48 44 F3 0F 10 05 ? ? ? ? F3 0F 11 40 48 F3 0F 10 0D ? ? ? ? F3 0F 11 48 4C F3 0F 10 05 ? ? ? ? F3 0F 11 81 10 02 00 00", tomb6DLL);
		BYTE* ResetPositionsTR6_2 = PatternScan("F3 0F 11 43 40 F3 0F 10 0D ? ? ? ? F3 0F 11 4B 44 F3 0F 10 05 ? ? ? ? F3 0F 11 43 48 F3 0F 10 0D ? ? ? ? F3 0F 11 4B 4C 48 8B 47 60", tomb6DLL);

		if (!ResetPositionTR4 || !ResetPositionTR5 || !ResetPositionsTR6_1 || !ResetPositionsTR6_2)
		{
			std::cout << "[!] Couldn't find position reset(s)!" << "\n";
			return;
		}

		BYTE PositionResetPatch1[] = { 0x90, 0x90, 0x90, 0x90 };
		BYTE PositionResetPatch2[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };

		Patch(PositionResetPatch1, ResetPositionTR4, sizeof(PositionResetPatch1));
		Patch(PositionResetPatch1, ResetPositionTR5, sizeof(PositionResetPatch1));
		Patch(PositionResetPatch2, ResetPositionsTR6_1, sizeof(PositionResetPatch2));
		Patch(PositionResetPatch2, ResetPositionsTR6_1 + 0xD, sizeof(PositionResetPatch2));
		Patch(PositionResetPatch2, ResetPositionsTR6_1 + 0x1A, sizeof(PositionResetPatch2));
		Patch(PositionResetPatch2, ResetPositionsTR6_2, sizeof(PositionResetPatch2));
		Patch(PositionResetPatch2, ResetPositionsTR6_2 + 0xD, sizeof(PositionResetPatch2));
		Patch(PositionResetPatch2, ResetPositionsTR6_2 + 0x1A, sizeof(PositionResetPatch2));
	}

	//Brightness

	BYTE* BrightnessResetTR4 = PatternScan("89 8B E8 09 00 00", tomb4DLL); //6
	BYTE* BrightnessResetTR5 = PatternScan("89 8B E8 09 00 00", tomb5DLL); //6

	BYTE* BrightnessMaxTR4 = PatternScan("47 89 84 3E 7C 9F 1B 00", tomb4DLL); //8
	BYTE* BrightnessMaxTR5 = PatternScan("47 89 84 3E 1C 49 1B 00", tomb5DLL); //8

	if (!BrightnessResetTR4 || !BrightnessResetTR5 || !BrightnessMaxTR4 || !BrightnessMaxTR5)
	{
		std::cout << "[!] Couldn't find brightness reset(s)!" << "\n";
		return;
	}

	BYTE BrightnessResetPatch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	BYTE BrightnessMaxPatch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	Patch(BrightnessResetPatch, BrightnessResetTR4, sizeof(BrightnessResetPatch));
	Patch(BrightnessResetPatch, BrightnessResetTR5, sizeof(BrightnessResetPatch));
	Patch(BrightnessMaxPatch, BrightnessMaxTR4, sizeof(BrightnessMaxPatch));
	Patch(BrightnessMaxPatch, BrightnessMaxTR5, sizeof(BrightnessMaxPatch));

	std::cout << "[-] Done!" << "\n";
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)PatchModules, hModule, 0, nullptr);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

__int64 hk_PhotoModeTickTR1()
{
	if (Costume != *SelectedCostumePTR)
	{
		Costume = *SelectedCostumePTR;
		SaveConfig();
		PatchCostumeLoads123();
	}

	return Orig_PhotoModeTickTR1();
}

__int64 hk_PhotoModeTickTR2()
{
	if (Costume != *SelectedCostumePTR)
	{
		Costume = *SelectedCostumePTR;
		SaveConfig();
		PatchCostumeLoads123();
	}

	return Orig_PhotoModeTickTR2();
}

__int64 hk_PhotoModeTickTR3()
{
	if (Costume != *SelectedCostumePTR)
	{
		Costume = *SelectedCostumePTR;
		SaveConfig();
		PatchCostumeLoads123();
	}

	return Orig_PhotoModeTickTR3();
}
