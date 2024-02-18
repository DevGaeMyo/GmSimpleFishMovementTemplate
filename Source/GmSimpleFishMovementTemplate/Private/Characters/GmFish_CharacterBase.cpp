// Copyright Dev.GaeMyo 2024. All Rights Reserved.


#include "Characters/GmFish_CharacterBase.h"

#include <EnhancedInputComponent.h>
#include <EnhancedInputSubsystems.h>
#include <Camera/CameraComponent.h>
#include <Components/CapsuleComponent.h>
#include <Components/SplineMeshComponent.h>
#include <Components/StaticMeshComponent.h>
#include <GameFramework/CharacterMovementComponent.h>
#include <GameFramework/SpringArmComponent.h>
#include <NavAreas/NavArea_Obstacle.h>
#include <Engine/World.h>
#include <Engine/EngineTypes.h>
#include <TimerManager.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/Controller.h>
#include <Engine/LocalPlayer.h>

// FName AGmFish_CharacterBase::FishSpringArmName(TEXT("SpringArm"));
// FName AGmFish_CharacterBase::FishCameraName(TEXT("Camera"));

AGmFish_CharacterBase::AGmFish_CharacterBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	if (UCapsuleComponent* CurrentCapsuleComp{GetCapsuleComponent()})
	{
		CurrentCapsuleComp->SetAreaClassOverride(UNavArea_Obstacle::StaticClass());
	}
	
	SM_CenterOfMass = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SM_CenterOfMass"));
	SM_CenterOfMass->SetMobility(EComponentMobility::Movable);
	SM_CenterOfMass->SetupAttachment(GetRootComponent());

	FishBody = CreateDefaultSubobject<USplineMeshComponent>(TEXT("FishBody"));
	FishBody->SetupAttachment(SM_CenterOfMass);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraLagSpeed = 4.f;
	SpringArm->CameraRotationLagSpeed = 0.7f;
	SpringArm->SetupAttachment(GetRootComponent());
	
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	
	Camera->SetupAttachment(SpringArm);

	UCharacterMovementComponent* CurrentMovementComponent{GetCharacterMovement()};
	CurrentMovementComponent->MaxAcceleration = 300.f;
	CurrentMovementComponent->GroundFriction = 1.f;
	CurrentMovementComponent->MaxWalkSpeed = 500.f;
	CurrentMovementComponent->BrakingDecelerationWalking = 5.f;
	CurrentMovementComponent->JumpZVelocity = 650.f;
	CurrentMovementComponent->RotationRate = FRotator(0.f, 200.f, 0.f);
}

void AGmFish_CharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// Reset Fish
	ForwardInputAndTurnRate = FVector2D::ZeroVector;
	MoveRate = FVector::ZeroVector;
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	
	if (const APlayerController* CurrentPC{Cast<APlayerController>(GetController())})
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(CurrentPC->GetLocalPlayer())})
		{
			Subsystem->AddMappingContext(IMC_SimpleFish, 0);
		}
	}
	
	FTimerHandle Th;
	GetWorld()->GetTimerManager().SetTimer(Th, FTimerDelegate::CreateLambda([this]()->void
	{
		FishStartLocation = SM_CenterOfMass->GetComponentLocation();
	}), 1.f, false);
	FishSwimFloatEventHandler.BindUFunction(this, "UpdateFishSwimTimeline");
	TL_FishSwim.AddInterpFloat(SwimTemplateFCurve, FishSwimFloatEventHandler);

	TL_FishSwim.SetLooping(true);
	TL_FishSwim.SetTimelineLength(4);
	TL_FishSwim.Play();
}

void AGmFish_CharacterBase::Move2D(const FInputActionValue&)
{
	// UE_LOG(LogTemp, Error, L"Move2D");
	// const FRotator CurrentControlRot{GetControlRotation()};
	
	AddMovementInput(MoveRate, (FishMovementInputValue.Y < 0 ? 0.25f : 1.f) * ForwardInputAndTurnRate.X, false);
}

