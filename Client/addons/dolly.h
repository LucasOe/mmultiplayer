#pragma once

#include "../addon.h"
#include "../engine.h"
#include <vector>

class Dolly : public Addon {
  public:
    bool Initialize();
    std::string GetName();

    void ForceRoll(bool force);

    ~Dolly();

    class Marker {
      public:
        inline Marker(int frame, float fov, Classes::FVector position,
                      Classes::FVector rotation)
            : Frame{frame}, FOV{fov}, Position{position}, Rotation{rotation} {}

        int Frame;
        float FOV;
        Classes::FVector Position;
        Classes::FVector Rotation;
    };

    class Recording {
      public:
        class Frame {
          public:
            Classes::FVector Position;
            Classes::FRotator Rotation;
            Classes::FBoneAtom Bones[PLAYER_PAWN_BONE_COUNT];
        };

        int StartFrame;
        Engine::Character Character;
        Classes::ASkeletalMeshActorSpawnable *Actor;
        std::vector<Frame> Frames;
    };
};