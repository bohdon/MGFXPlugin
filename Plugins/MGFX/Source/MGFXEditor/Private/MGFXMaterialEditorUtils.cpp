// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialEditorUtils.h"

#include "MGFXMaterial.h"
#include "MGFXMaterialLayer.h"
#include "UnrealExporter.h"
#include "Exporters/Exporter.h"
#include "Internationalization/TextPackageNamespaceUtil.h"
#include "UObject/Package.h"


// FMGFXMaterialLayerObjectTextFactory
// -----------------------------------

bool FMGFXMaterialLayerObjectTextFactory::CanCreateClass(UClass* ObjectClass, bool& bOmitSubObjs) const
{
	return ObjectClass->IsChildOf(UMGFXMaterialLayer::StaticClass());
}

void FMGFXMaterialLayerObjectTextFactory::ProcessConstructedObject(UObject* CreatedObject)
{
	check(CreatedObject);

	if (UMGFXMaterialLayer* Layer = Cast<UMGFXMaterialLayer>(CreatedObject))
	{
		NewLayersMap.Add(Layer->GetFName(), Layer);
	}
}


// FMGFXMaterialEditorUtils
// ------------------------

FString FMGFXMaterialEditorUtils::MakeUniqueLayerName(const FString& Name, const UMGFXMaterial* InMaterial)
{
	if (!InMaterial)
	{
		return Name;
	}

	TArray<UMGFXMaterialLayer*> AllLayers;
	InMaterial->GetAllLayers(AllLayers);

	FString NewName = Name;

	int32 Number = 1;
	while (AllLayers.FindByPredicate([NewName](const TObjectPtr<UMGFXMaterialLayer> Layer)
	{
		return Layer->Name.Equals(NewName);
	}))
	{
		NewName = Name + FString::FromInt(Number);
		++Number;
	}

	return NewName;
}

FString FMGFXMaterialEditorUtils::CopyLayers(const TArray<UMGFXMaterialLayer*>& LayersToCopy)
{
	// simplify list to only the topmost parents so children aren't added twice later
	const TArray<UMGFXMaterialLayer*> TopmostLayers = GetTopmostLayers(LayersToCopy);

	TArray<UMGFXMaterialLayer*> AllLayersToCopy = TopmostLayers;
	for (const TObjectPtr<UMGFXMaterialLayer> Layer : TopmostLayers)
	{
		// add all children as well
		Layer->GetAllLayers(AllLayersToCopy);
	}

	return ExportLayersToText(AllLayersToCopy);
}

TArray<UMGFXMaterialLayer*> FMGFXMaterialEditorUtils::PasteLayers(UMGFXMaterial* MGFXMaterial, const FString& TextToImport,
                                                                  IMGFXMaterialLayerParentInterface* Container, bool& bSuccess)
{
	// import the objects
	TArray<UMGFXMaterialLayer*> PastedLayers;
	ImportLayersFromText(MGFXMaterial, TextToImport, PastedLayers);

	if (PastedLayers.IsEmpty())
	{
		bSuccess = false;
		return TArray<UMGFXMaterialLayer*>();
	}

	MGFXMaterial->Modify();
	if (UObject* ContainerObject = Cast<UObject>(Container))
	{
		ContainerObject->Modify();
	}

	// add topmost layers to the container
	TArray<UMGFXMaterialLayer*> TopmostPastedLayers = GetTopmostLayers(PastedLayers);
	for (int32 Idx = TopmostPastedLayers.Num() - 1; Idx >= 0; --Idx)
	{
		// insert so that layers are stacked on top
		Container->AddLayer(TopmostPastedLayers[Idx], 0);
	}

	bSuccess = true;
	return TopmostPastedLayers;
}

