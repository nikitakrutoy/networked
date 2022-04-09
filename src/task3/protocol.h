#pragma once

#include <enet/enet.h>
#include <cstdint>
#include "entity.h"
#include <cstring>

enum MessageType : uint8_t {
    E_CLIENT_TO_SERVER_JOIN = 0,
    E_SERVER_TO_CLIENT_NEW_ENTITY,
    E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY,
    E_STATE,
    E_SERVER_TO_CLIENT_SNAPSHOT,
    E_RESPAWN
};

class BitStream {
    uint8_t* data;

public:
    explicit BitStream(uint8_t* data): data(data) {}

    template <typename T>
    void write(T* ptr) {
        memcpy(data, ptr, sizeof(T)); data += sizeof(T);
    }

    template <typename T>
    void write(const T* ptr) {
        memcpy(data, ptr, sizeof(T)); data += sizeof(T);
    }

    template <typename T>
    void write(T value) {
        *data = value;
        data += sizeof(T);
    }

    template <typename T>
    void read(T* ptr) {
        memcpy(ptr, data, sizeof(T)); data += sizeof(T);
    }

    template <typename T>
    void step() {
        data += sizeof(T);
    }
};

void send_join(ENetPeer *peer);

void send_new_entity(ENetPeer *peer, const Entity &ent);

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_respawn(ENetPeer *peer, uint16_t eid);

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y);

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float r);

MessageType get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, Entity &ent);

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_spawn(ENetPacket *packet, uint16_t &eid);

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y);

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &r);

