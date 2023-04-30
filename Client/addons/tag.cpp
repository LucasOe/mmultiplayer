#include "tag.h"
#include "client.h"

#include "../engine.h"
#include "../menu.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

// clang-format off

static float Distance(Classes::FVector from, Classes::FVector to) 
{ 
    return sqrt(((from.X - to.X) * (from.X - to.X)) + ((from.Y - to.Y) * (from.Y - to.Y)) + ((from.Z - to.Z) * (from.Z - to.Z))) / 100;
}

static void TagTab() 
{ 
    // Get a list of all players in the room. Client is not in this list which is why I'm getting the PlayerPawn below
    auto players = Client::GetPlayerList();
    auto pawn = Engine::GetPlayerPawn();

    ImGui::Text("Players: %d", players.size());
    ImGui::Spacing();
   
    for (int i = 0; i < players.size(); i++) 
    {
        // If actor is false, the player's mesh hasn't spawned or are in a different level
        if (!players[i]->Actor)
        {
            ImGui::Text("Failed to get %s actor. Level: %s", players[i]->Name.c_str(), players[i]->Level.c_str());
            continue;
        }

        // Calculate the distance from someone's location to our own location
        float dist = Distance(players[i]->Actor->Location, pawn->Location);
        ImGui::Text("X: %.2f Y: %.2f Z: %.2f (%.2f m) (%s)", players[i]->Actor->Location.X, players[i]->Actor->Location.Y, players[i]->Actor->Location.Z, dist, players[i]->Name.c_str());
    }
}


bool Tag::Initialize() 
{
    Menu::AddTab("Tag", TagTab);
    return true;
}

std::string Tag::GetName() { return "Tag"; }