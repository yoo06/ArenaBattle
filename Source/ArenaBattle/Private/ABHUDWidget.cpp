// Fill out your copyright notice in the Description page of Project Settings.


#include "ABHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "ABCharacterStatComponent.h"
#include "ABPlayerState.h"


void UABHUDWidget::BindCharacterStat(UABCharacterStatComponent* CharacterStat)
{
	CurrentCharacterStat = CharacterStat;
	CharacterStat->OnHPChanged.AddUObject(this, &UABHUDWidget::UpdateCharacterStat);
}

void UABHUDWidget::BindPlayerState(AABPlayerState* PlayerState)
{
	CurrentPlayerState = PlayerState;
	PlayerState->OnPlayerStateChanged.AddUObject(this, &UABHUDWidget::UpdatePlayerState);
}

void UABHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HPBar		 = Cast<UProgressBar>(GetWidgetFromName(TEXT("pbHP")));
	ExpBar		 = Cast<UProgressBar>(GetWidgetFromName(TEXT("pbExp")));
	PlayerName   = Cast<UTextBlock>  (GetWidgetFromName(TEXT("txtPlayerName")));
	PlayerLevel  = Cast<UTextBlock>  (GetWidgetFromName(TEXT("txtLevel")));
	CurrentScore = Cast<UTextBlock>  (GetWidgetFromName(TEXT("txtCurrentScore")));
	HighScore    = Cast<UTextBlock>  (GetWidgetFromName(TEXT("txtHighScore")));
}

void UABHUDWidget::UpdateCharacterStat()
{
		HPBar->SetPercent(CurrentCharacterStat->GetHPRatio());

}

void UABHUDWidget::UpdatePlayerState()
{
	ExpBar      ->SetPercent(CurrentPlayerState->GetExpRatio());
	PlayerName  ->SetText(FText::FromString(CurrentPlayerState->GetPlayerName()));
	PlayerLevel ->SetText(FText::FromString(FString::FromInt(CurrentPlayerState->GetCharacterLevel())));
	CurrentScore->SetText(FText::FromString(FString::FromInt(CurrentPlayerState->GetGameScore())));
	HighScore   ->SetText(FText::FromString(FString::FromInt(CurrentPlayerState->GetGameHighScore())));
}
