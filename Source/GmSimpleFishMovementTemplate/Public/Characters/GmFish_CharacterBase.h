// Copyright Dev.GaeMyo 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/TimelineComponent.h"

#include "GmFish_CharacterBase.generated.h"

class UInputMappingContext;
class UInputAction;
class USplineMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UCurveFloat;

UCLASS()
class GMSIMPLEFISHMOVEMENTTEMPLATE_API AGmFish_CharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	
	AGmFish_CharacterBase(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Components)
	TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Components)
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Components)
	TObjectPtr<UStaticMeshComponent> SM_CenterOfMass;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = Components)
	TObjectPtr<USplineMeshComponent> FishBody;

	UPROPERTY(EditDefaultsOnly, Category = InitSettings)
	TObjectPtr<UCurveFloat> SwimTemplateFCurve;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_SimpleFish;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

protected:
	
	virtual void BeginPlay() override;

	UFUNCTION()
	void Move2D(const FInputActionValue& InValue);
	
	UFUNCTION()
	void PhysicalMovementOfFish(const float InDeltaSeconds);

	UFUNCTION()
	void UpdateFishSwimTimeline(float InSwimFloat);

	// static FName FishSpringArmName;
	// static FName FishCameraName;

	FVector FishStartLocation{FVector::ZeroVector};
	
	FVector2D FishMovementInputValue{FVector2D::ZeroVector};
	
	FVector2D ForwardInputAndTurnRate{FVector2D::ZeroVector};
	FVector MoveRate{FVector::ZeroVector};
	FVector EndLocation{FVector::ZeroVector};
	FVector EndTangent{FVector::ZeroVector};
	float SwimRotation;
	FTimeline TL_FishSwim;
	

public:
	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Interpolate float from Current to Target. Scaled by distance to Target, so it has a strong start speed and ease out. */
	template<typename T1, typename T2 = T1, typename T3 = T2, typename T4 = T3>
	UE_NODISCARD static auto GmFInterpTo(T1  Current, T2 Target, T3 InDT, T4 InterpSpeed)
	{
		using RetType = decltype(T1() * T2() * T3() * T4());
		
		// If no interp speed, jump to target value
		if(InterpSpeed <= 0.f)
		{
			return static_cast<RetType>(Target);
		}

		// Distance to reach
		const RetType Dist{Target - Current};

		// If distance is too small, just set the desired location
		if( FMath::Square(Dist) < 1.e-8f )
		{
			return static_cast<RetType>(Target);
		}

		// Delta Move, Clamp so we do not over shoot.
		const RetType DeltaMove{Dist * FMath::Clamp<RetType>(InDT * InterpSpeed, 0.f, 1.f)};

		return Current + DeltaMove;				
	}

	/** Interpolate vector from Current to Target. Scaled by distance to Target, so it has a strong start speed and ease out. */
	UE_NODISCARD static FVector GmVInterpTo(const FVector& OutCurrent, const FVector& OutTarget, float InDT, float InterpSpeed);

private:

	FOnTimelineFloat FishSwimFloatEventHandler;
	bool bPressedForwardInput;
	bool bPressedRightInput;
};
