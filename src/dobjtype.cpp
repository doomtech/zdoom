/*
** dobjtype.cpp
** Implements the type information class
**
**---------------------------------------------------------------------------
** Copyright 1998-2010 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------

#include "dobject.h"
#include "i_system.h"
#include "actor.h"
#include "templates.h"
#include "autosegs.h"
#include "v_text.h"
#include "a_pickups.h"
#include "d_player.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FTypeTable TypeTable;
TArray<PClass *> PClass::AllClasses;
bool PClass::bShutdown;

PInt *TypeSInt8,  *TypeUInt8;
PInt *TypeSInt16, *TypeUInt16;
PInt *TypeSInt32, *TypeUInt32;
PFloat *TypeFloat32, *TypeFloat64;
PString *TypeString;
PName *TypeName;
PSound *TypeSound;
PColor *TypeColor;


// PRIVATE DATA DEFINITIONS ------------------------------------------------

// A harmless non-NULL FlatPointer for classes without pointers.
static const size_t TheEnd = ~(size_t)0;

// CODE --------------------------------------------------------------------

void DumpTypeTable()
{
	int used = 0;
	int min = INT_MAX;
	int max = 0;
	int all = 0;
	int lens[10] = {0};
	for (size_t i = 0; i < countof(TypeTable.TypeHash); ++i)
	{
		int len = 0;
		Printf("%4d:", i);
		for (PType *ty = TypeTable.TypeHash[i]; ty != NULL; ty = ty->HashNext)
		{
			Printf(" -> %s", ty->IsKindOf(RUNTIME_CLASS(PNamedType)) ? static_cast<PNamedType*>(ty)->TypeName.GetChars(): ty->GetClass()->TypeName.GetChars());
			len++;
			all++;
		}
		if (len != 0)
		{
			used++;
			if (len < min)
				min = len;
			if (len > max)
				max = len;
		}
		if (len < countof(lens))
		{
			lens[len]++;
		}
		Printf("\n");
	}
	Printf("Used buckets: %d/%u (%.2f%%) for %d entries\n", used, countof(TypeTable.TypeHash), double(used)/countof(TypeTable.TypeHash)*100, all);
	Printf("Min bucket size: %d\n", min);
	Printf("Max bucket size: %d\n", max);
	Printf("Avg bucket size: %.2f\n", double(all) / used);
	int j,k;
	for (k = k = countof(lens)-1; k > 0; --k)
		if (lens[k])
			break;
	for (j = 0; j <= k; ++j)
		Printf("Buckets of len %d: %d (%.2f%%)\n", j, lens[j], j!=0?double(lens[j])/used*100:-1.0);
}

/* PClassType *************************************************************/

IMPLEMENT_CLASS(PClassType)

//==========================================================================
//
// PClassType Constructor
//
//==========================================================================

PClassType::PClassType()
: TypeTableType(NULL)
{
}

//==========================================================================
//
// PClassType :: Derive
//
//==========================================================================

void PClassType::Derive(PClass *newclass)
{
	assert(newclass->IsKindOf(RUNTIME_CLASS(PClassType)));
	Super::Derive(newclass);
	static_cast<PClassType *>(newclass)->TypeTableType = TypeTableType;
}

/* PClassClass ************************************************************/

IMPLEMENT_CLASS(PClassClass)

//==========================================================================
//
// PClassClass Constructor
//
// The only thing we want to do here is automatically set TypeTableType
// to PClass.
//
//==========================================================================

PClassClass::PClassClass()
{
	TypeTableType = RUNTIME_CLASS(PClass);
}

/* PType ******************************************************************/

IMPLEMENT_ABSTRACT_POINTY_CLASS(PType)
 DECLARE_POINTER(HashNext)
END_POINTERS

//==========================================================================
//
// PType Default Constructor
//
//==========================================================================

PType::PType()
: Size(0), Align(1), HashNext(NULL)
{
}

//==========================================================================
//
// PType Parameterized Constructor
//
//==========================================================================

PType::PType(unsigned int size, unsigned int align)
: Size(size), Align(align), HashNext(NULL)
{
}

//==========================================================================
//
// PType Destructor
//
//==========================================================================

PType::~PType()
{
}

//==========================================================================
//
// PType :: IsMatch
//
//==========================================================================

bool PType::IsMatch(const void *id1, const void *id2) const
{
	return false;
}

//==========================================================================
//
// PType :: GetTypeIDs
//
//==========================================================================

