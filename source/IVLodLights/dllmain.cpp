#include "stdafx.h"
#include "CLODLightManager.h"
#include "Hooking.Patterns.h"
#include <ranges>
#include <thread>

using namespace injector;

const uint32_t NUM_HOURS = 11;
const uint32_t NUM_WEATHERS = 9;

struct TimeCycleParams
{
public:
    uint32_t mAmbient0Color;
    uint32_t mAmbient1Color;
    uint32_t mDirLightColor;
    float mDirLightMultiplier;
    float mAmbient0Multiplier;
    float mAmbient1Multiplier;
    float mAOStrength;
    float mPedAOStrength;
    float mRimLightingMultiplier;
    float mSkyLightMultiplier;
    float mDirLightSpecMultiplier;
    uint32_t mSkyBottomColorFogDensity;
    uint32_t mSunCore;
    float mCoronaBrightness;
    float mCoronaSize;
    float mDistantCoronaBrightness;
    float mDistantCoronaSize;
    float mFarClip;
    float mFogStart;
    float mDOFStart;
    float mNearDOFBlur;
    float mFarDOFBlur;
    uint32_t mLowCloudsColor;
    uint32_t mBottomCloudsColor;
    uint32_t mWater;
    float mUnused64[7];
    float mWaterReflectionMultiplier;
    float mParticleBrightness;
    float mExposure;
    float mBloomThreshold;
    float mMidGrayValue;
    float mBloomIntensity;
    uint32_t mColorCorrection;
    uint32_t mColorAdd;
    float mDesaturation;
    float mContrast;
    float mGamma;
    float mDesaturationFar;
    float mContrastFar;
    float mGammaFar;
    float mDepthFxNear;
    float mDepthFxFar;
    float mLumMin;
    float mLumMax;
    float mLumDelay;
    int32_t mCloudAlpha;
    float mUnusedD0;
    float mTemperature;
    float mGlobalReflectionMultiplier;
    float mUnusedDC;
    float mSkyColor[3];
    float mUnusedEC;
    float mSkyHorizonColor[3];
    float mUnusedFC;
    float mSkyEastHorizonColor[3];
    float mUnused10C;
    float mCloud1Color[3];
    float mUnknown11C;
    float mSkyHorizonHeight;
    float mSkyHorizonBrightness;
    float mSunAxisX;
    float mSunAxisY;
    float mCloud2Color[3];
    float mUnused13C;
    float mCloud2ShadowStrength;
    float mCloud2Threshold;
    float mCloud2Bias1;
    float mCloud2Scale;
    float mCloudInScatteringRange;
    float mCloud2Bias2;
    float mDetailNoiseScale;
    float mDetailNoiseMultiplier;
    float mCloud2Offset;
    float mCloudWarp;
    float mCloudsFadeOut;
    float mCloud1Bias;
    float mCloud1Detail;
    float mCloud1Threshold;
    float mCloud1Height;
    float mUnused17C;
    float mCloud3Color[3];
    float mUnused18C;
    float mUnknown190;
    float mUnused198[3];
    float mSunColor[3];
    float mUnused1AC;
    float mCloudsBrightness;
    float mDetailNoiseOffset;
    float mStarsBrightness;
    float mVisibleStars;
    float mMoonBrightness;
    float mUnused1C4[3];
    float mMoonColor[3];
    float mUnused1DC;
    float mMoonGlow;
    float mMoonParam3;
    float SunCenterStart;
    float SunCenterEnd;
    float mSunSize;
    float mUnused1F8[3];
    float mUnknown200;
    float mSkyBrightness;
    float mUnused208;
    int32_t mFilmGrain;
};

struct Timecycle
{
    TimeCycleParams mParams[NUM_HOURS][NUM_WEATHERS];

    static int32_t GameTimeToTimecycTimeIndex(const int32_t gameTime)
    {
        const int32_t gameTimeToTimecycTimeIndex[24] = { 0, 0, 0, 0, 0, 1, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 7, 8, 9, 10, 10 };
        return gameTimeToTimecycTimeIndex[gameTime];
    }
};

Timecycle* mTimeCycle = nullptr;

#define NewLimitExponent 14

