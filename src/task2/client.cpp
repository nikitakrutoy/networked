#include <enet/enet.h>
#include "utils.h"
#include <iostream>
#include <future>


void send_fragmented_packet(ENetPeer *peer)
{
    const char *baseMsg = "Stay awhile and listen. ";
    const size_t msgLen = strlen(baseMsg);

    const size_t sendSize = 2500;
    char *hugeMessage = new char[sendSize];
    for (size_t i = 0; i < sendSize; ++i)
        hugeMessage[i] = baseMsg[i % msgLen];
    hugeMessage[sendSize-1] = '\0';

    ENetPacket *packet = enet_packet_create(hugeMessage, sendSize, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);

    delete[] hugeMessage;
}

void send_time_packet(ENetPeer *peer)
{
    auto now = std::chrono::system_clock::now();
    auto msg = serialize_time_point(now, "UTC: %Y-%m-%d %H:%M:%S");
    ENetPacket *packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
    int err = enet_peer_send(peer, 0, packet);
    if (err) {
        printf("Could not communicate to server\n");
    }
}

void send_start_packet(ENetPeer *peer)
{
    const char *msg = "start";
    ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, 1, packet);
}

static std::string get_input()
{
    std::string input;
    std::cin >> input;
    return input;
}

int main(int argc, const char **argv)
{
    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }

    ENetHost *client = enet_host_create(nullptr, 2, 4, 0, 0);
    if (!client)
    {
        printf("Cannot create ENet client\n");
        return 1;
    }

    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = 10887;

    ENetAddress server_address;
    enet_address_set_host(&server_address, "localhost");

    ENetPeer *serverPeer = nullptr;
    ENetPeer *lobbyPeer = enet_host_connect(client, &address, 2, 0);
    if (!lobbyPeer)
    {
        printf("Cannot connect to lobby");
        return 1;
    }

    uint32_t timeStart = enet_time_get();
    uint32_t lastFragmentedSendTime = timeStart;
    uint32_t lastMicroSendTime = timeStart;
    bool connected = false;
    std::future<std::string> future = std::async(get_input);
    while (true)
    {
        ENetEvent event;
        std::string command;
        bool is_input_ready = future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        while (enet_host_service(client, &event, 10) > 0 || is_input_ready)
        {
            if (is_input_ready && future.get() == "start") {
                is_input_ready = false;
                future = std::async(get_input);
                send_start_packet(lobbyPeer);
            }

            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                    connected = true;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    printf("Packet received '%s'\n", event.packet->data);
                    if (!serverPeer) {
                        char *p_end;
                        server_address.port = std::strtol((char *) event.packet->data, &p_end, 10);
                        serverPeer = enet_host_connect(client, &server_address, 2, 0);
                        if (!serverPeer) {
                            printf("Cannot connect to server");
                            return 1;
                        }
                    }
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
            };
        }
        if (connected)
        {
            uint32_t curTime = enet_time_get();
            if (curTime - lastMicroSendTime > 1000 && serverPeer)
            {
                lastMicroSendTime = curTime;
                send_time_packet(serverPeer);
            }
        }
    }
    return 0;
}
