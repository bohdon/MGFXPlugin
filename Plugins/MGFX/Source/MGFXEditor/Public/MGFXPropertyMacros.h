/**
 * Property access util macros for setting object values via object references.
 */

#pragma once

#include "UObject/PropertyAccessUtil.h"


/** Returns FName(TEXT("MemberName")) while statically checking that the member exists on Object */
#define GET_INST_MEMBER_NAME_CHECKED(Object, MemberName) \
	((void)sizeof(UEAsserts_Private::GetMemberNameCheckedJunk(Object->MemberName)), FName(TEXT(#MemberName)))

/** Find an FProperty, while statically checking that the member exists on Object */
#define FIND_PROP(Object, MemberName) \
	PropertyAccessUtil::FindPropertyByName(GET_INST_MEMBER_NAME_CHECKED(Object, MemberName), Object->GetClass())

/** Set the property of an object, ensuring property change callbacks are triggered. */
#define SET_PROP(Object, MemberName, Value) \
	{ \
		const FProperty* Prop = FIND_PROP(Object, MemberName); \
		PropertyAccessUtil::SetPropertyValue_Object(Prop, Object, Prop, &Value, \
													INDEX_NONE, PropertyAccessUtil::EditorReadOnlyFlags, EPropertyAccessChangeNotifyMode::Default); \
	}

/** Set the property of an object, accepting an r-value. */
#define SET_PROP_R(Object, MemberName, RValue) \
	{ \
		const FProperty* Prop = FIND_PROP(Object, MemberName); \
		auto Value = RValue; \
		PropertyAccessUtil::SetPropertyValue_Object(Prop, Object, Prop, &Value, \
													INDEX_NONE, PropertyAccessUtil::EditorReadOnlyFlags, EPropertyAccessChangeNotifyMode::Default); \
	}

#define SET_PROP_PTR(Object, MemberName, PtrValue) \
	{ \
		const FProperty* Prop = FIND_PROP(Object, MemberName); \
		PropertyAccessUtil::SetPropertyValue_Object(Prop, Object, Prop, PtrValue, \
													INDEX_NONE, PropertyAccessUtil::EditorReadOnlyFlags, EPropertyAccessChangeNotifyMode::Default); \
	}
