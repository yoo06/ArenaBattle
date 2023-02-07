// Fill out your copyright notice in the Description page of Project Settings.


#include "ABCharacterSelectWidget.h"
#include "ABCharacterSetting.h"
#include "Engine/Assetmanager.h"
#include "EngineUtils.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "ABSaveGame.h"
#include "ABPlayerState.h"

void UABCharacterSelectWidget::NextCharacter(bool bForward)
{
	bForward ? CurrentIndex++ : CurrentIndex--;

	if (CurrentIndex == -1)       CurrentIndex = MaxIndex - 1;
	if (CurrentIndex == MaxIndex) CurrentIndex = 0;

	auto CharacterSetting = GetDefault<UABCharacterSetting>();
	auto AssetRef = CharacterSetting->CharacterAssets[CurrentIndex];

	
	USkeletalMesh* Asset = UAssetManager::GetStreamableManager().LoadSynchronous<USkeletalMesh>(AssetRef);
	if (nullptr != Asset)
		TargetComponent->SetSkeletalMesh(Asset);
}

void UABCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentIndex = 0;
	auto CharacterSetting = GetDefault<UABCharacterSetting>();
	MaxIndex = CharacterSetting->CharacterAssets.Num();

	for (TActorIterator<ASkeletalMeshActor> It(GetWorld()); It; ++It)
	{
		TargetComponent = It->GetSkeletalMeshComponent();
		break;
	}

	PrevButton	  = Cast<UButton>(GetWidgetFromName(TEXT("btnPrev")));
	NextButton	  = Cast<UButton>(GetWidgetFromName(TEXT("btnNext")));
	ConfirmButton = Cast<UButton>(GetWidgetFromName(TEXT("btnConfirm")));
	TextBox		  = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("edtPlayerName")));

	PrevButton   ->OnClicked.AddDynamic(this, &UABCharacterSelectWidget::OnPrevClicked);
	NextButton   ->OnClicked.AddDynamic(this, &UABCharacterSelectWidget::OnNextClicked);
	ConfirmButton->OnClicked.AddDynamic(this, &UABCharacterSelectWidget::OnConfirmClicked);
}

void UABCharacterSelectWidget::OnPrevClicked()
{
	NextCharacter(false);
}

void UABCharacterSelectWidget::OnNextClicked()
{
	NextCharacter(true);
}

void UABCharacterSelectWidget::OnConfirmClicked()
{
	FString CharacterName = TextBox->GetText().ToString();
	if (CharacterName.Len() <= 0 || CharacterName.Len() > 10) return;

	UABSaveGame* NewPlayerData	  = NewObject<UABSaveGame>();
	NewPlayerData->PlayerName	  = CharacterName;
	NewPlayerData->Level		  = 1;
	NewPlayerData->Exp			  = 0;
	NewPlayerData->HighScore	  = 0;
	NewPlayerData->CharacterIndex = CurrentIndex;

	auto ABPlayerState = GetDefault<AABPlayerState>();
	if (UGameplayStatics::SaveGameToSlot(NewPlayerData, ABPlayerState->SaveSlotName, 0))
		UGameplayStatics::OpenLevel(GetWorld(), TEXT("Gameplay"));
	
}