void PType::GetTypeIDs(const void *&id1, const void *&id2) const
{
	id1 = NULL;
	id2 = NULL;
}

//==========================================================================
//
// PType :: StaticInit												STATIC
//
// Set up TypeTableType values for every PType child and create basic types.
//
//==========================================================================

void PType::StaticInit()
{
	RUNTIME_CLASS(PInt)->TypeTableType = RUNTIME_CLASS(PInt);
	RUNTIME_CLASS(PFloat)->TypeTableType = RUNTIME_CLASS(PFloat);
	RUNTIME_CLASS(PString)->TypeTableType = RUNTIME_CLASS(PString);
	RUNTIME_CLASS(PName)->TypeTableType = RUNTIME_CLASS(PName);
	RUNTIME_CLASS(PSound)->TypeTableType = RUNTIME_CLASS(PSound);
	RUNTIME_CLASS(PColor)->TypeTableType = RUNTIME_CLASS(PColor);
	RUNTIME_CLASS(PPointer)->TypeTableType = RUNTIME_CLASS(PPointer);
	RUNTIME_CLASS(PClassPointer)->TypeTableType = RUNTIME_CLASS(PPointer);	// not sure about this yet
	RUNTIME_CLASS(PEnum)->TypeTableType = RUNTIME_CLASS(PEnum);
	RUNTIME_CLASS(PArray)->TypeTableType = RUNTIME_CLASS(PArray);
	RUNTIME_CLASS(PDynArray)->TypeTableType = RUNTIME_CLASS(PDynArray);
	RUNTIME_CLASS(PVector)->TypeTableType = RUNTIME_CLASS(PVector);
	RUNTIME_CLASS(PMap)->TypeTableType = RUNTIME_CLASS(PMap);
	RUNTIME_CLASS(PStruct)->TypeTableType = RUNTIME_CLASS(PStruct);
	RUNTIME_CLASS(PPrototype)->TypeTableType = RUNTIME_CLASS(PPrototype);
	RUNTIME_CLASS(PFunction)->TypeTableType = RUNTIME_CLASS(PFunction);
	RUNTIME_CLASS(PClass)->TypeTableType = RUNTIME_CLASS(PClass);

	TypeTable.AddType(TypeSInt8 = new PInt(1, false));
	TypeTable.AddType(TypeUInt8 = new PInt(1, true));
	TypeTable.AddType(TypeSInt16 = new PInt(2, false));
	TypeTable.AddType(TypeUInt16 = new PInt(2, true));
	TypeTable.AddType(TypeSInt32 = new PInt(4, false));
	TypeTable.AddType(TypeUInt32 = new PInt(4, true));
	TypeTable.AddType(TypeFloat32 = new PFloat(4));
	TypeTable.AddType(TypeFloat64 = new PFloat(8));
	TypeTable.AddType(TypeString = new PString);
	TypeTable.AddType(TypeName = new PName);
	TypeTable.AddType(TypeSound = new PSound);
	TypeTable.AddType(TypeColor = new PColor);

}


/* PBasicType *************************************************************/

IMPLEMENT_ABSTRACT_CLASS(PBasicType)

//==========================================================================
//
// PBasicType Default Constructor
//
//==========================================================================

PBasicType::PBasicType()
{
}

//==========================================================================
//
// PBasicType Parameterized Constructor
//
//==========================================================================

PBasicType::PBasicType(unsigned int size, unsigned int align)
: PType(size, align)
{
}

/* PCompoundType **********************************************************/

IMPLEMENT_ABSTRACT_CLASS(PCompoundType)

/* PNamedType *************************************************************/

IMPLEMENT_ABSTRACT_POINTY_CLASS(PNamedType)
 DECLARE_POINTER(Outer)
END_POINTERS

//==========================================================================
//
// PNamedType :: IsMatch
//
//==========================================================================

bool PNamedType::IsMatch(const void *id1, const void *id2) const
{
	const DObject *outer = (const DObject *)id1;
	FName name = (ENamedName)(intptr_t)id2;
	
	return Outer == outer && TypeName == name;
}

//==========================================================================
//
// PNamedType :: GetTypeIDs
//
//==========================================================================

void PNamedType::GetTypeIDs(const void *&id1, const void *&id2) const
{
	id1 = Outer;
	id2 = (void *)(intptr_t)TypeName;
}

/* PInt *******************************************************************/

IMPLEMENT_CLASS(PInt)

//==========================================================================
//
// PInt Default Constructor
//
//==========================================================================

