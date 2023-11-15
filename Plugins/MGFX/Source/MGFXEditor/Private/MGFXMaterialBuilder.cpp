﻿// Copyright Bohdon Sayre, All Rights Reserved.


#include "MGFXMaterialBuilder.h"

#include "IMaterialEditor.h"
#include "MaterialEditingLibrary.h"
#include "MGFXEditorModule.h"
#include "MGFXPropertyMacros.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "UObject/PropertyAccessUtil.h"


FMGFXMaterialBuilder::FMGFXMaterialBuilder(IMaterialEditor* InMaterialEditor, UMaterialGraph* InMaterialGraph)
	: MaterialEditor(InMaterialEditor),
	  MaterialGraph(InMaterialGraph)
{
}

UMaterialExpression* FMGFXMaterialBuilder::Create(TSubclassOf<UMaterialExpression> ExpressionClass, const FVector2D& NodePos) const
{
	return MaterialEditor->CreateNewMaterialExpression(ExpressionClass, NodePos, false, false);
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

UMaterialExpressionNamedRerouteUsage* FMGFXMaterialBuilder::CreateNamedRerouteUsage(const FVector2D& NodePos, const FName Name) const
{
	const TObjectPtr<UMaterialExpressionNamedRerouteDeclaration> Declaration = FindNamedReroute(Name);
	if (!Declaration)
	{
		UE_LOG(LogMGFXEditor, Error, TEXT("Named reroute not found: %s"), *Name.ToString());
		return nullptr;
	}

	UMaterialExpressionNamedRerouteUsage* UsageExp = Create<UMaterialExpressionNamedRerouteUsage>(NodePos);
	UsageExp->Declaration = Declaration;
	UsageExp->DeclarationGuid = Declaration->VariableGuid;
	// SET_PROP(UsageExp, Declaration, Declaration);
	// SET_PROP(UsageExp, DeclarationGuid, Declaration->VariableGuid);
	return UsageExp;
}

UMaterialExpressionNamedRerouteDeclaration* FMGFXMaterialBuilder::FindNamedReroute(const FName Name) const
{
	for (auto Node : MaterialGraph->Nodes)
	{
		if (const UMaterialGraphNode* MaterialNode = Cast<UMaterialGraphNode>(Node))
		{
			UMaterialExpressionNamedRerouteDeclaration* Declaration = Cast<UMaterialExpressionNamedRerouteDeclaration>(MaterialNode->MaterialExpression);
			if (Declaration && Declaration->Name.IsEqual(Name))
			{
				return Declaration;
			}
		}
	}
	return nullptr;
}

UMaterialExpressionComment* FMGFXMaterialBuilder::CreateComment(const FVector2D& NodePos, const FString& Text, FLinearColor Color) const
{
	UMaterialExpressionComment* CommentExp = MaterialEditor->CreateNewMaterialExpressionComment(NodePos, MaterialGraph);
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