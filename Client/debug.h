#pragma once

namespace Debug 
{
    static bool IsConsoleOpened = false;

    void Initialize();
    void ToggleConsole();
    void CreateConsole();
    void CloseConsole();
} // namespace Debug