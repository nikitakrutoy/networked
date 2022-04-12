// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <bx/bx.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/timer.h>
#include <debugdraw/debugdraw.h>
#include <functional>
#include "app.h"
#include <enet/enet.h>
#include <math.h>

//for scancodes
#include <GLFW/glfw3.h>


#include <vector>
#include <deque>
#include <map>
#include "entity.h"
#include "protocol.h"

struct Snapshot {
    uint16_t eid;
    float x;
    float y;
    float ori;
    uint32_t time;
};

static std::map<uint16_t , std::deque<Snapshot>> snapshots;
static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;

static std::map<uint16_t, Snapshot> prev_snapshot;
static std::map<uint16_t, Snapshot> current_snapshot;

static std::map<uint16_t, uint32_t> start_client_time;

bool INTERPOLATION_ON = false;

void on_new_entity_packet(ENetPacket *packet) {
    Entity newEntity;
    deserialize_new_entity(packet, newEntity);
    // TODO: Direct adressing, of course!
    for (const Entity &e: entities)
        if (e.eid == newEntity.eid)
            return; // don't need to do anything, we already have entity
    entities.push_back(newEntity);
    if (INTERPOLATION_ON) {
        Entity e = newEntity;
        start_client_time[e.eid] = enet_time_get();
        current_snapshot[e.eid] = Snapshot({e.eid,e.x,e.y, e.ori, start_client_time[e.eid]});
        prev_snapshot[e.eid] = current_snapshot[e.eid];
        prev_snapshot[e.eid].time = current_snapshot[e.eid].time - 1;
    }


}

void on_set_controlled_entity(ENetPacket *packet) {
    deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetEvent *event) {
    uint16_t eid = invalid_entity;
    float x = 0.f;
    float y = 0.f;
    float ori = 0.f;
    deserialize_snapshot(event->packet, eid, x, y, ori);
    if (INTERPOLATION_ON)
        snapshots[eid].push_back({
            eid, x, y, ori, enet_time_get() - event->peer->roundTripTime
        });
    else
        for (Entity &e: entities)
            if (e.eid == my_entity) {
                e.x = x;
                e.y = y;
                e.ori = ori;
            }
}


int main(int argc, const char **argv) {
    if (enet_initialize() != 0) {
        printf("Cannot init ENet");
        return 1;
    }

    ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!client) {
        printf("Cannot create ENet client\n");
        return 1;
    }

    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = 20131;

    ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
    if (!serverPeer) {
        printf("Cannot connect to server");
        return 1;
    }

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

    bool keyGate = false;

    while (!app_should_close()) {
        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                    send_join(serverPeer);
                    connected = true;
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    switch (get_packet_type(event.packet)) {
                        case E_SERVER_TO_CLIENT_NEW_ENTITY:
                            on_new_entity_packet(event.packet);
                            break;
                        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
                            on_set_controlled_entity(event.packet);
                            break;
                        case E_SERVER_TO_CLIENT_SNAPSHOT:
                            on_snapshot(&event);
                            break;
                        default:
                            break;
                    };
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

            if (app_keypressed(GLFW_KEY_I) && !keyGate && !INTERPOLATION_ON) {
                INTERPOLATION_ON = true;
            }

            if (!app_keypressed(GLFW_KEY_I) && !keyGate && INTERPOLATION_ON) {
                keyGate = true;
            }

            if (app_keypressed(GLFW_KEY_I) && keyGate && INTERPOLATION_ON) {
                INTERPOLATION_ON = false;
            }

            if (!app_keypressed(GLFW_KEY_I) && keyGate && !INTERPOLATION_ON) {
                keyGate = false;
            }

            // TODO: Direct adressing, of course!
            for (Entity &e: entities) {
                if (e.eid == my_entity) {
                    // Update
                    float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
                    float steer = (left ? 1.f : 0.f) + (right ? -1.f : 0.f);

                    // Send
                    send_entity_input(serverPeer, my_entity, thr, steer);

                }

                if (!INTERPOLATION_ON)
                    continue;
                
                // Interpolation
                uint32_t current_client_time = enet_time_get();
                float alpha = float(current_client_time - start_client_time[e.eid]) /
                              float(current_snapshot[e.eid].time - prev_snapshot[e.eid].time);

                if (alpha <= 1) {
                    e.x = prev_snapshot[e.eid].x * (1 - alpha) + current_snapshot[e.eid].x * alpha;
                    e.y = prev_snapshot[e.eid].y * (1 - alpha) + current_snapshot[e.eid].y * alpha;
                    e.ori = prev_snapshot[e.eid].ori * (1 - alpha) + current_snapshot[e.eid].ori * alpha;
                }
                if (!snapshots[e.eid].empty()) {
                    start_client_time[e.eid] = enet_time_get();
                    prev_snapshot[e.eid] = current_snapshot[e.eid];
                    current_snapshot[e.eid] = snapshots[e.eid].front();
                    snapshots[e.eid].pop_front();
                }
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
            bx::Vec3 dir = {cosf(e.ori), sinf(e.ori), 0.f};
            bx::Vec3 pos = {e.x, e.y, -0.01f};
            dde.drawCapsule(sub(pos, dir), add(pos, dir), 1.f);

            dde.pop();
        }

        dde.end();
        // Advance to next frame. Process submitted rendering primitives.
        bgfx::frame();
        const double freq = double(bx::getHPFrequency());
        int64_t now = bx::getHPCounter();
        dt = (float) ((now - last));
//        printf("%f\n", dt);
        last = now;
    }
    ddShutdown();
    bgfx::shutdown();
    app_terminate();
    return 0;
}
