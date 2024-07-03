#include "effect.h"
#include <stack>

std::vector<Effect*>& Effects()
{
    static std::vector<Effect*> effects;
    return effects;
}

int Effect::RandomInt(const int min, const int max) const
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

float Effect::RandomFloat(const float min, const float max) const
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

bool Effect::RandomBool(const float probability) const
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng) < ImClamp(probability, 0.0f, 1.0f);
}

Classes::UTdPlayerInput* Effect::GetTdPlayerInput()
{
    const auto controller = Engine::GetPlayerController();
    if (!controller)
    {
        return nullptr;
    }

    if (!controller->PlayerInput)
    {
        return nullptr;
    }

    const auto input = static_cast<Classes::UTdPlayerInput*>(controller->PlayerInput);
    if (!input)
    {
        return nullptr;
    }

    return input;
}

Classes::TArray<Classes::ATdAIController*> Effect::GetTdAIControllers()
{
    const auto world = Engine::GetWorld();
    if (!world)
    {
        return Classes::TArray<Classes::ATdAIController*>();
    }

    const auto gameInfo = static_cast<Classes::ATdSPGame*>(world->Game); 
    if (!gameInfo)
    {
        return Classes::TArray<Classes::ATdAIController*>();
    }

    return gameInfo->AIManager->AIControllers;
}

std::vector<Classes::USequenceObject*> FindSequenceObjects(Classes::USequence* gameSequence, const ImVec2& pos)
{
    if (!gameSequence)
    {
        return std::vector<Classes::USequenceObject*>();
    }

    std::vector<Classes::USequenceObject*> foundSequenceObjects;
    std::stack<Classes::USequence*> sequenceStack;
    sequenceStack.push(gameSequence);

    while (!sequenceStack.empty())
    {
        const auto currentSequence = sequenceStack.top();
        sequenceStack.pop();

        for (size_t i = 0; i < currentSequence->SequenceObjects.Num(); i++)
        {
            const auto sequence = currentSequence->SequenceObjects[i];

            if (sequence->ObjPosX == pos.x && sequence->ObjPosY == pos.y)
            {
                foundSequenceObjects.push_back(sequence);
            }
        }

        for (size_t i = 0; i < currentSequence->NestedSequences.Num(); i++)
        {
            const auto nestedSequence = currentSequence->NestedSequences[i];
            if (nestedSequence)
            {
                sequenceStack.push(nestedSequence);
            }
        }
    }

    return foundSequenceObjects;
}

std::vector<Classes::USequenceObject*> Effect::GetKismetSequenceObjects(const std::string& levelName, const ImVec2& pos) const
{
    const auto world = Engine::GetWorld();
    if (!world)
    {
	    return std::vector<Classes::USequenceObject*>();
    }

    std::vector<Classes::USequenceObject*> sequenceObjects;

    for (size_t i = 0; i < world->StreamingLevels.Num(); i++)
    {
        const auto level = world->StreamingLevels[i];
        if (!level || !level->LoadedLevel)
        {
            continue;
        }

        if (level->PackageName.GetName() != levelName)
        {
            continue;
        }

        sequenceObjects = FindSequenceObjects(level->LoadedLevel->GameSequences[0], pos);
        break;
    }

    // Search in persistent level since some original levels have some kismet in the persistent level
    // and a lot of modded maps uses persistent level for their kismet
    const auto persistentSequenceObjects = FindSequenceObjects(world->GetGameSequence(), pos);

    sequenceObjects.insert(sequenceObjects.end(), persistentSequenceObjects.begin(), persistentSequenceObjects.end());
    return sequenceObjects;
}

std::string Effect::GetLevelName() const
{
    const auto world = Engine::GetWorld();
    if (!world)
    {
        return std::string();
    }

    auto name = world->GetMapName(false);
    if (!name.IsValid())
    {
        return std::string();
    }

    auto levelName = name.ToString();
    std::transform(levelName.begin(), levelName.end(), levelName.begin(), [](char c)
    {
        return std::tolower(c);
    });

    return levelName;
}
