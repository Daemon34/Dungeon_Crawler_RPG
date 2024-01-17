// Copyright Clément Lecoeur for learning purpose


#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AttributeInfo.h"
#include "AuraGameplayTags.h"

void UAttributeMenuWidgetController::BindCallbacksToDependencies()
{
}

void UAttributeMenuWidgetController::BroadcastInitialValues()
{
	UAuraAttributeSet* AttributeSetPtr = CastChecked<UAuraAttributeSet>(AttributeSet);

	check(AttributeInfo);

	for (auto& Pair : AttributeSetPtr->TagsToAttributes) {
		FAuraAttributeInfo Info = AttributeInfo->FindAttributeInfoForTag(Pair.Key);
		Info.AttributeValue = Pair.Value().GetNumericValue(AttributeSetPtr);
		AttributeInfoDelegate.Broadcast(Info);
	}
}
