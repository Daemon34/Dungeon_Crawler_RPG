// Copyright Clément Lecoeur for learning purpose


#include "Character/AuraCharacterBase.h"
#include "AbilitySystemComponent.h"

AAuraCharacterBase::AAuraCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

UAbilitySystemComponent* AAuraCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAuraCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAuraCharacterBase::InitAbilityActorInfo()
{
}

void AAuraCharacterBase::InitializePrimaryAttributes() const
{
	UAbilitySystemComponent* AbilitySystemComponentPtr = GetAbilitySystemComponent();
	check(IsValid(AbilitySystemComponentPtr));
	check(DefaultPrimaryAttributes);
	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponentPtr->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponentPtr->MakeOutgoingSpec(DefaultPrimaryAttributes, 1.0f, ContextHandle);
	AbilitySystemComponentPtr->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponentPtr);
}

