// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"


#define COLLISION_PROJECTILE					ECollisionChannel::ECC_GameTraceChannel1
#define COLLISION_ABILITY						ECollisionChannel::ECC_GameTraceChannel2
#define COLLISION_INTERACTABLE					ECollisionChannel::ECC_GameTraceChannel3

UENUM(BlueprintType)
enum class EGSAbilityInputID : uint8
{
	None				UMETA(DisplayName = "None"),
	Confirm				UMETA(DisplayName = "Confirm"),
	Cancel				UMETA(DisplayName = "Cancel"),
	Interact			UMETA(DisplayName = "Interact")
};