char* CLODLightManager::IV::CurrentTimeHours;
char* CLODLightManager::IV::CurrentTimeMinutes;
int(__cdecl *CLODLightManager::IV::DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
int(__cdecl *CLODLightManager::IV::DrawCorona2)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14);
int(__cdecl *CLODLightManager::IV::DrawCorona3)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14);
void(__stdcall *CLODLightManager::IV::GetRootCam)(int *camera);
void(__stdcall *CLODLightManager::IV::GetGameCam)(int *camera);
bool(__cdecl *CLODLightManager::IV::CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
void(__cdecl *CLODLightManager::IV::GetCamPos)(int camera, float *pX, float *pY, float *pZ);
int* CTimer::m_snTimeInMillisecondsPauseMode = nullptr;
float* CTimer::ms_fTimeStep = nullptr;
float fCamHeight;
std::map<unsigned int, CRGBA> FestiveLights;
extern bool bIsIVEFLC;
float fCoronaAlphaMultiplier = 1.0f;

int CCoronasRegisterFestiveCoronaForEntity(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14)
{
    auto it = FestiveLights.find(id);
    if (it != FestiveLights.end())
    {
        return CLODLightManager::IV::DrawCorona2(id, it->second.r, it->second.g, it->second.b, a5, pos, radius, a8, a9, a10, a11, a12, a13, a14);
    }
    else
    {
        FestiveLights[id] = CRGBA(random(0, 255), random(0, 255), random(0, 255), 0);
        return CLODLightManager::IV::DrawCorona2(id, r, g, b, a5, pos, radius, a8, a9, a10, a11, a12, a13, a14);
    }
}

void CLODLightManager::IV::Init()
{
    bIsIVEFLC = true;
    CIniReader iniReader("");
    bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1) != 0;
    fCoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
    fCoronaAlphaMultiplier = iniReader.ReadFloat("LodLights", "CoronaAlphaMultiplier", 1.0f);
    bSlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
    fCoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f);
    bool DisableDefaultLodLights = iniReader.ReadInteger("LodLights", "DisableDefaultLodLights", 1) != 0;
    int32_t DisableCoronasWaterReflection = iniReader.ReadInteger("LodLights", "DisableCoronasWaterReflection", 0);
    bFestiveLights = iniReader.ReadInteger("Misc", "FestiveLights", 1) != 0;
    bFestiveLightsAlways = iniReader.ReadInteger("Misc", "FestiveLightsAlways", 0) != 0;

    struct LoadObjectInstanceHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = *(uintptr_t*)(regs.ebp + 0x8);
            regs.ecx = *(uintptr_t*)(regs.ebx + 0x1C);

            PossiblyAddThisEntity((WplInstance*)regs.ebx);

            // regs.esi = *(uintptr_t*)(regs.ebp + 0x8);
            // regs.eax = (regs.esp + 0x1C);

            // PossiblyAddThisEntity((WplInstance*)regs.esi);
        }
    };

    if (bRenderLodLights)
    {
        GetMemoryAddresses();
        IncreaseCoronaLimit();
        LoadDatFile();
        RegisterCustomCoronas();

        auto pattern = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? 00 74 05 E8 ? ? ? ? 6A 05"); //+
        if (!pattern.empty()) {
            injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), RegisterLODLights, true);
        }
        else {
            pattern = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? ? 74 05 E8 ? ? ? ? 6A 05");
            if (!pattern.empty()) {
                injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), RegisterLODLights, true);
            }
        }

        pattern = hook::pattern("8B 75 08 8D 44 24 1C 50 FF 76 1C C6 44 24"); //+
        if (!pattern.empty()) {
            injector::MakeInline<LoadObjectInstanceHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(7));
        }
        else {
            pattern = hook::pattern("8B 5D 08 8B 4B 1C 8D 44 24 14 50 51");
            if (!pattern.empty()) {
                injector::MakeInline<LoadObjectInstanceHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(6));
            }
        }
    }

    if (DisableDefaultLodLights)
    {
        auto pattern = hook::pattern("83 F8 08 0F 8C ? ? ? ? 83 3D");
        if (!pattern.empty()) {
            injector::WriteMemory<uint8_t>(pattern.get_first(2), 0, true);
        }
        else {
            auto pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC B4 00 00 00 F3 0F 10 45 08");
            if (!pattern.empty()) {
                injector::WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(0), 0xC3, true);
            }
        }
    }

    auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 80 3D ? ? ? ? ? 74 32 6A 00 6A 0C");
    if (!pattern.empty()) {
        static auto jmp = pattern.get(0).get<uintptr_t>(8);
        if (DisableCoronasWaterReflection == 1)
        {
            injector::MakeNOP(pattern.get(0).get<uintptr_t>(0), 5, true);
        }
        else
        {
            if (DisableCoronasWaterReflection == 2)
            {
                struct ReflectionsHook
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        regs.edx = *(uintptr_t*)(regs.esi + 0x938);
                        if (fCamHeight < 100.0f)
                        {
                            *(uintptr_t*)(regs.esp - 4) = (uintptr_t)jmp;
                        }
                    }
                }; injector::MakeInline<ReflectionsHook>(pattern.get(0).get<uintptr_t>(-6), pattern.get(0).get<uintptr_t>(0));
                injector::WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(-1), 0x52, true); // push edx
            }
        }
    }
    else {
        pattern = hook::pattern("E8 68 ? ? ? 83 C4 04 80 3D ? ? ? ? 00 74 ? 6A 00 6A 0C");
        if (!pattern.empty()) {
            static auto jmp = pattern.get(0).get<uintptr_t>(8);
            if (DisableCoronasWaterReflection == 1)
            {
                injector::MakeNOP(pattern.get(0).get<uintptr_t>(0), 5, true);
            }
            else
            {
                if (DisableCoronasWaterReflection == 2)
                {
                    struct ReflectionsHook
                    {
                        void operator()(injector::reg_pack& regs)
                        {
                            regs.edx = *(uintptr_t*)(regs.esi + 0x938);
                            if (fCamHeight < 100.0f)
                            {
                                *(uintptr_t*)(regs.esp - 4) = (uintptr_t)jmp;
                            }
                        }
                    }; injector::MakeInline<ReflectionsHook>(pattern.get(0).get<uintptr_t>(-6), pattern.get(0).get<uintptr_t>(0));
                    injector::WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(-1), 0x52, true); // push edx
                }
            }
        }
    }

    if (bFestiveLights)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm *date = std::localtime(&now_c);
        if (bFestiveLightsAlways || (date->tm_mon == 0 && date->tm_mday <= 1) || (date->tm_mon == 11 && date->tm_mday >= 31))
        {
            DrawCorona3 = &CCoronasRegisterFestiveCoronaForEntity;
            pattern = hook::pattern("E8 ? ? ? ? 83 C4 3C E9 ? ? ? ? A1");
            injector::MakeCALL(pattern.get_first(0), CCoronasRegisterFestiveCoronaForEntity, true);
            pattern = hook::pattern("E8 ? ? ? ? F3 0F 10 4C 24 ? 8A 44 24 4E");
            if (!pattern.empty()) {
                injector::MakeCALL(pattern.get_first(0), CCoronasRegisterFestiveCoronaForEntity, true);
            }
            else {
                pattern = hook::pattern("E8 ? ? ? ? 83 C4 3C EB 03");
                if (!pattern.empty()) {
                    injector::MakeCALL(pattern.get_first(0), CCoronasRegisterFestiveCoronaForEntity, true);
                }
            }
        }
    }
}

