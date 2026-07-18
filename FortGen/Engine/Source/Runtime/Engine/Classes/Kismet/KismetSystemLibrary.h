#pragma once
#include "framework.h"
#include "../../../CoreUObject/Public/UObject/Object.h"

class UKismetSystemLibrary : public UObject // TODO: UBlueprintFunctionLibrary
{
public:
	DEFINE_STATICCLASS("/Script/Engine.KismetSystemLibrary", UKismetSystemLibrary)
public:
	static class FString GetEngineVersion();
	static class FString GetPathName(const class UObject* Object);
};