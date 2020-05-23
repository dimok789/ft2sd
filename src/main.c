#include <vpad/input.h>
#include <proc_ui/procui.h>
#include <sysapp/launch.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <coreinit/foreground.h>
#include <coreinit/screen.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <limits.h>
#include <iosuhax_devoptab.h>
#include <iosuhax.h>
#include "sd_dumper.h"
#include "console.h"

int CheckCancel(void)
{
    VPADReadError vpadError = -1;
    VPADStatus vpad;

    //! update only at 50 Hz, thats more than enough
    VPADRead(0, &vpad, 1, &vpadError);

    if(vpadError == 0 && ((vpad.hold | vpad.trigger) & VPAD_BUTTON_B))
    {
        return 1;
    }
    return 0;
}

/* Entry point */
int main(int argc, char **argv)
{
    ProcUIInit(&OSSavesDone_ReadyToRelease);
    ProcUIRegisterCallback(PROCUI_CALLBACK_ACQUIRE, &console_acquire, NULL, 100);
    ProcUIRegisterCallback(PROCUI_CALLBACK_RELEASE, &console_release, NULL, 100);

    console_acquire(NULL);

    int res = IOSUHAX_Open(NULL);
    if(res < 0)
    {
        console_printf(1, "IOSUHAX_open failed\n");
        OSSleepTicks(OSSecondsToTicks(2));
        SYSLaunchMenu();
    }

    int fsaFd = IOSUHAX_FSA_Open();
    if(fsaFd < 0)
    {
        console_printf(1, "IOSUHAX_FSA_Open failed\n");
        OSSleepTicks(OSSecondsToTicks(2));
        SYSLaunchMenu();
    }

    VPADReadError vpadError = -1;
    VPADStatus vpad;

    int initScreen = 1;

    static const char* selection_paths[] =
    {
        "/dev/odd01",
        "/dev/odd02",
        "/dev/odd03",
        "/dev/odd04",
        "/dev/slccmpt01",
        "/vol/system",
        "/vol/storage_mlc01",
        "/vol/storage_usb01",
    };

    static const char* selection_paths_description[] =
    {
        "(disc tickets)",
        "(disc update)",
        "(disc content)",
        "(disc content)",
        "(vWii slc content)",
        "(slc content)",
        "(mlc content)",
        "(usb01 content)",
    };

    int selectedItem = 0;
    ProcUIStatus status;

    while((status = ProcUIProcessMessages(TRUE)) != PROCUI_STATUS_EXITING)
    {
        if(status == PROCUI_STATUS_RELEASE_FOREGROUND)
            ProcUIDrawDoneRelease();
        if(status != PROCUI_STATUS_IN_FOREGROUND)
        {
            //! force screen redraw when we return in foreground
            initScreen = 1;
            continue;
        }

        //! update only at 50 Hz, thats more than enough
        VPADRead(0, &vpad, 1, &vpadError);

        if(initScreen)
        {
            initScreen = 0;

            // free memory
            console_cleanup();

            // Clear screens
            OSScreenClearBufferEx(0, 0);
            OSScreenClearBufferEx(1, 0);


            console_print_pos(0, 1, "-- File Tree 2 SD v0.1 by Dimok --");

            console_print_pos(0, 3, "Select what to dump to sd:/ft2sd and press A to start dump.");
            console_print_pos(0, 4, "Hold B to cancel dump.");

            uint32_t i;
            for(i = 0; i < (sizeof(selection_paths) / 4); i++)
            {
                if(selectedItem == (int)i)
                {
                    console_print_pos(0, 6 + i, "--> %s %s", selection_paths[i], selection_paths_description[i]);
                }
                else
                {
                    console_print_pos(0, 6 + i, "    %s %s", selection_paths[i], selection_paths_description[i]);
                }
            }
            // Flip buffers
            OSScreenFlipBuffersEx(0);
            OSScreenFlipBuffersEx(1);
        }

        if(vpadError == 0 && ((vpad.hold | vpad.trigger) & VPAD_BUTTON_DOWN))
        {
            selectedItem = (selectedItem + 1) % (sizeof(selection_paths) / 4);
            initScreen = 1;            
            OSSleepTicks(OSMillisecondsToTicks(100));
        }

        if(vpadError == 0 && ((vpad.hold | vpad.trigger) & VPAD_BUTTON_UP))
        {
            selectedItem--;
            if(selectedItem < 0)
                selectedItem =  (sizeof(selection_paths) / 4) - 1;
            initScreen = 1;
            OSSleepTicks(OSMillisecondsToTicks(100));
        }

        if(vpadError == 0 && ((vpad.hold | vpad.trigger) & VPAD_BUTTON_A))
        {
            const char *dev_path = (selectedItem < 5) ? selection_paths[selectedItem] : NULL;
            const char *mount_path = (selectedItem >= 5) ? selection_paths[selectedItem] : "/vol/storage_ft_content";

            int res = mount_fs("dev", fsaFd, dev_path, mount_path);
            if(res < 0)
            {
                console_printf(1, "Mount of %s to %s failed", dev_path, mount_path);
            }
            else
            {
                char *targetPath = (char*)malloc(PATH_MAX);
                if(targetPath)
                {
                    strcpy(targetPath, "dev:/");
                    DumpDir(targetPath, "fs:/vol/external01/ft2sd");

                    free(targetPath);
                }
                unmount_fs("dev");
                console_printf(1, "Dump complete");
            }
            OSSleepTicks(OSSecondsToTicks(3));
            initScreen = 1;
        }

        OSSleepTicks(OSMillisecondsToTicks(50));
    }

    IOSUHAX_FSA_Close(fsaFd);
    IOSUHAX_Close();

    console_cleanup();
    console_release(NULL);

    ProcUIShutdown();
    return 0;
}

