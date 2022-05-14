// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GSAT_WaitInteractableTarget.h"
#include "../../GASTest.h"
#include "GAS/GSInteractabel.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"


UGSAT_WaitInteractableTarget::UGSAT_WaitInteractableTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

UGSAT_WaitInteractableTarget* UGSAT_WaitInteractableTarget::WaitForInteractableTarget(UGameplayAbility* OwningAbility, FName TaskInstanceName, FCollisionProfileName TraceProfile, float MaxRange, float TimerPeriod, bool bShowDebug)
{

}


void UGSAT_WaitInteractableTarget::Activate()
{
	GetWorld()->GetTimerManager().SetTimer(TraceTimerHandle, this, &UGSAT_WaitInteractableTarget::PerformTrace, TimerPeriod, true);
}

void UGSAT_WaitInteractableTarget::OnDestroy(bool AbilityEnded)
{
	GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);

	Super::OnDestroy(AbilityEnded);
}

void UGSAT_WaitInteractableTarget::LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params, bool bLookForInteractableActor) const
{
	check(World);

	TArray<FHitResult> HitResults;
	World->LineTraceMultiByProfile(HitResults, Start, End, ProfileName, Params);
	//World->LineTraceSingleByProfile(HitResults, Start, End, ProfileName, Params);
	OutHitResult.TraceStart = Start;
	OutHitResult.TraceEnd = End;

	for (int32 HitIdx = 0; HitIdx < HitResults.Num(); ++HitIdx)
	{
		const FHitResult& Hit = HitResults[HitIdx];

		if (!Hit.HitObjectHandle.IsValid() /* Fixme:   || Hit.Actor != Ability->GetCurrentActorInfo()->AvatarActor.Get()  这一句是为了剔除Ability的AvatarActor，
												但是实际上在PerformTrace内已经确定了IgnoreActor，这里似乎不用再次判断 */)
		{
			// 如果bLookForInteractableActor是假的，我们要寻找一个端点来追踪。
			if (bLookForInteractableActor && Hit.HitObjectHandle.IsValid())
			{
				// bLookForInteractableActor为真，击中的组件必须与COLLISION_INTERACTABLE追踪通道重叠。
				// 这是为了让像电脑这样的 big Actor 能有一个可互动的小按钮。
				if (Hit.Component.IsValid() && Hit.Component.Get()->GetCollisionResponseToChannel(COLLISION_INTERACTABLE)
					== ECollisionResponse::ECR_Overlap)
				{
					// Component/Actor 必须可以进行互动
					bool bIsInteractable = Hit.HitObjectHandle.GetManagingActor()->Implements<UGSInteractabel>();

					if (bIsInteractable && IGSInteractabel::Execute_IsAvailableForInteraction(Hit.HitObjectHandle.GetManagingActor(), Hit.Component.Get()))
					{
						OutHitResult = Hit;
						OutHitResult.bBlockingHit = true; // 将其视为 a blocking hit
						return;
					}
				}

				OutHitResult.TraceEnd = Hit.Location;
				OutHitResult.bBlockingHit = false; // False 意思是，它不能与之交互
				return;
			}


			// 这是为了在第一次 line trace 中获得一个端点来追踪。
			// !Hit.Actor.IsValid() 意味着我们没有碰到任何东西，所以返回端点作为 a blocking hit。
			// 或者如果我们击中了其他东西
			OutHitResult = Hit;
			OutHitResult.bBlockingHit = true; // 将其视为 a blocking hit
			return;
		}
	}
}


void UGSAT_WaitInteractableTarget::AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, FVector& OutTraceEnd, bool bIgnorePitch) const
{
	if (!Ability) // Server and launching client only
	{
		return;
	}
	APlayerController* playContrloller = Ability->GetCurrentActorInfo()->PlayerController.Get();

	// Default to TraceStart if no PlayerController
	FVector ViewStart = TraceStart;
	FRotator ViewRot(0.0f);
	if (playContrloller)
	{
		playContrloller->GetPlayerViewPoint(ViewStart, ViewRot);
	}

	const FVector ViewDir = ViewRot.Vector();
	FVector ViewEnd = ViewStart + (ViewDir * MaxRange);

	ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd);

	FHitResult HitResult;
	LineTrace(HitResult, InSourceActor->GetWorld(), ViewStart, ViewEnd, TraceProfile.Name, Params, false);


}