namespace CWeather
{
    enum eWeatherType : uint32_t
    {
        EXTRASUNNY,
        SUNNY,
        SUNNY_WINDY,
        CLOUDY,
        RAIN,
        DRIZZLE,
        FOGGY,
        LIGHTNING
    };

    eWeatherType* CurrentWeather = nullptr;
}

void CLODLightManager::IV::GetMemoryAddresses()
{
    auto pattern = hook::pattern("A3 ? ? ? ? 8B 44 24 0C A3 ? ? ? ? 8B 44 24 10 A3 ? ? ? ? A1 ? ? ? ? A3 ? ? ? ? C3"); //+
    if (!pattern.empty()) {
        CLODLightManager::IV::CurrentTimeHours = *pattern.get(0).get<char*>(1);
        CLODLightManager::IV::CurrentTimeMinutes = *pattern.get(0).get<char*>(10);
        pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 F3 0F 10 4D ? F3 0F 10 45"); //+
        CLODLightManager::IV::DrawCorona = (int(__cdecl*)(float, float, float, float, unsigned int, float, unsigned char, unsigned char, unsigned char))(pattern.get(0).get<uintptr_t>(0));
        pattern = hook::pattern("8B 15 ? ? ? ? 56 8D 72 01"); //+
        CLODLightManager::IV::DrawCorona2 = (int(__cdecl*)(int id, char r, char g, char b, float a5, CVector * pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14))(pattern.get(0).get<uintptr_t>(0)); //0x7E1970
        CLODLightManager::IV::DrawCorona3 = CLODLightManager::IV::DrawCorona2;
        pattern = hook::pattern("FF 35 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00"); //+
        CLODLightManager::IV::GetRootCam = (void(__stdcall*)(int* camera))(pattern.get(0).get<uintptr_t>(0));
        pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00"); //+
        CLODLightManager::IV::GetGameCam = (void(__stdcall*)(int* camera))(pattern.get(0).get<uintptr_t>(0));
        pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 F3 0F 10 45 ? F3 0F 11 04 24 F3 0F 10 45 ? F3 0F 11 44 24 ? F3 0F 10 45 ? 51 F3 0F 11 44 24 ? F3 0F 10 45 ? F3 0F 11 04 24");
        CLODLightManager::IV::CamIsSphereVisible = (bool(__cdecl*)(int camera, float pX, float pY, float pZ, float radius))(pattern.get(0).get<uintptr_t>(0));
        pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 8D 04 24 50 FF 75 08"); //+
        CLODLightManager::IV::GetCamPos = (void(__cdecl*)(int camera, float* pX, float* pY, float* pZ))(pattern.get(0).get<uintptr_t>(0));
        pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 8B 43 20 53");
        CTimer::ms_fTimeStep = *pattern.get_first<float*>(4);
        pattern = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? EB 3A");
        CTimer::m_snTimeInMillisecondsPauseMode = *pattern.get_first<int32_t*>(1); //m_snTimeInMilliseconds
        pattern = hook::pattern("A1 ? ? ? ? 83 C4 08 8B CF");
        CWeather::CurrentWeather = *pattern.get_first<CWeather::eWeatherType*>(1);
        pattern = hook::pattern("05 ? ? ? ? 50 8D 4C 24 60");
        mTimeCycle = *pattern.get_first<Timecycle*>(1);
    }
    else {
        auto pattern = hook::pattern("01 15 ? ? ? ? 8D 0C 81 89 0D"); //820F75
        if (!pattern.empty()) {
            CLODLightManager::IV::CurrentTimeHours = *pattern.get(0).get<char*>(2); //0x11D5300
            CLODLightManager::IV::CurrentTimeMinutes = *pattern.get(0).get<char*>(11); //0x11D52FC
            pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 20 D9 05 ? ? ? ? F3 0F 10 45 08 6A 00");
            CLODLightManager::IV::DrawCorona = (int(__cdecl*)(float, float, float, float, unsigned int, float, unsigned char, unsigned char, unsigned char))(pattern.get(0).get<uintptr_t>(0)); //0xA6E240
            pattern = hook::pattern("A1 ? ? ? ? 56 8D 70 01 81 FE");
            CLODLightManager::IV::DrawCorona2 = (int(__cdecl*)(int id, char r, char g, char b, float a5, CVector * pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14))(pattern.get(0).get<uintptr_t>(0)); //0x7E1970
            CLODLightManager::IV::DrawCorona3 = CLODLightManager::IV::DrawCorona2;
            pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00");
            CLODLightManager::IV::GetRootCam = (void(__stdcall*)(int* camera))(pattern.get(0).get<uintptr_t>(0)); //0xB006C0
            pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00");
            CLODLightManager::IV::GetGameCam = (void(__stdcall*)(int* camera))(pattern.get(0).get<uintptr_t>(0)); //0xB006E0
            pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 F3 0F 10 45 0C D9 45 18 51");
            CLODLightManager::IV::CamIsSphereVisible = (bool(__cdecl*)(int camera, float pX, float pY, float pZ, float radius))(pattern.get(0).get<uintptr_t>(0)); //0xBB9340
            pattern = hook::pattern("55 8B EC 83 E4 F0 8B 4D 08 83 EC 10 8D 04 24 50 51 B9 ? ? ? ? E8 ? ? ? ? F3 ? ? ? ? 8B 55 0C");
            CLODLightManager::IV::GetCamPos = (void(__cdecl*)(int camera, float* pX, float* pY, float* pZ))(pattern.get(0).get<uintptr_t>(0)); //0xBB8510
            pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 8B 44 24 04");
            CTimer::ms_fTimeStep = *pattern.get_first<float*>(4);
            pattern = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? EB 15");
            CTimer::m_snTimeInMillisecondsPauseMode = *pattern.get_first<int32_t*>(1); //m_snTimeInMilliseconds
            pattern = hook::pattern("A1 ? ? ? ? 8D 48 01 81 E1 07 00 00 80");
            CWeather::CurrentWeather = *pattern.get_first<CWeather::eWeatherType*>(1);
            pattern = hook::pattern("05 ? ? ? ? D9 5C 24 04 8D 4C 24 38");
            mTimeCycle = *pattern.get_first<Timecycle*>(7);
        }
    }
}

