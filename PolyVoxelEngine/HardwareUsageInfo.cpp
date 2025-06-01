#include "HardwareUsageInfo.h"
#include <psapi.h>
#include <iostream>

ULARGE_INTEGER HardwareUsageInfo::lastCPU, HardwareUsageInfo::lastSysCPU, HardwareUsageInfo::lastUserCPU;
int HardwareUsageInfo::numProcessors;
HANDLE HardwareUsageInfo::self;

nvmlDevice_t HardwareUsageInfo::device = 0;
nvmlUtilization_t HardwareUsageInfo::utilization;
nvmlMemory_t HardwareUsageInfo::vramUsage;

int HardwareUsageInfo::init()
{
    // cpu
    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));

    self = GetCurrentProcess();
    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
    
    // gpu
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) 
    {
        std::cerr << "Failed to initialize NVML: " << nvmlErrorString(result) << "\n";
        return -1;
    }
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS) 
    {
        nvmlShutdown();
        std::cerr << "Failed to get device handle: " << nvmlErrorString(result) << "\n";
        return -1;
    }
}

void HardwareUsageInfo::destroy()
{
    nvmlShutdown();
}

int HardwareUsageInfo::getCPUUsage()
{
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));

    double percent = (sys.QuadPart - lastSysCPU.QuadPart) +
        (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent /= numProcessors;

    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;
    return percent * 100.0;
}

size_t HardwareUsageInfo::getRAMUsage()
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(self, &pmc, sizeof(pmc))) 
    {
        return pmc.WorkingSetSize;
    }
    else 
    {
        return 0;
    }
}

nvmlUtilization_t* HardwareUsageInfo::getGPUUtilization()
{
    nvmlReturn_t result = nvmlDeviceGetUtilizationRates(device, &utilization);
    if (result != NVML_SUCCESS) 
    {
        std::cerr << "Failed to get GPU utilization: " << nvmlErrorString(result) << "\n";
        return nullptr;
    }
    return &utilization;
}

nvmlMemory_t* HardwareUsageInfo::getVRAMUsage()
{
    nvmlReturn_t result = nvmlDeviceGetMemoryInfo(device, &vramUsage);
    if (result != NVML_SUCCESS)
    {
        std::cerr << "Failed to get VRAM usage: " << nvmlErrorString(result) << "\n";
        return nullptr;
    }
    return &vramUsage;
}
