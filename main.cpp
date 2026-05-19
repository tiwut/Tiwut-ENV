#include <wayland-server-core.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <memory>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define namespace _namespace
#define class _class
extern "C" {
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>
}
#undef namespace
#undef class

struct Server {
    wl_display* display;
    wlr_backend* backend;
    wlr_renderer* renderer;
    wlr_allocator* allocator;
    wlr_scene* scene;
    wlr_scene_tree* scene_layout;
    wlr_scene_output_layout* scene_output_layout;
    wlr_output_layout* output_layout;

    wlr_xdg_shell* xdg_shell;
    wlr_seat* seat;
    wlr_cursor* cursor;
    wlr_xcursor_manager* cursor_mgr;

    wl_listener new_output;
    wl_listener new_xdg_surface;
    wl_listener new_input;
    wl_listener cursor_motion;
    wl_listener cursor_motion_absolute;
    wl_listener cursor_button;
    wl_listener cursor_axis;
};

struct View {
    Server* server;
    wlr_xdg_toplevel* toplevel;
    wlr_scene_tree* scene_tree;
    wl_listener map;
    wl_listener unmap;
    wl_listener destroy;
};

static void focus_view(View* view, wlr_surface* surface) {
    if (view == nullptr) return;
    wlr_seat* seat = view->server->seat;
    wlr_surface* prev_surface = seat->keyboard_state.focused_surface;
    if (prev_surface == surface) return;
    if (prev_surface) {
        wlr_xdg_toplevel* prev_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);
        if (prev_toplevel) wlr_xdg_toplevel_set_activated(prev_toplevel, false);
    }
    wlr_scene_node_raise_to_top(&view->scene_tree->node);
    wlr_keyboard* keyboard = wlr_seat_get_keyboard(seat);
    
    wlr_scene_node_set_position(&view->scene_tree->node, 50, 50);
    
    wlr_xdg_toplevel_set_activated(view->toplevel, true);
    if (keyboard) {
        wlr_seat_keyboard_notify_enter(seat, surface, keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
    }
}

static void xdg_toplevel_map(wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, map);
    focus_view(view, view->toplevel->base->surface);
}

static void xdg_toplevel_unmap(wl_listener* listener, void* data) {
}

static void xdg_toplevel_destroy(wl_listener* listener, void* data) {
    View* view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->map.link);
    wl_list_remove(&view->unmap.link);
    wl_list_remove(&view->destroy.link);
    delete view;
}

static void server_new_xdg_surface(wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_xdg_surface);
    wlr_xdg_surface* xdg_surface = static_cast<wlr_xdg_surface*>(data);

    if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        View* view = new View();
        view->server = server;
        view->toplevel = xdg_surface->toplevel;
        view->scene_tree = wlr_scene_xdg_surface_create(&server->scene_layout->tree, xdg_surface);
        view->scene_tree->node.data = view;

        view->map.notify = xdg_toplevel_map;
        wl_signal_add(&xdg_surface->surface->events.map, &view->map);
        view->unmap.notify = xdg_toplevel_unmap;
        wl_signal_add(&xdg_surface->surface->events.unmap, &view->unmap);
        view->destroy.notify = xdg_toplevel_destroy;
        wl_signal_add(&xdg_surface->events.destroy, &view->destroy);
    }
}

static void server_new_output(wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_output);
    wlr_output* wlr_output = static_cast<struct wlr_output*>(data);

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);
    
    wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    
    wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output);
    if (mode != nullptr) wlr_output_state_set_mode(&state, mode);
    
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    wlr_output_layout_add_auto(server->output_layout, wlr_output);
    wlr_scene_output_create(server->scene, wlr_output);
}

static void server_new_input(wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, new_input);
    wlr_input_device* device = static_cast<wlr_input_device*>(data);

    if (device->type == WLR_INPUT_DEVICE_POINTER) {
        wlr_cursor_attach_input_device(server->cursor, device);
    } else if (device->type == WLR_INPUT_DEVICE_KEYBOARD) {
        wlr_keyboard* keyboard = wlr_keyboard_from_input_device(device);
        xkb_context* context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        xkb_keymap* keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
        wlr_keyboard_set_keymap(keyboard, keymap);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        wlr_seat_set_keyboard(server->seat, keyboard);
    }
    
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD;
    wlr_seat_set_capabilities(server->seat, caps);
}

static void server_cursor_motion(wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, cursor_motion);
    wlr_pointer_motion_event* event = static_cast<wlr_pointer_motion_event*>(data);
    wlr_cursor_move(server->cursor, &event->pointer->base, event->delta_x, event->delta_y);
}

