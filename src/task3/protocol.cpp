#include "protocol.h"
#include <cstring> // memcpy

void send_join(ENetPeer *peer) {
    ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
    *packet->data = E_CLIENT_TO_SERVER_JOIN;

    enet_peer_send(peer, 0, packet);
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