void AGmFish_CharacterBase::PhysicalMovementOfFish(const float InDeltaSeconds)
{
	TL_FishSwim.TickTimeline(InDeltaSeconds);
	AddActorLocalRotation(FRotator(0.f, ForwardInputAndTurnRate.Y * InDeltaSeconds * 60.f, 0.f));

	const FVector CenterOfMassForwardVec{SM_CenterOfMass->GetForwardVector()};
	EndLocation = GmVInterpTo(
		EndLocation,
		FVector(FRotator(0.f, SwimRotation * 2.f, 0.f).RotateVector(CenterOfMassForwardVec) * -150.f),
		InDeltaSeconds, 4.f);

	EndTangent = GmVInterpTo(EndTangent,
		FRotationMatrix::MakeFromX(EndLocation - CenterOfMassForwardVec * -80.f).Rotator().Vector(),
		InDeltaSeconds,
		10.f);

	FishBody->SetStartAndEnd(
		FVector::ZeroVector,
		SM_CenterOfMass->GetComponentRotation().Vector() * -150.0,
		EndLocation,
		EndTangent * 200.0,
		true
		);

	FishBody->SetStartRoll(ForwardInputAndTurnRate.Y * -0.16f, true);

	SM_CenterOfMass->SetRelativeRotation(
		FRotator(GmFInterpTo(
			SM_CenterOfMass->GetRelativeRotation().Pitch,
			FMath::Clamp(GetVelocity().Z * 0.25f, -80.f, 80.f),
			InDeltaSeconds,
		15.f
		),
			ForwardInputAndTurnRate.Y * 5.f + /*Swim rotation + turn rate determines yaw*/SwimRotation,
			ForwardInputAndTurnRate.Y * 7.f)
		);
	// UE_LOG(LogTemp, Error, L"Current Velocity is %f", GetVelocity().Size());

	if (InputComponent)
	{
		if (UEnhancedInputComponent* EnhancedInputComponent{CastChecked<UEnhancedInputComponent>(InputComponent)})
		{
			FishMovementInputValue = EnhancedInputComponent->BindActionValue(IA_Move).GetValue().Get<FVector2D>();
		}
	}

	// Move Forward and Backward.
	ForwardInputAndTurnRate.X = GmFInterpTo(ForwardInputAndTurnRate.X, FishMovementInputValue.Y, /*DeltaSeconds*/ InDeltaSeconds, 5.f);

	/* Set MoveRate */
	MoveRate = GmVInterpTo(MoveRate, /*GetForwardVector()*/ GetActorRotation().Vector(), InDeltaSeconds, 15.f);

	// Move Right and Left.
	ForwardInputAndTurnRate.Y = GmFInterpTo(ForwardInputAndTurnRate.Y, FishMovementInputValue.X * 6.f, /*DeltaSeconds*/ InDeltaSeconds, 2.f);
}

void AGmFish_CharacterBase::UpdateFishSwimTimeline(const float InSwimFloat)
{
	const double NewPlayRate{FMath::Clamp(
		FMath::GetMappedRangeValueUnclamped(FVector2D(0.f, 450.f), FVector2D(8.f, 0.f), SM_CenterOfMass->GetComponentRotation().UnrotateVector(GetVelocity()).X)
		+
		FMath::GetMappedRangeValueUnclamped(FVector2D(0.f, 6.f), FVector2D(5.f, -4.f), FMath::Abs(ForwardInputAndTurnRate.Y)),
		2.f,
		12.f)};
	SwimRotation = ForwardInputAndTurnRate.X * InSwimFloat * (NewPlayRate* 1.7f);
	TL_FishSwim.SetPlayRate(NewPlayRate);
	// UE_LOG(LogTemp, Error, L"Current Play Rate is %f.", TL_FishSwim.GetPlayRate());
}

void AGmFish_CharacterBase::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	PhysicalMovementOfFish(DeltaTime);

	// UE_LOG(LogTemp, Error, L"Forward Input is %f. TurnRate is %f.", ForwardInputAndTurnRate.X, ForwardInputAndTurnRate.Y);
}

void AGmFish_CharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent{CastChecked<UEnhancedInputComponent>(PlayerInputComponent)})
	{
		EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ThisClass::Move2D);
		// EnhancedInputComponent->axis
	}
}

FVector AGmFish_CharacterBase::GmVInterpTo(const FVector& OutCurrent, const FVector& OutTarget, const float InDT,
	const float InterpSpeed)
{
	// If no interp speed, jump to target value
	if(InterpSpeed <= 0.f)
	{
		return OutTarget;
	}

	// Distance to reach
	const FVector Dist{OutTarget - OutCurrent};

	// If distance is too small, just set the desired location
	if(Dist.SizeSquared() < 1.e-4f)
	{
		return OutTarget;
	}

	// Delta Move, Clamp so we do not over shoot.
	const FVector DeltaMove{Dist * FMath::Clamp<float>(InDT * InterpSpeed, 0.f, 1.f)};

	return OutCurrent + DeltaMove;
}
