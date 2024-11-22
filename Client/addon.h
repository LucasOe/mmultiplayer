#pragma once

#include <string>

class Addon {
public:
    Addon(const Addon &Other) = default;
    Addon(Addon &&Other) noexcept = default;
    Addon &operator=(const Addon &Other) = default;
    Addon &operator=(Addon &&Other) noexcept = default;
    Addon() = default;
    virtual ~Addon() = default;
    virtual bool Initialize() = 0;
    virtual std::string GetName() = 0;
};