void CLODLightManager::IV::IncreaseCoronaLimit()
{
    auto nCoronasLimit = static_cast<uint32_t>(3 * pow(2.0, NewLimitExponent)); // 49152, default 3 * pow(2, 8) = 768

    static std::vector<uint32_t> aCoronas;
    static std::vector<uint32_t> aCoronas2;
    aCoronas.resize(nCoronasLimit * 0x3C * 4);
    aCoronas2.resize(nCoronasLimit * 0x3C * 4);

    int32_t counter1 = 0;
    int32_t counter2 = 0;

    auto pattern = hook::pattern("33 C0 C7 80 ? ? ? ? ? ? ? ? 83 C0 40 3D ? ? ? ? 72 EC C7 05 ? ? ? ? ? ? ? ? C3");
    if (!pattern.empty()) {
        uintptr_t range_start = (uintptr_t)hook::get_pattern("33 C0 C7 80 ? ? ? ? ? ? ? ? 83 C0 40 3D ? ? ? ? 72 EC C7 05 ? ? ? ? ? ? ? ? C3"); //+
        uintptr_t range_end = (uintptr_t)hook::get_pattern("5E C3 FF 05 ? ? ? ? 5E C3"); //+

        uintptr_t dword_temp = (uintptr_t)*hook::pattern("89 82 ? ? ? ? F3 0F 11 82 ? ? ? ? F3 0F 10 44 24 ? F3 0F 11 8A ? ? ? ? 8B 41 0C 0F B6 4C 24 ? 89 82").get(0).get<uint32_t*>(2); //+

        for (size_t i = dword_temp; i <= (dword_temp + 0x3C); i++)
        {
            auto GoThroughPatterns = [&](const char* pattern_str, int32_t pos) -> void
                {
                    auto patternl = hook::range_pattern(range_start, range_end, pattern_str);
                    for (size_t j = 0; j < patternl.size(); j++)
                    {
                        if (*patternl.get(j).get<uintptr_t>(pos) == i)
                        {
                            AdjustPointer(patternl.get(j).get<uint32_t>(pos), &aCoronas[0], dword_temp, dword_temp + 0x3C);
                            counter1++;
                        }
                    }
                };

            GoThroughPatterns("83 8A", 2);
            GoThroughPatterns("88 82", 2);
            GoThroughPatterns("89 82", 2);
            GoThroughPatterns("89 82", 2);
            GoThroughPatterns("89 8A", 2);
            GoThroughPatterns("8B 82", 2);
            GoThroughPatterns("BE", 1);
            GoThroughPatterns("C7 80", 2);
            GoThroughPatterns("C7 82", 2);
            GoThroughPatterns("F3 0F 11 82", 4);
            GoThroughPatterns("F3 0F 11 8A", 4);
        }


        range_start = (uintptr_t)hook::get_pattern("8B 44 24 1C F3 0F 10 4C 24 ? F3 0F 10 46 ? F3 0F 59 05 ? ? ? ? 8D 0C 40"); //+
        range_end = (uintptr_t)hook::get_pattern("0F 29 85 ? ? ? ? 0F 28 9D ? ? ? ? F3 0F 5C C2 F3 0F 11 5C 24 ? 0F 28 9D ? ? ? ? F3 0F 11 5C 24 ? 0F 28 D0"); //+

        dword_temp = (uintptr_t)*hook::pattern("F3 0F 11 89 ? ? ? ? F3 0F 10 4C 24 ? F3 0F 11 89").get(0).get<uint32_t*>(4);

        for (size_t i = dword_temp; i <= (dword_temp + 0x1B); i++)
        {
            auto GoThroughPatterns = [&](const char* pattern_str, int32_t pos) -> void
                {
                    auto patternl = hook::range_pattern(range_start, range_end, pattern_str);
                    for (size_t j = 0; j < patternl.size(); j++)
                    {
                        if (*patternl.get(j).get<uintptr_t>(pos) == i)
                        {
                            AdjustPointer(patternl.get(j).get<uint32_t>(pos), &aCoronas2[0], dword_temp, dword_temp + 0x1B);
                            counter2++;
                        }
                    }
                };

            GoThroughPatterns("0F 28 81", 3);
            GoThroughPatterns("0F B6 81", 3);
            GoThroughPatterns("0F B6 B1", 3);
            GoThroughPatterns("80 B8", 2);
            GoThroughPatterns("88 81", 2);
            GoThroughPatterns("88 91", 2);
            GoThroughPatterns("89 81", 2);
            GoThroughPatterns("BE", 1);
            GoThroughPatterns("F3 0F 10 81", 4);
            GoThroughPatterns("F3 0F 10 89", 4);
            GoThroughPatterns("F3 0F 11 81", 4);
            GoThroughPatterns("F3 0F 11 89", 4);
            GoThroughPatterns("F3 0F 11 91", 4);
        }

        if (counter1 != 24 || counter2 != 18)
            MessageBox(0, L"IV.Project2DFX", L"Project2DFX is not fully compatible with this version of the game", 0);

        auto p = hook::pattern("BF FF 02 00 00"); //+
        AdjustPointer(p.get_first(-4), &aCoronas[0], dword_temp, dword_temp + 0x3C);
        p = hook::pattern("BF FF 05 00 00"); //+
        AdjustPointer(p.get_first(-4), &aCoronas2[0], dword_temp, dword_temp + 0x1B);

        auto pattern = hook::pattern("C1 E1 ? 03 4C 24 18 C1");
        WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true);
        pattern = hook::pattern("C1 E1 ? 03 CF C1 E1");
        WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true);
        pattern = hook::pattern("C1 E0 ? 03 C2 C1 E0 05 80 B8");
        WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true);

        pattern = hook::pattern("81 FE ? ? ? ? 0F 8D ? ? ? ? 8B 44 24 08 8B 4C 24 1C F3 0F 10 44 24 ? C1 E2 06");
        WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(2), nCoronasLimit, true);
        pattern = hook::pattern("3D ? ? ? ? 0F 8D ? ? ? ? 8B 44 24 1C F3 0F 10 4C 24");
        WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(1), nCoronasLimit, true);
        pattern = hook::pattern("3D ? ? ? ? 72 EC C7 05 ? ? ? ? ? ? ? ? C3");
        WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(1), nCoronasLimit * 64, true);
    }
    else
    {
        auto pattern = hook::pattern("BE ? ? ? ? 83 7E 18 00");
        if (!pattern.empty()) {
            uintptr_t range_start = (uintptr_t)hook::get_pattern("BE ? ? ? ? 83 7E 18 00"); //0x7E0F95
            uintptr_t range_end = (uintptr_t)hook::get_pattern("5E C3 83 05 ? ? ? ? ? 5E C3"); //0x7E1AAB

            uintptr_t dword_temp = (uintptr_t)*hook::pattern("D9 98 ? ? ? ? F3 0F 10 44 24 20 F3 0F 11 88").get(0).get<uint32_t*>(2); //+

            for (size_t i = dword_temp; i <= (dword_temp + 0x3C); i++)
            {
                auto GoThroughPatterns = [&](const char* pattern_str, int32_t pos) -> void
                    {
                        auto patternl = hook::range_pattern(range_start, range_end, pattern_str);
                        for (size_t j = 0; j < patternl.size(); j++)
                        {
                            if (*patternl.get(j).get<uintptr_t>(pos) == i)
                            {
                                AdjustPointer(patternl.get(j).get<uint32_t>(pos), &aCoronas[0], dword_temp, dword_temp + 0x3C);
                                counter1++;
                            }
                        }
                    };

                GoThroughPatterns("D9 98", 2);
                GoThroughPatterns("F3 0F 11 80", 4);
                GoThroughPatterns("F3 0F 11 88", 4);
                GoThroughPatterns("83 88 ? ? ? ? 10", 2);
                GoThroughPatterns("83 88 ? ? ? ? 06", 2);
                GoThroughPatterns("BE", 1);
                GoThroughPatterns("89 88", 2);
                GoThroughPatterns("88 90", 2);
                GoThroughPatterns("88 88", 2);
                GoThroughPatterns("8B 90", 2);
            }


            range_start = (uintptr_t)hook::get_pattern("D9 46 20 F3 0F 10 54 24 ? 0F B6 56 30"); //0x7E10AB
            range_end = (uintptr_t)hook::get_pattern("F3 0F 59 C3 F3 0F 5C F0 F3 0F 59 CB"); //0x7E1671

            dword_temp = (uintptr_t)*hook::pattern("F3 0F 11 90 ? ? ? ? F3 0F 10 54 24 24 F3 0F").get(0).get<uint32_t*>(4);

            for (size_t i = dword_temp; i <= (dword_temp + 0x1B); i++)
            {
                auto GoThroughPatterns = [&](const char* pattern_str, int32_t pos) -> void
                    {
                        auto patternl = hook::range_pattern(range_start, range_end, pattern_str);
                        for (size_t j = 0; j < patternl.size(); j++)
                        {
                            if (*patternl.get(j).get<uintptr_t>(pos) == i)
                            {
                                AdjustPointer(patternl.get(j).get<uint32_t>(pos), &aCoronas2[0], dword_temp, dword_temp + 0x1B);
                                counter2++;
                            }
                        }
                    };

                GoThroughPatterns("0F 28 89", 3);
                GoThroughPatterns("88 90", 2);
                GoThroughPatterns("88 88", 2);
                GoThroughPatterns("F3 0F 11 90", 4);
                GoThroughPatterns("F3 0F 11 80", 4);
                GoThroughPatterns("F3 0F 10 81", 4);
                GoThroughPatterns("F3 0F 11 88", 4);
                GoThroughPatterns("F3 0F 10 A1", 4);
                GoThroughPatterns("F3 0F 10 99", 4);
                GoThroughPatterns("D9 98", 2);
                GoThroughPatterns("80 B9 ? ? ? ? 00", 2);
                GoThroughPatterns("0F B6 B1", 3);
                GoThroughPatterns("0F B6 81", 3);
                GoThroughPatterns("0F B6 91", 3);
            }

            if (counter1 != 24 || counter2 != 18)
                MessageBox(0, L"IV.Project2DFX", L"Project2DFX is not fully compatible with this version of the game", 0);

            auto p = hook::pattern("BF ? ? ? ? 8D 64 24 00 68 ? ? ? ? 6A 05 6A 04 56"); //+
            AdjustPointer(p.get_first(-4), &aCoronas[0], dword_temp, dword_temp + 0x3C);
            p = hook::pattern("BF ? ? ? ? 8D 64 24 00 68 ? ? ? ? 6A 05 6A 04 83 EE 14"); //+
            AdjustPointer(p.get_first(-4), &aCoronas2[0], dword_temp, dword_temp + 0x1B);

            auto pattern = hook::pattern("C1 E0 ? 03 C3 C1 E0");
            WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true); //aslr_ptr(0x7E109F + 0x2)
            pattern = hook::pattern("C1 E1 ? 03 CF C1 E1");
            WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true); //aslr_ptr(0x7E149A + 0x2)
            pattern = hook::pattern("C1 E1 ? 03 C8 C1 E1");
            WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true); //aslr_ptr(0x7E130E + 0x2)

            pattern = hook::pattern("81 FE ? ? ? ? 0F 8D ? ? ? ? 8B 4C 24 08 8A 54 24 0C");
            WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(2), nCoronasLimit, true);      //aslr_ptr(0x7E1979 + 0x2)
            pattern = hook::pattern("81 FF ? ? ? ? 0F 8D ? ? ? ? 8B 44 24 1C D9 46 20");
            WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(2), nCoronasLimit, true);      //aslr_ptr(0x7E1072 + 0x2)
            pattern = hook::pattern("3D ? ? ? ? 72 ? 89 0D ? ? ? ? C3");
            WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(1), nCoronasLimit * 64, true); //aslr_ptr(0x7E1189 + 0x1)
        }
    }
}

