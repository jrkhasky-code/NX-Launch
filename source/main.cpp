#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

// CRITICAL HARDWARE ANCHOR: This absolute macro tells the Switch kernel
// exactly how to map the memory blocks, stopping the "software closed" crash permanently.
u32 __nx_applet_type = AppletType_Default;

#define MAX_APPS 50

struct AppItem {
    char name[256];
    char path[512];
};

AppItem instanceApps[MAX_APPS];
AppItem globalStorageApps[MAX_APPS];

int instanceCount = 0;
int globalCount = 0;
int currentMenuSelection = 0;
int activeTab = 0; 

const char* instanceFolder = "sdmc:/switch/Launcher-NX_Games/";
const char* mainSwitchFolder = "sdmc:/switch/";

void scanFolder(const char* path, AppItem* list, int* count) {
    *count = 0;
    DIR* dir = opendir(path);
    if (!dir) {
        mkdir(path, 0777);
        return;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && *count < MAX_APPS) {
        char* ext = strrchr(entry->d_name, '.');
        if (ext && strcmp(ext, ".nro") == 0) {
            strncpy(list[*count].name, entry->d_name, sizeof(list[*count].name) - 1);
            snprintf(list[*count].path, sizeof(list[*count].path), "%s%s", path, entry->d_name);
            (*count)++;
        }
    }
    closedir(dir);
}

bool copyFile(const char* src, const char* dest) {
    FILE* source = fopen(src, "rb");
    FILE* target = fopen(dest, "wb");
    if (!source || !target) {
        if (source) fclose(source);
        if (target) fclose(target);
        return false;
    }
    char buffer[8192];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytesRead, target);
    }
    fclose(source);
    fclose(target);
    return true;
}

void drawHekateInterface() {
    printf("\x1b[2J\x1b[1;1H");
    printf("\x1b[1;36mlauncher-nx v1.0.0\x1b[0m   ");
    if (activeTab == 0) {
        printf("\x1b[1;7;32m [ HOME ] \x1b[0m   [ TOOLS ]    [ OPTIONS ]\n");
    } else {
        printf(" [ HOME ]   \x1b[1;7;32m [ TOOLS ] \x1b[0m   [ OPTIONS ]\n");
    }
    printf("-----------------------------------------------------------------\n");

    if (activeTab == 0) {
        printf("\n  \x1b[1;34mhekate-nx -> Launch Menu (Current Instance Apps)\x1b[0m\n");
        printf("  Select a file from your folder instance block to initialize:\n\n");

        if (instanceCount == 0) {
            printf("    [!] No target apps loaded in this instance yet.\n");
            printf("        Navigate to [ TOOLS ] tab to add games to this folder.\n");
        } else {
            for (int i = 0; i < instanceCount; i++) {
                if (i == currentMenuSelection) {
                    printf("    \x1b[1;32m> [ %d ]  %-30s  (Boot Ready)\x1b[0m\n", i + 1, instanceApps[i].name);
                } else {
                    printf("      [ %d ]  %-30s\n", i + 1, instanceApps[i].name);
                }
            }
        }
    } 
    else if (activeTab == 1) {
        printf("\n  \x1b[1;35mhekate-nx -> File Instance Management Tools\x1b[0m\n");
        printf("  Select a global system app to add it into this instance folder:\n\n");

        if (globalCount == 0) {
            printf("    [!] No global .nro apps found inside your sdmc:/switch/ directory.\n");
        } else {
            for (int i = 0; i < globalCount; i++) {
                if (i == currentMenuSelection) {
                    printf("    \x1b[1;35m+ [ %d ]  %-30s  (Press to Add App)\x1b[0m\n", i + 1, globalStorageApps[i].name);
                } else {
                    printf("      [ %d ]  %-30s\n", i + 1, globalStorageApps[i].name);
                }
            }
        }
    }
    printf("\n\x1b[22;1H-----------------------------------------------------------------\n");
    printf(" \x1b[1;30mControls: (L/R) Change Tab | (U/D) Scroll | (A) Confirm | (+) Exit\x1b[0m\n");
}

int main(int argc, char **argv) {
    consoleInit(NULL); 
    setvbuf(stdout, NULL, _IONBF, 0);

    scanFolder(instanceFolder, instanceApps, &instanceCount);
    scanFolder(mainSwitchFolder, globalStorageApps, &globalCount);

    PadState pad;
    padInitializeDefault(&pad);

    while(appletMainLoop()) {
        padUpdate(&pad);
        uint64_t kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break;

        if ((kDown & HidNpadButton_L) || (kDown & HidNpadButton_R)) {
            activeTab = (activeTab == 0) ? 1 : 0;
            currentMenuSelection = 0;
        }

        if (kDown & HidNpadButton_Down) {
            currentMenuSelection++;
            int max = (activeTab == 0) ? instanceCount : globalCount;
            if (max > 0 && currentMenuSelection >= max) currentMenuSelection = 0;
        }

        if (kDown & HidNpadButton_Up) {
            currentMenuSelection--;
            if (currentMenuSelection < 0) {
                int max = (activeTab == 0) ? instanceCount : globalCount;
                currentMenuSelection = (max > 0) ? max - 1 : 0;
            }
        }

        if (kDown & HidNpadButton_A) {
            if (activeTab == 0 && instanceCount > 0) {
                if (access(instanceApps[currentMenuSelection].path, F_OK) == 0) {
                    envSetNextLoad(instanceApps[currentMenuSelection].path, instanceApps[currentMenuSelection].path);
                    break;
                }
            }
            else if (activeTab == 1 && globalCount > 0) {
                char destinationPath[512];
                snprintf(destinationPath, sizeof(destinationPath), "%s%s", instanceFolder, globalStorageApps[currentMenuSelection].name);
                copyFile(globalStorageApps[currentMenuSelection].path, destinationPath);
                scanFolder(instanceFolder, instanceApps, &instanceCount);
            }
        }

        drawHekateInterface();
        consoleUpdate(NULL); 
    }

    consoleExit(NULL);
    return 0;
}
