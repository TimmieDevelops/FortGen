#pragma once
#include "framework.h"
#include "Class.h"
#include "ObjectBase.h"

class UProperty : public UField
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.Property", UProperty)
	DEFINE_MEMBER(ArrayDim, Address::UProperty_ArrayDim, int32_t)
	DEFINE_MEMBER(ElementSize, Address::UProperty_ElementSize, int32_t)
	DEFINE_MEMBER(PropertyFlags, Address::UProperty_PropertyFlags, EPropertyFlags)
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

class UBoolProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.BoolProperty", UBoolProperty)
	DEFINE_MEMBER(FieldSize, Address::UBoolProperty_FieldSize, uint8_t)
	DEFINE_MEMBER(ByteOffset, Address::UBoolProperty_ByteOffset, uint8_t)
	DEFINE_MEMBER(ByteMask, Address::UBoolProperty_ByteMask, uint8_t)
	DEFINE_MEMBER(FieldMask, Address::UBoolProperty_FieldMask, uint8_t)
};

class UIntProperty : public UNumericProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.IntProperty", UIntProperty)
};

class UFloatProperty : public UNumericProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.FloatProperty", UFloatProperty)
};

// TUObjectPropertyBase
class UObjectPropertyBase : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.ObjectPropertyBase", UObjectPropertyBase)
	DEFINE_MEMBER(PropertyClass, Address::UObjectPropertyBase_PropertyClass, UProperty*)
};

class UObjectProperty : public UObjectPropertyBase
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.ObjectProperty", UObjectProperty)
};

class UStrProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.StrProperty", UStrProperty)
};

class UWeakObjectProperty : public UObjectPropertyBase
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.WeakObjectProperty", UWeakObjectProperty)
};

class UNameProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.NameProperty", UNameProperty)
};

class UDoubleProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.DoubleProperty", UDoubleProperty)
};

class UUInt32Property : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.UInt32Property", UUInt32Property)
};

class UUInt64Property : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.UInt64Property", UUInt64Property)
};

class UInt16Property : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.Int16Property", UInt16Property)
};

class UInt8Property : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.Int8Property", UInt8Property)
};

class UInt64Property : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.Int64Property", UInt64Property)
};

class UAssetObjectProperty : public UObjectPropertyBase
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.AssetObjectProperty", UAssetObjectProperty)
};

class ULazyObjectProperty : public UObjectPropertyBase
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.LazyObjectProperty", ULazyObjectProperty)
};

class UUInt16Property : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.UInt16Property", UUInt16Property)
};

class UInterfaceProperty : public UProperty
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.InterfaceProperty", UInterfaceProperty)
	DEFINE_MEMBER(InterfaceClass, Address::UInterfaceProperty_InterfaceClass, UClass*)
};