PInt::PInt()
: PBasicType(4, 4)
{
}

//==========================================================================
//
// PInt Parameterized Constructor
//
//==========================================================================

PInt::PInt(unsigned int size, bool unsign)
: PBasicType(size, size), Unsigned(unsign)
{
}

/* PFloat *****************************************************************/

IMPLEMENT_CLASS(PFloat)

//==========================================================================
//
// PFloat Default Constructor
//
//==========================================================================

PFloat::PFloat()
: PBasicType(4, 4)
{
}

//==========================================================================
//
// PFloat Parameterized Constructor
//
//==========================================================================

PFloat::PFloat(unsigned int size)
: PBasicType(size, size)
{
}

/* PString ****************************************************************/

IMPLEMENT_CLASS(PString)

//==========================================================================
//
// PString Default Constructor
//
//==========================================================================

PString::PString()
: PBasicType(sizeof(FString), __alignof(FString))
{
}

/* PName ******************************************************************/

IMPLEMENT_CLASS(PName)

//==========================================================================
//
// PName Default Constructor
//
//==========================================================================

PName::PName()
: PInt(sizeof(FName), true)
{
	assert(sizeof(FName) == __alignof(FName));
}

/* PSound *****************************************************************/

IMPLEMENT_CLASS(PSound)

//==========================================================================
//
// PSound Default Constructor
//
//==========================================================================

PSound::PSound()
: PInt(sizeof(FSoundID), true)
{
	assert(sizeof(FSoundID) == __alignof(FSoundID));
}

/* PColor *****************************************************************/

IMPLEMENT_CLASS(PColor)

//==========================================================================
//
// PColor Default Constructor
//
//==========================================================================

PColor::PColor()
: PInt(sizeof(PalEntry), true)
{
	assert(sizeof(PalEntry) == __alignof(PalEntry));
}

/* PPointer ***************************************************************/

IMPLEMENT_POINTY_CLASS(PPointer)
 DECLARE_POINTER(PointedType)
END_POINTERS

//==========================================================================
//
// PPointer :: IsMatch
//
//==========================================================================

bool PPointer::IsMatch(const void *id1, const void *id2) const
{
	assert(id2 == NULL);
	PType *pointat = (PType *)id1;

	return pointat == PointedType;
}

//==========================================================================
//
// PPointer :: GetTypeIDs
//
//==========================================================================

void PPointer::GetTypeIDs(const void *&id1, const void *&id2) const
{
	id1 = PointedType;
	id2 = NULL;
}


/* PClassPointer **********************************************************/

IMPLEMENT_POINTY_CLASS(PClassPointer)
 DECLARE_POINTER(ClassRestriction)
END_POINTERS

//==========================================================================
//
// PClassPointer :: IsMatch
//
//==========================================================================

bool PClassPointer::IsMatch(const void *id1, const void *id2) const
{
	const PType *pointat = (const PType *)id1;
	const PClass *classat = (const PClass *)id2;

	assert(pointat->IsKindOf(RUNTIME_CLASS(PClass)));
	return classat == ClassRestriction;
}

//==========================================================================
//
// PClassPointer :: GetTypeIDs
//
//==========================================================================

void PClassPointer::GetTypeIDs(const void *&id1, const void *&id2) const
{
	assert(PointedType == RUNTIME_CLASS(PClass));
	id1 = PointedType;
	id2 = ClassRestriction;
}


/* PEnum ******************************************************************/

IMPLEMENT_POINTY_CLASS(PEnum)
 DECLARE_POINTER(ValueType)
END_POINTERS

/* PArray *****************************************************************/

IMPLEMENT_POINTY_CLASS(PArray)
 DECLARE_POINTER(ElementType)
END_POINTERS

//==========================================================================
//
// PArray :: IsMatch
//
//==========================================================================

bool PArray::IsMatch(const void *id1, const void *id2) const
{
	const PType *elemtype = (const PType *)id1;
	unsigned int count = (unsigned int)(intptr_t)id2;

	return elemtype == ElementType && count == ElementCount;
}

//==========================================================================
//
// PArray :: GetTypeIDs
//
//==========================================================================

void PArray::GetTypeIDs(const void *&id1, const void *&id2) const
{
	id1 = ElementType;
	id2 = (void *)(intptr_t)ElementCount;
}

/* PVector ****************************************************************/

IMPLEMENT_CLASS(PVector)

/* PDynArray **************************************************************/

IMPLEMENT_POINTY_CLASS(PDynArray)
 DECLARE_POINTER(ElementType)
