// copyright JTJ


#include "GAS/GSInteractabel.h"

// Add default functionality here for any IGSInteractabel functions that are not pure virtual.


float IGSInteractabel::GetInteractionDuration_Implementation(UPrimitiveComponent* primitiveComponent) const {
	return 0.0f;
}

void IGSInteractabel::GetInteractSyncType_Implementation(bool& bShouldSync, EAbilityTaskNetSyncType& Type, UPrimitiveComponent* InteractionComponent) const {
	bShouldSync = false;
	Type = EAbilityTaskNetSyncType::OnlyServerWait;
}