#include <codecvt>
#include <locale>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include <WS2tcpip.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#include "../engine.h"
#include "../imgui/imgui.h"
#include "../json.h"
#include "../menu.h"
#include "../settings.h"

#include "client.h"
#include "../util.h"

static char RoomInput[0xFF] = {0};
static char NameInput[0xFF] = {0};
static char ChatInput[0x200] = {0};

static bool IsConnected = false;
static bool IsLoading = false;
static bool IsMultiplayerDisabled = false;
static std::string Room;

static bool ShowTagDistanceOverlay = false;
static bool ShowTagCooldownOverlay = true;
static bool PlayerDiedAndSentJsonMessage = false;
static int TagCooldown = 5;
static ULONGLONG TaggedTimed = 0;
static int PreviousTaggedId = 0;

static sockaddr_in Server = {0};
static SOCKET TCPSocket = 0;
static SOCKET UDPSocket = 0;

static struct 
{
    bool Focused = false;
    bool ShowOverlay = true;
    int Keybind = 0;
    std::string Raw;
    unsigned long long LastTime = 0ULL;
    std::mutex Mutex;
} Chat;

static Client::Player UserClient = {0};

static struct 
{
    bool ShowNameTags = true;
    std::vector<Client::Player *> List;
    std::shared_mutex Mutex;
} Players;

static Client::Player *GetPlayerById(unsigned int id) 
{
    for (const auto &p : Players.List) 
    {
        if (p->Id == id) 
        {
            return p;
        }
    }

    return nullptr;
}

std::vector<Client::Player *> Client::GetPlayerList() 
{ 
    return Players.List; 
}

static void IgnorePlayerInput(bool ignoreInput) 
{
    const auto controller = Engine::GetPlayerController();

    if (!controller) 
    {
        return;
    }

    controller->bIgnoreMoveInput = ignoreInput ? 1 : 0;
    controller->bIgnoreButtonInput = ignoreInput ? 1 : 0;
    controller->bIgnoreMovementFocus = ignoreInput;
}

static bool Setup() 
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) 
    {
        printf("client: WSAStartup failed\n");
        return false;
    }

    addrinfo *result = nullptr;
    if (getaddrinfo("176.58.101.83", nullptr, nullptr, &result)) 
    {
        printf("client: getaddrinfo failed\n");
        return false;
    }

    IN_ADDR serverAddr = {0};
    for (; result; result = result->ai_next) 
    {
        if (result->ai_family == AF_INET) 
        {
            serverAddr = reinterpret_cast<SOCKADDR_IN *>(result->ai_addr)->sin_addr;
        }
    }

    freeaddrinfo(result);
    if (!serverAddr.S_un.S_addr) 
    {
        printf("client: found no server address\n");
        return false;
    }

    Server = {0};
    Server.sin_family = AF_INET;
    Server.sin_port = htons(Client::Port);
    Server.sin_addr = serverAddr;

    return true;
}

static bool RecvJsonMessage(json &msg) 
{
    static char buffer[0xFFF] = {0};
    static char *nextMessage = nullptr;

    for (;;) 
    {
        if (nextMessage && *nextMessage) 
        {
            const auto message = nextMessage;
            nextMessage += strlen(message) + 1;

            try 
            {
                msg = json::parse(message);
                return true;
            } 
            catch (...) 
            {
                printf("client: failed parse -> %s\n", message);

                nextMessage = nullptr;
                return false;
            }
        }

        nextMessage = nullptr;
        memset(buffer, 0, sizeof(buffer));

        if (recv(TCPSocket, buffer, sizeof(buffer), 0) <= 0) 
        {
            return false;
        }

        nextMessage = buffer;
    }
}

static bool SendJsonMessage(json msg) 
{
    static std::mutex sendMutex;

    const auto data = msg.dump();

    sendMutex.lock();
    if (send(TCPSocket, data.c_str(), data.length(), 0) != data.length()) 
    {
        sendMutex.unlock();
        return false;
    }

    sendMutex.unlock();
    return true;
}

static void AddChatMessage(std::string message) 
{
    static const auto maxMessages = 100;

    SYSTEMTIME time;
    GetLocalTime(&time);

    char formattedTime[0xFF];
    sprintf_s(formattedTime, sizeof(formattedTime), "%d:%02d: ", time.wHour, time.wMinute);

    const auto formattedMsg = formattedTime + message + "\n";

    Chat.Mutex.lock();

    Chat.Raw += formattedMsg;
    Chat.LastTime = GetTickCount64();

    if (std::count(Chat.Raw.begin(), Chat.Raw.end(), '\n') > maxMessages) 
    {
        Chat.Raw.erase(0, Chat.Raw.find('\n') + 1);
    }

    Chat.Mutex.unlock();
}

static void SendChatInput() 
{
    if (IsConnected) 
    {
        for (auto c = &ChatInput[0]; *c; ++c) 
        {
            if (!isblank(*c)) 
            {
                SendJsonMessage({
                    {"type", "chat"},
                    {"id", UserClient.Id},
                    {"body", ChatInput},
                });

                break;
            }
        }
    }

    ChatInput[0] = 0;
}

static void Disconnect() 
{
    if (IsConnected) 
    {
        SendJsonMessage({
            {"type", "disconnect"},
            {"id", UserClient.Id},
        });

        AddChatMessage("Disconnected");
    }

    if (TCPSocket) 
    {
        shutdown(TCPSocket, SD_BOTH);
        closesocket(TCPSocket);
        TCPSocket = 0;
    }

    if (UDPSocket) 
    {
        shutdown(UDPSocket, SD_BOTH);
        closesocket(UDPSocket);
        UDPSocket = 0;
    }

    Players.Mutex.lock();
    for (const auto &p : Players.List) 
    {
        if (p->Actor) 
        {
            Engine::Despawn(p->Actor);
        }

        delete p;
    }

    Players.List.clear();
    Players.List.shrink_to_fit();
    Players.Mutex.unlock();

    IsConnected = false;
}

static void PlayerHandler() 
{
    while (IsConnected) 
    {
        Client::PACKET_COMPRESSED packet;

        int serverSize = sizeof(Server);
        if (recvfrom(UDPSocket, reinterpret_cast<char *>(&packet), sizeof(packet), 0, reinterpret_cast<sockaddr *>(&Server), &serverSize) < 0) 
        {
            continue;
        }

        Players.Mutex.lock_shared();

        const auto player = GetPlayerById(packet.Id);
        if (player) 
        {
            memcpy(&player->LastPacket, &packet, FIELD_OFFSET(Client::PACKET_COMPRESSED, CompressedBones));

            const auto bonesBase = reinterpret_cast<byte *>(player->LastPacket.Bones);
            for (auto i = 0; i < ARRAYSIZE(CompressedBoneOffsets); ++i) 
            {
                *reinterpret_cast<float *>(bonesBase + CompressedBoneOffsets[i]) = static_cast<float>(packet.CompressedBones[i]) / 215.f;
            }
        }

        Players.Mutex.unlock_shared();
    }
}

static bool Join() 
{
    if (UserClient.Level == "") 
    {
        const auto world = Engine::GetWorld();
        if (!world) 
        {
            printf("client: no world available\n");
            return false;
        }

        UserClient.Level = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(world->GetMapName(false).c_str());

        std::transform(UserClient.Level.begin(), UserClient.Level.end(), UserClient.Level.begin(), [](char c) 
        { 
            return tolower(c); 
        });

        if (UserClient.Level == Map_MainMenu) 
        {
            IsLoading = true;
        }
    }

    if (!SendJsonMessage({
            {"type", "connect"},
            {"room", Room},
            {"name", UserClient.Name},
            {"level", UserClient.Level},
            {"character", UserClient.Character},
        })) 
    {
        printf("client: failed to send connect msg\n");
        return false;
    }

    json msg;
    if (!RecvJsonMessage(msg)) 
    {
        printf("client: failed to receive connect\n");
        return false;
    }

    const auto msgType = msg["type"];
    const auto msgId = msg["id"];
    const auto msgGameMode = msg["gameMode"];
    const auto msgTaggedPlayerId = msg["taggedPlayerId"];
    const auto msgCanTag = msg["canTag"];

    if (!msgType.is_string() || msgType != "id" || !msgId.is_number_integer() || !msgGameMode.is_string() || !msgTaggedPlayerId.is_number_integer() || !msgCanTag.is_boolean()) 
    {
        printf("client: malformed connect response\n");
        return false;
    }

    UserClient.Id = msgId;
    UserClient.GameMode = msgGameMode.get<std::string>();
    UserClient.TaggedPlayerId = PreviousTaggedId = msgTaggedPlayerId;
    UserClient.CanTag = msgCanTag;

    printf("client: joined with id %x\n", UserClient.Id);
    return true;
}

