#include "coretemp.h"

#define CORETEMP_MAP_NAME  L"CoreTempMappingObjectEx"

int coretemp_read_average(float *outCelsius)
{
    if (!outCelsius)
        return 1;

    HANDLE hMap = OpenFileMappingW(FILE_MAP_READ, FALSE, CORETEMP_MAP_NAME);
    if (!hMap)
        return 1;

    CoreTempSharedDataEx *p = (CoreTempSharedDataEx *)MapViewOfFile(
        hMap, FILE_MAP_READ, 0, 0, 0);
    if (!p) {
        CloseHandle(hMap);
        return 1;
    }

    unsigned int n = p->uiCoreCnt;
    if (n > 256)
        n = 256;

    double sum = 0.0;
    unsigned int valid = 0;

    for (unsigned int i = 0; i < n; i++) {
        float t = p->fTemp[i];
        /* Sanity filter: reject clearly bogus values. */
        if (t > 0.0f && t < 200.0f) {
            if (p->ucFahrenheit)
                t = (t - 32.0f) * (5.0f / 9.0f);
            sum += (double)t;
            valid++;
        }
    }

    UnmapViewOfFile(p);
    CloseHandle(hMap);

    if (valid == 0)
        return 1;

    *outCelsius = (float)(sum / (double)valid);
    return 0;
}
