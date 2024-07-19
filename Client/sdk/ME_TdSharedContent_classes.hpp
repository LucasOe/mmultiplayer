#pragma once

// Mirror's Edge (1.0) SDK

#ifdef _MSC_VER
	#pragma pack(push, 0x4)
#endif

namespace Classes
{
//---------------------------------------------------------------------------
//Classes
//---------------------------------------------------------------------------

class ATdWeapon_AssaultRifle_FNSCARL : public ATdWeapon_Heavy
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_AssaultRifle_FNSCARL");
		return ptr;
	}

};

class ATdWeapon_AssaultRifle_HKG36 : public ATdWeapon_Heavy
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_AssaultRifle_HKG36");
		return ptr;
	}

};

class ATdWeapon_AssaultRifle_MP5K : public ATdWeapon_Heavy
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_AssaultRifle_MP5K");
		return ptr;
	}

};

class ATdWeapon_FlashbangGrenade : public ATdWeapon_Grenade
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_FlashbangGrenade");
		return ptr;
	}

};

class ATdWeapon_Machinegun_FNMinimi : public ATdWeapon_Heavy
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Machinegun_FNMinimi");
		return ptr;
	}

};

class ATdWeapon_Pistol_BerettaM93R : public ATdWeapon_Light
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Pistol_BerettaM93R");
		return ptr;
	}

};

class ATdWeapon_Pistol_Colt1911 : public ATdWeapon_Light
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Pistol_Colt1911");
		return ptr;
	}

};

class ATdWeapon_Pistol_DE05 : public ATdWeapon_Light
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Pistol_DE05");
		return ptr;
	}

};

class ATdWeapon_Pistol_Glock18c : public ATdWeapon_Light
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Pistol_Glock18c");
		return ptr;
	}

};

class ATdWeapon_Pistol_TaserContent : public ATdWeapon_Pistol_Taser
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Pistol_TaserContent");
		return ptr;
	}

};

class ATdWeapon_SMG_SteyrTMP : public ATdWeapon_Light
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_SMG_SteyrTMP");
		return ptr;
	}

};

class ATdWeapon_Shotgun_Neostead : public ATdWeapon_Heavy
{
public:
	int PelletCount;

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Shotgun_Neostead");
		return ptr;
	}

};

class ATdWeapon_Shotgun_Remington870 : public ATdWeapon_Heavy
{
public:
	int PelletCount;

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Shotgun_Remington870");
		return ptr;
	}

};

class ATdWeapon_SmokeGrenade : public ATdWeapon_Grenade
{
public:

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_SmokeGrenade");
		return ptr;
	}

};

class ATdWeapon_Sniper_BarretM95 : public ATdWeapon_Heavy
{
public:
	class UMaterialEffect* ScopeEffect;
	class UMaterialInstanceConstant* ScopeEffectMaterialInstance;
	unsigned long LasersAttached : 1;
	unsigned long bZoomed : 1;
	unsigned long bIsZooming : 1;
	unsigned long bWasZoomed : 1;
	class UParticleSystem* LaserBeamTemplate;
	class UParticleSystem* LaserHitTemplate;
	class ULensFlare* LaserSourceLensFlareTemplate;
	struct FName LaserSightSocket;
	class ATdEmitter* LaserBeamEmitter;
	class ATdLensFlareSource* LaserSourceLensFlare;
	class ATdEmitter* LaserHitEmitter;
	float LaserBeamFadeParameter;
	float ZoomFOV;
	float ZoomRate;
	float ZoomDelay;
	float AdditionalUnzoomedSpread;
	float ScopeDelayX;
	float ScopeDelayY;
	struct FRotator ScopeRotationCache;

	static UClass* StaticClass()
	{
		static auto ptr = UObject::FindClass("Class TdSharedContent.TdWeapon_Sniper_BarretM95");
		return ptr;
	}

};

}

#ifdef _MSC_VER
	#pragma pack(pop)
#endif