static void ClientListener() 
{
    for (;; Disconnect(), Sleep(500)) 
    {
        if (IsMultiplayerDisabled) 
        {
            continue;
        }

        printf("client: connecting\n");

        if (!Setup()) 
        {
            continue;
        }

        TCPSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (TCPSocket < 0) 
        {
            TCPSocket = 0;

            printf("client: failed to create tcp socket\n");
            continue;
        }

        UDPSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (UDPSocket < 0) 
        {
            UDPSocket = 0;

            printf("client: failed to create udp socket\n");
            continue;
        }

        if (connect(TCPSocket, reinterpret_cast<const sockaddr *>(&Server), sizeof(Server))) 
        {
            printf("client: failed to connect\n");
            continue;
        }

        if (!Join()) 
        {
            continue;
        }

        IsConnected = true;
        AddChatMessage("Connected");

        std::thread playerHandlerThread(PlayerHandler);

        auto lastPing = GetTickCount64();
        std::thread statusThread([&lastPing]() 
        {
            while (IsConnected) 
            {
                Sleep(500);

                if (GetTickCount64() - lastPing > 5000) 
                {
                    printf("client: timed out\n");
                    Disconnect();
                    return;
                }
            }
        });

        json msg;
        while (RecvJsonMessage(msg)) 
        {
            auto msgType = msg["type"];
            if (!msgType.is_string()) 
            {
                continue;
            }

            if (msgType == "connect") 
            {
                const auto msgId = msg["id"];
                const auto msgName = msg["name"];
                const auto msgCharacter = msg["character"];
                const auto msgLevel = msg["level"];

                if (!msgId.is_number_integer() || !msgName.is_string() || !msgCharacter.is_number_integer() || !msgLevel.is_string()) 
                {
                    continue;
                }

                Players.Mutex.lock();

                const auto player = new Client::Player();
                player->Id = msgId;
                player->Name = msgName.get<std::string>();
                player->Character = msgCharacter;
                player->Level = msgLevel.get<std::string>();
                player->LastPacket = {0};

                static const unsigned long defaultBones[] = {
                    0x0,        0x0,        0x0,        0x3f800000, 0x0,
                    0x80000000, 0x0,        0x3f800000, 0xbe8605c3, 0x3e813707,
                    0xbf24045a, 0x3f2d1e27, 0x3d6d4fd8, 0xc2d86b2f, 0x3f7b81d2,
                    0x3f7fffff, 0x3d4f3d5f, 0x3d4b4f09, 0xbd6731d4, 0x3f7ef26f,
                    0x40ad8500, 0xbe38fe00, 0xbd30e280, 0x3f7fffff, 0x3d6d25b8,
                    0x3c50a631, 0xbbcb5123, 0x3f7f8b7b, 0x4176d5a8, 0xbcc36c00,
                    0x3bc3b800, 0x3f7fffff, 0x3b56e31b, 0x3c1d44cc, 0x3c9b6f10,
                    0x3f7ff0d5, 0x416df1c0, 0x3a218000, 0xbc4c6c00, 0x3f7fffff,
                    0x3c344e05, 0xbeaa3619, 0x3d038638, 0x3f71487c, 0x413aca60,
                    0x0,        0x0,        0x3f800000, 0x3e4ff5a3, 0x3dd3f4fe,
                    0xbe113006, 0x3f769aab, 0x40ee5180, 0x0,        0x0,
                    0x3f7fffff, 0xbef44c3d, 0x3f059855, 0xbf059855, 0xbef44c44,
                    0x3fdec8e6, 0x38cfd167, 0x4053a616, 0x3f800000, 0x0,
                    0x80000000, 0x3d1ee107, 0xbf7fcead, 0x40183689, 0x3f932ccd,
                    0xa73c3627, 0x3f800000, 0x0,        0x80000000, 0x0,
                    0xbf7fffff, 0x4019fca4, 0xbf065204, 0xa6d0b2f0, 0x3f800000,
                    0xbdca5ca3, 0xbf2b69a6, 0x3da5375f, 0xbf3b50ed, 0x40ebb7d3,
                    0x3f2c634a, 0x3da798ec, 0x3f800000, 0xbd3c39e0, 0xbf346eaf,
                    0x3d7908a2, 0xbf348daa, 0x40e06ab0, 0x3f451e0d, 0xbfaf8ce4,
                    0x3f800000, 0xbd627d92, 0xbf34770a, 0x3d627d92, 0xbf34770a,
                    0x40dbf9e7, 0x40429764, 0x2824eaa5, 0x3f800000, 0x3f346eaf,
                    0xbd3c39e0, 0xbf348da9, 0xbd79096b, 0x40e06a5d, 0x3f4504ff,
                    0x3faf8d1c, 0x3f800000, 0xbcedb9ef, 0xbcedb9ef, 0xbf34dde7,
                    0xbf34dde9, 0x40f9968e, 0xa96d0eaf, 0x40f84d50, 0x3f800000,
                    0xba53b730, 0xbc798018, 0x0,        0xbf7ff860, 0x40491fe2,
                    0x3d2e3000, 0x3e51f880, 0x3f800000, 0x39a8334d, 0xbb468806,
                    0x3ab56d4b, 0xbf7fffa1, 0x4045f6c0, 0xbe705ebb, 0x3fc2c718,
                    0x3f800000, 0xb7faebf0, 0xbb944618, 0xbb1f532a, 0xbf7fff22,
                    0x403f32ed, 0xb9d4fc15, 0xbb162336, 0x3f800000, 0xbaa24902,
                    0xbac6c3e9, 0x376865f3, 0xbf7fffdf, 0xb1f42200, 0x34a2c140,
                    0xb322c140, 0x3f800000, 0x3f7ffa3c, 0x0,        0x3c476a39,
                    0xbbac98de, 0xc0491fde, 0x3d2e4106, 0x3e51f94f, 0x3f800000,
                    0xbf7ffe13, 0x3ae63dcc, 0xb7faebf4, 0xbbf4385f, 0xc03ed4ca,
                    0xb9e01b3a, 0xb69dcd8f, 0x3f800000, 0xbf7ffe15, 0xba93e861,
                    0x0,        0xbbf82c92, 0xc045d174, 0xbe6e1474, 0x3fc2cac1,
                    0x3f800000, 0xbcedb9ef, 0xbcedb9ef, 0xbf34dde7, 0xbf34dde9,
                    0x411a1f57, 0x4084ad09, 0x412109a8, 0x3f800000, 0xbd0cd319,
                    0xbd0cd319, 0xbf34cd79, 0xbf34cecf, 0x410d1ea7, 0x3f1c3052,
                    0x412a1350, 0x3f800000, 0xbcedb9ef, 0xbcedb9ef, 0xbf34dde7,
                    0xbf34dde9, 0x3fd93a61, 0x3d04bdba, 0x412e9e84, 0x3f800000,
                    0xbce68df9, 0xbd079441, 0xbf3464f3, 0xbf354d25, 0x3f50f1e8,
                    0x400bd200, 0x41191f24, 0x3f800000, 0xbcedb9ef, 0xbcedb9ef,
                    0xbf34dde7, 0xbf34dde9, 0x40ab40f2, 0x408b19e5, 0x4120911b,
                    0x3f800000, 0xbccb5e78, 0xbd02db18, 0xbf34ec4a, 0xbf34d1b9,
                    0x4031478e, 0x40050827, 0x41252b37, 0x3f800000, 0xbced58cd,
                    0xbceeef46, 0xbf348168, 0xbf3539f1, 0x3fbaec25, 0x3fc0e126,
                    0x412648e9, 0x3f800000, 0xbcedb9ef, 0xbcedb9ef, 0xbf34dde7,
                    0xbf34dde9, 0x407b6e40, 0xbd02f406, 0x4143ae0a, 0x3f800000,
                    0xbced8cde, 0xbcedb13b, 0xbf34deb4, 0xbf34dd2e, 0x4003b5d5,
                    0x4093c217, 0x41097a2f, 0x3f800000, 0xbdb90a4b, 0xbf34d3de,
                    0xbf338915, 0xbd054524, 0x41813d01, 0x40da93b4, 0x402d2cd5,
                    0x3f800000, 0x0,        0xbe503700, 0x0,        0xbf7aa6e7,
                    0x41429ba7, 0xa8d00000, 0x3904d5cd, 0x3f800000, 0x0,
                    0x3f2836bb, 0x0,        0xbf40fa1c, 0x408a6fd6, 0x28500000,
                    0xbdfd83ab, 0x3f800000, 0x3f338915, 0x3d05413b, 0xbdb90a4b,
                    0xbf34d3e1, 0x41813d01, 0xc0da93b4, 0x402d2cd5, 0x3f800000,
                    0x0,        0xbe503700, 0x0,        0xbf7aa6e7, 0xc1429ba7,
                    0x286f5d3f, 0xb904d5cd, 0x3f800000, 0x0,        0x3f2836bb,
                    0x0,        0xbf40fa1c, 0xc08a6fd6, 0xa67ae9f6, 0x3dfd83ab,
                    0x3f800000, 0x3f34deb5, 0x3f34dd2c, 0xbced8cdf, 0xbcedb15c,
                    0x4003b0f0, 0xc093c13f, 0x41097a45, 0x3f800000, 0x3f34ed30,
                    0x3f34d0d2, 0xbccb5f87, 0xbd02db33, 0x40314ef4, 0xc005079a,
                    0x41252af7, 0x3f800000, 0x3f34caae, 0x3f34caae, 0xbd0fa646,
                    0xbd12bc15, 0x410d077a, 0xbf1ba6c1, 0x412a29ba, 0x3f800000,
                    0x3f3464f3, 0x3f354d23, 0xbce68df9, 0xbd07964e, 0x3f50eef6,
                    0xc00bd0b0, 0x41191f02, 0x3f800000, 0x3f34dde7, 0x3f34dde7,
                    0xbcedb9ef, 0xbcedbf6c, 0x407b7519, 0x3d02f406, 0x4143add4,
                    0x3f800000, 0x3f34dde7, 0x3f34dde7, 0xbcedb9ef, 0xbcedbf6c,
                    0x411a6ffe, 0xc0849306, 0x4120d2a1, 0x3f800000, 0x3f34dde7,
                    0x3f34dde7, 0xbcedb9ef, 0xbcedbf6c, 0x40ab40ee, 0xc08b19e3,
                    0x412090f6, 0x3f800000, 0x3f34816a, 0x3f3539f0, 0xbced58d0,
                    0xbceeef96, 0x3fbae0ba, 0xbfc0e143, 0x41264926, 0x3f800000,
                    0xbe1938d4, 0xbd07bf75, 0x3f39aa5e, 0x3f2bd417, 0x40a17f68,
                    0x404a9f91, 0x3f29f5cc, 0x3f7fffff, 0xbd2c8e15, 0x3d0b98ae,
                    0x3f22505c, 0x3f457a80, 0x4153afac, 0x3f517700, 0xc00b3680,
                    0x3f7fffff, 0x3cefb5d3, 0x3e60295c, 0xbbd71cc6, 0xbf79ac40,
                    0x41c49739, 0xa91668f7, 0xa6b99ea0, 0x3f7fffff, 0x3eb77684,
                    0x3ab8a283, 0x3d64c7af, 0xbf6e9283, 0x41ce7127, 0xb1cea788,
                    0x34f4e59b, 0x3f7fffff, 0xbcd48a5b, 0x3a2c8fcc, 0x3c1a1235,
                    0xbf7fe706, 0x2960c564, 0xa5777e2b, 0x28c7ca73, 0x3f7fffff,
                    0x3ca2de97, 0x3d837a86, 0x3b92c526, 0xbf7f6b2c, 0x3f4cc3af,
                    0xbe15e7bc, 0x3e1cd9b4, 0x3f7fffff, 0xbd87334a, 0xbd95525e,
                    0xbe9101a3, 0xbf74394d, 0x4103e95b, 0x279b7e36, 0xa6abc62e,
                    0x3f7fffff, 0x0,        0xbcd097ca, 0xbe9b4d2f, 0xbf73da27,
                    0x40860f23, 0x27dffcfd, 0x28577026, 0x3f7fffff, 0x0,
                    0xbcd64b7d, 0xbe9f8bf5, 0xbf732943, 0x40276d9c, 0xa98f2007,
                    0x2a257039, 0x3f7fffff, 0x3c3c011a, 0xbca70299, 0xbbf1c595,
                    0xbf7fec48, 0x3f519d23, 0xbdfb9e8a, 0xbe6f1b9f, 0x3f7fffff,
                    0xbd2ca327, 0x3d567487, 0xbed0636a, 0xbf6933e6, 0x40fe0834,
                    0xa65a5ec5, 0x25fe205e, 0x3f7fffff, 0x0,        0xbd09fe13,
                    0xbecbaaee, 0xbf6ab73c, 0x4086c8f3, 0xa78983de, 0x2843cf75,
                    0x3f7fffff, 0x0,        0xbcb51efb, 0xbe85a947, 0xbf770ed5,
                    0x3ff3aab2, 0xa90cf2b4, 0x29c3e539, 0x3f7fffff, 0xbc31aad3,
                    0xbda1afc3, 0xbd55a785, 0xbf7ed611, 0x3f5c6a28, 0xbead3b06,
                    0xbf4a6ecb, 0x3f7fffff, 0xbc5cd8a4, 0x3d7c5dfb, 0xbf05997b,
                    0xbf59c6b5, 0x40fbdd91, 0xa88e8676, 0xa86b14e1, 0x3f7fffff,
                    0x0,        0xbcfd5a0e, 0xbeb9e44b, 0xbf6e664e, 0x405667c5,
                    0xa7d099d0, 0x281578c5, 0x3f7fffff, 0x0,        0xbcbcb0ff,
                    0xbe8a72d1, 0xbf766476, 0x3fb51c88, 0xa789f7f7, 0x29ac9b0e,
                    0x3f7fffff, 0x3d4d6f76, 0x3df509a7, 0x3d358a95, 0xbf7d9530,
                    0x3f527f05, 0x3d330f25, 0x3f3a15d9, 0x3f7fffff, 0xbda5ab59,
                    0xbdda1918, 0xbe87cac5, 0xbf747248, 0x41032140, 0x27a61d5c,
                    0x282b64c1, 0x3f7fffff, 0x0,        0xbc24341e, 0xbdf53051,
                    0xbf7e2553, 0x4082d2ca, 0xa84b044d, 0xa80d6134, 0x3f7fffff,
                    0x0,        0xbcb0d45c, 0xbe84055a, 0xbf774809, 0x401734de,
                    0x29e6fed8, 0xaa01d153, 0x3f7fffff, 0x3f190049, 0x3e860e26,
                    0xbbc627d7, 0xbf41fd13, 0x3f7c2fda, 0x3f6495ec, 0x40004305,
                    0x3f7fffff, 0x3d5d49d2, 0x3db285c9, 0xbd035d35, 0xbf7e8492,
                    0x408d7e6b, 0x28c8a59d, 0xa9552f76, 0x3f7fffff, 0x0,
                    0x80000000, 0xbd8a96fd, 0xbf7f69c6, 0x407c81e7, 0x29984d45,
                    0xa9bf88c8, 0x3f7fffff, 0x3da34e58, 0x0,        0x0,
                    0xbf7f2f51, 0x414e7127, 0xa69b22f2, 0x28bbecf1, 0x3f7fffff,
                    0x3d2350a1, 0xbb6762cf, 0x3b469adb, 0x3f7fcb2d, 0x0,
                    0x0,        0xb7000000, 0x3f7fffff, 0x3f393865, 0x3f2cd425,
                    0x3e02fe7b, 0x3d8794a6, 0x40a18142, 0xc04a9f94, 0x3f29f9a7,
                    0x3f7fffff, 0xbe596946, 0x3a2e15b4, 0x3f0b7695, 0x3f4fae8f,
                    0xc153afc0, 0xbf516800, 0x400b3670, 0x3f7fffff, 0xbd1dc0a9,
                    0xbeae7ab7, 0x3d3d253d, 0x3f702f11, 0xc1c49838, 0xb9f20000,
                    0x37000000, 0x3f7fffff, 0xbe2ef847, 0xbcae27a9, 0xbd857e01,
                    0x3f7b9fb0, 0xc1ce7060, 0x39ce0000, 0xb9bca000, 0x3f7fffff,
                    0x3cdf815d, 0xbc547158, 0xbc38585f, 0x3f7fddf1, 0xbcc6a000,
                    0xbd2dec00, 0x3e6326c0, 0x3f7fffff, 0x3c85cd1c, 0x3d057f0e,
                    0x3d82cd80, 0xbf7f4e8c, 0xbf4ce3ac, 0x3e15466a, 0xbe1c6b4b,
                    0x3f7fffff, 0xbce04975, 0x3afb0dff, 0xbe72b68c, 0xbf789b0e,
                    0xc103e7be, 0x3a122d9e, 0xb9aa2e1f, 0x3f7fffff, 0x0,
                    0x80000000, 0xbefac829, 0xbf5f3043, 0xc0861178, 0xb98eb65f,
                    0x39121d63, 0x3f7fffff, 0x0,        0x80000000, 0xbea7055b,
                    0xbf71fef2, 0xc0276fa7, 0xb8ee403f, 0x390f8902, 0x3f7fffff,
                    0x3d61ed0a, 0xbd27425a, 0x3d32060c, 0xbf7f2769, 0xbf51a871,
                    0x3dfb643e, 0x3e6f4d8c, 0x3f7fffff, 0xbdef358b, 0x3d98e303,
                    0xbed912fa, 0xbf651ef5, 0xc0fe09a8, 0xb9a151b0, 0x38a26694,
                    0x3f7fffff, 0x0,        0x80000000, 0xbebb2bbe, 0xbf6e47e5,
                    0xc086c73c, 0x39994d35, 0xb93ef555, 0x3f7fffff, 0x0,
                    0x80000000, 0xbe9b0dc6, 0xbf73fa88, 0xbff3ad14, 0xb903b537,
                    0x37e5cfda, 0x3f7fffff, 0x3d033d9a, 0xbdba0bdb, 0xbd4b678d,
                    0xbf7e7dff, 0xbf5c848d, 0x3eacfbe3, 0x3f4a8acf, 0x3f7fffff,
                    0xbe02c377, 0x3ddde49a, 0xbed0e1db, 0xbf65c2c7, 0xc0fbdcd3,
                    0x38360c40, 0xb7fe7ab0, 0x3f7fffff, 0x0,        0x80000000,
                    0xbef3b87a, 0xbf6122ba, 0xc056653b, 0x398c0975, 0xb92284a0,
                    0x3f7fffff, 0x0,        0x80000000, 0xbed0e10e, 0xbf69ba27,
                    0xbfb51fad, 0xb915c318, 0x3895aa60, 0x3f7fffff, 0x3daf56f4,
                    0x3dc6f260, 0x3dc38476, 0xbf7caa63, 0xbf528d9c, 0xbd3404e4,
                    0xbf3a09aa, 0x3f7fffff, 0xbe26b78d, 0xbde3a8e7, 0xbe140887,
                    0xbf783b87, 0xc10321d8, 0xb95bb3d6, 0x39257720, 0x3f7fffff,
                    0x0,        0x80000000, 0xbe44c718, 0xbf7b3a93, 0xc082d30f,
                    0xb8cc24f4, 0x38bfe843, 0x3f7fffff, 0x0,        0x80000000,
                    0xbe732d15, 0xbf78ad39, 0xc01732a7, 0x38b5d673, 0xb8e125c8,
                    0x3f7fffff, 0x3f1c3b12, 0x3e8fde0a, 0xbd9777c0, 0xbf3caa9d,
                    0xbf7c47ca, 0xbf64b275, 0xc0003d5f, 0x3f7fffff, 0x3ec3e948,
                    0xbd9143ea, 0xbdf215c5, 0xbf69dec4, 0xc08d7ccc, 0x39c2dd0a,
                    0x38f3d3f2, 0x3f7fffff, 0x0,        0x80000000, 0xbddec879,
                    0xbf7e7b17, 0xc07c824d, 0xb8841d0e, 0x389bc125, 0x3f7fffff,
                    0xbe4db1c3, 0x3f37eb89, 0x3f29c65a, 0xbd78a9d8, 0x4163bf40,
                    0xc1c152e0, 0xc1dd425f, 0x3f7fffff, 0xbea8c024, 0x398a6773,
                    0x3b3ce4c5, 0x3f71b1ce, 0xc14e7198, 0xb8f40000, 0x38b38000,
                    0x3f7fffff, 0x3e0c83fc, 0xbb1e2be6, 0x3bf1ada7, 0x3f7d920d,
                    0x0,        0xb6800000, 0x0,        0x3f7fffff, 0x3f3a7c56,
                    0x3f28e7cd, 0xbe367eb7, 0xbd42f9d8, 0xc0e19d70, 0x411bde30,
                    0xc015ae9b, 0x3f7fffff, 0xbe3d544f, 0xbac32fe1, 0x3c8d4eb9,
                    0x3f7b8c16, 0x3cbdaa00, 0xc236a106, 0xbc863000, 0x3f7fffff,
                    0x3eb0e7a6, 0xbd8465a3, 0xbc8dbbc1, 0x3f6f9f35, 0x3c971c00,
                    0xc238d924, 0xbd8cec00, 0x3f7fffff, 0xbed6efb5, 0x0,
                    0x0,        0xbf6859a4, 0x29043834, 0xc1780629, 0x2791c62d,
                    0x3f7fffff, 0xbb5706c0, 0x3d91659d, 0x3b082757, 0x3f7f5a24,
                    0xb5800000, 0x0,        0x0,        0x3f7fffff, 0x3daeebee,
                    0xbc484791, 0x3f35be95, 0x3f32eee1, 0xc0e19e60, 0xc11bde34,
                    0xc015ae6d, 0x3f7fffff, 0xbe02ab3d, 0x3a009807, 0x3bf1076c,
                    0x3f7de670, 0xb5800000, 0x42361576, 0x36000000, 0x3f7fffff,
                    0x3eef9c11, 0xbd752f9a, 0xbc76aa3d, 0x3f61af06, 0x36000000,
                    0x42385c1a, 0x35000000, 0x3f7fffff, 0xbebfa9db, 0x0,
                    0x0,        0xbf6d62e7, 0x2755baea, 0x41780617, 0x37bb54b8,
                    0x3f7fffff, 0x3a049678, 0x3d3ec44a, 0x3bb2a169, 0x3f7fb7e6,
                    0x0,        0x37800000, 0xb5800000, 0x3f7fffff};

                memcpy(player->LastPacket.Bones, defaultBones, sizeof(defaultBones));

                if (player->Level == UserClient.Level && !IsLoading) 
                {
                    Engine::SpawnCharacter(player->Character, player->Actor);
                } 
                else 
                {
                    player->Actor = nullptr;
                }

                AddChatMessage(player->Name + " joined the room");

                Players.List.push_back(player);
                Players.Mutex.unlock();
            } 
            else if (msgType == "name") 
            {
                const auto msgId = msg["id"];
                const auto msgName = msg["name"];

                if (!msgId.is_number_integer() || !msgName.is_string()) 
                {
                    continue;
                }

                Players.Mutex.lock_shared();

                const auto player = GetPlayerById(msgId);
                if (player) 
                {
                    auto newName = msgName.get<std::string>();
                    AddChatMessage(player->Name + " renamed to " + newName);
                    player->Name = newName;
                }

                Players.Mutex.unlock_shared();
            } 
            else if (msgType == "chat" || msgType == "announce") 
            {
                const auto msgBody = msg["body"];
                if (!msgBody.is_string()) 
                {
                    continue;
                }

                Players.Mutex.lock_shared();
                AddChatMessage(msgBody.get<std::string>());
                Players.Mutex.unlock_shared();
            } 
            else if (msgType == "level") 
            {
                const auto msgId = msg["id"];
                const auto msgLevel = msg["level"];

                if (!msgId.is_number_integer() || !msgLevel.is_string()) 
                {
                    continue;
                }

                Players.Mutex.lock_shared();

                const auto player = GetPlayerById(msgId);
                if (player) 
                {
                    player->Level = msgLevel.get<std::string>();

                    if (player->Level == UserClient.Level) 
                    {
                        if (!player->Actor && !IsLoading) 
                        {
                            Engine::SpawnCharacter(player->Character, player->Actor);
                        }
                    } 
                    else 
                    {
                        if (player->Actor) 
                        {
                            Engine::Despawn(player->Actor);
                            player->Actor = nullptr;
                        }
                    }
                }

                Players.Mutex.unlock_shared();
            } 
            else if (msgType == "character") 
            {
                const auto msgId = msg["id"];
                const auto msgCharacter = msg["character"];

                if (!msgId.is_number_integer() || !msgCharacter.is_number_integer() || msgCharacter < 0 || msgCharacter >= Engine::Character::Max) 
                {
                    continue;
                }

                Players.Mutex.lock_shared();

                const auto player = GetPlayerById(msgId);
                if (player) 
                {
                    player->Character = msgCharacter;

                    if (!IsLoading) 
                    {
                        if (player->Actor) 
                        {
                            Engine::Despawn(player->Actor);
                            player->Actor = nullptr;
                        }

                        Engine::SpawnCharacter(player->Character, player->Actor);
                    }
                }

                Players.Mutex.unlock_shared();
            } 
            else if (msgType == "ping") 
            {
                if (SendJsonMessage({
                        {"type", "pong"},
                        {"id", UserClient.Id},
                    })) 
                {
                    lastPing = GetTickCount64();
                }
            } 
            else if (msgType == "disconnect") 
            {
                const auto msgId = msg["id"];
                if (!msgId.is_number_integer()) 
                {
                    continue;
                }

                Players.Mutex.lock();
                Players.List.erase(std::remove_if(Players.List.begin(), Players.List.end(),[&msgId](Client::Player *p) 
                {
                    if (p->Id != msgId) 
                    {
                        return false;
                    }

                    if (p->Actor) 
                    {
                        Engine::Despawn(p->Actor);
                        p->Actor = nullptr;
                    }

                    AddChatMessage(p->Name + " left the room");

                    delete p;
                    return true;
                }));
                Players.Mutex.unlock();
            }
            else if (msgType == "gameMode") 
            {
                const auto msgGameMode = msg["gameMode"];

                if (!msgGameMode.is_string()) 
                {
                    continue;
                }

                TaggedTimed = 0;
                PreviousTaggedId = 0;

                IgnorePlayerInput(false);

                UserClient.CanTag = false;
                UserClient.TaggedPlayerId = 0;
                UserClient.GameMode = msgGameMode.get<std::string>();
            } 
            else if (msgType == "canTag") 
            {
                TaggedTimed = 0;
                UserClient.CanTag = true;

                if (UserClient.Id == UserClient.TaggedPlayerId) 
                {
                    IgnorePlayerInput(false);
                }
            } 
            else if (msgType == "tagged") 
            {
                const auto msgTaggedPlayerId = msg["taggedPlayerId"];
                const auto msgTagCooldown = msg["coolDown"];

                if (!msgTaggedPlayerId.is_number_integer() || !msgTagCooldown.is_number_integer()) 
                {
                    continue;
                }

                if (Players.List.size() == 0) 
                {
                    SendJsonMessage({
                        {"type", "endGameMode"},
                    });

                    AddChatMessage("[Tag] Tag has ended since you're the only one in this room");
                    continue;
                }

                UserClient.TaggedPlayerId = msgTaggedPlayerId;
                UserClient.CanTag = false;
                UserClient.CoolDownTag = msgTagCooldown;

                if (UserClient.Id == UserClient.TaggedPlayerId && PlayerDiedAndSentJsonMessage == false) 
                {
                    char buffer[0xFF];

                    if (PreviousTaggedId == 0) 
                    {
                        sprintf_s(buffer, sizeof(buffer), "[Tag] %s was randomly choosen to be tagged", UserClient.Name.c_str());
                    } 
                    else 
                    {
                        auto previousTaggedPlayer = GetPlayerById(PreviousTaggedId);

                        if (previousTaggedPlayer) 
                        {
                            sprintf_s(buffer, sizeof(buffer), "[Tag] %s tagged %s", previousTaggedPlayer->Name.c_str(), UserClient.Name.c_str());
                        }
                    }

                    SendJsonMessage({
                        {"type", "announce"},
                        {"body", buffer},
                    });
                }

                TaggedTimed = GetTickCount64();
                PreviousTaggedId = msgTaggedPlayerId;
                IgnorePlayerInput(UserClient.Id == msgTaggedPlayerId);
            }
        }

        if (statusThread.joinable()) 
        {
            statusThread.join();
        }

        if (playerHandlerThread.joinable()) 
        {
            playerHandlerThread.join();
        }

        printf("client: shutdown\n");
    }
}

