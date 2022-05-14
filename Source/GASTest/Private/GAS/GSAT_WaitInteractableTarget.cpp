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

		if (!Hit.HitObjectHandle.IsValid() /* Fixme:   || Hit.Actor != Ability->GetCurrentActorInfo()->AvatarActor.Get()  ��һ����Ϊ���޳�Ability��AvatarActor��
												����ʵ������PerformTrace���Ѿ�ȷ����IgnoreActor�������ƺ������ٴ��ж� */)
		{
			// ���bLookForInteractableActor�Ǽٵģ�����ҪѰ��һ���˵���׷�١�
			if (bLookForInteractableActor && Hit.HitObjectHandle.IsValid())
			{
				// bLookForInteractableActorΪ�棬���е����������COLLISION_INTERACTABLE׷��ͨ���ص���
				// ����Ϊ��������������� big Actor ����һ���ɻ�����С��ť��
				if (Hit.Component.IsValid() && Hit.Component.Get()->GetCollisionResponseToChannel(COLLISION_INTERACTABLE)
					== ECollisionResponse::ECR_Overlap)
				{
					// Component/Actor ������Խ��л���
					bool bIsInteractable = Hit.HitObjectHandle.GetManagingActor()->Implements<UGSInteractabel>();

					if (bIsInteractable && IGSInteractabel::Execute_IsAvailableForInteraction(Hit.HitObjectHandle.GetManagingActor(), Hit.Component.Get()))
					{
						OutHitResult = Hit;
						OutHitResult.bBlockingHit = true; // ������Ϊ a blocking hit
						return;
					}
				}

				OutHitResult.TraceEnd = Hit.Location;
				OutHitResult.bBlockingHit = false; // False ��˼�ǣ���������֮����
				return;
			}


			// ����Ϊ���ڵ�һ�� line trace �л��һ���˵���׷�١�
			// !Hit.Actor.IsValid() ��ζ������û�������κζ��������Է��ض˵���Ϊ a blocking hit��
			// ����������ǻ�������������
			OutHitResult = Hit;
			OutHitResult.bBlockingHit = true; // ������Ϊ a blocking hit
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
	if (DotToCenter >= 0)		//���������ʧ���ˣ����Ǿͻᱻָ��Զ�����ĵĵط��������ǿ����������ڲ��������ҵ�һ���õĳ��ڵ㡣
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
	
	// �������û�л�����Ч�ġ����õĿɽ����Ľ�ɫ����Ĭ��Ϊ�����ߵĽ�����
	// bBlockingHit = ��Ч�ġ����õĿɽ����� Actor
	if (!ReturnHitResult.bBlockingHit)
	{
		// û����Ч�ġ����õĿɽ����� Actor

		ReturnHitResult.Location = TraceEnd;
		if (TargetData.Num() > 0 && TargetData.Get(0)->GetHitResult()->HitObjectHandle.GetManagingActor())
		{
			// ֮ǰ�ĸ�����һ����Ч�Ŀɽ����Ľ�ɫ����������û���ˡ�
			// Broadcast ���һ����Ч��Ŀ��
			LostInteractableTarget.Broadcast(TargetData);
		}

		TargetData = MakeTargetData(ReturnHitResult);
	}
	else
	{
		// ��Ч�ġ����õĿɽ����� Actor

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
				// Broadcast ���һ����Ч��Ŀ��
				LostInteractableTarget.Broadcast(TargetData);
			}
		}

		if (bBroadcastNewTarget)
		{
			// Broadcast �µ���ЧĿ��
			TargetData = MakeTargetData(ReturnHitResult);
			FoundNewInteractableTarget.Broadcast(TargetData);
		}
	}

#if ENABLE_DRAW_DEBUG//ENABLE_DRAW_DEBUG�����ǿ����ڵ���/�����汾�е�����Ⱦ���ݣ������ڷ�������԰汾�е��Ա༭��������һ������������������ǿ��ܵģ���
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
	/* ע�⣺�⽫��FGameplayAbilityTargetDataHandle��ͨ��һ���ڲ���TSharedPtr��������*/
	return StartLocationInfo.MakeTargetDataHandleFromHitResult(Ability, HitResult);
}