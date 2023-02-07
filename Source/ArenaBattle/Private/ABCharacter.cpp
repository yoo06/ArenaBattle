// Fill out your copyright notice in the Description page of Project Settings.


#include "ABCharacter.h"
#include "ABAnimInstance.h"
#include "DrawDebugHelpers.h"
#include "ABWeapon.h"
#include "ABCharacterStatComponent.h"
#include "Components/WidgetComponent.h"
#include "ABCharacterWidget.h"
#include "ABAIController.h"
#include "ABGameInstance.h"
#include "ABCharacterSetting.h"
#include "Engine/AssetManager.h"
#include "ABPlayerController.h"
#include "ABSection.h"
#include "ABPlayerState.h"
#include "ABHUDWidget.h"
#include "ABGameMode.h"

// Sets default values
AABCharacter::AABCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArm	  = CreateDefaultSubobject<USpringArmComponent>(TEXT("SPRINGARM"));
	Camera		  = CreateDefaultSubobject<UCameraComponent>(TEXT("CAMERA"));
	CharacterStat = CreateDefaultSubobject<UABCharacterStatComponent>(TEXT("CHARACTERSTAT"));
	HPBarWidget   = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBARWIDGET"));

	SpringArm  ->SetupAttachment(GetCapsuleComponent());
	Camera     ->SetupAttachment(SpringArm);
	HPBarWidget->SetupAttachment(GetMesh());


	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -88.0f), FRotator(0.0f, -90.0f, 0.0f));
	SpringArm->TargetArmLength = 400.0f;
	SpringArm->SetRelativeRotation(FRotator(-15.0f, 0.0f, 0.0f));

	HPBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	HPBarWidget->SetWidgetSpace(EWidgetSpace::Screen);

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("ABCharacter"));
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	static ConstructorHelpers::FClassFinder<UAnimInstance> WARRIOR_ANIM
	(TEXT("/Game/InfinityBladeWarriors/Character/Animations/WarriorAnimBlueprint.WarriorAnimBlueprint_C"));
	
	if (WARRIOR_ANIM.Succeeded())
		GetMesh()->SetAnimInstanceClass(WARRIOR_ANIM.Class);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SK_CARDBOARD
	(TEXT("/Game/InfinityBladeWarriors/Character/CompleteCharacters/SK_CharM_Cardboard.SK_CharM_Cardboard"));

	if (SK_CARDBOARD.Succeeded())
		GetMesh()->SetSkeletalMesh(SK_CARDBOARD.Object);

	static ConstructorHelpers::FClassFinder<UUserWidget> UI_HUD
	(TEXT("/Game/Book/UI/UI_HPBar.UI_HPBar_C"));

	if (UI_HUD.Succeeded())
	{
		HPBarWidget->SetWidgetClass(UI_HUD.Class);
		HPBarWidget->SetDrawSize(FVector2D(150.0f, 50.0f));
	}

	SetControlMode(EControlMode::TPS);

	ArmLengthSpeed   = 3.0f;
	ArmRotationSpeed = 10.0f;
	IsAttacking      = false;
	MaxCombo         = 4;
	AttackRange      = 80.0f;
	AttackRadius     = 50.0f;

	AttackEndComboState();
	GetCharacterMovement()->JumpZVelocity = 600.0f;

	AIControllerClass = AABAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;


	AssetIndex = 4;

	SetActorHiddenInGame(true);
	HPBarWidget->SetHiddenInGame(true);
	SetCanBeDamaged(false);
	
	DeadTimer = 3.0f;
}

