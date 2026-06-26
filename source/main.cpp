#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

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
    char buffer[4096]; 
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
    setvbuf(stdout, NULL, _IONBF, 0);
    scanFolder(instanceFolder, instanceApps, &instanceCount);
    scanFolder(mainSwitchFolder, globalStorageApps, &globalCount);

    // Completely self-contained interactive simulation logic
    int mockInputs[] = { 1, 2, 3 }; 
    int inputLength = sizeof(mockInputs) / sizeof(mockInputs[0]);

    for (int step = 0; step <= inputLength; step++) {
        drawHekateInterface();
        if (step == 0) { activeTab = 1; currentMenuSelection = 0; }
        else if (step == 1 && globalCount > 0) {
            char destinationPath[1024];
            snprintf(destinationPath, sizeof(destinationPath), "%s%s", instanceFolder, globalStorageApps[currentMenuSelection].name);
            copyFile(globalStorageApps[currentMenuSelection].path, destinationPath);
            scanFolder(instanceFolder, instanceApps, &instanceCount);
        }
        else if (step == 2) { activeTab = 0; currentMenuSelection = 0; }
        usleep(100000);
    }
    return 0;
}
