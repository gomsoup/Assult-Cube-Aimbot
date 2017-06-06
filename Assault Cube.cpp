#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <string>
#include <cmath>
#include <tchar.h>

#define DLL_NAME "ac.dll"
#define PI 3.141592654

using namespace std;

DWORD GetProcessID(wstring& TargetProcess) {
	PROCESSENTRY32 ProcessEntry;
	ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE ProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(ProcessSnapshot, &ProcessEntry))
	{
		while (Process32Next(ProcessSnapshot, &ProcessEntry)) {
			if (!TargetProcess.compare(ProcessEntry.szExeFile)) {
				CloseHandle(ProcessSnapshot);
				return ProcessEntry.th32ProcessID;
			}
		}
	}

	CloseHandle(ProcessSnapshot);
	return 0;
}

DWORD_PTR GetModuleBaseAddress(DWORD pid, TCHAR *TargetProcess) {
	DWORD_PTR ModuleBaseeAddress = 0;
	HANDLE ProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	
	if (ProcessSnapshot != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 ModuleEntry;
		ModuleEntry.dwSize = sizeof(MODULEENTRY32);

		if (Module32First(ProcessSnapshot, &ModuleEntry)) {
			do{
				if (_tcscmp(ModuleEntry.szModule, TargetProcess) == 0) {
					return (DWORD_PTR)ModuleEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(ProcessSnapshot, &ModuleEntry));
		}
	}

	CloseHandle(ProcessSnapshot);
	return 0;
}

typedef struct {
	DWORD Heath;
	float XPos;
	float YPos;
	float ZPos;
	float PlayerDistance;
	int Team;
	char *PlayerName;
}PlayerData;

class Aimbot {
private:
	bool IsAim = false;
	int PlayerAimed = 1;
	int NumberOfBot;
	float PlayerAngleX;
	DWORD pid;
	HANDLE ProcessHandle;
	wstring TargetProcess = L"ac_client.exe";

public:
	PlayerData cPlayer[32];
	PlayerData mPlayer;
	DWORD BaseAddress;
	DWORD PlayerBaseAddress;
	DWORD EntityBaseAddress;
	DWORD PlayerOffset = 0x109B74;
	DWORD EntityOffset = 0x110d90;

	Aimbot() {
		pid = GetProcessID(TargetProcess);
		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		BaseAddress = GetModuleBaseAddress(pid, _T("ac_client.exe"));

		ReadProcessMemory(ProcessHandle, (LPCVOID)(BaseAddress + PlayerOffset), &PlayerBaseAddress, sizeof(DWORD), 0);
		ReadProcessMemory(ProcessHandle, (LPCVOID)(BaseAddress + EntityOffset), &EntityBaseAddress, sizeof(DWORD), 0);
	}

	bool GetPlayerData() {
/*
		if (*(DWORD*)(BaseAddress + 0x110d90) == 0) // bot mode
			return false;
*/

		ReadProcessMemory(ProcessHandle, (LPCVOID)(BaseAddress + 0x10F500), &NumberOfBot, sizeof(DWORD), 0);

		ReadProcessMemory(ProcessHandle, (LPCVOID)(PlayerBaseAddress + 0xf8), &mPlayer.Heath, sizeof(DWORD), 0);
		ReadProcessMemory(ProcessHandle, (LPCVOID)(PlayerBaseAddress + 0x34), &mPlayer.XPos, sizeof(float), 0);
		ReadProcessMemory(ProcessHandle, (LPCVOID)(PlayerBaseAddress + 0x38), &mPlayer.YPos, sizeof(float), 0);
		ReadProcessMemory(ProcessHandle, (LPCVOID)(PlayerBaseAddress + 0x3c), &mPlayer.ZPos, sizeof(float), 0);
		ReadProcessMemory(ProcessHandle, (LPCVOID)(PlayerBaseAddress + 0x40), &PlayerAngleX, sizeof(float), 0);
		mPlayer.PlayerDistance = 0;


		DWORD EntityPointerTmp;

		for (int i = 0; i < NumberOfBot - 1; i++) { // pointer class -> dynamic alloc
			ReadProcessMemory(ProcessHandle, (LPCVOID)(EntityBaseAddress + i * 4), &EntityPointerTmp, sizeof(DWORD), 0);
			ReadProcessMemory(ProcessHandle, (LPCVOID)(EntityPointerTmp + 0xf8), &cPlayer[i + 1].Heath, sizeof(DWORD), 0);
			ReadProcessMemory(ProcessHandle, (LPCVOID)(EntityPointerTmp + 0x34), &cPlayer[i + 1].XPos, sizeof(float), 0);
			ReadProcessMemory(ProcessHandle, (LPCVOID)(EntityPointerTmp + 0x38), &cPlayer[i + 1].YPos, sizeof(float), 0);
			ReadProcessMemory(ProcessHandle, (LPCVOID)(EntityPointerTmp + 0x3c), &cPlayer[i + 1].ZPos, sizeof(float), 0);
			cPlayer[i + 1].PlayerDistance = GetDistance(i + 1);
		}
		return true;
	}
	float GetDistance(int PlayerID) {
		float pDistance;
		float dX, dY, dZ;
		
		dX = cPlayer[PlayerID].XPos - mPlayer.XPos;
		dY = cPlayer[PlayerID].YPos - mPlayer.YPos;
		dZ = cPlayer[PlayerID].ZPos - mPlayer.ZPos;
		
		pDistance = sqrt(dX*dX + dY*dY + dZ*dZ);
		return pDistance;
	}
	void AimToEntity(int PlayerID) {/*
		float MouseX, MouseY, GoMouseX, GoMouseY, dX, dY, YAW, dZ, HYP, PITCH;

		dX = cPlayer[PlayerID].XPos - mPlayer.XPos;
		dY = cPlayer[PlayerID].YPos - mPlayer.YPos;
		dZ = cPlayer[PlayerID].ZPos - mPlayer.ZPos;
		HYP = sqrt(pow(dZ, 2) + pow(dX, 2));

		GoMouseX = 90.0f + (float)(atan2f(dX, dZ) * 57.29577513082f);
		GoMouseY = (float)(atan2f(dY, HYP) * 57.29577513082f);

	//	cout << "cPlayer[ " << PlayerID << "]'s D : " << cPlayer[PlayerID].PlayerDistance << endl;
		WriteProcessMemory(ProcessHandle, (LPVOID)(PlayerBaseAddress + 0x40), &GoMouseX, sizeof(float), 0);
		WriteProcessMemory(ProcessHandle, (LPVOID)(PlayerBaseAddress + 0x44), &GoMouseY, sizeof(float), 0);
		*/


		float MouseX, MouseY, GoMouseX, GoMouseY, dX, dY, dZ;

		dX = cPlayer[PlayerID].XPos - mPlayer.XPos;
		dY = cPlayer[PlayerID].YPos - mPlayer.YPos;
		dZ = cPlayer[PlayerID].ZPos - mPlayer.ZPos;
		/*
				if (cPlayer[PlayerID].XPos > mPlayer.XPos && cPlayer[PlayerID].YPos < mPlayer.YPos)
					MouseX = atanf(dX / dY) * 180.0f / PI;
				else if (cPlayer[PlayerID].XPos > mPlayer.XPos && cPlayer[PlayerID].YPos > mPlayer.YPos)
					MouseX = atanf(dX / dY) * 180.0f / PI + 180.0f;
				else if (cPlayer[PlayerID].XPos < mPlayer.XPos && cPlayer[PlayerID].YPos > mPlayer.YPos)
					MouseX = atanf(dX / dY) * 180.0f / PI - 180.0f;
				else
					MouseX = atanf(dX / dY) * 180.0f / PI + 360.0f;

				MouseY = asinf(dZ / cPlayer[PlayerID].PlayerDistance) * 180.0f / PI;
		*/

		if (dX > 0 && dY > 0) {//Calculate MouseX 4
			//MouseX = 180.0f + atanf(dY / dX) * 180 / PI;
			MouseX = 180.0f - atanf(dX / dY) * 180 / PI;
			GoMouseX = MouseX;
		}
		else if (dX >0 && dY < 0) { // 1
			//MouseX = atanf(dY / dX) * 180 / PI;
			MouseX = - atanf(dX / dY) * 180 / PI;
			GoMouseX = MouseX;
		}
		else if (dX < 0 && dY>0){ //3
			//MouseX = 180.0f + atanf(dY / dX) * 180 / PI;
			MouseX = 180.0f - atanf(dX / dY) * 180 / PI;
			GoMouseX = MouseX;
		}
		else if (dX < 0 && dY < 0) { // 2
			//MouseX = 360.0f + atanf(dY / dX) * 180 / PI + 180;
			MouseX = 360.0f - atanf(dX / dY) * 180 / PI;
			GoMouseX = MouseX;
		}
		MouseY = asinf(dZ / cPlayer[PlayerID].PlayerDistance) * 180 / PI ;
		
		//GoMouseX = 180 - MouseX;
		GoMouseY = MouseY;

		WriteProcessMemory(ProcessHandle, (LPVOID)(PlayerBaseAddress + 0x40), &GoMouseX, sizeof(float), 0);
		WriteProcessMemory(ProcessHandle, (LPVOID)(PlayerBaseAddress + 0x44), &GoMouseY, sizeof(float), 0);

	}
	void FlyToEntity(int PlayerID) {
		float GotoX, GotoY, GotoZ;
		GotoX = cPlayer[PlayerID].XPos+1.5;
		GotoY = cPlayer[PlayerID].YPos+1.5;
		GotoZ = cPlayer[PlayerID].ZPos;

		WriteProcessMemory(ProcessHandle, (LPVOID)(PlayerBaseAddress + 0x34), &GotoX, sizeof(float), 0); //x
		WriteProcessMemory(ProcessHandle, (LPVOID)(PlayerBaseAddress + 0x38), &GotoY, sizeof(float), 0); //y
		WriteProcessMemory(ProcessHandle, (LPVOID)(PlayerBaseAddress + 0x3c), &GotoZ, sizeof(float), 0); // z
	}


	void DoAimbot() {
		if (!GetAsyncKeyState(VK_RBUTTON)) {
			if (GetPlayerData() == TRUE) {
				if (IsAim == false || (cPlayer[PlayerAimed].Heath <= 0 || cPlayer[PlayerAimed].Heath > 100)) { // new aim
					int Nearest = 1;

					for (int i = 1; i < NumberOfBot; i++) {
						cout << "Entity D : " << cPlayer[i].PlayerDistance << endl;
						if ((cPlayer[Nearest].PlayerDistance == 0 || cPlayer[Nearest].PlayerDistance > cPlayer[i].PlayerDistance)) // alive check
							if (cPlayer[i].Heath > 0 && cPlayer[i].Heath <= 100) {
								Nearest = i;
								cout << "cPlayer[" << i << "] Aimed" << endl;
								break;
							}
							
					}

					PlayerAimed = Nearest;

					//FlyToEntity(Nearest);
					AimToEntity(Nearest);

					IsAim = true;
				}
				else if (IsAim == true && (cPlayer[PlayerAimed].Heath > 0 || cPlayer[PlayerAimed].Heath <= 100)) {
					 // new aim
					
					
					//FlyToEntity(PlayerAimed);
					AimToEntity(PlayerAimed);
				}
				else
					IsAim = false;
			}

			
		}
		else IsAim = false;
		
	}
};





int main() {
	Aimbot test;

	while (true) {
		test.DoAimbot();
	}
	return 0;
}