END_POINTERS

//==========================================================================
//
// PDynArray :: IsMatch
//
//==========================================================================

bool PDynArray::IsMatch(const void *id1, const void *id2) const
{
	assert(id2 == NULL);
	const PType *elemtype = (const PType *)id1;

	return elemtype == ElementType;
}

//==========================================================================
//
// PDynArray :: GetTypeIDs
//
//==========================================================================

void PDynArray::GetTypeIDs(const void *&id1, const void *&id2) const
{
	id1 = ElementType;
	id2 = NULL;
}


/* PMap *******************************************************************/

IMPLEMENT_POINTY_CLASS(PMap)
 DECLARE_POINTER(KeyType)
 DECLARE_POINTER(ValueType)
END_POINTERS

//==========================================================================
//
// PMap :: IsMatch
//
//==========================================================================

bool PMap::IsMatch(const void *id1, const void *id2) const
{
	const PType *keyty = (const PType *)id1;
	const PType *valty = (const PType *)id2;

	return keyty == KeyType && valty == ValueType;
}

//==========================================================================
//
// PMap :: GetTypeIDs
//
//==========================================================================

void PMap::GetTypeIDs(const void *&id1, const void *&id2) const
{
	id1 = KeyType;
	id2 = ValueType;
}

/* PStruct ****************************************************************/

IMPLEMENT_CLASS(PStruct)

//==========================================================================
//
// PStruct :: PropagateMark
//
//==========================================================================

size_t PStruct::PropagateMark()
{
	GC::MarkArray(Fields);
	return Fields.Size() * sizeof(void*) + Super::PropagateMark();
}

/* PPrototype *************************************************************/

IMPLEMENT_CLASS(PPrototype)

//==========================================================================
//
// PPrototype :: IsMatch
//
//==========================================================================

bool PPrototype::IsMatch(const void *id1, const void *id2) const
{
	const TArray<PType *> *args = (const TArray<PType *> *)id1;
	const TArray<PType *> *rets = (const TArray<PType *> *)id2;

	return *args == ArgumentTypes && *rets == ReturnTypes;
}

//==========================================================================
//
// PPrototype :: GetTypeIDs
//
//==========================================================================

void PPrototype::GetTypeIDs(const void *&id1, const void *&id2) const
{
	id1 = &ArgumentTypes;
	id2 = &ReturnTypes;
}

//==========================================================================
//
// PPrototype :: PropagateMark
//
//==========================================================================

size_t PPrototype::PropagateMark()
{
	GC::MarkArray(ArgumentTypes);
	GC::MarkArray(ReturnTypes);
	return (ArgumentTypes.Size() + ReturnTypes.Size()) * sizeof(void*) +
		Super::PropagateMark();
}

/* PFunction **************************************************************/

IMPLEMENT_CLASS(PFunction)

//==========================================================================
//
// PFunction :: PropagataMark
//
//==========================================================================

size_t PFunction::PropagateMark()
{
	for (unsigned i = 0; i < Variants.Size(); ++i)
	{
		GC::Mark(Variants[i].Proto);
		GC::Mark(Variants[i].Implementation);
	}
	return Variants.Size() * sizeof(Variants[0]) + Super::PropagateMark();
}

/* PClass *****************************************************************/

IMPLEMENT_POINTY_CLASS(PClass)
 DECLARE_POINTER(ParentClass)
END_POINTERS

//==========================================================================
//
// cregcmp
//
// Sorter to keep built-in types in a deterministic order. (Needed?)
//
//==========================================================================

static int STACK_ARGS cregcmp (const void *a, const void *b)
{
	const PClass *class1 = *(const PClass **)a;
	const PClass *class2 = *(const PClass **)b;
	return strcmp(class1->TypeName, class2->TypeName);
}

//==========================================================================
//
// PClass :: StaticInit												STATIC
//
// Creates class metadata for all built-in types.
//
//==========================================================================

void PClass::StaticInit ()
{
	atterm (StaticShutdown);

	StaticBootstrap();

	FAutoSegIterator probe(CRegHead, CRegTail);

	while (*++probe != NULL)
	{
		((ClassReg *)*probe)->RegisterClass ();
	}

	// Keep built-in classes in consistant order. I did this before, though
	// I'm not sure if this is really necessary to maintain any sort of sync.
	qsort(&AllClasses[0], AllClasses.Size(), sizeof(AllClasses[0]), cregcmp);
}

