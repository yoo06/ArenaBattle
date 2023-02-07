// Fill out your copyright notice in the Description page of Project Settings.


#include "ABItem.h"
#include "ABCharacter.h"
#include "ABWeapon.h"

// Sets default values
AABItem::AABItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("TRIGGER"));
	Box     = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BOX"));
	Effect  = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Effect"));
	
	RootComponent = Trigger;
	Box   ->SetupAttachment(RootComponent);
	Effect->SetupAttachment(RootComponent);

	Trigger->SetBoxExtent(FVector(26.5f, 27.5f, 13.5f));
	Trigger->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SM_BOX
	(TEXT("/Game/InfinityBladeGrassLands/Environments/Breakables/StaticMesh/Box/SM_Env_Breakables_Box1.SM_Env_Breakables_Box1"));
	if (SM_BOX.Succeeded())
		Box->SetStaticMesh(SM_BOX.Object);

	static ConstructorHelpers::FObjectFinder < UParticleSystem> P_CHESTOPEN
	(TEXT("/Game/InfinityBladeGrassLands/Effects/FX_Treasure/Chest/P_TreasureChest_Open_Mesh.P_TreasureChest_Open_Mesh"));
	if (P_CHESTOPEN.Succeeded())
	{
		Effect->SetTemplate(P_CHESTOPEN.Object);
		Effect->bAutoActivate = false;
	}

	Box->SetRelativeLocation(FVector(0.0f, -3.5f, -13.5f));
	Effect->SetRelativeLocation(FVector(0.0f, 0.0f, -45.0f));

	Trigger->SetCollisionProfileName(TEXT("ItemBox"));
	Box    ->SetCollisionProfileName(TEXT("NoCollision"));

	WeaponItemClass = AABWeapon::StaticClass();
}

// Called when the game starts or when spawned
void AABItem::BeginPlay()
{
	Super::BeginPlay();
	
}

void AABItem::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AABItem::OnCharacterOverlap);
}

void AABItem::OnCharacterOverlap(UPrimitiveComponent* OVerlappedComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto ABCharacter = Cast<AABCharacter>(OtherActor);
	if (nullptr != ABCharacter && nullptr != WeaponItemClass)
	{
		if (ABCharacter->CanSetWeapon())
		{
			auto NewWeapon = GetWorld()->SpawnActor<AABWeapon>(WeaponItemClass, FVector::ZeroVector, FRotator::ZeroRotator);
			ABCharacter->SetWeapon(NewWeapon);
			Effect->Activate(true);
			Box->SetHiddenInGame(true, true);
			SetActorEnableCollision(false);
			Effect->OnSystemFinished.AddDynamic(this, &AABItem::OnEffectFinished);
		}
	}
}

void AABItem::OnEffectFinished(UParticleSystemComponent* PSystem)
{
	Destroy();
}