void CLODLightManager::IV::RegisterCustomCoronas()
{
    constexpr unsigned int nModelID = 0xFFFFFFFF;

    auto foundElements = *pFileContentMMap | std::views::filter([&nModelID](auto& v) {
        return v.first == nModelID;
    });

    for (auto& it : foundElements)
    {
        m_Lampposts.push_back(CLamppostInfo(it.second.vecPos, it.second.colour, it.second.fCustomSizeMult, it.second.nCoronaShowMode, it.second.nNoDistance, it.second.nDrawSearchlight, 0.0f));
    }
}

WplInstance* CLODLightManager::IV::PossiblyAddThisEntity(WplInstance* pInstance)
{
    if (m_bCatchLamppostsNow && pFileContentMMap->contains(pInstance->ModelNameHash))
        RegisterLamppost(pInstance);

    return pInstance;
}

void CLODLightManager::IV::RegisterLamppost(WplInstance* pObj)
{
    DWORD               nModelID = pObj->ModelNameHash;
    CMatrix             dummyMatrix;

    float qw = pObj->RotationW;
    float qx = pObj->RotationX;
    float qy = pObj->RotationY;
    float qz = pObj->RotationZ;

    float n = 1.0f / sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
    qx *= n;
    qy *= n;
    qz *= n;
    qw *= n;

    dummyMatrix.matrix.right.x = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
    dummyMatrix.matrix.right.y = 2.0f * qx * qy - 2.0f * qz * qw;
    dummyMatrix.matrix.right.z = 2.0f * qx * qz + 2.0f * qy * qw;

    dummyMatrix.matrix.up.x = 2.0f * qx * qy + 2.0f * qz * qw;
    dummyMatrix.matrix.up.y = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
    dummyMatrix.matrix.up.z = 2.0f * qy * qz - 2.0f * qx * qw;

    dummyMatrix.matrix.at.x = 2.0f * qx * qz - 2.0f * qy * qw;
    dummyMatrix.matrix.at.y = 2.0f * qy * qz + 2.0f * qx * qw;
    dummyMatrix.matrix.at.z = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;

    dummyMatrix.matrix.pos.x = pObj->PositionX;
    dummyMatrix.matrix.pos.y = pObj->PositionY;
    dummyMatrix.matrix.pos.z = pObj->PositionZ;

    {
        auto v1 = CVector(pObj->PositionX, pObj->PositionY, pObj->PositionZ);
        auto v2 = CVector(-278.37f, -1377.48f, 90.98f);
        if (GetDistance((RwV3d*)&v1, (RwV3d*)&v2) <= 300.0f)
            return;
    }

    auto foundElements = *pFileContentMMap | std::views::filter([&nModelID](auto& v) {
        return v.first == nModelID;
    });

    for (auto& it : foundElements)
    {
        m_Lampposts.push_back(CLamppostInfo(dummyMatrix * it.second.vecPos, it.second.colour, it.second.fCustomSizeMult, it.second.nCoronaShowMode, it.second.nNoDistance, it.second.nDrawSearchlight, atan2(dummyMatrix.GetUp()->y, -dummyMatrix.GetUp()->x), it.second.fObjectDrawDistance));
    }
}