static void OnTick(float deltaTime) 
{
    static float sum = 0;
    sum += deltaTime;

    if (!IsLoading && IsConnected && sum > 0.016f) 
    {
        auto pawn = Engine::GetPlayerPawn();
        if (pawn && pawn->Mesh3p) 
        {
            Client::PACKET_COMPRESSED packet;
            packet.Id = UserClient.Id;
            packet.Position = pawn->Location;
            packet.Position.Z += pawn->TargetMeshTranslationZ;
            packet.Yaw = pawn->Rotation.Yaw % 0x10000;

            const auto bonesBase = reinterpret_cast<byte *>(pawn->Mesh3p->LocalAtoms.Buffer());

            for (auto i = 0; i < ARRAYSIZE(CompressedBoneOffsets); ++i) 
            {
                packet.CompressedBones[i] = static_cast<short>( roundf(*reinterpret_cast<float *>(bonesBase + CompressedBoneOffsets[i]) * 215.0f));
            }

            sendto(UDPSocket, reinterpret_cast<const char *>(&packet), sizeof(packet), 0, reinterpret_cast<const sockaddr *>(&Server), sizeof(Server));

            sum = 0;
        }
    }
}

static void OnTickGames(float deltaTime) 
{
    if (UserClient.GameMode != GameMode_Tag) 
    {
        return;
    }

    static float sum = 0;
    sum += deltaTime;

    if (!IsLoading && IsConnected && sum > 0.16f) 
    {
        auto pawn = Engine::GetPlayerPawn();

        if (!pawn) 
        {
            return;
        }

        sum = 0;

        if (UserClient.Id != UserClient.TaggedPlayerId) 
        {
            if (pawn->Health <= 0 && PlayerDiedAndSentJsonMessage == false) 
            {
                SendJsonMessage({
                    {"type", "dead"},
                });

                char buffer[0xFF];
                sprintf_s(buffer, sizeof(buffer), "[Tag] %s died and they will chase instead", UserClient.Name.c_str());

                SendJsonMessage({
                    {"type", "announce"},
                    {"body", buffer},
                });

                PlayerDiedAndSentJsonMessage = true;
                UserClient.CanTag = false;
            }
        }
        else 
        {
            if (!UserClient.CanTag) 
            {
                IgnorePlayerInput(true);
            }
        }

        if (PlayerDiedAndSentJsonMessage == true && pawn->Health == 100) 
        {
            PlayerDiedAndSentJsonMessage = false;
        }
    }
}

