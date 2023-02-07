// Fill out your copyright notice in the Description page of Project Settings.


#include "ABGameState.h"

AABGameState::AABGameState()
{
	TotalGameScore = 0;
	bGameCleared   = false;
}

void AABGameState::SetGameCleared()
{
	bGameCleared = true;
}

bool AABGameState::IsGameCleared() const
{
	return bGameCleared;
}

int32 AABGameState::GetTotalGameScore() const
{
	return TotalGameScore;
}

void AABGameState::AddGameScore()
{
	TotalGameScore++;
}
