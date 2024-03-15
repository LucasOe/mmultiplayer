#include <WinUser.h>
#include <chrono>

#pragma once

/*
* Virtual key codes defined which you can't find in winuser.h
* These are here only because I cba to learn that 0x54 is T and makes the code more readable (for me at least)
*/

#define VK_NONE					0x0
#define VK_0					0x30
#define VK_1					0x31
#define VK_2					0x32
#define VK_3					0x33
#define VK_4					0x34
#define VK_5					0x35
#define VK_6					0x36
#define VK_7					0x37
#define VK_8					0x38
#define VK_9					0x39
#define VK_A					0x41
#define VK_B					0x42
#define VK_C					0x43
#define VK_D					0x44
#define VK_E					0x45
#define VK_F					0x46
#define VK_G					0x47
#define VK_H					0x48
#define VK_I					0x49
#define VK_J					0x4A
#define VK_K					0x4B
#define VK_L					0x4C
#define VK_M					0x4D
#define VK_N					0x4E
#define VK_O					0x4F
#define VK_P					0x50
#define VK_Q					0x51
#define VK_R					0x52
#define VK_S					0x53
#define VK_T					0x54
#define VK_U					0x55
#define VK_V					0x56
#define VK_W					0x57
#define VK_X					0x58
#define VK_Y					0x59
#define VK_Z					0x5A

#define Map_MainMenu            "tdmainmenu"
#define Map_Prologue            "edge_p"
#define Map_Flight              "escape_p"
#define Map_Jacknife            "stromdrains_p"
#define Map_Heat                "cranes_p"
#define Map_Ropeburn            "subway_p"
#define Map_NewEden             "mall_p"
#define Map_Factory             "factory_p"
#define Map_TheBoat             "boat_p"
#define Map_Kate                "convoy_p"
#define Map_TheShard            "scraper_p"

#define GameMode_None           ""
#define GameMode_Tag            "tag"

#define MS_TO_KPH               3.6f        // Meter per seconds to Kilometers per hour
#define CMS_TO_KPH              0.036f      // Centimeter per seconds to Kilometers per hour

// Calculates the distance and returns a float value in meters
inline static float Distance(Classes::FVector from, Classes::FVector to) 
{ 
	return sqrt(((from.X - to.X) * (from.X - to.X)) + ((from.Y - to.Y) * (from.Y - to.Y)) + ((from.Z - to.Z) * (from.Z - to.Z))) / 100;
}

// Converts 4294918136 to -270.0439453125f
inline static float ConvertRotationToFloat(int rotation)
{
    return (static_cast<float>(rotation % 0x10000) / static_cast<float>(0x10000)) * 360.0f;
}

template <typename T>
static std::string FormatHelper(const char* format, const T value)
{
    char buff[0x69];
    snprintf(buff, sizeof(buff), format, value);
    return std::string(buff);
}

static std::string FormatTime(const float time)
{
    if (time <= 0.0f)
    {
        return "--:--.---";
    }

    if (time > 3600.0f)
    {
        return "+59:59.999";
    }

    const int minutes = (int)floorf(time / 60.0f);
    const int seconds = (int)floorf(time - (minutes * 60.0f));
    const int milliseconds = (int)floorf((time - floorf(time)) * 1000.0f);

    char buff[0xF];
    snprintf(buff, sizeof(buff), "%02d:%02d.%03d", minutes, seconds, milliseconds);
    return std::string(buff);
}

static std::string FormatAverageSpeed(const float avgSpeed, const bool isMetric)
{
    if (avgSpeed < 0.0f)
    {
        return "n/a";
    }

    if (isMetric)
    {
        return FormatHelper("%.2f km/h", avgSpeed);
    }

    return FormatHelper("%.2f mph", avgSpeed / 1.609344f);
}

static std::string FormatDistance(const float distance, const bool isMetric)
{
    if (distance < 0.0f)
    {
        return "n/a";
    }

    if (isMetric)
    {
        return FormatHelper("%.0f m", distance);
    }

    return FormatHelper("%.0f yds", distance / 0.9144f);
}

inline static std::chrono::seconds GetCurrentTimeInSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
}

static INPUT input = {0};

inline static void PressKey(int keyCode) {
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    input.ki.wVk = keyCode;
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(INPUT));
}

inline static void ReleaseKey() {
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}