static void OnRender(IDirect3DDevice9 *device) 
{
    static const auto inputHeightOffset = 50.0f;
    static const auto inputWidthOffset = 50.0f;
    static const auto unfocusedChatMessages = 5;

    if (Players.ShowNameTags) 
    {
        auto window = ImGui::BeginRawScene("##client-backbuffer-nametags");
        Players.Mutex.lock_shared();

        for (auto p : Players.List) 
        {
            if (p->Level == UserClient.Level && p->Actor && p->Actor->SkeletalMeshComponent) 
            {
                auto pos = p->Actor->Location;
                pos.Z = p->MaxZ + 27.5f;

                if (Engine::WorldToScreen(device, pos)) 
                {
                    auto size = ImGui::CalcTextSize(p->Name.c_str());
                    auto topLeft = ImVec2(pos.X - size.x / 2.0f, pos.Y - size.y);

                    window->DrawList->AddRectFilled(topLeft - ImVec2(3.0f, 1.0f), ImVec2(pos.X + size.x / 2.0f, pos.Y) + ImVec2(3.0f, 1.0f), ImColor(ImVec4(0, 0, 0, 0.4f)));

                    if (UserClient.GameMode == GameMode_Tag && p->Id == UserClient.TaggedPlayerId) 
                    {
                        window->DrawList->AddText(topLeft, ImColor(ImVec4(1.0f, 0.0f, 0.0f, 1.0f)), p->Name.c_str());
                    } 
                    else 
                    {
                        window->DrawList->AddText(topLeft, ImColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), p->Name.c_str());
                    }
                }
            }
        }

        Players.Mutex.unlock_shared();
        ImGui::EndRawScene();
    }

    const auto window = ImGui::BeginRawScene("##client-backbuffer-chat");
    const auto &io = ImGui::GetIO();

    const auto width = io.DisplaySize.x / 3.0f;

    auto opacity = 1.0f;
    if (!Chat.Focused) 
    {
        if (Chat.ShowOverlay) 
        {
            auto diff = static_cast<float>(GetTickCount64() - Chat.LastTime) / 1000.0f;
            if (diff > 5.0f) 
            {
                opacity = max(0, 1.0f - (diff - 5.0f));
            }
        } 
        else 
        {
            opacity = 0.0f;
        }
    }

    if (opacity > 0.0f) 
    {
        Chat.Mutex.lock();

        auto body = Chat.Raw;
        if (!Chat.Focused) 
        {
            auto messages = 0;

            for (auto i = static_cast<int>(body.size()) - 1; i >= 0; --i) 
            {
                if (body[i] == '\n') 
                {
                    ++messages;
                }

                if (messages > unfocusedChatMessages) 
                {
                    body = body.substr(i + 1);
                    break;
                }
            }
        }

        const auto height = ImGui::CalcTextSize(body.c_str(), nullptr, false, width).y + (ImGui::GetTextLineHeight() / 6.0f);
        const auto pos = ImVec2(inputWidthOffset, io.DisplaySize.y - inputHeightOffset - height);

        ImGui::SetWindowPos(pos, ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(window->Size.x, max(window->Size.y, height)));

        window->DrawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), ImColor(ImVec4(0, 0, 0, 0.4f * opacity)));

        ImGui::PushTextWrapPos(width);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, opacity));
        ImGui::TextWrapped("%s", body.c_str());
        ImGui::PopStyleColor();
        ImGui::PopTextWrapPos();

        Chat.Mutex.unlock();
    }

    ImGui::EndRawScene();

    if (Chat.Focused) 
    {
        ImGui::BeginRawScene("##client-backbuffer-chatinput");

        ImGui::SetWindowPos(ImVec2(inputWidthOffset, io.DisplaySize.y - inputHeightOffset), ImGuiCond_Always);
        ImGui::SetKeyboardFocusHere(0);

        ImGui::PushItemWidth(io.DisplaySize.x - inputWidthOffset * 2);
        ImGui::InputText("##client-chat-overlay-input", ChatInput, sizeof(ChatInput));
        ImGui::PopItemWidth();

        ImGui::EndRawScene();
    }
}