void AABCharacter::SetCharacterState(ECharacterState NewState)
{
	CurrentState = NewState;
	switch (CurrentState)
	{
	case ECharacterState::LOADING:
	{
		if (bIsPlayer)
		{
			DisableInput(ABPlayerController);

			ABPlayerController->GetHUDWidget()->BindCharacterStat(CharacterStat);

			auto ABPlayerState = Cast<AABPlayerState>(GetPlayerState());
			CharacterStat->SetNewLevel(ABPlayerState->GetCharacterLevel());
		}
		else
		{
			auto ABGameMode   = Cast<AABGameMode>(GetWorld()->GetAuthGameMode());
			int32 TargetLevel = FMath::CeilToInt(((float)ABGameMode->GetScore() * 0.8f));
			int32 FinalLevel  = FMath::Clamp<int32>(TargetLevel, 1, 20);
			CharacterStat->SetNewLevel(FinalLevel);
		}
		SetActorHiddenInGame(true);
		HPBarWidget->SetHiddenInGame(true);
		SetCanBeDamaged(false);
		break;
	}


	case ECharacterState::READY:
	{
		SetActorHiddenInGame(false);
		HPBarWidget->SetHiddenInGame(false);
		SetCanBeDamaged(true);

		CharacterStat->OnHPIsZero.AddLambda([this]() -> void { SetCharacterState(ECharacterState::DEAD);	});

		auto CharacterWidget = Cast<UABCharacterWidget>(HPBarWidget->GetUserWidgetObject());
		CharacterWidget->BindCharacterStat(CharacterStat);

		if (bIsPlayer)
		{
			SetControlMode(EControlMode::QUARTERVIEW);
			GetCharacterMovement()->MaxWalkSpeed = 600.0f;
			EnableInput(ABPlayerController);
		}
		else
		{
			SetControlMode(EControlMode::NPC);
			GetCharacterMovement()->MaxWalkSpeed = 300.0f;
			ABAIController->RunAI();
		}

		break;
	}


	case ECharacterState::DEAD:
	{
		SetActorEnableCollision(false);
		GetMesh()->SetHiddenInGame(false);
		HPBarWidget->SetHiddenInGame(true);
		ABAnim->SetDeadAnim();
		SetCanBeDamaged(false);

		if (bIsPlayer)
			DisableInput(ABPlayerController);
		else
			ABAIController->StopAI();


		GetWorld()->GetTimerManager().SetTimer(DeadTimerHandle, FTimerDelegate::CreateLambda([this]() ->void
		{
			if (bIsPlayer)
			ABPlayerController->ShowResultUI();
			else
				Destroy();
		}), DeadTimer, false);

		break;
	}
	}
}

ECharacterState AABCharacter::GetCharacterState() const
{
	return CurrentState;
}

int32 AABCharacter::GetExp() const
{
	return CharacterStat->GetDropExp();
}

float AABCharacter::GetFinalAttackRange() const
{
	return (nullptr != CurrentWeapon) ? CurrentWeapon->GetAttackRange() : AttackRange;
}

float AABCharacter::GetFinalAttackDamage() const
{
	float AttackDamage = (nullptr != CurrentWeapon) ? 
		(CharacterStat->GetAttack() + CurrentWeapon->GetAttackDamage()) : (CharacterStat->GetAttack());

	float AttackModifier = (nullptr != CurrentWeapon) ? CurrentWeapon->GetAttackModifier() : 1.0f;

	return AttackDamage * AttackModifier;
}

// Called when the game starts or when spawned
void AABCharacter::BeginPlay()
{
	Super::BeginPlay();

	bIsPlayer = IsPlayerControlled();
	if (bIsPlayer)
		ABPlayerController = Cast<AABPlayerController>(GetController());
	else
		ABAIController = Cast<AABAIController>(GetController());

	auto DefaultSetting = GetDefault<UABCharacterSetting>();

	if (bIsPlayer)
	{
		auto ABPlayerState = Cast<AABPlayerState>(GetPlayerState());
		AssetIndex = ABPlayerState->GetCharacterIndex();
	}
	else
		AssetIndex = FMath::RandRange(0, DefaultSetting->CharacterAssets.Num() - 1);

	CharacterAssetToLoad = DefaultSetting->CharacterAssets[AssetIndex];
	AssetStreamingHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad
	(CharacterAssetToLoad, FStreamableDelegate::CreateUObject(this, &AABCharacter::OnAssetLoadCompleted));
	SetCharacterState(ECharacterState::LOADING);

}

