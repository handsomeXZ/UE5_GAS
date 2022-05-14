// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/CollisionProfile.h"
#include "GSAT_WaitInteractableTarget.generated.h"

/*申明一个动态多播
	@param 1. 类型名
	@param 2. 参数
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitInteractableTargetDelegate, const FGameplayAbilityTargetDataHandle&, Data);

/**
 * 
 */
UCLASS()
class GASTEST_API UGSAT_WaitInteractableTarget : public UAbilityTask
{
	GENERATED_BODY()
	UPROPERTY(BlueprintAssignable)
	FWaitInteractableTargetDelegate FoundNewInteractableTarget;

	UPROPERTY(BlueprintAssignable)
	FWaitInteractableTargetDelegate LostInteractableTarget;

	/**
	* Traces a line on a timer looking for InteractableTargets.
	* @param MaxRange How far to trace.
	* @param TimerPeriod Period of trace timer.
	* @param bShowDebug Draws debug lines for traces.
	*/
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks")
	static UGSAT_WaitInteractableTarget* WaitForInteractableTarget(UGameplayAbility* OwningAbility, FName TaskInstanceName, FCollisionProfileName TraceProfile, float MaxRange = 200.0f, float TimerPeriod = 0.1f, bool bShowDebug = true);


	virtual void Activate() override;

private:
	FGameplayAbilityTargetingLocationInfo StartLocationInfo;

	float MaxRange;

	float TimerPeriod;

	bool bShowDebug;

	FCollisionProfileName TraceProfile;

	FGameplayAbilityTargetDataHandle TargetData;

	FTimerHandle TraceTimerHandle;

	virtual void OnDestroy(bool AbilityEnded) override;

	/** Traces as normal, but will manually filter all hit actors */
	void LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params, bool bLookForInteractableActor) const;
	
	void AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, FVector& OutTraceEnd, bool bIgnorePitch = false) const;

	//Calculate the points within the AbilityRange from the camera direction, relative to the AbilityCenter.
	bool ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition) const;


	UFUNCTION()
	void PerformTrace();

	FGameplayAbilityTargetDataHandle MakeTargetData(const FHitResult& HitResult) const;
};
