// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

#include "Helpers/Public/Utils.h"
#include "Helpers/Public/Logger.h"
#include "Helpers/Public/Scanner.h"
#include "Helpers/Public/Finder.h"
#include "Helpers/Public/Address.h"
#include "Helpers/Public/VersionInfo.h"

#include "Engine/Source/Runtime/Core/Public/Containers/PropertyMarcos.h"
#include "Engine/Source/Runtime/Core/Public/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Public/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/Public/HAL/UnrealMemory.h"
#include "Engine/Source/Runtime/Core/Public/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectArray.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBase.h" 
#include "Engine/Source/Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

#endif //PCH_H