void AABCharacter::SetControlMode(EControlMode NewControlMode)
{
	CurrentControlMode = NewControlMode;
	switch (CurrentControlMode)
	{
	case EControlMode::TPS:
		//bUsePawnControlRotation와 bUseControllerRotationYaw 가 모두 true 라면, 카메라 방향으로 움직이게 됨
		//bUseControllerRotationYaw가 false 라면 카메라 방향과 상관없이 움직임
		
		//SpringArm->TargetArmLength         = 450.0f;
		//SpringArm->SetRelativeRotation(FRotator::ZeroRotator);
		ArmlengthTo						   = 450.0f;
		SpringArm->bUsePawnControlRotation = true;
		SpringArm->bInheritPitch		   = true;
		SpringArm->bInheritRoll			   = true;
		SpringArm->bInheritYaw			   = true;
		SpringArm->bDoCollisionTest		   = true;
		bUseControllerRotationYaw		   = false;

		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

		break;

	case EControlMode::QUARTERVIEW:
		//SpringArm->TargetArmLength		   = 800.0f;
		//SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
		ArmlengthTo						   = 800.0f;
		ArmRotationTo					   = FRotator(-45.0f, 0.0f, 0.0f);
		SpringArm->bUsePawnControlRotation = false;
		SpringArm->bInheritPitch		   = false;
		SpringArm->bInheritRoll			   = false;
		SpringArm->bInheritYaw			   = false;
		SpringArm->bDoCollisionTest		   = false;
		bUseControllerRotationYaw		   = false;

		GetCharacterMovement()->bOrientRotationToMovement = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

		break;

	case EControlMode::NPC:
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bUseControllerDesiredRotation = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);

		break;
	}
}

// Called every frame
void AABCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, ArmlengthTo, DeltaTime, ArmLengthSpeed);

	switch (CurrentControlMode)
	{
	case EControlMode::QUARTERVIEW:
		SpringArm->SetRelativeRotation(FMath::RInterpTo(SpringArm->GetRelativeRotation(), ArmRotationTo, DeltaTime, ArmRotationSpeed));
		if (DirectionToMove.SizeSquared() > 0.0f)
		{
			GetController()->SetControlRotation(FRotationMatrix::MakeFromX(DirectionToMove).Rotator());
			AddMovementInput(DirectionToMove);
		}
		break;
	}
}

void AABCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ABAnim = Cast<UABAnimInstance>(GetMesh()->GetAnimInstance());

	ABAnim->OnMontageEnded.AddDynamic(this, &AABCharacter::OnAttackMontageEnded);
	ABAnim->OnAttackHitCheck.AddUObject(this, &AABCharacter::AttackCheck);

	ABAnim->OnNextAttackCheck.AddLambda([this]() -> void
	{
		CanNextCombo = false;

		if (IsComboInputOn)
		{
			AttackStartComboState();
			ABAnim->JumpToAttackMontageSection(CurrentCombo);
		}
	});

	CharacterStat->OnHPIsZero.AddLambda([this]() -> void
	{
			ABAnim->SetDeadAnim();
			SetActorEnableCollision(false);
	});


}

float AABCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	CharacterStat->SetDamage(FinalDamage);
	if (CurrentState == ECharacterState::DEAD)
	{
		if (EventInstigator->IsPlayerController())
		{
			auto TempABPlayerController = Cast<AABPlayerController>(EventInstigator);
			TempABPlayerController->NPCKill(this);
		}
	}

	return FinalDamage;
}

