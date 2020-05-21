#include <coreinit/memfrmheap.h>
#include <coreinit/memheap.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <coreinit/cache.h>
#include <coreinit/time.h>
#include <proc_ui/procui.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_CONSOLE_LINES_TV    27
#define MAX_CONSOLE_LINES_DRC   18
#define FRAME_HEAP_TAG          0x66747364

static char *consoleArrayTv[MAX_CONSOLE_LINES_TV] = {0};
static char *consoleArrayDrc[MAX_CONSOLE_LINES_DRC] = {0};
static int isForeground = 0;

void console_print_pos(int x, int y, const char *format, ...)
{
    char * tmp = NULL;

    if(!isForeground)
        return;

    va_list va;
    va_start(va, format);
    if((vasprintf(&tmp, format, va) >= 0) && tmp)
    {
        if(strlen(tmp) > 79)
            tmp[79] = 0;
        
        OSScreenPutFontEx(0, x, y, tmp);
        OSScreenPutFontEx(1, x, y, tmp);
        
    }
    va_end(va);
    
    if(tmp)
        free(tmp);
}

void console_printf(int newline, const char *format, ...)
{
    char * tmp = NULL;

    if(!isForeground)
        return;

    va_list va;
    va_start(va, format);
    if((vasprintf(&tmp, format, va) >= 0) && tmp)
    {
        if(newline)
        {
            if(consoleArrayTv[0])
                free(consoleArrayTv[0]);
            if(consoleArrayDrc[0])
                free(consoleArrayDrc[0]);
            
            for(int i = 1; i < MAX_CONSOLE_LINES_TV; i++)
                consoleArrayTv[i-1] = consoleArrayTv[i];
            
            for(int i = 1; i < MAX_CONSOLE_LINES_DRC; i++)
                consoleArrayDrc[i-1] = consoleArrayDrc[i];
        }
        else
        {
            if(consoleArrayTv[MAX_CONSOLE_LINES_TV-1])
                free(consoleArrayTv[MAX_CONSOLE_LINES_TV-1]);
            if(consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1])
                free(consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1]);
            
            consoleArrayTv[MAX_CONSOLE_LINES_TV-1] = NULL;
            consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1] = NULL;
        }
        
        if(strlen(tmp) > 79)
            tmp[79] = 0;
        
        consoleArrayTv[MAX_CONSOLE_LINES_TV-1] = (char*)malloc(strlen(tmp) + 1);
        if(consoleArrayTv[MAX_CONSOLE_LINES_TV-1])
            strcpy(consoleArrayTv[MAX_CONSOLE_LINES_TV-1], tmp);
        
        consoleArrayDrc[MAX_CONSOLE_LINES_DRC-1] = (tmp);
    }
    va_end(va);
    
    // Clear screens
    OSScreenClearBufferEx(0, 0);
    OSScreenClearBufferEx(1, 0);
    
    
    for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
    {
        if(consoleArrayTv[i])
            OSScreenPutFontEx(0, 0, i, consoleArrayTv[i]);
    }
    
    for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
    {
        if(consoleArrayDrc[i])
            OSScreenPutFontEx(1, 0, i, consoleArrayDrc[i]);
    }

    OSScreenFlipBuffersEx(0);
    OSScreenFlipBuffersEx(1);
}

void console_cleanup()
{
    for(int i = 0; i < MAX_CONSOLE_LINES_TV; i++)
    {
        if(consoleArrayTv[i])
            free(consoleArrayTv[i]);
        consoleArrayTv[i] = 0;
    }

    for(int i = 0; i < MAX_CONSOLE_LINES_DRC; i++)
    {
        if(consoleArrayDrc[i])
            free(consoleArrayDrc[i]);
        consoleArrayDrc[i] = 0;
    }
}

uint32_t console_acquire(void *context)
{
    if(isForeground)
        return 0;

    MEMHeapHandle heapMEM1 = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
    MEMRecordStateForFrmHeap(heapMEM1, FRAME_HEAP_TAG);

    OSScreenInit();
    OSScreenSetBufferEx(0, MEMAllocFromFrmHeapEx(heapMEM1, OSScreenGetBufferSizeEx(0), 4));
    OSScreenSetBufferEx(1, MEMAllocFromFrmHeapEx(heapMEM1, OSScreenGetBufferSizeEx(1), 4));
    OSScreenEnableEx(0, 1);
    OSScreenEnableEx(1, 1);

    OSScreenClearBufferEx(0, 0);
    OSScreenClearBufferEx(1, 0);
    OSScreenFlipBuffersEx(0);
    OSScreenFlipBuffersEx(1);

    isForeground = 1;

    return 0;
}

uint32_t console_release(void *context)
{
    if(!isForeground)
        return 0;

    isForeground = 0;
    MEMFreeByStateToFrmHeap(MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1), FRAME_HEAP_TAG);

    return 0;
}
