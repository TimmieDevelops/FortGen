#pragma once
#include "framework.h"

/* FUObjectItem
* Object: 0x0
* ClusterAndFlags: 0x4
* SerialNumber: 0x8
* StructSize: 0xC
*/

/* FFixedUObjectArray
* Objects: 0x0 (resize)
* MaxElements: 0x4
* NumElements: 0x8
*/

/* FUObjectArray
* ObjObjects: 0x0
*/

/**
* Single item in the UObject array.
*/
struct FUObjectItem
{
};

/**
* Fixed size UObject array.
*/
class FFixedUObjectArray
{
public:
};

/***
*
* FUObjectArray replaces the functionality of GObjObjects and UObject::Index
*
* Note the layout of this data structure is mostly to emulate the old behavior and minimize code rework during code restructure.
* Better data structures could be used in the future, for example maybe all that is needed is a TSet<UObject *>
* One has to be a little careful with this, especially with the GC optimization. I have seen spots that assume
* that non-GC objects come before GC ones during iteration.
*
**/
class FUObjectArray
{
public:
};