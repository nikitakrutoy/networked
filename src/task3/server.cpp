#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <random>

struct Point {
    float x;
    float y;
};

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer *> controlledMap;
static size_t ai_entities_num = 5;
static std::vector<Point> dests;


int width = 8;
int height = 8;

uint16_t randint(int min, int max) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(min, max); // distribution in range [1, 6]
    return dist(rng);
}

void create_ai_entities() {
    for (uint16_t i = 0; i < ai_entities_num; i++) {
        Entity ent{
            .color = 0xff22ff88,
            .x = float(randint(0, width * 2) - width),
            .y = float(randint(0, height * 2) - height),
            .eid = i,
            .r = float(randint(1, 10)) / 10.f,
        };
        entities.push_back(ent);
        dests.push_back(Point({ent.x, ent.y}));
    }
}

void respawn(Entity &e) {
    e.x = float(randint(0, width * 2) - width);
    e.y = float(randint(0, height * 2) - height);
}

float speed = 0.05;
void update_ai_entities() {
    for (int i = 0; i < ai_entities_num; i++) {
        float dir_x = dests[i].x - entities[i].x;
        float dir_y = dests[i].y - entities[i].y;
        float dist = dir_x * dir_x + dir_y * dir_y;
        if (dist < 0.01) {
            dests[i] = Point({float(randint(0, width)), float(randint(0, height))});
        }
        else {
            entities[i].x += dir_x / std::sqrt(dist) * speed;
            entities[i].y += dir_y / std::sqrt(dist) * speed;
        }
    }
}

void collide(ENetHost *server ) {
    for (auto &e1: entities) {
        for (auto &e2: entities) {
            if (e1.eid == e2.eid)
                continue;
            float dir_x = e1.x - e2.x;
            float dir_y = e1.y - e2.y;
            float dist = dir_x * dir_x + dir_y * dir_y;
            float k = 0.2f;
            if (dist < (e1.r + e2.r) * (e1.r + e2.r)) {
                Entity* e;
                if (e1.r > e2.r) {
                    e1.r = min(e1.r + e2.r * k, 3.f);
                    respawn(e2);
                    e = &e2;
                }
                else {
                    e2.r = min(e2.r + e1.r * k, 3.f);
                    respawn(e1);
                    e = &e1;
                }

                if (e->eid >= ai_entities_num) {
                    e->respawning = true;
                    for (size_t i = 0; i < server->peerCount; ++i) {
                        ENetPeer *peer = &server->peers[i];
                        send_entity_state(peer, e->eid, e->x, e->y);
                    }
                }
            }
        }
    }
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host) {
    // send all entities
    for (const Entity &ent: entities)
        send_new_entity(peer, ent);

    // find max eid
    uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
    for (const Entity &e: entities)
        maxEid = max(maxEid, e.eid);
    uint16_t newEid = maxEid + 1;
    uint32_t color = 0xff000000 +
                     0x00440000 * (rand() % 5) +
                     0x00004400 * (rand() % 5) +
                     0x00000044 * (rand() % 5);
    float x = (rand() % 4) * 2.f;
    float y = (rand() % 4) * 2.f;
    Entity ent{.color = color, .x = x, .y = y, .eid = newEid};
    entities.push_back(ent);

    controlledMap[newEid] = peer;


    // send info about new entity to everyone
    for (size_t i = 0; i < host->peerCount; ++i)
        send_new_entity(&host->peers[i], ent);
    // send info about controlled entity
    send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet) {
    uint16_t eid = invalid_entity;
    float x = 0.f;
    float y = 0.f;
    deserialize_entity_state(packet, eid, x, y);
    for (Entity &e: entities)
        if (e.eid == eid && !e.respawning) {
            e.x = x;
            e.y = y;
        }
}

void on_respawn(ENetPacket *packet) {
    uint16_t eid;
    deserialize_spawn(packet, eid);
    for (Entity &e: entities)
        if (e.eid == eid ) {
            e.respawning = false;
        }
}

int main(int argc, const char **argv) {
    create_ai_entities();

    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10131;

    ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server) {
        printf("Cannot create ENet server\n");
        return 1;
    }

    while (true) {
        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    switch (get_packet_type(event.packet)) {
                        case E_CLIENT_TO_SERVER_JOIN:
                            on_join(event.packet, event.peer, server);
                            break;
                        case E_STATE:
                            on_state(event.packet);
                            break;
                        case E_RESPAWN:
                            on_respawn(event.packet);
                    };
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
            };
        }
        static int t = 0;
        for (Entity &e: entities)
            for (size_t i = 0; i < server->peerCount; ++i) {
                ENetPeer *peer = &server->peers[i];
                send_snapshot(peer, e.eid, e.x, e.y, e.r);
            }
        update_ai_entities();
        collide(server);
        Sleep(100000);
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}


