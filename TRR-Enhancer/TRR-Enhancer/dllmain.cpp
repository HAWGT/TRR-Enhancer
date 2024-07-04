#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Helpers.h"

const char ConfigFileName[] = "TRREnhancer.ini";
int bConsole = 0;
int Costume = 1;
int bOverrideOriginals = 1;
int bSavePhotoModeSettings = 1;
int bFreeMove = 0;

HMODULE tomb1DLL = nullptr;
HMODULE tomb2DLL = nullptr;
HMODULE tomb3DLL = nullptr;

int* SelectedCostumePTR = nullptr;

typedef __int16(*PhotoModeTickTR1_t)(void);
PhotoModeTickTR1_t Orig_PhotoModeTickTR1;
__int16 hk_PhotoModeTickTR1();

typedef __int64(*PhotoModeTickTR23_t)(void);
PhotoModeTickTR23_t Orig_PhotoModeTickTR2;
PhotoModeTickTR23_t Orig_PhotoModeTickTR3;
__int64 hk_PhotoModeTickTR2();
__int64 hk_PhotoModeTickTR3();

void SaveConfig()
{
	std::ofstream ConfigFile;

	ConfigFile.open(ConfigFileName, std::ofstream::out | std::ofstream::trunc);

	ConfigFile << "Console " << bConsole << std::endl;
	ConfigFile << "Costume " << Costume << std::endl;
	ConfigFile << "OverrideOriginals " << bOverrideOriginals << std::endl;
	ConfigFile << "SavePhotoModeSettings " << bSavePhotoModeSettings << std::endl;
	ConfigFile << "FreeMove " << bFreeMove << std::endl;

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
				std::cout << "[*] TRR Enhancer" << std::endl;
				std::cout << "[+] Settings Loaded: " << std::endl;
				std::cout << param << " " << value << std::endl;
			}
			if (!param.compare("Costume"))
			{
				Costume = (value >= 1 && value <= 14) ? value : 1;
				std::cout << param << " " << Costume << std::endl;
			}
			if (!param.compare("OverrideOriginals"))
			{
				bOverrideOriginals = value == 1;
				std::cout << param << " " << bOverrideOriginals << std::endl;
			}
			if (!param.compare("SavePhotoModeSettings"))
			{
				bSavePhotoModeSettings = value == 1;
				std::cout << param << " " << bSavePhotoModeSettings << std::endl;
			}
			if (!param.compare("FreeMove"))
			{
				bFreeMove = value == 1;
				std::cout << param << " " << bFreeMove << std::endl;
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

bool PatchCostumeLoads()
{
	//WB1
	BYTE CostumePatch1[] = { 0xBE, (BYTE)Costume, 0x00, 0x00, 0x00 };
	BYTE CostumePatch2[] = { 0x41, 0xBB, (BYTE)Costume, 0x00, 0x00, 0x00 };

	BYTE* WB1CL1 = PatternScan("BE ? ? ? ? 74 ? 8B 82 C4 02 00 00", tomb1DLL);
	BYTE* WB1CL2 = PatternScan("41 BB ? ? ? ? 8B 05 ? ? ? ?", tomb1DLL);

	if (!WB1CL1 || !WB1CL2)
	{
		std::cout << "[!] WB1 costume load not found!" << std::endl;
		return false;
	}

	Patch(CostumePatch1, WB1CL1, sizeof(CostumePatch1));
	Patch(CostumePatch2, WB1CL2, sizeof(CostumePatch2));

	//WB2
	BYTE CostumePatch3[] = { 0x41, 0xB9, (BYTE)Costume, 0x00, 0x00, 0x00 };

	BYTE* WB2CL = PatternScan("41 B9 ? ? ? ? 66 83 0D ? ? ? ? ?", tomb1DLL);

	if (!WB2CL)
	{
		std::cout << "[!] WB2 costume load not found!" << std::endl;
		return false;
	}

	Patch(CostumePatch3, WB2CL, sizeof(CostumePatch3));

	//TR2 LOAD
	BYTE* TR2CL = PatternScan("C7 82 CC 02 00 00 ? ? ? ? 44 0F B7 0D ? ? ? ?", tomb2DLL);

	if (!TR2CL)
	{
		std::cout << "[!] TR2 costume load not found!" << std::endl;
		return false;
	}

	BYTE CostumePatch5[] = { 0xC7, 0x82, 0xCC, 0x02, 0x00, 0x00, (BYTE)Costume, 0x00, 0x00, 0x00 };
	Patch(CostumePatch5, TR2CL, sizeof(CostumePatch5));

	//TR3 LOAD
	BYTE* TR3CL = PatternScan("C7 82 CC 02 00 00 ? ? ? ? 0F B7 15 ? ? ? ?", tomb3DLL);

	if (!TR3CL)
	{
		std::cout << "[!] TR3 costume load not found!" << std::endl;
		return false;
	}

	Patch(CostumePatch5, TR3CL, sizeof(CostumePatch5));

	return true;
}

void PatchModules()
{
	LoadConfig();

	//Modules

	std::cout << "[?] Finding tomb1..." << std::endl;

	while (!tomb1DLL)
	{
		tomb1DLL = GetModuleHandle("tomb1.dll");
	}

	std::cout << "[?] Finding tomb2..." << std::endl;

	while (!tomb2DLL)
	{
		tomb2DLL = GetModuleHandle("tomb2.dll");
	}

	std::cout << "[?] Finding tomb3..." << std::endl;

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
		std::cout << "[!] Couldn't find TRR Game PTR Address!" << std::endl;
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
				std::cout << "[!] Couldn't find any TRR Game PTR!" << std::endl;
				return;
			}
		}
	}

	SelectedCostumePTR = (int*)(TRRGamePTR + 0x2CC);
	*SelectedCostumePTR = Costume;

	//Keep photo mode costume

	BYTE* OnPhotoModeExitCostume_TR1 = PatternScan("0F 11 83 CC 02 00 00", tomb1DLL);
	BYTE* OnPhotoModeExitCostume_TR2 = PatternScan("0F 11 83 CC 02 00 00", tomb2DLL);
	BYTE* OnPhotoModeExitCostume_TR3 = PatternScan("0F 11 83 CC 02 00 00", tomb3DLL);

	BYTE OnPhotoModeExitCostumePatch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

	if (!OnPhotoModeExitCostume_TR1 || !OnPhotoModeExitCostume_TR2 || !OnPhotoModeExitCostume_TR3)
	{
		std::cout << "[!] Couldn't find OnPhotoModeExit costume setter in module(s)!" << std::endl;
		return;
	}

	Patch(OnPhotoModeExitCostumePatch, OnPhotoModeExitCostume_TR1, sizeof(OnPhotoModeExitCostumePatch));
	Patch(OnPhotoModeExitCostumePatch, OnPhotoModeExitCostume_TR2, sizeof(OnPhotoModeExitCostumePatch));
	Patch(OnPhotoModeExitCostumePatch, OnPhotoModeExitCostume_TR3, sizeof(OnPhotoModeExitCostumePatch));

	//Force selected costume everywhere

	if (bOverrideOriginals)
	{

		if (!PatchCostumeLoads())
		{
			return;
		}

		Orig_PhotoModeTickTR1 = reinterpret_cast<PhotoModeTickTR1_t>(PatternScan("48 89 5C 24 10 48 89 74 24 18 57 48 83 EC ? E8 ? ? ? ?", tomb1DLL));
		Orig_PhotoModeTickTR2 = reinterpret_cast<PhotoModeTickTR23_t>(PatternScan("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC ? E8 ? ? ? ?", tomb2DLL));
		Orig_PhotoModeTickTR3 = reinterpret_cast<PhotoModeTickTR23_t>(PatternScan("48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC ? E8 ? ? ? ?", tomb3DLL));

		if (!Orig_PhotoModeTickTR1 || !Orig_PhotoModeTickTR2 || !Orig_PhotoModeTickTR3)
		{
			std::cout << "[!] Couldn't find photo mode tick(s)!" << std::endl;
			return;
		}

		Orig_PhotoModeTickTR1 = reinterpret_cast<PhotoModeTickTR1_t>(TrampHook64((BYTE*)Orig_PhotoModeTickTR1, (BYTE*)hk_PhotoModeTickTR1, 15));
		Orig_PhotoModeTickTR2 = reinterpret_cast<PhotoModeTickTR23_t>(TrampHook64((BYTE*)Orig_PhotoModeTickTR2, (BYTE*)hk_PhotoModeTickTR2, 15));
		Orig_PhotoModeTickTR3 = reinterpret_cast<PhotoModeTickTR23_t>(TrampHook64((BYTE*)Orig_PhotoModeTickTR3, (BYTE*)hk_PhotoModeTickTR3, 15));

		//C7 82 CC 02 00 00 03 00 00 00
		BYTE* CostumeSwitchTableTR2 = PatternScan("77 ? 4C 8D 0D ? ? ? ? 43 8B 8C 99 40 88 06 00", tomb2DLL);
		BYTE* CostumeSwitchTableTR3 = PatternScan("77 ? 4C 8D 0D ? ? ? ? 41 8B 8C 81 48 66 0A 00", tomb3DLL);

		if (!CostumeSwitchTableTR2 || !CostumeSwitchTableTR3)
		{
			std::cout << "[!] Couldn't find costume switch table(s)!" << std::endl;
			return;
		}

		BYTE SwitchTablePatch[] = { 0xEB };
		Patch(SwitchTablePatch, CostumeSwitchTableTR2, sizeof(SwitchTablePatch));
		Patch(SwitchTablePatch, CostumeSwitchTableTR3, sizeof(SwitchTablePatch));
	}

	//Save pose, face, roll and FOV

	if (bSavePhotoModeSettings)
	{
		BYTE* PoseFaceResetTR1 = PatternScan("0F 11 49 F0 0F 10 48 10", tomb1DLL);
		BYTE* PoseFaceResetTR2 = PatternScan("0F 11 49 F0 0F 10 48 10", tomb2DLL);
		BYTE* PoseFaceResetTR3 = PatternScan("0F 11 49 F0 0F 10 48 10", tomb3DLL);

		if (!PoseFaceResetTR1 || !PoseFaceResetTR2 || !PoseFaceResetTR3)
		{
			std::cout << "[!] Couldn't find pose and face reset(s)!" << std::endl;
			return;
		}

		BYTE PoseFaceResetPatch[] = { 0x90, 0x90, 0x90, 0x90 };
		Patch(PoseFaceResetPatch, PoseFaceResetTR1, sizeof(PoseFaceResetPatch));
		Patch(PoseFaceResetPatch, PoseFaceResetTR2, sizeof(PoseFaceResetPatch));
		Patch(PoseFaceResetPatch, PoseFaceResetTR3, sizeof(PoseFaceResetPatch));

		BYTE* RollResetTR1 = PatternScan("0F 11 01 0F 10 40 20 48 8D 80 80 00 00 00 0F 11 49 10 0F 10 48 B0 0F 11 41 20 0F 10 40 C0 0F 11 49 30 0F 10 48 D0 0F 11 41 40 0F 10 40 E0 0F 11 49 50 0F 10 48 F0]", tomb1DLL);
		BYTE* RollResetTR2 = PatternScan("0F 11 01 0F 10 40 20 48 8D 80 80 00 00 00 0F 11 49 10 0F 10 48 B0 0F 11 41 20 0F 10 40 C0 0F 11 49 30 0F 10 48 D0 0F 11 41 40 0F 10 40 E0 0F 11 49 50 0F 10 48 F0]", tomb2DLL);
		BYTE* RollResetTR3 = PatternScan("0F 11 01 0F 10 40 20 48 8D 80 80 00 00 00 0F 11 49 10 0F 10 48 B0 0F 11 41 20 0F 10 40 C0 0F 11 49 30 0F 10 48 D0 0F 11 41 40 0F 10 40 E0 0F 11 49 50 0F 10 48 F0]", tomb3DLL);

		if (!RollResetTR1 || !RollResetTR2 || !RollResetTR3)
		{
			std::cout << "[!] Couldn't find roll reset(s)!" << std::endl;
			return;
		}

		BYTE RollResetPatch[] = { 0x90, 0x90, 0x90 };
		Patch(RollResetPatch, RollResetTR1, sizeof(RollResetPatch));
		Patch(RollResetPatch, RollResetTR2, sizeof(RollResetPatch));
		Patch(RollResetPatch, RollResetTR3, sizeof(RollResetPatch));

		BYTE* FovResetTR1 = PatternScan("0F 11 41 20 0F 10 40 C0 0F 11 49 30 0F 10 48 D0 0F 11 41 40 0F 10 40 E0 0F 11 49 50 0F 10 48 F0", tomb1DLL);
		BYTE* FovResetTR2 = PatternScan("0F 11 41 20 0F 10 40 C0 0F 11 49 30 0F 10 48 D0 0F 11 41 40 0F 10 40 E0 0F 11 49 50 0F 10 48 F0", tomb2DLL);
		BYTE* FovResetTR3 = PatternScan("0F 11 41 20 0F 10 40 C0 0F 11 49 30 0F 10 48 D0 0F 11 41 40 0F 10 40 E0 0F 11 49 50 0F 10 48 F0", tomb3DLL);

		if (!FovResetTR1 || !FovResetTR2 || !FovResetTR3)
		{
			std::cout << "[!] Couldn't find FOV reset(s)!" << std::endl;
			return;
		}

		BYTE FOVResetPatch[] = { 0x90, 0x90, 0x90, 0x90 };
		Patch(FOVResetPatch, FovResetTR1, sizeof(FOVResetPatch));
		Patch(FOVResetPatch, FovResetTR2, sizeof(FOVResetPatch));
		Patch(FOVResetPatch, FovResetTR3, sizeof(FOVResetPatch));
	}

	//Free move

	if (bFreeMove)
	{
		BYTE* ResetPositionTR1 = PatternScan("41 0F 11 41 58", tomb1DLL);
		BYTE* ResetPositionTR2 = PatternScan("41 0F 11 41 58", tomb2DLL);
		BYTE* ResetPositionTR3 = PatternScan("41 0F 11 41 58", tomb3DLL);

		if (!ResetPositionTR1 || !ResetPositionTR2 || !ResetPositionTR3)
		{
			std::cout << "[!] Couldn't find position reset(s)!" << std::endl;
			return;
		}

		BYTE PositionResetPatch[] = { 0x90, 0x90, 0x90, 0x90, 0x90 };
		Patch(PositionResetPatch, ResetPositionTR1, sizeof(PositionResetPatch));
		Patch(PositionResetPatch, ResetPositionTR2, sizeof(PositionResetPatch));
		Patch(PositionResetPatch, ResetPositionTR3, sizeof(PositionResetPatch));
	}

	std::cout << "[-] Done!" << std::endl;
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

__int16 hk_PhotoModeTickTR1()
{
	if (Costume != *SelectedCostumePTR)
	{
		Costume = *SelectedCostumePTR;
		SaveConfig();
		PatchCostumeLoads();
	}

	return Orig_PhotoModeTickTR1();
}

__int64 hk_PhotoModeTickTR2()
{
	if (Costume != *SelectedCostumePTR)
	{
		Costume = *SelectedCostumePTR;
		SaveConfig();
		PatchCostumeLoads();
	}

	return Orig_PhotoModeTickTR2();
}

__int64 hk_PhotoModeTickTR3()
{
	if (Costume != *SelectedCostumePTR)
	{
		Costume = *SelectedCostumePTR;
		SaveConfig();
		PatchCostumeLoads();
	}

	return Orig_PhotoModeTickTR3();
}
