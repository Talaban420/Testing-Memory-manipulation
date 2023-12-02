#include <iostream>
#include <windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <vector>

DWORD GetModuleBaseAddress(const TCHAR *lpszModuleName, DWORD pID)
{
    DWORD dwModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID); // make snapshot of all modules within process
    MODULEENTRY32 ModuleEntry32 = {0};
    ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &ModuleEntry32)) // store first Module in ModuleEntry32
    {
        do
        {
            if (_tcsicmp(ModuleEntry32.szModule, lpszModuleName) == 0) // if Found Module matches Module we look for -> done!
            {
                dwModuleBaseAddress = (uintptr_t)ModuleEntry32.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &ModuleEntry32)); // go through Module entries in Snapshot and store in ModuleEntry32
    }
    CloseHandle(hSnapshot);
    return dwModuleBaseAddress;
}

DWORD GetPointerAddress(HWND hwnd, uintptr_t gameBaseAddr, uintptr_t address, std::vector<uintptr_t> offsets)
{
    DWORD pID = 0; // Game process ID
    GetWindowThreadProcessId(hwnd, &pID);
    HANDLE phandle = NULL;
    phandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pID);
    if (!phandle)
    {
        std::cout << "OpenProcess failed with error " << GetLastError() << std::endl;
        return 1; // or handle the error in another way
    }
    uintptr_t offset_null = 0;
    ReadProcessMemory(phandle, (LPVOID *)(gameBaseAddr + address), &offset_null, sizeof(offset_null), 0);
    if (!ReadProcessMemory(phandle, (LPVOID *)(gameBaseAddr + address), &offset_null, sizeof(offset_null), 0))
    {
        std::cout << "ReadProcessMemory failed with error " << GetLastError() << std::endl;
        return 0; // or handle the error in another way
    }

    uintptr_t pointeraddress = offset_null;      // the address we need
    for (int i = 0; i < offsets.size() - 1; i++) // we dont want to change the last offset value so we do -1
    {
        ReadProcessMemory(phandle, (LPVOID *)(pointeraddress + offsets.at(i)), &pointeraddress, sizeof(pointeraddress), 0);
    }
    CloseHandle(phandle);
    return pointeraddress += offsets.at(offsets.size() - 1); // adding the last offset
}

int main(int argc, char const *argv[])
{
    HWND hwnd = FindWindowW(NULL, L"Super Meat Boy v1.2.5");
    SetForegroundWindow(hwnd);
    if (hwnd)
    {
        // Get process handle and id write to console window
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);
        std::cout << "Found Window With Process ID: 0x" << std::hex << processId << std::endl;
        HANDLE Phandle = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
        if (!Phandle)
        {
            std::cout << "OpenProcess failed with error " << GetLastError() << std::endl;
            return 1; // or handle the error in another way
        }
        // Get Module Base address write to console window
        TCHAR GamemoduleNum1[] = _T("SuperMeatBoy.exe");
        DWORD gamebaseaddress1 = GetModuleBaseAddress(GamemoduleNum1, processId);
        std::cout << "Module Base Address of Game: 0x" << std::hex << gamebaseaddress1 << std::endl;

        // Pointers And Address

        uintptr_t Timeraddress = 0x00302D18;
        std::vector<uintptr_t> timeoffset{0x384};
        float resetValue = 0;
        uintptr_t buffer = 0;
        // Get Module TimerpointerBase address write to console window
        uintptr_t baseaddi = GetPointerAddress(hwnd, gamebaseaddress1, Timeraddress, timeoffset);
        std::cout << "Base Address of Timer: 0x" << std::hex << baseaddi <<"\n\n\n";
        bool toggle = false;
        while (true)
        {

            if (GetAsyncKeyState(VK_NUMPAD1) & 1)
            {
                toggle = !toggle; //
            }
            if (toggle)
            {
                ReadProcessMemory(Phandle, (LPCVOID)baseaddi, &buffer, sizeof(buffer), 0);
                int i = 0;
                float timerValue = *(float *)&buffer;
                std::cout << "Clock Timer: " << timerValue << i << "\r" << std::flush;
                i++;
                WriteProcessMemory(Phandle, (LPVOID)baseaddi, &resetValue, sizeof(resetValue), 0);
            }

            Sleep(50); // Add a delay of 50 milliseconds
        }
        CloseHandle(Phandle);
        CloseHandle(hwnd);
    }

    else

    {
        std::cout << "Error!: Super Meat Boy Not Found!.." << std::endl;
        Sleep(5000);
        exit(0);
    }

    return 0;
}