bool UGSAT_WaitInteractableTarget::ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition) const
{
	FVector CameraToCenter = AbilityCenter - CameraLocation;
	float DotToCenter = FVector::DotProduct(CameraToCenter, CameraDirection);
	if (DotToCenter >= 0)		//如果这样做失败了，我们就会被指向远离中心的地方，但我们可能在球体内部，并能找到一个好的出口点。
	{
		float DistanceSquared = CameraToCenter.SizeSquared() - (DotToCenter * DotToCenter);
		float RadiusSquared = (AbilityRange * AbilityRange);
		if (DistanceSquared <= RadiusSquared)
		{
			float DistanceFromCamera = FMath::Sqrt(RadiusSquared - DistanceSquared);
			float DistanceAlongRay = DotToCenter + DistanceFromCamera;						
			ClippedPosition = CameraLocation + (DistanceAlongRay * CameraDirection);		
			return true;
		}
	}
	return false;
}

void UGSAT_WaitInteractableTarget::PerformTrace() 
{
	bool bTraceComplex = false;

	TArray<AActor*> ActorsToIgnore;

	AActor* SourceActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	if (!SourceActor)
	{
		// Hero is dead
		//UE_LOG(LogTemp, Error, TEXT("%s %s SourceActor was null"), *FString(__FUNCTION__), *UGSBlueprintFunctionLibrary::GetPlayerEditorWindowRole(GetWorld()));
		return;
	}
	else 
	
	ActorsToIgnore.Emplace(SourceActor);
	

	FCollisionQueryParams param(SCENE_QUERY_STAT(AGameplayAbilityTargetActor_SingleLineTrace), bTraceComplex);

	param.bReturnPhysicalMaterial = true;
	param.AddIgnoredActors(ActorsToIgnore);


	// Calculate TraceEnd
	FVector TraceStart = StartLocationInfo.GetTargetingTransform().GetLocation();
	FVector TraceEnd;
	AimWithPlayerController(SourceActor, param, TraceStart, TraceEnd); //Effective on server and launching client only

	// ------------------------------------------------------

	FHitResult ReturnHitResult;
	LineTrace(ReturnHitResult, GetWorld(), TraceStart, TraceEnd, TraceProfile.Name, param, true);
	
	// 如果我们没有击中有效的、可用的可交互的角色，则默认为跟踪线的结束。
	// bBlockingHit = 有效的、可用的可交互的 Actor
	if (!ReturnHitResult.bBlockingHit)
	{
		// 没有有效的、可用的可交互的 Actor

		ReturnHitResult.Location = TraceEnd;
		if (TargetData.Num() > 0 && TargetData.Get(0)->GetHitResult()->HitObjectHandle.GetManagingActor())
		{
			// 之前的跟踪有一个有效的可交互的角色，现在我们没有了。
			// Broadcast 最后一个有效的目标
			LostInteractableTarget.Broadcast(TargetData);
		}

		TargetData = MakeTargetData(ReturnHitResult);
	}
	else
	{
		// 有效的、可用的可交互的 Actor

		bool bBroadcastNewTarget = true;

		if (TargetData.Num() > 0)
		{
			const AActor* OldTarget = TargetData.Get(0)->GetHitResult()->HitObjectHandle.GetManagingActor();

			if (OldTarget == ReturnHitResult.HitObjectHandle.GetManagingActor())
			{
				// Old target is the same as the new target, don't broadcast the target
				bBroadcastNewTarget = false;
			}
			else if (OldTarget)
			{
				// Old target exists and is different from the new target
				// Broadcast 最后一个有效的目标
				LostInteractableTarget.Broadcast(TargetData);
			}
		}

		if (bBroadcastNewTarget)
		{
			// Broadcast 新的有效目标
			TargetData = MakeTargetData(ReturnHitResult);
			FoundNewInteractableTarget.Broadcast(TargetData);
		}
	}

#if ENABLE_DRAW_DEBUG//ENABLE_DRAW_DEBUG：我们可以在调试/开发版本中调试渲染数据，或者在发货或测试版本中调试编辑器（不是一个常见的情况，但它是可能的）。
	if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, TimerPeriod);

		if (ReturnHitResult.bBlockingHit)
		{
			DrawDebugSphere(GetWorld(), ReturnHitResult.Location, 20.0f, 16, FColor::Red, false, TimerPeriod);
		}
		else
		{
			DrawDebugSphere(GetWorld(), ReturnHitResult.TraceEnd, 20.0f, 16, FColor::Green, false, TimerPeriod);
		}
	}
#endif // ENABLE_DRAW_DEBUG

}


FGameplayAbilityTargetDataHandle UGSAT_WaitInteractableTarget::MakeTargetData(const FHitResult& HitResult) const
{
	/* 注意：这将由FGameplayAbilityTargetDataHandle（通过一个内部的TSharedPtr）来清理*/
	return StartLocationInfo.MakeTargetDataHandleFromHitResult(Ability, HitResult);
}