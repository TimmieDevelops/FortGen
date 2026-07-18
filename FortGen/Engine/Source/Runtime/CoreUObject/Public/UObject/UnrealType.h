#pragma once
#include "framework.h"
#include "Class.h"

class UProperty : public UField
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.Property", UProperty)
	DEFINE_MEMBER(ArrayDim, Address::UProperty_ArrayDim, int32_t)
	DEFINE_MEMBER(ElementSize, Address::UProperty_ElementSize, int32_t)
	DEFINE_MEMBER(PropertyFlags, Address::UProperty_PropertyFlags, uint64_t)
	DEFINE_MEMBER(Offset_Internal, Address::UProperty_OffsetInternal, int32_t)
};

class UStructProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.StructProperty", UStructProperty)
	DEFINE_MEMBER(Struct, Address::UStructProperty_Struct, UScriptStruct*)
};

// TProperty_Numeric
class UNumericProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.NumericProperty", UNumericProperty)
};

class UByteProperty : public UNumericProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.ByteProperty", UByteProperty)
	DEFINE_MEMBER(Enum, Address::UByteProperty_Enum, UEnum*)
};

class UArrayProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.ArrayProperty", UArrayProperty)
	DEFINE_MEMBER(Inner, Address::UArrayProperty_Inner, UProperty*)
};

class UMapProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.MapProperty", UMapProperty)
	DEFINE_MEMBER(KeyProp, Address::UMapProperty_KeyProp, UProperty*)
	DEFINE_MEMBER(ValueProp, Address::UMapProperty_ValueProp, UProperty*)
};

class UDelegateProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.DelegateProperty", UDelegateProperty)
	DEFINE_MEMBER(SignatureFunction, Address::UDelegateProperty_SignatureFunction, UFunction*)
};

class UMulticastDelegateProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.MulticastDelegateProperty", UMulticastDelegateProperty)
	DEFINE_MEMBER(SignatureFunction, Address::UMulticastDelegateProperty_SignatureFunction, UFunction*)
};