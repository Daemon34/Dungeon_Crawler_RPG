// Copyright Clément Lecoeur for learning purpose


#include "Player/AuraPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Input/AuraInputComponent.h"
#include <Interaction/EnemyInterface.h>
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Components/SplineComponent.h"
#include "AuraGameplayTags.h"
#include "Runtime/NavigationSystem/Public/NavigationSystem.h"
#include "Runtime/NavigationSystem/Public/NavigationPath.h"

AAuraPlayerController::AAuraPlayerController()
{
	bReplicates = true;

	Spline = CreateDefaultSubobject<USplineComponent>("Spline");
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	CursorTrace();

	if (bAutoRunning) {
		AutoRun();
	}
}

void AAuraPlayerController::AutoRun()
{
	if (APawn* ControlledPawn = GetPawn()) {
		const FVector LocationOnSpline = Spline->FindLocationClosestToWorldLocation(ControlledPawn->GetActorLocation(), ESplineCoordinateSpace::World);
		const FVector Direction = Spline->FindDirectionClosestToWorldLocation(LocationOnSpline, ESplineCoordinateSpace::World);
		ControlledPawn->AddMovementInput(Direction);

		const float DistanceToDestination = (LocationOnSpline - CachedDestination).Length();
		if (DistanceToDestination <= AutoRunAcceptanceRadius) {
			bAutoRunning = false;
		}
	}
}


void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	check(AuraContext);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem) {
		Subsystem->AddMappingContext(AuraContext, 0);
	}

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UAuraInputComponent* AuraInputComponent = CastChecked<UAuraInputComponent>(InputComponent);
	AuraInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Move);
	AuraInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
}

void AAuraPlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>()) {
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}

void AAuraPlayerController::CursorTrace()
{
	FHitResult CursorHit;
	GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
	if (CursorHit.bBlockingHit) {
		LastActor = CurrentActor;
		CurrentActor = Cast<IEnemyInterface>(CursorHit.GetActor());
		/**
		* Line trace from curstor. There are several scenarios :
		*  1. LastActor is null && CurrentActor is null
		*		- Do nothing
		*  2. LastActor is null && CurrentActor is valid
		*		- Highlight CurrentActor
		*  3. Last Actor is valid && CurrentActor is null
		*		- UnHilight LastActor
		*  4. Last Actor is valid && CurrentActor is valid
		*		4.1 LastActor != CurrentActor
		*			- UnHilight LastActor && highlight CurrentActor
		*		4.2 LastActor == CurrentActor
		*			- Do nothing
		*/
		if (LastActor == nullptr) {
			if (CurrentActor != nullptr) {
				// Case 2
				CurrentActor->HighlightActor();
			} // Else, case 1 => Do nothing
		}
		else {
			// Last Actor is valid
			if (CurrentActor == nullptr) {
				// Case 3
				LastActor->UnHighlightActor();
			}
			else {
				// Both Actor are valid
				if (LastActor != CurrentActor) {
					// Case 4.1
					LastActor->UnHighlightActor();
					CurrentActor->HighlightActor();
				} // Else, case 4.2 => Do nothing
			}
		}
	}
}

void AAuraPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (InputTag.MatchesTag(FAuraGameplayTags::Get().InputTag_RMB)) {
		bTargeting = CurrentActor ? true : false;
		bAutoRunning = false;
	}
}

void AAuraPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	// If we're not releasing Right Mouse Button, activate ability
	if (!InputTag.MatchesTag(FAuraGameplayTags::Get().InputTag_RMB)) {
		if (GetAbilitySystemComponent() != nullptr) {
			GetAbilitySystemComponent()->AbilityInputTagReleased(InputTag);
		}
	}
	else {
		// If we're releasing RMB, check if we're targeting an enemy, in which case activate ability
		if (bTargeting) {
			if (GetAbilitySystemComponent() != nullptr) {
				GetAbilitySystemComponent()->AbilityInputTagReleased(InputTag);
			}
		}
		else {
			APawn* ControlledPawn = GetPawn();
			if (FollowTime <= ShortPressThreshold && ControlledPawn) {
				UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, ControlledPawn->GetActorLocation(), CachedDestination);
				if (NavPath) {
					Spline->ClearSplinePoints();
					for (const FVector& PointLocation : NavPath->PathPoints) {
						Spline->AddSplinePoint(PointLocation, ESplineCoordinateSpace::World);
						DrawDebugSphere(GetWorld(), PointLocation, 8.0f, 8, FColor::Green, false, 5.0f);
					}
					CachedDestination = NavPath->PathPoints[NavPath->PathPoints.Num() - 1];
					bAutoRunning = true;
				}
			}
			FollowTime = 0.0f;
			bTargeting = false;
		}
	}
}

void AAuraPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
	if (!InputTag.MatchesTag(FAuraGameplayTags::Get().InputTag_RMB)) {
		if (GetAbilitySystemComponent() != nullptr) {
			GetAbilitySystemComponent()->AbilityInputTagHeld(InputTag);
		}
	}
	else {
		if (bTargeting) {
			if (GetAbilitySystemComponent() != nullptr) {
				GetAbilitySystemComponent()->AbilityInputTagHeld(InputTag);
			}
		}
		else {
			FollowTime += GetWorld()->GetDeltaSeconds();
			FHitResult Hit;
			if (GetHitResultUnderCursor(ECC_Visibility, false, Hit)) {
				CachedDestination = Hit.ImpactPoint;
			}

			if (APawn* ControlledPawn = GetPawn()) {
				const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
				ControlledPawn->AddMovementInput(WorldDirection);
			}
		}
	}
}

UAuraAbilitySystemComponent* AAuraPlayerController::GetAbilitySystemComponent()
{
	if (AuraAbilitySystemComponent == nullptr) {
		AuraAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
	}
	return AuraAbilitySystemComponent;
}