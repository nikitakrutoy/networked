#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <map>

#include "utils.h"

struct user_info {
    std::string uuid;
    ENetPeer* peer;
};

std::map<int, user_info> peers;

#define CHANNEL_NUM 32

void send_users_packet(ENetPeer *peer)
{
    const char *baseMsg = "Stay awhile and listen. ";
    const size_t msgLen = strlen(baseMsg);

    const size_t sendSize = 2500;
    std::string message = "Connected users: ";
    for (const auto& data : peers)
        message += data.second.uuid + " ";

    ENetPacket *packet = enet_packet_create(message.c_str(), message.size(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

void send_time_packet(ENetPeer *peer)
{

}

void broadcast_ping()
{
    std::string message = "Pings: ";
    for (const auto& data : peers)
        message += std::to_string(data.second.peer->roundTripTime) + " ";
    ENetPacket *packet = enet_packet_create(message.c_str(), message.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);

    int channel_id = 0;
    for (const auto& data : peers) {
        enet_peer_send(data.second.peer, channel_id, packet);
        channel_id++;
    }
}

void broadcast_time()
{
    auto now = std::chrono::system_clock::now();
    auto msg = serialize_time_point(now, "UTC: %Y-%m-%d %H:%M:%S");
    ENetPacket *packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);

    int channel_id = 0;
    for (const auto& data : peers) {
        enet_peer_send(data.second.peer, channel_id, packet);
        channel_id++;
    }
}

int main(int argc, const char **argv)
{
    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet\n");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10888;

    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server)
    {
        printf("Cannot create ENet server\n");
        return 1;
    }

    uint32_t timeStart = enet_time_get();
    uint32_t lastFragmentedSendTime = timeStart;

    std::string uuid;
    while (true)
    {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                    uuid = get_uuid();
                    peers[event.peer->address.port] = {uuid, event.peer};
                    printf("Added user %s\n", uuid.c_str());
                    send_users_packet(event.peer);
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    printf("Packet received '%s' from '%s'\n",
                           event.packet->data, peers[event.peer->address.port].uuid.c_str());
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
            };
        }
        uint32_t curTime = enet_time_get();
        if (curTime - lastFragmentedSendTime > 1000)
        {
            lastFragmentedSendTime = curTime;
            broadcast_ping();
            broadcast_time();
        }
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}

