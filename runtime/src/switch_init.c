// SPDX-License-Identifier: AGPL-3.0-only
#include <switch.h>
#include <stdlib.h>
#include <stdio.h>

// Parsing the official minified modules needs more than libnx's default stack.
u32 __nx_applet_main_thread_stack_size = 8 * 1024 * 1024;

void userAppInit(void) {
    if (appletGetAppletType() != AppletType_Application && appletGetAppletType() != AppletType_SystemApplication) {
        consoleInit(NULL);
        printf("PokeRogue Native Test needs full application memory.\n\n");
        printf("Open hbmenu using title override (hold R while launching a game),\nthen launch this test again.\n\nPress + to return.\n");
        PadState pad;
        padConfigureInput(1, HidNpadStyleSet_NpadStandard);
        padInitializeDefault(&pad);
        while (appletMainLoop()) {
            padUpdate(&pad);
            if (padGetButtonsDown(&pad) & HidNpadButton_Plus) break;
            consoleUpdate(NULL);
        }
        consoleExit(NULL);
        exit(0);
    }
    Result rc = romfsInit();
    if (R_FAILED(rc)) fatalThrow(rc);
    // The offline game itself needs no network, but curl must have a valid
    // socket service when a user follows an external link in the runtime.
    socketInitializeDefault();
}

void userAppExit(void) {
    socketExit();
    romfsExit();
}
