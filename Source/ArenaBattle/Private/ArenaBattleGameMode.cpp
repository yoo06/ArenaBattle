// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArenaBattleGameMode.h"
#include "ArenaBattleCharacter.h"
#include "UObject/ConstructorHelpers.h"

AArenaBattleGameMode::AArenaBattleGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
