#pragma once
#include <Windows.h>
#include <iostream>

template<typename InElementType>
class TArray
{
public:
	InElementType* Data;
	int32_t	  ArrayNum;
	int32_t	  ArrayMax;
};

class FScriptArray
{
public:
	void* Data;
	int32_t ArrayNum;
	int32_t ArrayMax;
};

class FScriptBitArray
{
public:
	FScriptArray Data;
	int32_t NumBits;
};

typedef FScriptBitArray FArrayBitArray;

class FString : public TArray<wchar_t>
{

};

class FName
{
public:
	uint32_t ComparisonIndex;
	uint32_t Number;
};

class FReferenceControllerBase
{
public:
	int32_t SharedReferenceCount;
	int32_t WeakReferenceCount;
};

class FSharedReferencer
{
public:
	FReferenceControllerBase* ReferenceController;
};

template<class ObjectType>
class TSharedRef
{
	ObjectType* Object;
	FSharedReferencer SharedReferenceCount;
};

class ITextData
{

};

class FText
{
public:
	TSharedRef<ITextData> TextData;
	uint32_t Flags;
};

struct FScriptDelegate
{

};

struct FMulticastScriptDelegate
{

};

struct FWeakObjectPtr
{
	int32_t ObjectIndex;
	int32_t ObjectSerialNumber;
};

template<class T>
struct TWeakObjectPtr : public FWeakObjectPtr
{

};

template<typename KeyType, typename ValueType>
class TPair
{
public:
	KeyType Key;
	ValueType Value;
};

class FSetElementId
{
public:
	int32_t Index;
};

class FDefaultAllocator
{
public:
	template<typename ElementType>
	class ForElementType
	{
	public:
		ElementType* Data;
	};
};

class FDefaultSetAllocator
{
public:
	typedef FDefaultAllocator SparseArrayAllocator;
	typedef FDefaultAllocator HashAllocator;
};

template<typename ElementType>
class TSparseArray
{
public:
	FScriptArray Data;
	FScriptBitArray AllocationFlags;
	int32_t FirstFreeIndex;
	int32_t NumFreeIndices;
};

template<typename InElementType, typename Allocator = FDefaultSetAllocator>
class TSet
{
public:
	typedef typename Allocator::HashAllocator::template ForElementType<FSetElementId> HashType;

	TSparseArray<InElementType> Elements;
	HashType Hash;
	int32_t HashSize;
};

template<typename KeyType, typename ValueType>
class TMap
{
public:
	TSet<TPair<KeyType, ValueType>> Pairs;
};

template<typename T>
class TAssetPtr
{
public:
	FString ObjectID;
};