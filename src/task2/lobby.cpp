#include <enet/enet.h>
#include <iostream>
#include <vector>

std::vector<ENetPeer*> peers;

#define CHANNEL_NUM 32

void send_server_data(ENetPeer* peer, int channel_id, ENetAddress &game_server_address) {
    std::string msg = std::to_string(game_server_address.port);
    ENetPacket *packet = enet_packet_create(msg.c_str(), msg.size() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
    enet_peer_send(peer, channel_id,  packet);
}

void broadcast_server_data(ENetAddress &game_server_address) {
    int channel_id = 0;
    for (auto peer : peers) {
        send_server_data(peer, channel_id % CHANNEL_NUM, game_server_address);
        channel_id++;
    }
}


int main(int argc, const char **argv)
{
    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10887;

    ENetHost *server = enet_host_create(&address, CHANNEL_NUM, 2, 0, 0);

    ENetAddress game_server_address;
    enet_address_set_host(&game_server_address, "localhost");
    game_server_address.port = 10888;

    bool is_game_started = false;

    if (!server)
    {
        printf("Cannot create ENet server\n");
        return 1;
    }

    while (true)
    {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                    peers.push_back(event.peer);
                    if (is_game_started)
                        send_server_data(event.peer, event.channelID, game_server_address);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    if (is_game_started)
                        break;
                    if (std::string((char*)(event.packet->data)) == "start" && !is_game_started) {
                        is_game_started = true;
                        broadcast_server_data(game_server_address);
                    }
                    printf("Packet received '%s'\n", event.packet->data);
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
            };
        }
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}