//==========================================================================
//
// PClass :: StaticShutdown											STATIC
//
// Frees FlatPointers belonging to all classes. Only really needed to avoid
// memory leak warnings at exit.
//
//==========================================================================

void PClass::StaticShutdown ()
{
	TArray<size_t *> uniqueFPs(64);
	unsigned int i, j;

	for (i = 0; i < PClass::AllClasses.Size(); ++i)
	{
		PClass *type = PClass::AllClasses[i];
		PClass::AllClasses[i] = NULL;
		if (type->FlatPointers != &TheEnd && type->FlatPointers != type->Pointers)
		{
			// FlatPointers are shared by many classes, so we must check for
			// duplicates and only delete those that are unique.
			for (j = 0; j < uniqueFPs.Size(); ++j)
			{
				if (type->FlatPointers == uniqueFPs[j])
				{
					break;
				}
			}
			if (j == uniqueFPs.Size())
			{
				uniqueFPs.Push(const_cast<size_t *>(type->FlatPointers));
			}
		}
	}
	for (i = 0; i < uniqueFPs.Size(); ++i)
	{
		delete[] uniqueFPs[i];
	}
	TypeTable.Clear();
	bShutdown = true;
}

//==========================================================================
//
// PClass :: StaticBootstrap										STATIC
//
// PClass and PClassClass have intermingling dependencies on their
// definitions. To sort this out, we explicitly define them before
// proceeding with the RegisterClass loop in StaticInit().
//
//==========================================================================

void PClass::StaticBootstrap()
{
	PClassClass *clscls = new PClassClass;
	PClassClass::RegistrationInfo.SetupClass(clscls);

	PClassClass *cls = new PClassClass;
	PClass::RegistrationInfo.SetupClass(cls);

	// The PClassClass constructor initialized these to NULL, because the
	// PClass metadata had not been created yet. Now it has, so we know what
	// they should be and can insert them into the type table successfully.
	clscls->TypeTableType = cls;
	cls->TypeTableType = cls;
	clscls->InsertIntoHash();
	cls->InsertIntoHash();

	// Create parent objects before we go so that these definitions are complete.
	clscls->ParentClass = PClassType::RegistrationInfo.ParentType->RegisterClass();
	cls->ParentClass = PClass::RegistrationInfo.ParentType->RegisterClass();
}

//==========================================================================
//
// PClass Constructor
//
//==========================================================================

PClass::PClass()
{
	Size = sizeof(DObject);
	ParentClass = NULL;
	Pointers = NULL;
	FlatPointers = NULL;
	HashNext = NULL;
	Defaults = NULL;
	bRuntimeClass = false;
	ConstructNative = NULL;

	PClass::AllClasses.Push(this);
}

//==========================================================================
//
// PClass Destructor
//
//==========================================================================

PClass::~PClass()
{
	Symbols.ReleaseSymbols();
	if (Defaults != NULL)
	{
		M_Free(Defaults);
		Defaults = NULL;
	}
}

//==========================================================================
//
// ClassReg :: RegisterClass
//
// Create metadata describing the built-in class this struct is intended
// for.
//
//==========================================================================

PClass *ClassReg::RegisterClass()
{
	static ClassReg *const metaclasses[] =
	{
		&PClass::RegistrationInfo,
		&PClassActor::RegistrationInfo,
		&PClassInventory::RegistrationInfo,
		&PClassAmmo::RegistrationInfo,
		&PClassHealth::RegistrationInfo,
		&PClassPuzzleItem::RegistrationInfo,
		&PClassWeapon::RegistrationInfo,
		&PClassPlayerPawn::RegistrationInfo,
		&PClassType::RegistrationInfo,
		&PClassClass::RegistrationInfo,
	};

	// Skip classes that have already been registered
	if (MyClass != NULL)
	{
		return MyClass;
	}

	// Add type to list
	PClass *cls;

	if (MetaClassNum >= countof(metaclasses))
	{
		assert(0 && "Class registry has an invalid meta class identifier");
	}

	if (metaclasses[MetaClassNum]->MyClass == NULL)
	{ // Make sure the meta class is already registered before registering this one
		metaclasses[MetaClassNum]->RegisterClass();
	}
	cls = static_cast<PClass *>(metaclasses[MetaClassNum]->MyClass->CreateNew());

	SetupClass(cls);
	cls->InsertIntoHash();
	if (ParentType != NULL)
	{
		cls->ParentClass = ParentType->RegisterClass();
	}
	return cls;
}

