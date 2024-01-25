// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialBuilder.h"

#include "MaterialEditingLibrary.h"
#include "MGFXEditorModule.h"
#include "MGFXPropertyMacros.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/PropertyAccessUtil.h"


FMGFXMaterialBuilder::FMGFXMaterialBuilder(UMaterial* InMaterial)
	: Material(InMaterial)
{
}

UMaterialExpression* FMGFXMaterialBuilder::Create(TSubclassOf<UMaterialExpression> ExpressionClass, const FVector2D& NodePos) const
{
	UMaterialExpression* NewExp = UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionClass, NodePos.X, NodePos.Y);

	// UMaterialEditingLibrary doesn't add comments to the correct collection, fix it
	if (UMaterialExpressionComment* CommentExp = Cast<UMaterialExpressionComment>(NewExp))
	{
		Material->GetExpressionCollection().RemoveExpression(CommentExp);
		Material->GetExpressionCollection().AddComment(CommentExp);
	}

	return NewExp;
}

UMaterialExpressionComponentMask* FMGFXMaterialBuilder::CreateComponentMask(const FVector2D& NodePos, uint32 R, uint32 G, uint32 B, uint32 A) const
{
	UMaterialExpressionComponentMask* ComponentMaskExp = Create<UMaterialExpressionComponentMask>(NodePos);
	ComponentMaskExp->R = R;
	ComponentMaskExp->G = G;
	ComponentMaskExp->B = B;
	ComponentMaskExp->A = A;
	return ComponentMaskExp;
}

UMaterialExpressionComponentMask* FMGFXMaterialBuilder::CreateComponentMaskRGB(const FVector2D& NodePos) const
{
	return CreateComponentMask(NodePos, 1, 1, 1, 0);
}

UMaterialExpressionComponentMask* FMGFXMaterialBuilder::CreateComponentMaskA(const FVector2D& NodePos) const
{
	return CreateComponentMask(NodePos, 0, 0, 0, 1);
}

UMaterialExpressionAppendVector* FMGFXMaterialBuilder::CreateAppend(const FVector2D& NodePos, UMaterialExpression* A, UMaterialExpression* B) const
{
	UMaterialExpressionAppendVector* AppendExp = Create<UMaterialExpressionAppendVector>(NodePos);
	Connect(A, "", AppendExp, "A");
	Connect(B, "", AppendExp, "B");
	return AppendExp;
}

UMaterialExpressionScalarParameter* FMGFXMaterialBuilder::CreateScalarParam(const FVector2D& NodePos, FName ParameterName, FName Group,
                                                                            int32 SortPriority) const
{
	UMaterialExpressionScalarParameter* ScalarExp = Create<UMaterialExpressionScalarParameter>(NodePos);
	ConfigureParameter(ScalarExp, ParameterName, Group, SortPriority);
	return ScalarExp;
}

UMaterialExpressionNamedRerouteDeclaration* FMGFXMaterialBuilder::CreateNamedReroute(const FVector2D& NodePos, const FName Name) const
{
	UMaterialExpressionNamedRerouteDeclaration* RerouteExp = Create<UMaterialExpressionNamedRerouteDeclaration>(NodePos);
	SET_PROP(RerouteExp, Name, Name);
	return RerouteExp;
}

UMaterialExpressionNamedRerouteDeclaration* FMGFXMaterialBuilder::CreateNamedReroute(const FVector2D& NodePos, const FName Name, const FLinearColor Color) const
{
	UMaterialExpressionNamedRerouteDeclaration* RerouteExp = CreateNamedReroute(NodePos, Name);
	SET_PROP(RerouteExp, NodeColor, Color);
	return RerouteExp;
}

UMaterialExpressionNamedRerouteUsage* FMGFXMaterialBuilder::CreateNamedRerouteUsage(const FVector2D& NodePos,
                                                                                    UMaterialExpressionNamedRerouteDeclaration* Declaration) const
{
	check(Declaration);
	UMaterialExpressionNamedRerouteUsage* UsageExp = Create<UMaterialExpressionNamedRerouteUsage>(NodePos);
	UsageExp->Declaration = Declaration;
	UsageExp->DeclarationGuid = Declaration->VariableGuid;
	return UsageExp;
}

UMaterialExpressionNamedRerouteUsage* FMGFXMaterialBuilder::CreateNamedRerouteUsage(const FVector2D& NodePos, const FName Name) const
{
	const TObjectPtr<UMaterialExpressionNamedRerouteDeclaration> Declaration = FindNamedReroute(Name);
	if (!Declaration)
	{
		UE_LOG(LogMGFXEditor, Error, TEXT("Named reroute not found: %s"), *Name.ToString());
		return nullptr;
	}
	return CreateNamedRerouteUsage(NodePos, Declaration);
}