static void server_cursor_motion_absolute(wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, cursor_motion_absolute);
    wlr_pointer_motion_absolute_event* event = static_cast<wlr_pointer_motion_absolute_event*>(data);
    wlr_cursor_warp_absolute(server->cursor, &event->pointer->base, event->x, event->y);
}

static void server_cursor_button(wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, cursor_button);
    wlr_pointer_button_event* event = static_cast<wlr_pointer_button_event*>(data);
    wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
}

static void server_cursor_axis(wl_listener* listener, void* data) {
    Server* server = wl_container_of(listener, server, cursor_axis);
    wlr_pointer_axis_event* event = static_cast<wlr_pointer_axis_event*>(data);
    wlr_seat_pointer_notify_axis(server->seat, event->time_msec, event->orientation, event->delta, event->delta_discrete, event->source, event->relative_direction);
}

void startIpcServer() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) return;
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) return;
    if (listen(server_fd, 3) < 0) return;

    while (true) {
        int new_socket = accept(server_fd, nullptr, nullptr);
        if (new_socket < 0) continue;
        char buffer[1024] = {0};
        read(new_socket, buffer, 1024);
        std::string request(buffer);

        if (request.find("GET /launch?app=") == 0) {
            size_t start = 16;
            size_t end = request.find(" HTTP", start);
            if (end != std::string::npos) {
                std::string appExec = request.substr(start, end - start);
                size_t pos;
                while ((pos = appExec.find("%20")) != std::string::npos) appExec.replace(pos, 3, " ");
                pid_t pid = fork();
                if (pid == 0) {
                    execl("/bin/sh", "sh", "-c", appExec.c_str(), nullptr);
                    exit(0);
                }
            }
        }
        std::string response = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nOK";
        write(new_socket, response.c_str(), response.length());
        close(new_socket);
    }
}

int main() {
    wlr_log_init(WLR_DEBUG, NULL);
    std::cout << "Starting Tiwut-ENV Native Wayland Bridge..." << std::endl;

    std::thread ipcThread(startIpcServer);
    ipcThread.detach();

    Server server = {0};
    server.display = wl_display_create();
    server.backend = wlr_backend_autocreate(wl_display_get_event_loop(server.display), nullptr);
    server.renderer = wlr_renderer_autocreate(server.backend);
    wlr_renderer_init_wl_display(server.renderer, server.display);
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);

    wlr_compositor_create(server.display, 5, server.renderer);
    wlr_subcompositor_create(server.display);
    wlr_data_device_manager_create(server.display);

    server.output_layout = wlr_output_layout_create(server.display);
    server.scene = wlr_scene_create();
    server.scene_layout = wlr_scene_tree_create(&server.scene->tree);
    server.scene_output_layout = wlr_scene_attach_output_layout(server.scene, server.output_layout);

    server.new_output.notify = server_new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    server.xdg_shell = wlr_xdg_shell_create(server.display, 3);
    server.new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server.xdg_shell->events.new_surface, &server.new_xdg_surface);

    server.seat = wlr_seat_create(server.display, "seat0");
    server.cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
    server.cursor_mgr = wlr_xcursor_manager_create(nullptr, 24);

    server.new_input.notify = server_new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input);

    server.cursor_motion.notify = server_cursor_motion;
    wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
    server.cursor_motion_absolute.notify = server_cursor_motion_absolute;
    wl_signal_add(&server.cursor->events.motion_absolute, &server.cursor_motion_absolute);
    server.cursor_button.notify = server_cursor_button;
    wl_signal_add(&server.cursor->events.button, &server.cursor_button);
    server.cursor_axis.notify = server_cursor_axis;
    wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);

    const char* socket = wl_display_add_socket_auto(server.display);
    if (!socket) return 1;

    std::cout << "===================================================" << std::endl;
    std::cout << "WAYLAND_DISPLAY=" << socket << std::endl;
    std::cout << "Bridge is fully operational! Starting UI Shell..." << std::endl;
    std::cout << "===================================================" << std::endl;

    setenv("WAYLAND_DISPLAY", socket, true);
    
    // Auto-launch the HTML shell UI fullscreen in the background
    if (fork() == 0) {
        char ui_path[1024];
        getcwd(ui_path, sizeof(ui_path));
        std::string cmd = "chromium --kiosk --app=file://" + std::string(ui_path) + "/ui/index.html &";
        execl("/bin/sh", "sh", "-c", cmd.c_str(), nullptr);
        exit(0);
    }

    if (!wlr_backend_start(server.backend)) {
        wlr_backend_destroy(server.backend);
        wl_display_destroy(server.display);
        return 1;
    }

    wl_display_run(server.display);
    wl_display_destroy_clients(server.display);
    wl_display_destroy(server.display);
    return 0;
}
