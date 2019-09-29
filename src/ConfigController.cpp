#include "ConfigController.h"

ConfigController * ConfigController::instance = nullptr;

ConfigController * ConfigController::getInstance() {
    if (instance == nullptr) {
        instance = new ConfigController();
    }
    return instance;
}

void ConfigController::loadConfig() {
    TCHAR szPath[MAX_PATH];
    SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, szPath);
    PathAppendA(szPath, TEXT("OWStreamRecordEx"));
    struct stat buffer{};
    if (stat(szPath, &buffer) != 0)
        CreateDirectory(szPath, nullptr);
    PathAppendA(szPath, TEXT("config.json"));
    if (stat(szPath, &buffer) != 0) {
        HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(101), "TEXT");
        if (hResource) {
            HGLOBAL hLoadedResource = LoadResource(nullptr, hResource);
            if (hLoadedResource) {
                LPVOID pLockedResource = LockResource(hLoadedResource);
                if (pLockedResource) {
                    DWORD dwResourceSize = SizeofResource(nullptr, hResource);
                    if (0 != dwResourceSize) {
                        ofstream out(szPath);
                        out << (char *)pLockedResource;
                        out.close();
                    }
                }
            }
        }
    } else {
        ifstream in(szPath);
        json j = json::parse(in);
        config.delay = j["delay"];
        config.obs = j["obs"];
        config.path = j["path"];
        config.preview = j["preview"];
        config.placeholder = j["placeholder"];
        //printf("delay: %x\nobs: %x\npath: %s\npreview: %x\nplaceholder: %s\n", config.delay, config.obs, config.path.c_str(), config.preview, config.placeholder.c_str());
    }
}

void ConfigController::saveConfig() {
    TCHAR szPath[MAX_PATH];
    SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, szPath);
    PathAppendA(szPath, TEXT("OWStreamRecordEx"));
    struct stat buffer{};
    if (stat(szPath, &buffer) != 0)
        CreateDirectory(szPath, nullptr);
    PathAppendA(szPath, TEXT("config.json"));
    if (stat(szPath, &buffer) != 0) {
        HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(101), "TEXT");
        if (hResource) {
            HGLOBAL hLoadedResource = LoadResource(nullptr, hResource);
            if (hLoadedResource) {
                LPVOID pLockedResource = LockResource(hLoadedResource);
                if (pLockedResource) {
                    DWORD dwResourceSize = SizeofResource(nullptr, hResource);
                    if (0 != dwResourceSize) {
                        ofstream out(szPath);
                        out << (char *)pLockedResource;
                        out.close();
                    }
                }
            }
        }
    } else {
        json j;
        j["delay"] = config.delay;
        j["obs"] = config.obs;
        j["path"] = config.path;
        j["preview"] = config.preview;
        j["placeholder"] = config.placeholder;
        ofstream out(szPath);
        out << j.dump();
        out.close();
    }
}

ConfigController::ConfigController() = default;
