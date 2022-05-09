// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GSGameplayAbility.h"
#include "AbilitySystemComponent.h"


UGSGameplayAbility::UGSGameplayAbility() {
	// Default to Instance Per Actor
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	bActivateAbilityOnGranted = false;



}

void UGSGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) {
	
	Super::OnAvatarSet(ActorInfo, Spec);

	if (bActivateAbilityOnGranted)
	{
		bool ActivatedAbility = ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}

}