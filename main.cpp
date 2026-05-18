#include <wayland-server-core.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <vector>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
namespace fs = std::filesystem;
struct AppEntry {
    std::string name;
    std::string exec;
    std::string icon;
    std::string terminal;
};
std::vector<AppEntry> parseDesktopFiles() {
    std::vector<AppEntry> apps;
    std::vector<std::string> searchPaths = {
        "/usr/share/applications",
        std::string(getenv("HOME")) + "/.local/share/applications"
    };
    std::regex nameRegex("^Name=(.*)$");
    std::regex execRegex("^Exec=(.*)$");
    std::regex iconRegex("^Icon=(.*)$");
    std::regex termRegex("^Terminal=(.*)$");
    for (const auto& path : searchPaths) {
        if (!fs::exists(path)) continue;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.path().extension() == ".desktop") {
                std::ifstream file(entry.path());
                std::string line;
                AppEntry app;
                while (std::getline(file, line)) {
                    std::smatch match;
                    if (std::regex_search(line, match, nameRegex) && app.name.empty()) app.name = match[1];
                    if (std::regex_search(line, match, execRegex) && app.exec.empty()) app.exec = match[1];
                    if (std::regex_search(line, match, iconRegex) && app.icon.empty()) app.icon = match[1];
                    if (std::regex_search(line, match, termRegex) && app.terminal.empty()) app.terminal = match[1];
                }
                if (!app.name.empty() && !app.exec.empty()) {
                    apps.push_back(app);
                }
            }
        }
    }
    return apps;
}
void generateAppsJson(const std::vector<AppEntry>& apps) {
    std::ofstream out("ui/apps.json");
    if (!out.is_open()) return;
    out << "[\n";
    for (size_t i = 0; i < apps.size(); ++i) {
        out << "  {\n"
            << "    \"name\": \"" << apps[i].name << "\",\n"
            << "    \"exec\": \"" << apps[i].exec << "\",\n"
            << "    \"icon\": \"" << apps[i].icon << "\",\n"
            << "    \"terminal\": \"" << (apps[i].terminal == "true" ? "true" : "false") << "\"\n"
            << "  }" << (i == apps.size() - 1 ? "" : ",") << "\n";
    }
    out << "]\n";
    std::cout << "Successfully generated ui/apps.json with " << apps.size() << " native apps." << std::endl;
}
std::string getSystemStats() {
    std::string capacity = "100";
    std::string status = "Charging";
    std::ifstream batCap("/sys/class/power_supply/BAT0/capacity");
    if (batCap.is_open()) std::getline(batCap, capacity);
    std::ifstream batStat("/sys/class/power_supply/BAT0/status");
    if (batStat.is_open()) std::getline(batStat, status);
    return "{ \"battery\": " + capacity + ", \"status\": \"" + status + "\" }";
}
void startIpcServer() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "Failed to create IPC socket." << std::endl;
        return;
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "IPC bind failed on port 8080." << std::endl;
        return;
    }
    if (listen(server_fd, 3) < 0) {
        std::cerr << "IPC listen failed." << std::endl;
        return;
    }
    std::cout << "Tiwut-ENV IPC Server listening on http:
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
                while ((pos = appExec.find("%20")) != std::string::npos) {
                    appExec.replace(pos, 3, " ");
                }
                std::cout << "[IPC] Launching native application: " << appExec << std::endl;
                pid_t pid = fork();
                if (pid == 0) {
                    execl("/bin/sh", "sh", "-c", appExec.c_str(), nullptr);
                    exit(0);
                }
            }
        }
        else if (request.find("GET /stats ") == 0) {
            std::string stats = getSystemStats();
            std::string response = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n" + stats;
            write(new_socket, response.c_str(), response.length());
            close(new_socket);
            continue;
        }
        std::string response = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/plain\r\n\r\nOK";
        write(new_socket, response.c_str(), response.length());
        close(new_socket);
    }
}
int main() {
    std::cout << "---------------------------------------------------" << std::endl;
    std::cout << "Tiwut-ENV Backend Booting..." << std::endl;
    std::vector<AppEntry> systemApps = parseDesktopFiles();
    generateAppsJson(systemApps);
    std::thread ipcThread(startIpcServer);
    ipcThread.detach();
    struct wl_display *display = wl_display_create();
    if (!display) {
        std::cerr << "Failed to create Wayland display." << std::endl;
        return 1;
    }
    const char *socket = wl_display_add_socket_auto(display);
    if (!socket) {
        std::cerr << "Failed to add Wayland socket." << std::endl;
        wl_display_destroy(display);
        return 1;
    }
    std::cout << "Tiwut-ENV is listening on WAYLAND_DISPLAY=" << socket << std::endl;
    std::cout << "WebRTC / WebSocket Bridge is ready for UI connections (Stub)." << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;
    wl_display_run(display);
    wl_display_destroy(display);
    return 0;
}