//==========================================================================
//
// ClassReg :: SetupClass
//
// Copies the class-defining parameters from a ClassReg to the Class object
// created for it.
//
//==========================================================================

void ClassReg::SetupClass(PClass *cls)
{
	assert(MyClass == NULL);
	MyClass = cls;
	cls->TypeName = FName(Name+1);
	cls->Size = SizeOf;
	cls->Pointers = Pointers;
	cls->ConstructNative = ConstructNative;
}

//==========================================================================
//
// PClass :: InsertIntoHash
//
// Add class to the type table.
//
//==========================================================================

void PClass::InsertIntoHash ()
{
	size_t bucket;
	PType *found;

	found = TypeTable.FindType(RUNTIME_CLASS(PClass), Outer, (void*)(intptr_t)(int)TypeName, &bucket);
	if (found != NULL)
	{ // This type has already been inserted
	  // ... but there is no need whatsoever to make it a fatal error!
		Printf (TEXTCOLOR_RED"Tried to register class '%s' more than once.\n", TypeName.GetChars());
	}
	else
	{
		TypeTable.AddType(this, RUNTIME_CLASS(PClass), Outer, (void*)(intptr_t)(int)TypeName, bucket);
	}
}

//==========================================================================
//
// PClass :: FindClass
//
// Find a type, passed the name as a name.
//
//==========================================================================

PClass *PClass::FindClass (FName zaname)
{
	if (zaname == NAME_None)
	{
		return NULL;
	}
	return static_cast<PClass *>(TypeTable.FindType(RUNTIME_CLASS(PClass),
		/*FIXME:Outer*/NULL, (void*)(intptr_t)(int)zaname, NULL));
}

//==========================================================================
//
// PClass :: CreateNew
//
// Create a new object that this class represents
//
//==========================================================================

DObject *PClass::CreateNew() const
{
	BYTE *mem = (BYTE *)M_Malloc (Size);
	assert (mem != NULL);

	// Set this object's defaults before constructing it.
	if (Defaults != NULL)
		memcpy (mem, Defaults, Size);
	else
		memset (mem, 0, Size);

	ConstructNative (mem);
	((DObject *)mem)->SetClass (const_cast<PClass *>(this));
	return (DObject *)mem;
}

//==========================================================================
//
// PClass :: Derive
//
// Copies inheritable values into the derived class and other miscellaneous setup.
//
//==========================================================================

void PClass::Derive(PClass *newclass)
{
	newclass->ParentClass = this;
	newclass->ConstructNative = ConstructNative;

	// Set up default instance of the new class.
	newclass->Defaults = (BYTE *)M_Malloc(newclass->Size);
	if (Defaults) memcpy(newclass->Defaults, Defaults, Size);
	if (newclass->Size > Size)
	{
		memset(newclass->Defaults + Size, 0, newclass->Size - Size);
	}

	newclass->Symbols.SetParentTable(&this->Symbols);
}

//==========================================================================
//
// PClass :: CreateDerivedClass
//
// Create a new class based on an existing class
//
//==========================================================================

PClass *PClass::CreateDerivedClass(FName name, unsigned int size)
{
	assert (size >= Size);
	PClass *type;
	bool notnew;

	const PClass *existclass = FindClass(name);

	// This is a placeholder so fill it in
	if (existclass != NULL && existclass->Size == (unsigned)-1)
	{
		type = const_cast<PClass*>(existclass);
		if (!IsDescendantOf(type->ParentClass))
		{
			I_Error("%s must inherit from %s but doesn't.", name.GetChars(), type->ParentClass->TypeName.GetChars());
		}
		DPrintf("Defining placeholder class %s\n", name.GetChars());
		notnew = true;
	}
	else
	{
		// Create a new type object of the same type as us. (We may be a derived class of PClass.)
		type = static_cast<PClass *>(GetClass()->CreateNew());
		notnew = false;
	}

	type->TypeName = name;
	type->Size = size;
	type->bRuntimeClass = true;
	Derive(type);
	if (!notnew)
	{
		type->InsertIntoHash();
	}
	return type;
}

//==========================================================================
//
// PClass:: Extend
//
// Add <extension> bytes to the end of this class. Returns the previous
// size of the class.
//
//==========================================================================

unsigned int PClass::Extend(unsigned int extension)
{
	assert(this->bRuntimeClass);

	unsigned int oldsize = Size;
	Size += extension;
	Defaults = (BYTE *)M_Realloc(Defaults, Size);
	memset(Defaults + oldsize, 0, extension);
	return oldsize;
}

