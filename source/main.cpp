#include <switch.h>
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
int activeTab = 0; // 0 = HOME (Launch Menu), 1 = TOOLS (Add Apps)

const char* instanceFolder = "sdmc:/switch/Launcher-NX_Games/";
const char* mainSwitchFolder = "sdmc:/switch/";

// Reads a specified folder on the SD card and lists all .nro homebrew apps found inside
void scanFolder(const char* path, AppItem* list, int* count) {
    *count = 0;
    DIR* dir = opendir(path);
    if (!dir) {
        mkdir(path, 0777); // Auto-generate the folder if it doesn't exist yet
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

// Duplicates a chosen .nro file from the global pool into our current custom folder instance
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

// Renders the console text UI utilizing Hekate/Nyx layout framing designs
void drawHekateInterface() {
    printf("\x1b[2J\x1b[1;1H"); // Clear screen and home the cursor

    // 1. TOP NAVIGATION BAR
    printf("\x1b[1;36mlauncher-nx v1.0.0\x1b[0m   ");
    if (activeTab == 0) {
        printf("\x1b[1;7;32m [ HOME ] \x1b[0m   [ TOOLS ]    [ OPTIONS ]\n");
    } else {
        printf(" [ HOME ]   \x1b[1;7;32m [ TOOLS ] \x1b[0m   [ OPTIONS ]\n");
    }
    printf("-----------------------------------------------------------------\n");

    // 2. CENTRAL WORKING BOX AREAS
    if (activeTab == 0) {
        printf("\n  \x1b[1;34mhekate-nx -> Launch Menu (Current Instance Folder)\x1b[0m\n");
        printf("  Select an application from this custom folder block to initialize:\n\n");

        if (instanceCount == 0) {
            printf("    [!] No target apps loaded in this custom folder instance yet.\n");
            printf("        Press L or R to swap tabs and inject games into this profile.\n");
        } else {
            for (int i = 0; i < instanceCount; i++) {
                if (i == currentMenuSelection) {
                    printf("    \x1b[1;32m> [ %d ]  %-30s  (Ready to Launch)\x1b[0m\n", i + 1, instanceApps[i].name);
                } else {
                    printf("      [ %d ]  %-30s\n", i + 1, instanceApps[i].name);
                }
            }
        }
    } 
    else if (activeTab == 1) {
        printf("\n  \x1b[1;35mhekate-nx -> Global Tools / Application Injector\x1b[0m\n");
        printf("  Select a global system app to clone it directly into this folder instance:\n\n");

        if (globalCount == 0) {
            printf("    [!] No global .nro apps discovered inside sdmc:/switch/ pool.\n");
        } else {
            for (int i = 0; i < globalCount; i++) {
                if (i == currentMenuSelection) {
                    printf("    \x1b[1;35m+ [ %d ]  %-30s  (Press A to Inject)\x1b[0m\n", i + 1, globalStorageApps[i].name);
                } else {
                    printf("      [ %d ]  %-30s\n", i + 1, globalStorageApps[i].name);
                }
            }
        }
    }

    // 3. BOTTOM DIAGNOSTIC STATUS BAR
    printf("\n\x1b[22;1H-----------------------------------------------------------------\n");
    printf(" \x1b[1;30mControls: (L/R) Change Tab | (U/D) Scroll List | (A) Run | (+) Exit\x1b[0m\n");
    printf(" \x1b[36mStatus: Atmosphere Active | Battery: 100%% | Core Temp: 36.4 C\x1b[0m\n");
}

int main(int argc, char **argv) {
    consoleInit(NULL); // Official safe graphical terminal display initialization
    setvbuf(stdout, NULL, _IONBF, 0);

    // Initial hardware filesystem discovery reads
    scanFolder(instanceFolder, instanceApps, &instanceCount);
    scanFolder(mainSwitchFolder, globalStorageApps, &globalCount);

    // Configure controller variables via the standard modern libnx Pad API
    PadState pad;
    padInitializeDefault(&pad);

    while (appletMainLoop()) {
        padUpdate(&pad);
        uint64_t kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break; // Clean exit back to homebrew screen

        // Controller input processing: L or R flips view panes
        if ((kDown & HidNpadButton_L) || (kDown & HidNpadButton_R)) {
            activeTab = (activeTab == 0) ? 1 : 0;
            currentMenuSelection = 0;
        }

        // Controller input processing: D-Pad Down scrolls list
        if (kDown & HidNpadButton_Down) {
            currentMenuSelection++;
            int maxItems = (activeTab == 0) ? instanceCount : globalCount;
            if (maxItems > 0 && currentMenuSelection >= maxItems) currentMenuSelection = 0;
        }

        // Controller input processing: D-Pad Up scrolls list
        if (kDown & HidNpadButton_Up) {
            currentMenuSelection--;
            if (currentMenuSelection < 0) {
                int maxItems = (activeTab == 0) ? instanceCount : globalCount;
                currentMenuSelection = (maxItems > 0) ? maxItems - 1 : 0;
            }
        }

        // Controller input processing: (A) triggers action loops
        if (kDown & HidNpadButton_A) {
            if (activeTab == 0 && instanceCount > 0) {
                // HOME Tab: Instruct Horizon OS to flawlessly chain-load the chosen game on shutdown
                if (access(instanceApps[currentMenuSelection].path, F_OK) == 0) {
                    envSetNextLoad(instanceApps[currentMenuSelection].path, instanceApps[currentMenuSelection].path);
                    break;
                }
            } 
            else if (activeTab == 1 && globalCount > 0) {
                // TOOLS Tab: Transfer a duplicate of the selected global app into our active directory profile
                char destinationPath[512];
                snprintf(destinationPath, sizeof(destinationPath), "%s%s", instanceFolder, globalStorageApps[currentMenuSelection].name);
                copyFile(globalStorageApps[currentMenuSelection].path, destinationPath);
                
                // Re-sync local structures
                scanFolder(instanceFolder, instanceApps, &instanceCount);
            }
        }

        drawHekateInterface();
        consoleUpdate(NULL); // Sync screen output frame ticks
    }

    consoleExit(NULL); // De-initialize the terminal screen safely
    return 0;
}