static void OnRenderGames(IDirect3DDevice9 *device) 
{
    if (!ShowTagDistanceOverlay && !ShowTagCooldownOverlay) 
    {
        return;
    }

    auto pawn = Engine::GetPlayerPawn();
    auto controller = Engine::GetPlayerController();

    if (!pawn || !controller) 
    {
        return;
    }

    if (UserClient.Level.empty() || UserClient.Level == Map_MainMenu) 
    {
        return;
    }

    int playersInTheSameLevel = 0;
    float longestNameWidth = 0.0f;

    for (const auto &p : Players.List) 
    {
        if (p->Level == UserClient.Level && p->Actor) 
        {
            playersInTheSameLevel++;
            float nameWidth = ImGui::CalcTextSize(p->Name.c_str(), nullptr, false).x;

            if (nameWidth > longestNameWidth) 
            {
                longestNameWidth = nameWidth;
            }
        }
    }

    if (playersInTheSameLevel == 0) 
    {
        return;
    }

    char buffer[0x200] = {0};
    auto &io = ImGui::GetIO();
    float textHeight = ImGui::GetTextLineHeight();

    static float padding = 5.0f;
    static float maxNameWidth = 192.0f;

    if (ShowTagDistanceOverlay) 
    {
        auto window = ImGui::BeginRawScene("##tag-info");

        static float rightPadding = 80.0f;
        float y = (playersInTheSameLevel * textHeight) - textHeight + (padding / 2);
        float x = min(maxNameWidth, longestNameWidth);

        window->DrawList->AddRectFilled(ImVec2(), ImVec2(rightPadding + x + padding, y + padding + textHeight), ImColor(ImVec4(0, 0, 0, 0.4f)));

        for (const auto &p : Players.List) 
        {
            if (!p->Actor || p->Level != UserClient.Level) 
            {
                continue;
            }

            float dist = Distance(p->Actor->Location, pawn->Location);

            if (dist >= 10.0f) 
            {
                sprintf_s(buffer, "%.0f m", dist);
            } 
            else 
            {
                sprintf_s(buffer, "%.1f m", dist);
            }

            auto name = p->Name;
            auto color = ImColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

            if (UserClient.GameMode == GameMode_Tag && p->Id == UserClient.TaggedPlayerId) {
                color = ImColor(ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            }

            if (ImGui::CalcTextSize(name.c_str(), nullptr, false).x > maxNameWidth) {
                int charactersToRemove = 3;
                do {
                    name = p->Name;
                    name = name.substr(0, name.size() - charactersToRemove++) + "...";
                } while (ImGui::CalcTextSize(name.c_str(), nullptr, false).x > maxNameWidth);
            }

            window->DrawList->AddText(ImVec2(padding, y), color, buffer);
            window->DrawList->AddText(ImVec2(rightPadding, y), color, name.c_str());

            y -= textHeight;
        }

        ImGui::EndRawScene();
    }

    if (ShowTagCooldownOverlay) 
    {
        if (TaggedTimed == 0) 
        {
            return;
        }

        auto timeLeftTick = TaggedTimed + (static_cast<unsigned long long>(UserClient.CoolDownTag) * 1000) - GetTickCount64();
        auto timeLeft = (float)timeLeftTick / 1000;

        if (timeLeft < 0.0f || timeLeft > UINT_MAX) 
        {
            return;
        }

        auto playerName = UserClient.Name;

        if (UserClient.Id != UserClient.TaggedPlayerId) 
        {
            auto player = GetPlayerById(UserClient.TaggedPlayerId);

            if (!player) 
            {
                return;
            }

            playerName = player->Name;
        }

        auto name = playerName;
        if (ImGui::CalcTextSize(name.c_str(), nullptr, false).x > maxNameWidth) 
        {
            int charactersToRemove = 3;

            do 
            {
                name = playerName;
                name = name.substr(0, name.size() - charactersToRemove++) + "...";
            } while (ImGui::CalcTextSize(name.c_str(), nullptr, false).x > maxNameWidth);
        }

        sprintf_s(buffer, "%s can move in %.1f seconds", name.c_str(), timeLeft);

        auto window = ImGui::BeginRawScene("##tag-timeleft");

        float textSize = ImGui::CalcTextSize(buffer, nullptr, false).x;
        float topMiddleX = io.DisplaySize.x / 2 - textSize / 2;

        window->DrawList->AddRectFilled(ImVec2(topMiddleX - padding, 0), ImVec2(topMiddleX + textSize + padding, textHeight + padding), ImColor(ImVec4(0, 0, 0, 0.4f)));
        window->DrawList->AddText(ImVec2(topMiddleX, padding / 2), ImColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)), buffer);

        ImGui::EndRawScene();
    }
}