// Called to bind functionality to input
void AABCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction(TEXT("ViewChange"), EInputEvent::IE_Pressed, this, &AABCharacter::ViewChange);
	PlayerInputComponent->BindAction(TEXT("Jump"),       EInputEvent::IE_Pressed, this, &AABCharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Attack"),     EInputEvent::IE_Pressed, this, &AABCharacter::Attack);

	PlayerInputComponent->BindAxis(TEXT("UpDown"),    this, &AABCharacter::UpDown);
	PlayerInputComponent->BindAxis(TEXT("LeftRight"), this, &AABCharacter::LeftRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"),    this, &AABCharacter::LookUp);
	PlayerInputComponent->BindAxis(TEXT("Turn"),      this, &AABCharacter::Turn);

}

bool AABCharacter::CanSetWeapon()
{
	return true;
}

void AABCharacter::SetWeapon(AABWeapon* NewWeapon)
{

	if (nullptr != CurrentWeapon)
	{
		CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FName WeaponSocket(TEXT("hand_rSocket"));
	if (nullptr != NewWeapon)
	{
		NewWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
		NewWeapon->SetOwner(this);
		CurrentWeapon = NewWeapon;
	}
}

void AABCharacter::UpDown(float NewAxisValue)
{
	if (!IsAttacking)
	{
		switch (CurrentControlMode)
		{
		case EControlMode::TPS:
			AddMovementInput(FRotationMatrix(FRotator(0.0f, GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X), NewAxisValue);
			break;
		case EControlMode::QUARTERVIEW:
			DirectionToMove.X = NewAxisValue;
			break;
		}
	}
}

void AABCharacter::LeftRight(float NewAxisValue)
{
	if (!IsAttacking)
	{
		switch (CurrentControlMode)
		{
		case EControlMode::TPS:
			AddMovementInput(FRotationMatrix(FRotator(0.0f, GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::Y), NewAxisValue);
			break;
		case EControlMode::QUARTERVIEW:
			DirectionToMove.Y = NewAxisValue;
			break;
		}
	}
}

void AABCharacter::ViewChange()
{
	switch (CurrentControlMode)
	{
	case EControlMode::TPS:
		GetController()->SetControlRotation(GetActorRotation());
		SetControlMode(EControlMode::QUARTERVIEW);
		break;
	case EControlMode::QUARTERVIEW:
		GetController()->SetControlRotation(SpringArm->GetRelativeRotation());
		SetControlMode(EControlMode::TPS);
		break;
	}
}

void AABCharacter::OnAssetLoadCompleted()
{
	USkeletalMesh* AssetLoaded = Cast<USkeletalMesh>(AssetStreamingHandle->GetLoadedAsset());
	AssetStreamingHandle.Reset();
	GetMesh()->SetSkeletalMesh(AssetLoaded);

	SetCharacterState(ECharacterState::READY);
}

void AABCharacter::Attack()
{
	DirectionToMove = FVector::ZeroVector;
	if (IsAttacking)
	{
		if (CanNextCombo)
		{
			IsComboInputOn = true;
		}
	}
	else
	{
		AttackStartComboState();
		ABAnim->PlayAttackMontage();
		ABAnim->JumpToAttackMontageSection(CurrentCombo);
		IsAttacking = true;
	}
}

void AABCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	IsAttacking = false;
	AttackEndComboState();
	OnAttackEnd.Broadcast();
}

void AABCharacter::AttackStartComboState()
{
	CanNextCombo   = true;
	IsComboInputOn = false;
	CurrentCombo   = FMath::Clamp<int32>(CurrentCombo + 1, 1, MaxCombo);
}

void AABCharacter::AttackEndComboState()
{
	IsComboInputOn = false;
	CanNextCombo   = false;
	CurrentCombo   = 0;
}

void AABCharacter::AttackCheck()
{
	FHitResult HitResult;
	FCollisionQueryParams Params(NAME_None, false, this);
	bool bResult = GetWorld()->SweepSingleByChannel
	(
		HitResult,
		GetActorLocation(),
		GetActorLocation() + GetActorForwardVector() * GetFinalAttackRange(),
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel2,
		FCollisionShape::MakeSphere(AttackRadius),
		Params
	);

#if ENABLE_DRAW_DEBUG
	
	FVector TraceVec    = GetActorForwardVector() * GetFinalAttackRange();
	FVector Center      = GetActorLocation() + TraceVec * 0.5f;
	float HalfHeight    = GetFinalAttackRange() * 0.5f + AttackRadius;
	FQuat CapsuleRot    = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
	FColor DrawColor    = bResult ? FColor::Green : FColor::Red;
	float DebugLifeTime = 3.0f;

	DrawDebugCapsule
	(
		GetWorld(),
		Center,
		HalfHeight,
		AttackRadius,
		CapsuleRot,
		DrawColor,
		false,
		DebugLifeTime
	);

#endif

	if (bResult)
	{
		if (HitResult.GetActor()->IsValidLowLevel())
		{
			UGameplayStatics::ApplyDamage(HitResult.GetActor(), GetFinalAttackDamage(), GetController(), this, UDamageType::StaticClass());
		}
	}
}

void AABCharacter::Turn(float NewAxisValue)
{
	switch (CurrentControlMode)
	{
	case EControlMode::TPS:
		AddControllerYawInput(NewAxisValue);
		break;
	}
}

void AABCharacter::LookUp(float NewAxisValue)
{
	switch (CurrentControlMode)
	{
	case EControlMode::TPS:
		AddControllerPitchInput(NewAxisValue);
		break;
	}
}

void AABCharacter::Jump()
{
	if (!IsAttacking)
		ACharacter::Jump();
}