//==========================================================================
//
// PClass :: FindClassTentative
//
// Like FindClass but creates a placeholder if no class is found.
// CreateDerivedClass will automatically fill in the placeholder when the
// actual class is defined.
//
//==========================================================================

PClass *PClass::FindClassTentative(FName name)
{
	if (name == NAME_None)
	{
		return NULL;
	}
	size_t bucket;

	PType *found = TypeTable.FindType(RUNTIME_CLASS(PClass),
		/*FIXME:Outer*/NULL, (void*)(intptr_t)(int)name, &bucket);

	if (found != NULL)
	{
		return static_cast<PClass *>(found);
	}
	PClass *type = static_cast<PClass *>(GetClass()->CreateNew());
	DPrintf("Creating placeholder class %s : %s\n", name.GetChars(), TypeName.GetChars());

	type->TypeName = name;
	type->ParentClass = this;
	type->Size = -1;
	type->bRuntimeClass = true;
	TypeTable.AddType(type, RUNTIME_CLASS(PClass), type->Outer, (void*)(intptr_t)(int)name, bucket);
	return type;
}

//==========================================================================
//
// PClass :: BuildFlatPointers
//
// Create the FlatPointers array, if it doesn't exist already.
// It comprises all the Pointers from superclasses plus this class's own
// Pointers. If this class does not define any new Pointers, then
// FlatPointers will be set to the same array as the super class.
//
//==========================================================================

void PClass::BuildFlatPointers ()
{
	if (FlatPointers != NULL)
	{ // Already built: Do nothing.
		return;
	}
	else if (ParentClass == NULL)
	{ // No parent: FlatPointers is the same as Pointers.
		if (Pointers == NULL)
		{ // No pointers: Make FlatPointers a harmless non-NULL.
			FlatPointers = &TheEnd;
		}
		else
		{
			FlatPointers = Pointers;
		}
	}
	else
	{
		ParentClass->BuildFlatPointers ();
		if (Pointers == NULL)
		{ // No new pointers: Just use the same FlatPointers as the parent.
			FlatPointers = ParentClass->FlatPointers;
		}
		else
		{ // New pointers: Create a new FlatPointers array and add them.
			int numPointers, numSuperPointers;

			// Count pointers defined by this class.
			for (numPointers = 0; Pointers[numPointers] != ~(size_t)0; numPointers++)
			{ }
			// Count pointers defined by superclasses.
			for (numSuperPointers = 0; ParentClass->FlatPointers[numSuperPointers] != ~(size_t)0; numSuperPointers++)
			{ }

			// Concatenate them into a new array
			size_t *flat = new size_t[numPointers + numSuperPointers + 1];
			if (numSuperPointers > 0)
			{
				memcpy (flat, ParentClass->FlatPointers, sizeof(size_t)*numSuperPointers);
			}
			memcpy (flat + numSuperPointers, Pointers, sizeof(size_t)*(numPointers+1));
			FlatPointers = flat;
		}
	}
}

//==========================================================================
//
// PClass :: NativeClass
//
// Finds the underlying native type underlying this class.
//
//==========================================================================

const PClass *PClass::NativeClass() const
{
	const PClass *cls = this;

	while (cls && cls->bRuntimeClass)
		cls = cls->ParentClass;

	return cls;
}

//==========================================================================
//
// PClass :: PropagateMark
//
//==========================================================================

size_t PClass::PropagateMark()
{
	size_t marked;

	// Mark symbols
	marked = Symbols.MarkSymbols();

	return marked + Super::PropagateMark();
}

/* FTypeTable **************************************************************/

//==========================================================================
//
// FTypeTable :: FindType
//
//==========================================================================

PType *FTypeTable::FindType(PClass *metatype, const void *parm1, const void *parm2, size_t *bucketnum)
{
	size_t bucket = Hash(metatype, parm1, parm2) % HASH_SIZE;
	if (bucketnum != NULL)
	{
		*bucketnum = bucket;
	}
	for (PType *type = TypeHash[bucket]; type != NULL; type = type->HashNext)
	{
		if (type->GetClass()->TypeTableType == metatype && type->IsMatch(parm1, parm2))
		{
			return type;
		}
	}
	return NULL;
}

//==========================================================================
//
// FTypeTable :: AddType - Fully Parameterized Version
//
//==========================================================================

