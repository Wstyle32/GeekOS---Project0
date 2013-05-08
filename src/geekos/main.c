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

#define SCREEN_WIDTH    80
#define SCREEN_HEIGHT   24
#define SCREEN_SIZE     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(char)


// Global Variables

int indicator = 0;          // 다음 문자가 입력될 위치를 가리키는 변수.
int pageIndex = 0;

char* tempHeapPtr = NULL;
char* screenBuffer = NULL;

int rowValue = 0;
int columnValue = 0;

// Function Prototypes

void Kernel_Thread(void * args);

void Display(int pageIndex);

void Buffer(char data);
void* Calloc(int size);

void GetBack();
void GetDelete();

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


void Kernel_Thread(void * args) {
    
    int j, k;   // 테스트용 변수: 삭제 필요.
    int i;
    
    
    bool isNumlockOn = false;
    bool isCapslockOn = false;
    bool isScrlockOn = false;
    
    Keycode keyCode;
    
    bool isSpecialKey = false;
    
    // Initializations
    
    screenBuffer = (char *)Calloc(SCREEN_SIZE);
    
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
                        
                    case 0x8381: Print("L"); break;
                    case 0x8389: Print("D"); break;
                    case 0x8384: Print("L"); break;
                    case 0x8386: Print("R"); break;
                        
                    case 0x8380: Print("Home"); break;
                    case 0x8382: Print("PgUp"); break;
                    case 0x838c: Print("Delete"); break;
                    case 0x8388: Print("End"); break;
                    case 0x838a: Print("PgDn"); break;
                }
                
                isSpecialKey = false;
            }
            
            continue;
        }
        
        if(isSpecialKey == true)
            continue;
        
        // 미구현부: LED PORT 반영.
        if(keyCode == 0x0114) {
            
            isCapslockOn = !isCapslockOn;
            continue;
        }
        
        if(keyCode == 0x0115) {
            isNumlockOn = !isNumlockOn;
            continue;
        }
        
        if(keyCode == 0x0116) { // 확인 필요.
            isScrlockOn = !isScrlockOn;
            continue;
        }
        
        
        // Enter Logic
        if(keyCode == 0x000d) {
            
            Buffer('\n');
            continue;
        }
        
        // Backspace Logic
        if(keyCode == 0x0008) {
            
            GetBack();
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
        
        if(0x0101 <= keyCode && keyCode <= 0x010c)
        {
            switch (keyCode) {
                    
                case 0x0101: Print("F1"); break;
                case 0x0102: Print("F2"); break;
                case 0x0103: Print("F3"); break;
                case 0x0104: Print("F4"); break;
                case 0x0105: Print("F5"); break;
                case 0x0106: Print("F6"); break;
                case 0x0107: Print("F7"); break;
                case 0x0108: Print("F8"); break;
                case 0x0109: Print("F9"); break;
                case 0x010a: Print("F10"); break;
                case 0x010b: Print("F11"); break;
                case 0x010c: Print("F12"); break;
                    
                default: break;
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
                switch (keyCode) {
                        
                    case 0x0381: Print("U"); break;
                    case 0x0389: Print("D"); break;
                    case 0x0384: Print("L"); break;
                    case 0x0386: Print("R"); break;
                        
                    case 0x0380: Print("Home"); break;
                    case 0x0382: Print("PgUp"); break;
                    case 0x0388: Print("End");  break;
                    case 0x038a: Print("PgDn"); break;
                    case 0x038b: Print("Ins");  break;
                    case 0x038c: GetDelete();   break;
                }
            }
            
            // '-', '+'는 numlock 여부와 관련이 없으므로 'if(isNumlockOn == true) { ... }'와 독립적으로 실행한다.
            switch (keyCode) {
                    
                case 0x0383: Print("-"); break;
                case 0x0387: Print("+"); break;
            }
            
            continue;
        }
        
        // Plain Character Key 이벤트 핸들링.
        // 대소문자 및 키보드 레이아웃 반영해야 함: Keycode Convert(Mode mode, Keycode input)
        // Memory Buffer 구현시 저장 루틴까지 구현해야 함.
        
        Buffer(keyCode);
    }
}

void Buffer(char data) {
    
    Print("%c", data);
    screenBuffer[indicator++] = data;
}

void GetBack() {
    
    int i;
    int count = 0;
    
    Get_Cursor(&rowValue, &columnValue);
    
    while (indicator > 0) {
        
        if(screenBuffer[indicator] == 0) {
            
            screenBuffer[--indicator] = 0;
            break;
        }
        
        screenBuffer[indicator - 1] = screenBuffer[indicator];
        indicator++;
    }
    
    Display(pageIndex);
    
    
    if(columnValue != 0) {
        
        Put_Cursor(rowValue, columnValue -1);
    }
    else {
        
        i = indicator - 1;
        
        while(i != 0 && screenBuffer[i - 1] != '\n') {
            
            i--;
            count++;
        }
        
        Put_Cursor(rowValue - 1, count + 1);
    }
}

void GetDelete() {
    
    indicator--;
    GetBack();
}

void Display(int pageIndex) {
    
    int i;
    char temp;
    
    Get_Cursor(&rowValue, &columnValue);
    Put_Cursor(0, 0);
    
    Clear_Screen();

    for(i = 0; i < SCREEN_SIZE; i++) {

        temp = screenBuffer[pageIndex * SCREEN_SIZE + i];
        
        if(temp == 0)
            break;
        
        Print("%c", temp);
    }
    
    Put_Cursor(rowValue, columnValue);
}

void* Calloc(int size) {
    
    int i;
    char* allocator = (void *)Malloc(size);
    
    for(i = 0; i < size; i++)
        allocator[i] = 0;
    
    return (void *)allocator;
}



