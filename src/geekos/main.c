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

void Kernel_Thread(void * args) {
    
    int i;
    
    int rowValue = 0;
    int columnValue = 0;
    
    int rowMaxLine = 3;
    int rowIndicator = 0;
    
    int* tempHeapPtr = NULL;
    int* endLineBuffer = (int *)Malloc(sizeof(int) * rowMaxLine);

    bool isNumlockOn = false;
    bool isCapslockOn = false;
    bool isScrlockOn = false;
    
    Keycode keyCode;

    bool isSpecialKey = false;
    
    Clear_Screen();
    Put_Cursor(0, 0);
    
    while(1) {
	     
        keyCode = Wait_For_Key();
        
        // Release Key 이벤트 핸들링.
        if((keyCode & KEY_RELEASE_FLAG) != 0)
        {
            if(keyCode == 0x8100)
            {
                isSpecialKey = true;
                continue;
            }
            
            // Special Key로 분류되는 항목을 이 부분에서 처리.
            if(isSpecialKey == true){
                
                // 미구현부.
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
        
        if(keyCode == 0x0114) {
            isCapslockOn = !isCapslockOn;
            continue;
        }
        
        if(keyCode == 0x0115) {
            isNumlockOn = !isNumlockOn;
            continue;
        }
        
        // Enter Logic
        if(keyCode == 0x000d) {
            
            // 메모리 버퍼를 초과하게 될 경우, 메모리 버퍼 전체의 크기를 두 배로 늘리고 기존 내용을 복사한다.
            if(rowIndicator >= rowMaxLine) {
                
                rowMaxLine = 2 * rowMaxLine;
                
                tempHeapPtr = endLineBuffer;
                endLineBuffer = (int *)Malloc(sizeof(int) * rowMaxLine);
                
                for(i = 0; i < rowMaxLine / 2; i++) {
                    endLineBuffer[i] = tempHeapPtr[i];
                }
                
                Free(tempHeapPtr);
                tempHeapPtr = NULL;
            }
            
            Get_Cursor(&rowValue, &columnValue);
            endLineBuffer[rowValue] = columnValue + 1;
            
            Print("\n");
            rowIndicator++;
            
            continue;
        }
        
        // Backspace Logic
        if(keyCode == 0x0008) {
            
            Get_Cursor(&rowValue, &columnValue);
            
            if(columnValue == 0) {
                
                if(rowValue == 0) continue;
                
                Put_Cursor(rowValue - 1, endLineBuffer[--rowIndicator] - 1);
            }
            else {
                
                Put_Cursor(rowValue, columnValue - 1);
                Print(" ");
                Put_Cursor(rowValue, columnValue - 1);
            }
            
            continue;
        }
        
        // Ctrl Key 이벤트 핸들링.
        if((keyCode & KEY_CTRL_FLAG) != 0) {
            
            if (keyCode == 0x4064) {
                
                Clear_Screen();
                Put_Cursor(0, 0);
            }
            
            continue;
        }
        
        // 키 패드 이벤트 핸들링.
        if((keyCode & KEY_KEYPAD_FLAG) != 0)
        {
            if(isNumlockOn == true)
            {
                switch (keyCode) {
                        
                    case 0x0388: Print("1"); break;
                    case 0x0389: Print("2"); break;
                    case 0x038a: Print("3"); break;
                    case 0x0384: Print("4"); break;
                    case 0x0385: Print("5"); break;
                    case 0x0386: Print("6"); break;
                    case 0x0380: Print("7"); break;
                    case 0x0381: Print("8"); break;
                    case 0x0382: Print("9"); break;
                    case 0x038b: Print("0"); break;
                    case 0x038c: Print("."); break;
                }
            }
            else
            {
                // 미구현부.
                switch (keyCode) {
                        
                    case 0x0380: Print("Home "); break;
                    case 0x0382: Print("PgUp "); break;
                    case 0x0388: Print("End ");  break;
                    case 0x038a: Print("PgDn "); break;
                    case 0x038b: Print("Ins ");  break;
                    case 0x038c: Print("Del");   break;
                }
            }
            
            // '-', '+'는 numlock 여부와 관련이 없으므로 if(isNumlockOn == true) {}와 독립적으로 실행한다.
            switch (keyCode) {
                    
                case 0x0383: Print("-"); break;
                case 0x0387: Print("+"); break;
            }
            
            continue;
        }

        // Plain Character Key 이벤트 핸들링.
        // Memory Buffer 구현시 저장 루틴까지 구현해야 함.
        
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









