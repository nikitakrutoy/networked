#include <enet/enet.h>
#include <vector>
#include <array>
#include "protocol.h"
#include "utils.h"


#include <map>

#define SERVER_NUM 3
#define BASE_SERVER_PORT 10890

struct User;
struct Room;

std::map<uint16_t, Room> rooms;
std::map<uint32_t, User> users;
std::map<ENetPeer*, uint32_t> peer2id;

uint32_t idCounter = 0;

struct User {
    uint32_t id;
    ENetPeer* peer;
    uint32_t channel_id;
    int32_t rid = -1;

    bool operator==(const User p) const {
        return p.id == id;
    }
};


struct ServerSettings {
    uint16_t ai_number = 2;
};


struct Room {
    uint32_t id;
    ENetAddress server_address;
    ENetPeer* server_peer;
    ServerSettings settings;
    std::vector<uint32_t> uids;
    bool is_game_started = false;

    void broadcast_server_data() {
        for (auto uid : uids) {
            send_server_data(users[uid].peer, server_address);
        }
    }

    void start_server() {}
};


void on_connect(ENetPeer* peer, uint8_t channeld_id) {
    if(peer2id.contains(peer)) return;
    User p {idCounter++, peer, channeld_id};
    users[p.id] = p;
    peer2id[p.peer] = p.id;
}

void on_lobby_command(ENetPeer* peer, ENetPacket* packet) {
    std::string cmd;
    deserialize_message(packet, cmd);
    std::vector<std::string> cmd_w_args = split(cmd, " ");
    if (cmd_w_args.empty()) return;

    User* u = &users[peer2id[peer]];


    if (cmd_w_args[0] == "start") {
        if (u->rid < 0) {
            send_text_message(peer, "LOBBY: you are not in the room, nothing to start.");
            return;
        }
        Room* room = &rooms[u->rid];
        if (room->is_game_started) {
            send_text_message(peer, "LOBBY: game is already started.");
            return;
        }
        send_server_ai_num(room->server_peer, room->settings.ai_number);
        room->is_game_started = true;
        room->start_server();
        room->broadcast_server_data();
        return;
    }

    if (cmd_w_args[0] == "list") {
        std::vector<RoomInfo> data;
        for (auto &room: rooms) {
            uint32_t peers_num = room.second.uids.size();
            data.push_back({peers_num, room.second.settings.ai_number, room.second.is_game_started});
        }
        send_lobby_info(peer, data);
        return;
    }

    if (cmd_w_args[0] == "join") {
        if (cmd_w_args.size() < 2) {
            send_text_message(peer, "LOBBY: No. Example: join <rid>");
            return;
        }
        const long rid = parse_uint(cmd_w_args[1].c_str());
        if (rid < 0 || rid >= rooms.size()) {
            send_text_message(peer, "LOBBY: Where did you got this id from? I cant find room like this.");
            return;
        }
        if (u->rid >= 0) {
            send_text_message(peer, "LOBBY: exit room to join new one");
            return;
        }
        Room* room = &rooms[rid];
        if (room->is_game_started) {
            send_text_message(peer, "LOBBY: game is already started. You cant join.");
            return;
        }
        u->rid = rid;
        rooms[rid].uids.push_back(u->id);
        return;
    }

    if (cmd_w_args[0] == "exit") {
        if (u->rid < 0) {
            send_text_message(peer, "LOBBY: you are not in the room, you cant exit dummy.");
            return;
        }
        Room* room = &rooms[u->rid];
        auto findIter = std::find(room->uids.begin(), room->uids.end(), u->id);
        if (findIter != room->uids.end()) room->uids.erase(findIter);
        u->rid = -1;
        return;
    }


    if (cmd_w_args[0] == "set") {
        if (cmd_w_args.size() < 1) {
            send_text_message(peer, "LOBBY: It should be something like: set <number>");
            return;
        };
        if (u->rid < 0) {
            send_text_message(peer, "LOBBY: you are not in the room, you cant exit dummy.");
            return;
        }

        const long value = parse_uint(cmd_w_args[1].c_str());
        if (value < 0) {
            send_text_message(peer, "LOBBY: It is not a number, baby");
            return;
        }
        Room* room = &rooms[u->rid];
        room->settings.ai_number = value;
        return;
    }
    send_text_message(peer, "LOBBY: Unknown command");
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

    ENetHost *server = enet_host_create(&address, 16, 2, 0, 0);

    for (int i = 0; i < SERVER_NUM; i++) {
        enet_address_set_host(&rooms[i].server_address, "localhost");
        rooms[i].server_address.port = BASE_SERVER_PORT + i;
        rooms[i].uids.clear();
        rooms[i].id = i;
        ENetPeer* serverPeer = enet_host_connect(server, &rooms[i].server_address, 2, 0);
        rooms[i].server_peer = serverPeer;
    }

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
                    on_connect(event.peer, event.channelID);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    switch (get_packet_type(event.packet)) {
                        case E_TEXT_MESSAGE:
                            on_lobby_command(event.peer, event.packet);
                        default:
                            break;
                    }
                    printf("Packet received  %s\n", event.packet->data);
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