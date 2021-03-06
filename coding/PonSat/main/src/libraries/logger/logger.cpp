#include "logger.h"

void SerialLogger::init()
{
    Serial.begin(BAUD_RATE);
}

void SerialLogger::writeCanSat(String teamID, uint32_t time, uint16_t alt, uint16_t a, bool stp, bool spp, bool rcp, bool lnp)
{
    Serial.print(teamID);
    Serial.print(";");
    Serial.print(time);
    Serial.print(";");
    Serial.print(alt);
    Serial.print(";");
    Serial.print(a);
    Serial.print(";");
    Serial.print(stp);
    Serial.print(";");
    Serial.print(spp);
    Serial.print(";");
    Serial.print(rcp);
    Serial.print(";");
    Serial.print(lnp);
    Serial.print("\n");
}

void SDLogger::init()
{
    while (pf_mount(&fs))
    {
        Serial.print("Anime\n");
        delay(200);
    } 
    while (pf_open("FILE.TXT"))
    {
        Serial.print("For gays\n");
        delay(200);
    } 
}

void SDLogger::write(String str)
{
    uint8_t buf [str.length()];
    for(int i = 0; i < str.length(); i++)
    {
        buf[i] = str[i];
    }
    UINT nr;
    pf_write(buf, sizeof(buf), &nr);
}

void SDLogger::writeCanSat(String teamID, uint32_t time, uint16_t alt, int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz, int16_t mx, int16_t my, int16_t mz, uint16_t pressure, uint32_t temp, bool stp, bool spp, bool rcp, bool lnp)
{
    write(teamID);
    write(";");
    write(String(time));
    write(";");
    write(String(alt));
    write(";");
    write(String(ax));
    write(";");
    write(String(ay));
    write(";");
    write(String(az));
    write(";");
    write(String(gx));
    write(";");
    write(String(gy));
    write(";");
    write(String(gz));
    write(";");
    write(String(mx));
    write(";");
    write(String(my));
    write(";");
    write(String(mz));
    write(";");
    write(String(pressure));
    write(";");
    write(String(temp));
    write(";");
    write(String(stp));
    write(";");
    write(String(spp));
    write(";");
    write(String(rcp));
    write(";");
    write(String(lnp));
    write("\n");
}