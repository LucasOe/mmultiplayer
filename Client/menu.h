#pragma once

#include <string>

typedef void (*MenuTabCallback)();
typedef void (*MenuCallback)(bool show);

typedef struct {
    std::string Name;
    MenuTabCallback Callback;
} MenuTab;

namespace Menu {

void AddTab(const char *name, MenuTabCallback callback);
void Hide();
void Show();
bool Initialize();
void OnVisibilityChange(MenuCallback callback);
} // namespace Menu