static void MultiplayerTab() 
{
    ImGui::Text("Status: %s", IsConnected ? "Connected" : IsMultiplayerDisabled ? "Multiplayer Disabled" : "Connecting");
    ImGui::Separator(5.0f);

    const auto nameInputCallback = []() 
    {
        if (UserClient.Name != NameInput) 
        {
            bool empty = true;
            for (auto c : std::string(NameInput)) 
            {
                if (!isblank(c)) 
                {
                    empty = false;
                    break;
                }
            }

            if (!empty) 
            {
                AddChatMessage(UserClient.Name + " renamed to " + NameInput);
                UserClient.Name = NameInput;
                Settings::SetSetting({ "Client", "Name" }, UserClient.Name);

                if (IsConnected) 
                {
                    SendJsonMessage({
                        {"type", "name"},
                        {"id", UserClient.Id},
                        {"name", UserClient.Name},
                    });
                }
            }
        }
    };

    ImGui::Text("Name");
    ImGui::SameLine();
    if (ImGui::InputText("##client-name-input", NameInput, sizeof(NameInput), ImGuiInputTextFlags_EnterReturnsTrue)) 
    {
        nameInputCallback();
    }

    ImGui::SameLine();
    if (ImGui::Button("Change##client-name-button")) 
    {
        nameInputCallback();
    }

    ImGui::Text("Character");
    ImGui::SameLine();

    const auto selectedCharacter = Engine::Characters[static_cast<size_t>(UserClient.Character)];

    if (ImGui::BeginCombo("##client-character", selectedCharacter)) 
    {
        for (auto i = 0; i < IM_ARRAYSIZE(Engine::Characters); ++i) 
        {
            const auto label = Engine::Characters[i];
            const auto selected = (label == selectedCharacter);

            if (ImGui::Selectable(label, selected)) 
            {
                UserClient.Character = static_cast<Engine::Character>(i);
                Settings::SetSetting({ "Client", "Character" }, UserClient.Character);

                if (IsConnected) 
                {
                    SendJsonMessage({
                        {"type", "character"},
                        {"id", UserClient.Id},
                        {"character", UserClient.Character},
                    });
                }
            }

            if (selected) 
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    const auto roomInputCallback = []() 
    {
        if (Room != RoomInput) 
        {
            auto empty = true;
            for (auto c : std::string(RoomInput)) 
            {
                if (!isblank(c)) 
                {
                    empty = false;
                    break;
                }
            }

            if (!empty) 
            {
                Room = RoomInput;
                Settings::SetSetting({ "Client", "Room" }, Room);

                if (IsConnected) 
                {
                    Disconnect();
                }
            }
        }
    };

    ImGui::Text("Room");
    ImGui::SameLine();
    if (ImGui::InputText("##client-room-input", RoomInput, sizeof(RoomInput), ImGuiInputTextFlags_EnterReturnsTrue)) 
    {
        roomInputCallback();
    }

    ImGui::SameLine();
    if (ImGui::Button("Join##client-room-button")) 
    {
        roomInputCallback();
    }

    ImGui::Separator(5.0f);

    if (ImGui::Checkbox("Show Nametags##client-show-nametags", &Players.ShowNameTags)) 
    {
        Settings::SetSetting({ "Client", "ShowNameTags" }, Players.ShowNameTags);
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("Show Chat Overlay##client-show-chat", &Chat.ShowOverlay)) 
    {
        Settings::SetSetting({ "Client", "ShowChatOverlay" }, Chat.ShowOverlay);
    }

    ImGui::SameLine();

    if (ImGui::Checkbox("Disable Multiplayer##client-disabled", &IsMultiplayerDisabled)) 
    {
        if (IsMultiplayerDisabled) 
        {
            Disconnect();
        }

        Settings::SetSetting({ "Client", "Disabled" }, IsMultiplayerDisabled);
    }

    ImGui::Separator(5.0f);
    ImGui::Text("Chat");

    Chat.Mutex.lock();
    ImGui::InputTextMultiline("##client-chat", const_cast<char *>(Chat.Raw.c_str()), Chat.Raw.size(), {0, 0}, ImGuiInputTextFlags_ReadOnly);
    Chat.Mutex.unlock();

    if (ImGui::InputText("##client-chat-input", ChatInput, sizeof(ChatInput), ImGuiInputTextFlags_EnterReturnsTrue)) 
    {
        SendChatInput();
    }

    ImGui::SameLine();
    if (ImGui::Button("Send##client-chat-send")) 
    {
        SendChatInput();
    }

    if (ImGui::Hotkey("Chat Keybind##client-chat-keybind", &Chat.Keybind)) 
    {
        Settings::SetSetting({ "Client", "ChatKeybind" }, Chat.Keybind);
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset##chatKeybind")) 
    {
        Settings::SetSetting({ "Client", "ChatKeybind" }, Chat.Keybind = VK_T);
    }

    ImGui::Separator(5.0f);

    Players.Mutex.lock_shared();
    if (ImGui::TreeNode("##client-players", "Players (%d)", Players.List.size())) 
    {
        for (auto p : Players.List) 
        {
            ImGui::Text("%s - %s", p->Name.c_str(), p->Level.c_str());
            ImGui::SameLine();

            if (p->Level == UserClient.Level && p->Actor) 
            {
                if (ImGui::Button(("Goto##client-goto-" + std::to_string(p->Id)).c_str()))
                {
                    auto pawn = Engine::GetPlayerPawn();
                    if (pawn)
                    {
                        pawn->Location = p->Actor->Location;
                    }
                }
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Button(("Goto##client-goto-" + std::to_string(p->Id)).c_str());
                ImGui::EndDisabled();
            }
        }

        ImGui::TreePop();
    }
    Players.Mutex.unlock_shared();
}

static void GamesTab() 
{
    ImGui::SeparatorText("Tag##Tag");
    {
        if (ImGui::Checkbox("Distance Overlay##Tag-DistanceOverlay", &ShowTagDistanceOverlay))
        {
            Settings::SetSetting({ "Games", "Tag", "ShowDistanceOverlay" }, ShowTagDistanceOverlay);
        }
        ImGui::HelpMarker("Shows the distance to other players in meters. Tag doesn't need to be enabled for this");

        if (UserClient.GameMode == GameMode_Tag)
        {
            ImGui::Checkbox("Cooldown Overlay##Tag-CooldownOverlay", &ShowTagCooldownOverlay);
            ImGui::HelpMarker("When someone gets tagged, it will show at the top middle of the screen of the cooldown in seconds until they can move again");
        }

        if (UserClient.Level == Map_MainMenu || Players.List.size() == 0)
        {
            ImGui::Separator(5.0f);
            ImGui::TextWrapped("You can't start tag when you're in the main menu or when there are no other players in the room!");
        }
        else
        {
            char buffer[0xFF];

            if (UserClient.GameMode == GameMode_None)
            {
                ImGui::SliderInt("Cooldown Timer##Tag-CooldownSlider", &TagCooldown, 1, 60, "%d", ImGuiSliderFlags_AlwaysClamp);
                ImGui::HelpMarker("Change to cooldown to be anywhere from 1 to 60. Cooldown is in seconds and default is 5");

                ImGui::Separator(5.0f);

                if (ImGui::Button("Update Cooldown Timer##Tag-UpdateCooldownTime"))
                {
                    SendJsonMessage({
                        {"type", "cooldown"},
                        {"cooldown", UserClient.CoolDownTag},
                    });

                    sprintf_s(buffer, sizeof(buffer), "[Tag] %s changed the cooldown to be %d second%s", UserClient.Name.c_str(), TagCooldown, TagCooldown != 1 ? "s" : "");

                    SendJsonMessage({
                        {"type", "announce"},
                        {"body", buffer},
                    });
                }

                ImGui::SameLine();
                if (ImGui::Button("Start Tag##Tag-StartTag"))
                {
                    SendJsonMessage({
                        {"type", "startTagGameMode"},
                    });

                    sprintf_s(buffer, sizeof(buffer), "[Tag] %s started tag", UserClient.Name.c_str());

                    SendJsonMessage({
                        {"type", "announce"},
                        {"body", buffer},
                    });
                }
            }

            if (UserClient.GameMode == GameMode_Tag)
            {
                if (ImGui::Button("End Tag##Tag-EndTag"))
                {
                    SendJsonMessage({
                        {"type", "endGameMode"},
                    });

                    sprintf_s(buffer, sizeof(buffer), "[Tag] %s ended tag", UserClient.Name.c_str());

                    SendJsonMessage({
                        {"type", "announce"},
                        {"body", buffer},
                    });
                }
            }
        }
    }
}

bool Client::Initialize() 
{
    // Settings
    UserClient.Name = Settings::GetSetting({ "Client", "Name" }, "anonymous").get<std::string>();
    strncpy_s(NameInput, sizeof(NameInput) - 1, UserClient.Name.c_str(), sizeof(NameInput) - 1);

    Room = Settings::GetSetting({ "Client", "Room" }, "lobby").get<std::string>();
    strncpy_s(RoomInput, sizeof(RoomInput) - 1, Room.c_str(), sizeof(RoomInput) - 1);

    UserClient.Character = Settings::GetSetting({ "Client", "Character" }, Engine::Character::Faith).get<Engine::Character>();
    Chat.Keybind = Settings::GetSetting({ "Client", "ChatKeybind" }, VK_T);
    Players.ShowNameTags = Settings::GetSetting({ "Client", "ShowNameTags" }, true);
    Chat.ShowOverlay = Settings::GetSetting({ "Client", "ShowChatOverlay" }, true);
    IsMultiplayerDisabled = Settings::GetSetting({ "Client", "Disabled" }, false);

    ShowTagDistanceOverlay = Settings::GetSetting({ "Games", "Tag", "ShowDistanceOverlay" }, false);

    // Functions
    Menu::AddTab("Multiplayer", MultiplayerTab);
    Menu::AddTab("Games", GamesTab);

    Engine::OnTick(OnTick);
    Engine::OnTick(OnTickGames);

    Engine::OnRenderScene(OnRender);
    Engine::OnRenderScene(OnRenderGames);

    Engine::OnInput([](unsigned int &msg, int keycode) 
    {
        if (!Chat.Focused && msg == WM_KEYDOWN && keycode == Chat.Keybind) 
        {
            Chat.Focused = true;
            Engine::BlockInput(true);
        }
    });

    Engine::OnSuperInput([](unsigned int &msg, int keycode) 
    {
        if (Chat.Focused) 
        {
            if (msg == WM_KEYUP && keycode == VK_RETURN) 
            {
                SendChatInput();
                Chat.Focused = false;
                Engine::BlockInput(false);
            } 
            else if (msg == WM_KEYUP && keycode == VK_ESCAPE) 
            {
                Chat.Focused = false;
                Engine::BlockInput(false);
            }
        }
    });

    Engine::OnActorTick([](Classes::AActor *actor) 
    {
        if (!actor) {
            return;
        }

        Players.Mutex.lock_shared();

        for (const auto &p : Players.List) 
        {
            if (p->Actor == actor && p->Actor->SkeletalMeshComponent && p->Id == p->LastPacket.Id) 
            {
                p->Actor->Location = p->LastPacket.Position;
                p->Actor->Rotation = {0, p->LastPacket.Yaw, 0};
                p->MaxZ = p->Actor->SkeletalMeshComponent->GetBoneLocation("Neck", 0).Z;
            }
        }

        Players.Mutex.unlock_shared();
    });

    Engine::OnBonesTick([](Classes::TArray<Classes::FBoneAtom> *bones) 
    {
        Players.Mutex.lock_shared();

        for (const auto &p : Players.List) 
        {
            if (p->Actor && p->Actor->SkeletalMeshComponent && p->Actor->SkeletalMeshComponent->LocalAtoms.Buffer() == bones->Buffer() && p->Id == p->LastPacket.Id) 
            {
                Engine::TransformBones(p->Character, bones, p->LastPacket.Bones);
            }
        }

        Players.Mutex.unlock_shared();
    });

    Engine::OnPreLevelLoad([](const wchar_t *levelNameW) 
    {
        Players.Mutex.lock_shared();
        IsLoading = true;

        UserClient.Level = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(levelNameW);

        std::transform(UserClient.Level.begin(), UserClient.Level.end(), UserClient.Level.begin(), [](char c) 
        { 
            return tolower(c); 
        });

        for (const auto &p : Players.List) 
        {
            p->Actor = nullptr;
        }

        Players.Mutex.unlock_shared();
    });

    Engine::OnPostLevelLoad([](const wchar_t *) 
    {
        if (UserClient.Level != Map_MainMenu) 
        {
            Players.Mutex.lock_shared();

            for (const auto &p : Players.List) 
            {
                if (!p->Actor && p->Level == UserClient.Level) 
                {
                    Engine::SpawnCharacter(p->Character, p->Actor);
                }
            }

            IsLoading = false;
            Players.Mutex.unlock_shared();
        }

        if (IsConnected) 
        {
            SendJsonMessage({
                {"type", "level"},
                {"id", UserClient.Id},
                {"level", UserClient.Level},
            });
        }
    });

    Engine::OnPreDeath([]() 
    {
        Players.Mutex.lock_shared();
        IsLoading = true;
        Players.Mutex.unlock_shared();
    });

    Engine::OnPostDeath([]() 
    {
        Players.Mutex.lock_shared();
        IsLoading = false;

        for (const auto &p : Players.List) 
        {
            if (!p->Actor && p->Level == UserClient.Level) 
            {
                Engine::SpawnCharacter(p->Character, p->Actor);
            }
        }

        Players.Mutex.unlock_shared();
    });

    std::thread(ClientListener).detach();
    return true;
}

std::string Client::GetName() 
{ 
    return "Multiplayer Client"; 
}