void FTypeTable::AddType(PType *type, PClass *metatype, const void *parm1, const void *parm2, size_t bucket)
{
#ifdef _DEBUG
	size_t bucketcheck;
	assert(metatype == type->GetClass()->TypeTableType && "Metatype does not match passed object");
	assert(FindType(metatype, parm1, parm2, &bucketcheck) == NULL && "Type must not be inserted more than once");
	assert(bucketcheck == bucket && "Passed bucket was wrong");
#endif
	type->HashNext = TypeHash[bucket];
	TypeHash[bucket] = type;
	GC::WriteBarrier(type);
}

//==========================================================================
//
// FTypeTable :: AddType - Simple Version
//
//==========================================================================

void FTypeTable::AddType(PType *type)
{
	PClass *metatype;
	const void *parm1, *parm2;
	size_t bucket;

	metatype = type->GetClass()->TypeTableType;
	type->GetTypeIDs(parm1, parm2);
	bucket = Hash(metatype, parm1, parm2) % HASH_SIZE;
	assert(FindType(metatype, parm1, parm2, NULL) == NULL && "Type must not be inserted more than once");

	type->HashNext = TypeHash[bucket];
	TypeHash[bucket] = type;
	GC::WriteBarrier(type);
}

//==========================================================================
//
// FTypeTable :: Hash												STATIC
//
//==========================================================================

size_t FTypeTable::Hash(const void *p1, const void *p2, const void *p3)
{
	size_t i1 = (size_t)p1;
	size_t i2 = (size_t)p2;
	size_t i3 = (size_t)p3;

	// Swap the high and low halves of i1. The compiler should be smart enough
	// to transform this into a ROR or ROL.
	i1 = (i1 >> (sizeof(size_t)*4)) | (i1 << (sizeof(size_t)*4));

	return (~i1 ^ i2) + i3 * 961748927;	// i3 is multiplied by a prime
}

//==========================================================================
//
// FTypeTable :: Mark
//
// Mark all types in this table for the garbage collector.
//
//==========================================================================

void FTypeTable::Mark()
{
	for (int i = HASH_SIZE - 1; i >= 0; --i)
	{
		if (TypeHash[i] != NULL)
		{
			GC::Mark(TypeHash[i]);
		}
	}
}

//==========================================================================
//
// FTypeTable :: Clear
//
// Removes everything from the table. We let the garbage collector worry
// about deleting them.
//
//==========================================================================

void FTypeTable::Clear()
{
	memset(TypeHash, 0, sizeof(TypeHash));
}

#include "c_dispatch.h"
CCMD(typetable)
{
	DumpTypeTable();
}

// Symbol tables ------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS(PSymbol);
IMPLEMENT_CLASS(PSymbolConst);
IMPLEMENT_CLASS(PSymbolVariable);
IMPLEMENT_POINTY_CLASS(PSymbolActionFunction)
 DECLARE_POINTER(Function)
END_POINTERS
IMPLEMENT_POINTY_CLASS(PSymbolVMFunction)
 DECLARE_POINTER(Function)
END_POINTERS

//==========================================================================
//
//
//
//==========================================================================

PSymbol::~PSymbol()
{
}

PSymbolTable::PSymbolTable()
: ParentSymbolTable(NULL)
{
}

PSymbolTable::~PSymbolTable ()
{
	ReleaseSymbols();
}

size_t PSymbolTable::MarkSymbols()
{
	size_t count = 0;
	MapType::Iterator it(Symbols);
	MapType::Pair *pair;

	while (it.NextPair(pair))
	{
		GC::Mark(pair->Value);
		count++;
	}
	return count * sizeof(*pair);
}

void PSymbolTable::ReleaseSymbols()
{
	// The GC will take care of deleting the symbols. We just need to
	// clear our references to them.
	Symbols.Clear();
}

void PSymbolTable::SetParentTable (PSymbolTable *parent)
{
	ParentSymbolTable = parent;
}

PSymbol *PSymbolTable::FindSymbol (FName symname, bool searchparents) const
{
	PSymbol * const *value = Symbols.CheckKey(symname);
	if (value == NULL && searchparents && ParentSymbolTable != NULL)
	{
		return ParentSymbolTable->FindSymbol(symname, searchparents);
	}
	return value != NULL ? *value : NULL;
}

PSymbol *PSymbolTable::AddSymbol (PSymbol *sym)
{
	// Symbols that already exist are not inserted.
	if (Symbols.CheckKey(sym->SymbolName) != NULL)
	{
		return NULL;
	}
	Symbols.Insert(sym->SymbolName, sym);
	return sym;
}
