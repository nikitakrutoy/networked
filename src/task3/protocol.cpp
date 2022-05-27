#include "protocol.h"
#include <string>
#include <vector>
#include "utils.h"


void send_join(ENetPeer *peer) {
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
    *packet->data = E_CLIENT_TO_SERVER_JOIN;
    enet_peer_send(peer, 0, packet);
}

void send_server_data(ENetPeer* peer, ENetAddress &game_server_address) {
    uint32_t port = game_server_address.port;
    std::string text = std::to_string(port);
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + text.size(), ENET_PACKET_FLAG_RELIABLE);
    BitStream bit_stream(packet->data);
    bit_stream.write<MessageType>(E_SERVER_DATA);
    bit_stream.write((void*)text.c_str(), text.size());
    enet_peer_send(peer, 1,  packet);
}

void send_server_ai_num(ENetPeer* peer, uint32_t ai_num) {
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint32_t), ENET_PACKET_FLAG_RELIABLE);
    BitStream bit_stream(packet->data);
    bit_stream.write<MessageType>(E_SERVER_SET_AI_NUM);
    bit_stream.write<uint32_t>(ai_num);
    enet_peer_send(peer, 1,  packet);
}


void send_lobby_info(ENetPeer* peer, std::vector<RoomInfo> &rooms) {
    uint32_t roomInfoSize = sizeof(uint16_t) + sizeof(RoomInfo) * rooms.size();
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + roomInfoSize, ENET_PACKET_FLAG_RELIABLE);
    BitStream bit_stream(packet->data);
    bit_stream.write<MessageType>(E_LOBBY_INFO);
    bit_stream.write<uint32_t>(rooms.size());
    for (auto& roomInfo: rooms) {
        bit_stream.write<uint32_t>(roomInfo.peers_num);
        bit_stream.write<uint16_t>(roomInfo.ai_num);
        bit_stream.write<bool>(roomInfo.is_game_started);
    }
    enet_peer_send(peer, 0,  packet);
}


void send_text_message(ENetPeer *peer, std::string text) {
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + text.size(), ENET_PACKET_FLAG_RELIABLE);
    BitStream bit_stream(packet->data);
    bit_stream.write<MessageType>(E_TEXT_MESSAGE);
    bit_stream.write((void*)text.c_str(), text.size());
    enet_peer_send(peer, 0,  packet);
}


void send_respawn(ENetPeer *peer, uint16_t eid) {
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                            ENET_PACKET_FLAG_RELIABLE);
    BitStream bit_stream(packet->data);
    bit_stream.write<MessageType>(E_RESPAWN);
    bit_stream.write<uint16_t>(&eid);
    enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  BitStream bit_stream(packet->data);
  bit_stream.write<MessageType>(E_SERVER_TO_CLIENT_NEW_ENTITY);
  bit_stream.write<Entity>(&ent);
  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid) {
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                            ENET_PACKET_FLAG_RELIABLE);
    BitStream bit_stream(packet->data);
    bit_stream.write<MessageType>(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
    bit_stream.write<uint16_t>(&eid);
    enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y) {
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                     2 * sizeof(float),
                                            ENET_PACKET_FLAG_UNSEQUENCED);
    BitStream bit_stream(packet->data);
    bit_stream.write<MessageType>(E_STATE);
    bit_stream.write<uint16_t>(&eid);
    bit_stream.write<float>(&x);
    bit_stream.write<float>(&y);
    enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float r) {
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                     3 * sizeof(float),
                                            ENET_PACKET_FLAG_UNSEQUENCED);
    BitStream bit_stream(packet->data);
    bit_stream.write<MessageType>(E_SERVER_TO_CLIENT_SNAPSHOT);
    bit_stream.write<uint16_t>(&eid);
    bit_stream.write<float>(&x);
    bit_stream.write<float>(&y);
    bit_stream.write<float>(&r);
    enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet) {
    return (MessageType) *packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    bit_stream.read<Entity>(&ent);
}

void deserialize_server_data(ENetPacket *packet, uint32_t &port) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    std::string text;
    bit_stream.read_string(text);
    port = parse_uint(text.c_str());
}


void deserialize_message(ENetPacket *packet, std::string &command) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    bit_stream.read_string(command);
}

void deserialize_set_ai_num(ENetPacket *packet, uint32_t &ai_num) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    bit_stream.read(&ai_num);
}

void deserialize_lobby_info(ENetPacket *packet, std::vector<RoomInfo> &rooms) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    rooms.clear();
    uint32_t n;
    bit_stream.read<uint32_t>(&n);
    for (int i = 0; i < n; i++) {
        uint32_t peers_num;
        uint16_t ai_num;
        bool is_game_started;
        bit_stream.read<uint32_t>(&peers_num);
        bit_stream.read<uint16_t>(&ai_num);
        bit_stream.read<bool>(&is_game_started);
        rooms.push_back({peers_num, ai_num, is_game_started});
    }
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    bit_stream.read<uint16_t>(&eid);
}

void deserialize_spawn(ENetPacket *packet, uint16_t &eid) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    bit_stream.read<uint16_t>(&eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    bit_stream.read<uint16_t>(&eid);
    bit_stream.read<float>(&x);
    bit_stream.read<float>(&y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &r) {
    BitStream bit_stream(packet->data);
    bit_stream.step<MessageType>();
    bit_stream.read<uint16_t>(&eid);
    bit_stream.read<float>(&x);
    bit_stream.read<float>(&y);
    bit_stream.read<float>(&r);
}

