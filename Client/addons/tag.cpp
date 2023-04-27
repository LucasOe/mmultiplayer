#include "tag.h"
#include "../engine.h"
#include "../menu.h"

static void TagTab() {}

bool Tag::Initialize() {
    Menu::AddTab("Tag", TagTab);
    return true;
}

std::string Tag::GetName() { return "Tag"; }