void CLODLightManager::IV::RegisterLODLights()
{
    static auto SolveEqSys = [](float a, float b, float c, float d, float value) -> float
    {
        float determinant = a - c;
        float x = (b - d) / determinant;
        float y = (a * d - b * c) / determinant;
        return min(x * value + y, d);
    };

    if (m_bCatchLamppostsNow)
        m_bCatchLamppostsNow = false;

    static auto ff = GetModuleHandleW(L"GTAIV.EFLC.FusionFix.asi");
    if (ff)
    {
        static auto GetDistantLightsPrefValue = (int32_t(*)())GetProcAddress(ff, "GetDistantLightsPrefValue");
        if (GetDistantLightsPrefValue)
        {
            if (!GetDistantLightsPrefValue())
                return;
        }
    }

    int currentHour = *CurrentTimeHours;
    if (currentHour < 19 && currentHour > 7)
        return;

    // Calculate alpha based on time
    unsigned int nTime = currentHour * 60 + *CurrentTimeMinutes;
    unsigned int curMin = *CurrentTimeMinutes;
    unsigned char bAlpha = 0;

    if (nTime >= 19 * 60)
        bAlpha = static_cast<unsigned char>(SolveEqSys(19 * 60.0f, 30.0f, 24 * 60.0f, 255.0f, static_cast<float>(nTime)));
    else if (nTime < 3 * 60)
        bAlpha = 255;
    else
        bAlpha = static_cast<unsigned char>(SolveEqSys(8 * 60.0f, 30.0f, 3 * 60.0f, 255.0f, static_cast<float>(nTime)));

    // Get time cycle parameters
    auto timeIndex = Timecycle::GameTimeToTimecycTimeIndex(currentHour);
    auto& timeCycleParams = mTimeCycle->mParams[timeIndex][*CWeather::CurrentWeather];
    float fDistantCoronaBrightness = timeCycleParams.mDistantCoronaBrightness / 10.0f;
    float fDistantCoronaSize = timeCycleParams.mDistantCoronaSize;

    // Get camera info once
    int currentCamera;
    GetRootCam(&currentCamera);
    CVector camPos;
    GetCamPos(currentCamera, &camPos.x, &camPos.y, &camPos.z);

    for (const auto& lamppost : m_Lampposts)
    {
        // Check height bounds
        if (lamppost.vecPos.z < -15.0f || lamppost.vecPos.z > 1030.0f)
            continue;

        // Calculate distance
        float dx = camPos.x - lamppost.vecPos.x;
        float dy = camPos.y - lamppost.vecPos.y;
        float dz = camPos.z - lamppost.vecPos.z;
        float fDistSqr = dx * dx + dy * dy + dz * dz;
        float distance = std::sqrt(fDistSqr);

        float fCoronaDist = lamppost.fObjectDrawDistance - 30.0f;

        // Check if within corona range
        if (!lamppost.nNoDistance &&
            (fDistSqr <= fCoronaDist * fCoronaDist || fDistSqr >= fCoronaFarClip * fCoronaFarClip))
            continue;

        // Calculate radius
        float fRadius = lamppost.nNoDistance ? 3.5f :
            SolveEqSys(fCoronaDist, 0.0f, lamppost.fObjectDrawDistance, 3.5f, distance);

        if (bSlightlyIncreaseRadiusWithDistance)
            fRadius *= min(SolveEqSys(fCoronaDist, 1.0f, fCoronaFarClip, 4.0f, distance), 3.0f);

        float fAlphaDistMult = 110.0f - SolveEqSys(fCoronaDist / 4.0f, 10.0f, lamppost.fObjectDrawDistance * 4.0f, 100.0f, distance);

        // Calculate base alpha
        float baseAlpha = fCoronaAlphaMultiplier * ((bAlpha * (lamppost.colour.a / 255.0f)) / fAlphaDistMult);

        if (lamppost.fCustomSizeMult != 0.45f)
        {
            // Regular lamppost
            float finalAlpha = baseAlpha * fDistantCoronaBrightness;
            float finalSize = fRadius * lamppost.fCustomSizeMult * fCoronaRadiusMultiplier * fDistantCoronaSize * 1270.5f;

            if (lamppost.nCoronaShowMode)
            {
                // Blinking light
                static float blinking = 1.0f;
                if (IsBlinkingNeeded(lamppost.nCoronaShowMode))
                    blinking -= *CTimer::ms_fTimeStep / 1000.0f;
                else
                    blinking += *CTimer::ms_fTimeStep / 1000.0f;

                blinking = std::clamp(blinking, 0.0f, 1.0f);
                finalAlpha *= blinking;
                finalSize = fRadius * lamppost.fCustomSizeMult * fCoronaRadiusMultiplier * 1270.5f;
            }

            DrawCorona2(reinterpret_cast<unsigned int>(&lamppost),
                lamppost.colour.r, lamppost.colour.g, lamppost.colour.b,
                finalAlpha,
                const_cast<CVector*>(&lamppost.vecPos),
                finalSize,
                0.0, 0.0, 0, 0.0, 0, 0, 0);
        }
        else
        {
            fRadius *= 1.3f;
            float finalSize = fRadius * lamppost.fCustomSizeMult * fCoronaRadiusMultiplier * 1270.5f;

            // Color detection
            bool isYellow = (lamppost.colour.r >= 250 && lamppost.colour.g >= 100 && lamppost.colour.b <= 100);
            bool isRed = (lamppost.colour.r >= 250 && lamppost.colour.g < 100 && lamppost.colour.b == 0);
            bool isGreen = (lamppost.colour.r == 0 && lamppost.colour.g >= 100 && lamppost.colour.b == 0);
            bool isGreenAlt = (lamppost.colour.r == 0 && lamppost.colour.g >= 250 && lamppost.colour.b == 0);

            // Time periods
            bool isYellowTime = (curMin == 9 || curMin == 19 || curMin == 29 || curMin == 39 || curMin == 49 || curMin == 59);
            bool isRedTime = ((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49));
            bool isGreenTime = ((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59));

            // Heading check
            float absHeading = std::abs(lamppost.fHeading);
            bool isInHeadingRange = (absHeading >= (M_PI / 6.0f) && absHeading <= (5.0f * M_PI / 6.0f));

            bool shouldDraw = false;

            if (isYellow && isYellowTime)
            {
                shouldDraw = true;
            }
            else if (isInHeadingRange)
            {
                shouldDraw = (isRed && isRedTime) || (isGreen && isGreenTime);
            }
            else
            {
                shouldDraw = (isGreenAlt && isRedTime) || (isRed && isGreenTime);
            }

            if (shouldDraw)
            {
                DrawCorona2(reinterpret_cast<unsigned int>(&lamppost),
                    lamppost.colour.r, lamppost.colour.g, lamppost.colour.b,
                    baseAlpha,
                    const_cast<CVector*>(&lamppost.vecPos),
                    finalSize,
                    0.0, 0.0, 0, 0.0, 0, 0, 0);
            }
        }
    }
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        CLODLightManager::IV::Init();
    });
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}