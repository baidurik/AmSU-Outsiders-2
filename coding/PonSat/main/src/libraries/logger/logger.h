#ifndef LOGGER_H
#define LOGGER_H
#include "../../config.h"
#include "Arduino.h"
#include "PetitFS.h"

class SerialLogger
{
    public:
        void init();
        void writeCanSat(String teamID, long time, float alt, float a, bool stp, bool spp, bool rcp, bool lnp);
};

class SDLogger
{
    FATFS fs;
    public:
        void init();
        void write(String str);
        void writeCanSat(String teamID, long time, float alt, float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz, float pressure, bool stp, bool spp, bool rcp, bool lnp);
};

#endif