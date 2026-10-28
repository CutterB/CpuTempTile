#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 4)
typedef struct core_temp_shared_data_ex {
    unsigned int   uiLoad[256];
    unsigned int   uiTjMax[128];
    unsigned int   uiCoreCnt;
    unsigned int   uiCPUCnt;
    float          fTemp[256];
    float          fVID;
    float          fCPUSpeed;
    float          fFSBSpeed;
    float          fMultiplier;
    char           sCPUName[100];
    unsigned char  ucFahrenheit;
    unsigned char  ucDeltaToTjMax;
    unsigned char  ucTdpSupported;
    unsigned char  ucPowerSupported;
    unsigned int   uiStructVersion;
    unsigned int   uiTdp[128];
    float          fPower[128];
    float          fMultipliers[256];
} CoreTempSharedDataEx;
#pragma pack(pop)

/* Reads Core Temp's shared memory and returns the average temperature
 * across all cores in degrees Celsius. Returns 0 on success, non-zero
 * if Core Temp is not running or no valid reading is available. */
int coretemp_read_average(float *outCelsius);

#ifdef __cplusplus
}
#endif
