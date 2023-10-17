#pragma once

#define MMULTIPLAYER_VERSION "2.2.0" // Major, Minor, Patch
#define MMULTIPLAYER_DEBUG 0

#if MMULTIPLAYER_DEBUG
    // Whenever it should use the local address or the server address, no need to change in the client.cpp
    //   1 = Local address
    //   0 = Server address
    #define MMULTIPLAYER_DEBUG_LOCAL 1

    // If set to 1, it will include the demo for the library. The build size will be greatly increased
    #define MMULTIPLAYER_DEBUG_IMGUI 0
    #define MMULTIPLAYER_DEBUG_SPINNERS 0

    // Creates a debug tab with version info and if the demo library macro is true
    #define MMULTIPLAYER_DEBUG_INFO 0

    // Creates a debug window of the game's memory
    #define MMULTIPLAYER_DEBUG_GAMEMEMORY 0
#endif // MMULTIPLAYER_DEBUG

// Instead of having a number, we use a macro to make it more readable
#define TIMETRIAL 0
#define LEVELRACE 1
