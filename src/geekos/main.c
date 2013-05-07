/*
 * GeekOS C code entry point
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2004, Iulian Neamtiu <neamtiu@cs.umd.edu>
 * $Revision: 1.51 $
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/bootinfo.h>
#include <geekos/string.h>
#include <geekos/screen.h>
#include <geekos/mem.h>
#include <geekos/crc32.h>
#include <geekos/tss.h>
#include <geekos/int.h>
#include <geekos/kthread.h>
#include <geekos/trap.h>
#include <geekos/timer.h>
#include <geekos/keyboard.h>

#define SCREEN_MAX_CHARACTER    70

void Kernel_Thread(void * args) {
    
    int rowValue = 0;
    int columnValue = 0;

    bool isNumlockOn = false;
    bool isCapslockOn = false;
    bool isScrlockOn = false;
    
    Keycode keyCode;

    bool isSpecialKey = false;
    
    while(1) {
	     
        keyCode = Wait_For_Key();
        
        if((keyCode & KEY_RELEASE_FLAG) != 0)
        {
            if(keyCode == 0x8100)
            {
                isSpecialKey = true;
                continue;
            }
            
            if(isSpecialKey == true){
                
                // Special Key로 분류되는 항목을 이 부분에서 처리.
                
                switch (keyCode) {
                    case 0x8380: Print("Home "); break;
                    case 0x8382: Print("PgUp "); break;
                    case 0x838c: Print("Delete "); break;
                    case 0x8388: Print("End "); break;
                    case 0x838a: Print("PgDn "); break;
                }
                
                isSpecialKey = false;
            }
            
            continue;
        }
        
        if(isSpecialKey == true)
            continue;
        
        if (keyCode == 0x114) {
            isCapslockOn = !isCapslockOn;
            continue;
        }
        
        if(keyCode == 0x115){
            isNumlockOn = !isNumlockOn;
            continue;
        }
        
        if((keyCode & KEY_KEYPAD_FLAG) != 0)
        {
            if(isNumlockOn == true) {
                
                switch (keyCode) {
                    case 0x388: Print("1"); break;
                    case 0x389: Print("2"); break;
                    case 0x38a: Print("3"); break;
                    case 0x384: Print("4"); break;
                    case 0x385: Print("5"); break;
                    case 0x386: Print("6"); break;
                    case 0x380: Print("7"); break;
                    case 0x381: Print("8"); break;
                    case 0x382: Print("9"); break;
                    case 0x38b: Print("0"); break;
                    case 0x38c: Print("."); break;
                }
            }
            else {
                switch (keyCode) {
                    case 0x380: Print("Home "); break;
                    case 0x382: Print("PgUp "); break;
                    case 0x388: Print("End ");  break;
                    case 0x38a: Print("PgDn "); break;
                    case 0x38b: Print("Ins ");  break;
                    case 0x38c: Print("Del");   break;
                }
            }
            
            // '-', '+'는 numlock 여부와 관련이 없으므로 if(isNumlockOn == true) {}와 독립적으로 실행한다.
            switch (keyCode) {
                case 0x383: Print("-"); break;
                case 0x387: Print("+"); break;
            }
            
            continue;
        }

        Print("%c", keyCode);
    }
}

/*
 * Kernel C code entry point.
 * Initializes kernel subsystems, mounts filesystems,
 * and spawns init process.
 */

void Main(struct Boot_Info* bootInfo)
{
    Init_BSS();
    Init_Screen();
    Init_Mem(bootInfo);
    Init_CRC32();
    Init_TSS();
    Init_Interrupts();
    Init_Scheduler();
    Init_Traps();
    Init_Timer();
    Init_Keyboard();
    
    Start_Kernel_Thread(Kernel_Thread, 0, PRIORITY_NORMAL, true);
    
    Exit(0);
}