FString FMGFXMaterialEditorUtils::ExportLayersToText(const TArray<UMGFXMaterialLayer*>& LayersToExport)
{
	// clear the mark state for saving
	UnMarkAllObjects(EObjectMark(OBJECTMARK_TagExp | OBJECTMARK_TagImp));

	FStringOutputDevice Archive;

	// validate all layers are from the same scope
	// TArray<UObject*> LayersToIgnore;
	UObject* LastOuter = nullptr;
	for (UMGFXMaterialLayer* Layer : LayersToExport)
	{
		// all layers should be from the same scope
		UObject* ThisOuter = Layer->GetOuter();
		check((LastOuter == ThisOuter) || (LastOuter == nullptr));
		LastOuter = ThisOuter;
	}

	const FExportObjectInnerContext Context;
	// Export each of the selected nodes
	for (UMGFXMaterialLayer* Layer : LayersToExport)
	{
		UExporter::ExportToOutputDevice(&Context, Layer, nullptr, Archive, TEXT("copy"), 0,
		                                PPF_ExportsNotFullyQualified | PPF_Copy | PPF_Delimited, false, LastOuter);
	}

	return FString(Archive);
}

void FMGFXMaterialEditorUtils::ImportLayersFromText(UMGFXMaterial* MGFXMaterial, const FString& ImportText, TArray<UMGFXMaterialLayer*>& ImportedLayers)
{
	// create a transient package so that we can deserialize in isolation and ensure unreferenced
	// objects not part of the deserialization set are unresolved.
	UPackage* TempPackage = NewObject<UPackage>(nullptr, TEXT("/Engine/MGFXEditor/Transient"), RF_Transient);
	TempPackage->AddToRoot();

	// force the transient package to have the same namespace as the target package.
	// this ensures any text properties serialized from the buffer will be keyed correctly for the target package.
#if USE_STABLE_LOCALIZATION_KEYS
	{
		const FString PackageNamespace = TextNamespaceUtil::EnsurePackageNamespace(MGFXMaterial);
		if (!PackageNamespace.IsEmpty())
		{
			TextNamespaceUtil::ForcePackageNamespace(TempPackage, PackageNamespace);
		}
	}
#endif // USE_STABLE_LOCALIZATION_KEYS

	// resolve text buffer into objects
	FMGFXMaterialLayerObjectTextFactory Factory;
	Factory.ProcessBuffer(TempPackage, RF_Transactional, ImportText);

	for (const auto& Entry : Factory.NewLayersMap)
	{
		UMGFXMaterialLayer* Layer = Entry.Value;

		ImportedLayers.Add(Layer);

		Layer->SetFlags(RF_Transactional);

		// ensure layer object name is unique, and set the new outer
		FName OldObjName = Layer->GetFName();
		FName NewObjName = GetUniqueName(MGFXMaterial, OldObjName);
		if (NewObjName != OldObjName)
		{
			// rename and set outer
			Layer->Rename(*NewObjName.ToString(), MGFXMaterial);
		}
		else
		{
			// just set outer
			Layer->Rename(nullptr, MGFXMaterial);
		}


		// also ensure layer display name is unique
		const FString UniqueLayerName = MakeUniqueLayerName(Layer->Name, MGFXMaterial);
		if (Layer->Name != UniqueLayerName)
		{
			Layer->Name = UniqueLayerName;
		}
	}

	TempPackage->RemoveFromRoot();
}

TArray<UMGFXMaterialLayer*> FMGFXMaterialEditorUtils::GetTopmostLayers(const TArray<UMGFXMaterialLayer*>& Layers)
{
	TArray<UMGFXMaterialLayer*> Result;
	for (UMGFXMaterialLayer* Layer : Layers)
	{
		bool bHasParentInList = false;
		for (const UMGFXMaterialLayer* OtherLayer : Layers)
		{
			if (OtherLayer != Layer && OtherLayer->IsParentLayer(Layer))
			{
				bHasParentInList = true;
				break;
			}
		}

		if (bHasParentInList)
		{
			continue;
		}

		Result.Add(Layer);
	}
	return Result;
}

FName FMGFXMaterialEditorUtils::GetUniqueName(UObject* Outer, const FName& Name)
{
	int32 Number = Name.GetNumber();
	FName TestName = Name;
	while (FindObjectFast<UObject>(Outer, TestName))
	{
		++Number;
		TestName.SetNumber(Number);
	}
	return TestName;
}
