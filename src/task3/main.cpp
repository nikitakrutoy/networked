// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/timer.h>
#include <bx/bounds.h>
#include <debugdraw/debugdraw.h>
#include <functional>
#include "app.h"
#include <enet/enet.h>
#include <iostream>
#include <future>

//for scancodes
#include <GLFW/glfw3.h>

#include <vector>
#include "entity.h"
#include "protocol.h"


static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;

void on_server_data(ENetPacket *packet, ENetPeer* server, ENetPeer* lobby, ENetHost* host) {
    uint32_t port;
    deserialize_server_data(packet, port);
    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = port;
    server = enet_host_connect(host, &address, 2, 0);
    if (!server) {
        printf("Cannot connect to server localhost:%d", port);
    }

}

void on_lobby_info(ENetPacket *packet) {
    std::vector<RoomInfo> data;
    deserialize_lobby_info(packet, data);
    for (int i = 0; i < data.size(); i++) {
        printf("Room %d, players - %d, ai_num - %d. ", i, data[i].peers_num, data[i].ai_num);
        if (data[i].is_game_started)
            printf("Started.");
        printf("\n");
    }
}

void on_message(ENetPacket *packet) {
    std::string msg;
    deserialize_message(packet, msg);
    printf("%s\n", msg.c_str());
}

void on_new_entity_packet(ENetPacket *packet) {
    Entity newEntity;
    deserialize_new_entity(packet, newEntity);
    for (const Entity &e: entities)
        if (e.eid == newEntity.eid)
            return; // don't need to do anything, we already have entity
    entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket *packet) {
    deserialize_set_controlled_entity(packet, my_entity);
}

void on_state(ENetPacket *packet, ENetPeer *serverPeer) {
    uint16_t eid = invalid_entity;
    float x = 0.f;
    float y = 0.f;
    deserialize_entity_state(packet, eid, x, y);
    for (Entity &e: entities)
        if (e.eid == eid) {
            e.x = x;
            e.y = y;
        }
    send_respawn(serverPeer, eid);
}

void on_snapshot(ENetPacket *packet) {
    uint16_t eid = invalid_entity;
    float x = 0.f;
    float y = 0.f;
    float r = 0.5;
    deserialize_snapshot(packet, eid, x, y, r);
    for (Entity &e: entities)
        if (e.eid == eid) {
            if (e.eid != my_entity) {
                e.x = x;
                e.y = y;
            }
            e.r = r;
        }
}

static std::string get_input()
{
    std::string input;
    getline(std::cin, input);
    return input;
}


int main(int argc, const char **argv) {
    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }

    ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
    if (!client) {
        printf("Cannot create ENet client\n");
        return 1;
    }

    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = 10887;
    ENetPeer *lobbyPeer = nullptr;
    lobbyPeer = enet_host_connect(client, &address, 2, 0);
    if (!lobbyPeer) {
        printf("Cannot connect to lobby");
        return 1;
    }
//    enet_address_set_host(&address, "localhost");
//    address.port = 10890;
    ENetPeer *serverPeer = nullptr;
//    serverPeer = enet_host_connect(client, &address, 2, 0);
//    if (!serverPeer) {
//        printf("Cannot connect to server");
//        return 1;
//    }

    int width = 1920;
    int height = 1080;
    if (!app_init(width, height))
        return 1;
    ddInit();

    bx::Vec3 eye(0.f, 0.f, -16.f);
    bx::Vec3 at(0.f, 0.f, 0.f);
    bx::Vec3 up(0.f, 1.f, 0.f);

    float view[16];
    float proj[16];
    bx::mtxLookAt(view, bx::load<bx::Vec3>(&eye.x), bx::load<bx::Vec3>(&at.x), bx::load<bx::Vec3>(&up.x));

    bool connected = false;
    int64_t now = bx::getHPCounter();
    int64_t last = now;
    float dt = 0.f;
    std::future<std::string> future = std::async(get_input);
    while (!app_should_close()) {
        ENetEvent event;
        std::string command;
        bool is_input_ready = future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        while (enet_host_service(client, &event, 0) > 0 || is_input_ready) {

            if (is_input_ready) {
                std::string command = future.get();
                is_input_ready = false;
                future = std::async(get_input);
                send_text_message(lobbyPeer, command);
            }

            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:

                    if (event.peer == lobbyPeer) {
                        printf("Connection with lobby %x:%u established\n", event.peer->address.host,
                               event.peer->address.port);
                    }
                    else {
                        serverPeer = event.peer;
                        send_join(event.peer);
                        printf("Connection with server %x:%u established\n", event.peer->address.host,
                               event.peer->address.port);
                    }

                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    if (event.peer = serverPeer) {
                        switch (get_packet_type(event.packet)) {
                            case E_SERVER_TO_CLIENT_NEW_ENTITY:
                                on_new_entity_packet(event.packet);
                                break;
                            case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
                                on_set_controlled_entity(event.packet);
                                break;
                            case E_SERVER_TO_CLIENT_SNAPSHOT:
                                on_snapshot(event.packet);
                                break;
                            case E_STATE:
                                on_state(event.packet, serverPeer);
                            default:
                                break;
                        };
                    }
                    if (event.peer = lobbyPeer) {
                        switch (get_packet_type(event.packet)) {
                            case E_SERVER_DATA:
                                on_server_data(event.packet, serverPeer, lobbyPeer, client);
                                break;
                            case E_LOBBY_INFO:
                                on_lobby_info(event.packet);
                            case E_TEXT_MESSAGE:
                                on_message(event.packet);
                            default:
                                break;
                        };
                    }
                    break;
                default:
                    break;
            };
        }
        if (my_entity != invalid_entity) {
            bool left = app_keypressed(GLFW_KEY_LEFT);
            bool right = app_keypressed(GLFW_KEY_RIGHT);
            bool up = app_keypressed(GLFW_KEY_UP);
            bool down = app_keypressed(GLFW_KEY_DOWN);
            for (Entity &e: entities)
                if (e.eid == my_entity) {
                    // Update
                    e.x += ((left ? -dt : 0.f) + (right ? +dt : 0.f)) * 10.f;
                    e.y += ((up ? +dt : 0.f) + (down ? -dt : 0.f)) * 10.f;

                    // Send
                    send_entity_state(serverPeer, my_entity, e.x, e.y);
                }
        }

        app_poll_events();
        // Handle window resize.
        app_handle_resize(width, height);
        bx::mtxProj(proj, 60.0f, float(width) / float(height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
        bgfx::setViewTransform(0, view, proj);

        // This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view 0.
        const bgfx::ViewId kClearView = 0;
        bgfx::touch(kClearView);

        DebugDrawEncoder dde;

        dde.begin(0);

        for (const Entity &e: entities) {
            dde.push();

            dde.setColor(e.color);
            auto sphere = bx::Sphere({bx::Vec3(e.x, e.y, -0.01f), e.r});
            dde.draw(sphere);

            dde.pop();
        }

        dde.end();

        // Advance to next frame. Process submitted rendering primitives.
        bgfx::frame();
        const double freq = double(bx::getHPFrequency());
        int64_t now = bx::getHPCounter();
        dt = (float) ((now - last) / freq);
        last = now;
    }
    ddShutdown();
    bgfx::shutdown();
    app_terminate();
    return 0;
}
