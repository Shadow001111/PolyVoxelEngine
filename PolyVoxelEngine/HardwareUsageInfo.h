#pragma once
#include "NVIDIA/nvml.h"
#include "windows.h"
#undef max
#undef min

class HardwareUsageInfo
{
	static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
	static int numProcessors;
	static HANDLE self;

	static nvmlDevice_t device;
	static nvmlUtilization_t utilization;
	static nvmlMemory_t vramUsage;
public:
	static int init();
	static void destroy();

	static int getCPUUsage();
	static size_t getRAMUsage();

	static nvmlUtilization_t* getGPUUtilization();
	static nvmlMemory_t* getVRAMUsage();
};

