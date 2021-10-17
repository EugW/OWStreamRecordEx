#include "ConfigController.h"

ConfigController* ConfigController::instance = nullptr;

ConfigController* ConfigController::getInstance() {
    if (instance == nullptr) {
        instance = new ConfigController();
    }
    return instance;
}

void ConfigController::loadConfig() {
    TCHAR szPath[MAX_PATH];
    SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, szPath);
    PathAppend(szPath, TEXT("OWStreamRecordEx"));
    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES) {
        CreateDirectory(szPath, nullptr);
    }
    PathAppend(szPath, TEXT("config.json"));
    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES) {
        HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(101), TEXT("TEXT"));
        if (hResource) {
            HGLOBAL hLoadedResource = LoadResource(nullptr, hResource);
            if (hLoadedResource) {
                LPVOID pLockedResource = LockResource(hLoadedResource);
                if (pLockedResource) {
                    DWORD dwResourceSize = SizeofResource(nullptr, hResource);
                    if (0 != dwResourceSize) {
                        ofstream out(szPath);
                        out.write((char*)pLockedResource, dwResourceSize);
                        out.close();
                    }
                }
            }
        }
    }
    ifstream in(szPath);
    json j = json::parse(in);
    config.delay = j["delay"];
    config.mode = j["mode"];
    config.path = j["path"];
    config.preview = j["preview"];
    config.placeholder = j["placeholder"];
    printf("delay: %d\nmode: %d\npath: %s\npreview: %x\nplaceholder: %s\n", config.delay, config.mode, config.path.c_str(), config.preview, config.placeholder.c_str());
}

void ConfigController::saveConfig() {
    TCHAR szPath[MAX_PATH];
    SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, szPath);
    PathAppend(szPath, TEXT("OWStreamRecordEx"));
    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES) {
        CreateDirectory(szPath, nullptr);
    }
    PathAppend(szPath, TEXT("config.json"));
    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES) {
        HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(101), TEXT("TEXT"));
        if (hResource) {
            HGLOBAL hLoadedResource = LoadResource(nullptr, hResource);
            if (hLoadedResource) {
                LPVOID pLockedResource = LockResource(hLoadedResource);
                if (pLockedResource) {
                    DWORD dwResourceSize = SizeofResource(nullptr, hResource);
                    if (0 != dwResourceSize) {
                        ofstream out(szPath);
                        out.write((char*)pLockedResource, dwResourceSize);
                        out.close();
                    }
                }
            }
        }
    }
    json j;
    j["delay"] = config.delay;
    j["mode"] = config.mode;
    j["path"] = config.path;
    j["preview"] = config.preview;
    j["placeholder"] = config.placeholder;
    ofstream out(szPath);
    out << j.dump();
    out.close();
}

ConfigController::ConfigController() = default;