UMaterialExpressionNamedRerouteDeclaration* FMGFXMaterialBuilder::FindNamedReroute(const FName Name) const
{
	for (TObjectPtr<UMaterialExpression> Expression : Material->GetExpressionCollection().Expressions)
	{
		UMaterialExpressionNamedRerouteDeclaration* Declaration = Cast<UMaterialExpressionNamedRerouteDeclaration>(Expression);
		if (Declaration && Declaration->Name.IsEqual(Name))
		{
			return Declaration;
		}
	}
	return nullptr;
}

UMaterialExpressionParameter* FMGFXMaterialBuilder::FindNamedParameter(const FName ParameterName) const
{
	for (TObjectPtr<UMaterialExpression> Expression : Material->GetExpressions())
	{
		if (UMaterialExpressionParameter* ParamExp = Cast<UMaterialExpressionParameter>(Expression))
		{
			if (ParamExp->GetParameterName().IsEqual(ParameterName))
			{
				return ParamExp;
			}
		}
	}
	return nullptr;
}

UMaterialExpressionComment* FMGFXMaterialBuilder::CreateComment(const FVector2D& NodePos, const FString& Text, FLinearColor Color) const
{
	UMaterialExpressionComment* CommentExp = Create<UMaterialExpressionComment>(NodePos);
	CommentExp->SizeX = 400;
	CommentExp->SizeY = 100;
	SET_PROP(CommentExp, Text, Text);
	SET_PROP(CommentExp, CommentColor, Color);

	return CommentExp;
}

UMaterialExpressionMaterialFunctionCall* FMGFXMaterialBuilder::CreateFunction(const FVector2D& NodePos, UMaterialFunctionInterface* Function) const
{
	check(Function);
	UMaterialExpressionMaterialFunctionCall* FunctionExp = Create<UMaterialExpressionMaterialFunctionCall>(NodePos);
	SET_PROP(FunctionExp, MaterialFunction, Function);
	return FunctionExp;
}

UMaterialExpressionMaterialFunctionCall* FMGFXMaterialBuilder::CreateFunction(const FVector2D& NodePos,
                                                                              const TSoftObjectPtr<UMaterialFunctionInterface>& FunctionPtr) const
{
	check(!FunctionPtr.IsNull());
	return CreateFunction(NodePos, FunctionPtr.LoadSynchronous());
}

void FMGFXMaterialBuilder::SetScalarParameterValue(FName ParameterName, float Value)
{
	if (UMaterialExpressionScalarParameter* ParamExp = FindNamedParameter<UMaterialExpressionScalarParameter>(ParameterName))
	{
		SET_PROP(ParamExp, DefaultValue, Value);
	}
}

void FMGFXMaterialBuilder::SetVectorParameterValue(FName ParameterName, FLinearColor Value)
{
	if (UMaterialExpressionVectorParameter* ParamExp = FindNamedParameter<UMaterialExpressionVectorParameter>(ParameterName))
	{
		SET_PROP(ParamExp, DefaultValue, Value);
	}
}

bool FMGFXMaterialBuilder::Connect(UMaterialExpression* From, const FString& FromPin, UMaterialExpression* To, const FString& ToPin) const
{
	return UMaterialEditingLibrary::ConnectMaterialExpressions(From, FromPin, To, ToPin);
}

bool FMGFXMaterialBuilder::Connect(UMaterialExpression* From, UMaterialExpression* To) const
{
	return Connect(From, FString(), To, FString());
}

bool FMGFXMaterialBuilder::ConnectProperty(UMaterialExpression* From, const FString& FromPin, const EMaterialProperty& Property) const
{
	return UMaterialEditingLibrary::ConnectMaterialProperty(From, FString(FromPin), Property);
}

void FMGFXMaterialBuilder::ConfigureParameter(UMaterialExpressionParameter* ParameterExp, FName ParameterName, FName Group, int32 SortPriority) const
{
	check(ParameterExp);
	SET_PROP(ParameterExp, ParameterName, ParameterName);
	SET_PROP(ParameterExp, Group, Group);
	SET_PROP(ParameterExp, SortPriority, SortPriority);
}

void FMGFXMaterialBuilder::RecompileMaterial()
{
	UMaterialEditingLibrary::RecompileMaterial(Material);
}

void FMGFXMaterialBuilder::DeleteAll()
{
	UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();

	MaterialEditorOnly->BaseColor.Expression = nullptr;
	MaterialEditorOnly->EmissiveColor.Expression = nullptr;
	MaterialEditorOnly->SubsurfaceColor.Expression = nullptr;
	MaterialEditorOnly->Roughness.Expression = nullptr;
	MaterialEditorOnly->Metallic.Expression = nullptr;
	MaterialEditorOnly->Specular.Expression = nullptr;
	MaterialEditorOnly->Opacity.Expression = nullptr;
	MaterialEditorOnly->Refraction.Expression = nullptr;
	MaterialEditorOnly->OpacityMask.Expression = nullptr;
	MaterialEditorOnly->ClearCoat.Expression = nullptr;
	MaterialEditorOnly->ClearCoatRoughness.Expression = nullptr;
	MaterialEditorOnly->Normal.Expression = nullptr;

	Material->GetExpressionCollection().Empty();
}
