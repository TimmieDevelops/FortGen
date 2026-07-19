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
#include "Engine/Source/Runtime/Core/Public/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Public/HAL/UnrealMemory.h"
#include "Engine/Source/Runtime/Core/Public/UObject/NameTypes.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/ObjectBase.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/Package.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/Script.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/UnrealType.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectArray.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBase.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectBaseUtility.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/Public/UObject/UTextProperty.h"
#include "Engine/Source/Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

#include "Generator/Public/Dumper.h"
#include "Generator/Public/IDA.h"
#include "Generator/Public/Settings.h"

#endif //PCH_H
