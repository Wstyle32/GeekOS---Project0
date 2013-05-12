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

////////////////////////////////////////////////
// Declarations ////////////////////////////////
////////////////////////////////////////////////

#define TAB_COUNT       8

#define SCREEN_WIDTH    79
#define SCREEN_HEIGHT   25
#define SCREEN_SIZE     SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(char)


// Type definitions

typedef struct {
    
    int rowValue;
    int columnValue;
    
    int pageIndex;
    int pageCount;
    
    char* buffer;
    
} Storage;


// Global Variables

Storage* storages[4];

int pageIndex = 0;
int pageCount = 1;

char* screenBuffer = NULL;

int rowValue = 0;
int columnValue = 0;

bool isKernelThreadValid = true;


// Function Prototypes

void Kernel_Thread(void * args);

void Display();

void Buffer(char data);
void* Calloc(int size);
void PageReallocation(char* target);

void GetBackspace();
void GetDelete();

void MoveUp();
void MoveDown();
void MoveLeft();
void MoveRight();

void Home();
void End();

void PgUp();
void PgDn();

void Save(int index);
void Load(int index);

void Close();

////////////////////////////////////////////////

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

////////////////////////////////////////////////
// Implementations /////////////////////////////
////////////////////////////////////////////////

void Kernel_Thread(void * args) {
    
    int i;
    
    Keycode keyCode;
    COMMAND_TYPE type;
    
    // Initializations
    
    screenBuffer = (char *)Calloc(SCREEN_SIZE);
    
    for (i = 0; i < 4; i++)
        storages[i] = (Storage *)Calloc(sizeof(Storage));
    
    Clear_Screen();
    Put_Cursor(0, 0);
    
    // End initializations
    
    while(isKernelThreadValid == true) {
        
        type = COMMAND_NO_OPERATION;
        keyCode = Get_From_Keyboard(&type);
        
        if(keyCode == NULL) {
         
            switch (type) {
                    
                case COMMAND_NO_OPERATION:                  break;
                case COMMAND_MOVE_UP:       MoveUp();       break;
                case COMMAND_MOVE_DOWN:     MoveDown();     break;
                case COMMAND_MOVE_LEFT:     MoveLeft();     break;
                case COMMAND_MOVE_RIGHT:    MoveRight();    break;
                case COMMAND_HOME:          Home();         break;
                case COMMAND_END:           End();          break;
                case COMMAND_PAGE_UP:       PgUp();         break;
                case COMMAND_PAGE_DOWN:     PgDn();         break;
                case COMMAND_CTRL_D:        Close();        break;
                case COMMAND_F1:                            break;
                case COMMAND_F2:                            break;
                case COMMAND_F3:                            break;
                case COMMAND_F4:                            break;
                case COMMAND_F5:            Save(0);        break;
                case COMMAND_F6:            Load(0);        break;
                case COMMAND_F7:            Save(1);        break;
                case COMMAND_F8:            Load(1);        break;
                case COMMAND_F9:            Save(2);        break;
                case COMMAND_F10:           Load(2);        break;
                case COMMAND_F11:           Save(3);        break;
                case COMMAND_F12:           Load(3);        break;
                case COMMAND_DELETE:        GetDelete();    break;
                case COMMAND_BACKSPACE:     GetBackspace(); break;
                    
            }
            
            continue;
        }
        
        Buffer(keyCode);
    }
}

void Buffer(char data) {
    
    int i, j;
    int tempIndex;
    char tempCharacter;
 char tempBuffer[2];
	int temp_rowValue;
	int temp_columnValue;
    
    Get_Cursor(&rowValue, &columnValue);
	temp_rowValue=rowValue;
	temp_columnValue = columnValue;
    
    
    // 삽입하기에 앞서 삽입 가능성을 보고 부족할 경우 메모리를 재할당.
    if(screenBuffer[pageCount * SCREEN_SIZE - 1] != -1)
        PageReallocation(screenBuffer);
    
    
    // 삽입하려는 곳에 있는 문자.
    tempCharacter = screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue];
    
    // 문자를 새로 삽입하는 경우.
    if(tempCharacter == -1) {
        
        if(data == '\t') {
            
            for(i = 0; i < TAB_COUNT; i++)
                screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue + i] = data;
        }
        
        screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue] = data;
        Print("%c", data);
        
        Get_Cursor(&rowValue, &columnValue);
        
        if(columnValue >= SCREEN_WIDTH)
            Put_Cursor(rowValue + 1, 0);
    }
    else {
        
        // 문자와 문자 사이에 새로운 문자를 삽입하는 경우 다음을 고려.
        // 1. 삽입하는 위치에 있는 문자가 '\n'인 경우.
        // 2. 삽입하는 위치에 있는 문자가 '\n'가 아닌 경우.
        
        
        // case 1: 삽입하려는 위치에 '\n'이 있는 경우.
        if(tempCharacter == '\n') {
            
            // 마지막 열에 입력하는 경우.
            if(columnValue == SCREEN_WIDTH - 1) {
                
                // 새로운 줄을 삽입할 위치 인덱스 계산.
                tempIndex = pageIndex * SCREEN_SIZE + (rowValue + 1) * SCREEN_WIDTH;
                
                // rowValue를 한 줄 밀어넣음.
                for (i = tempIndex; i < pageCount * SCREEN_SIZE; i++) {
                    
                    for(j = 0; j < SCREEN_WIDTH; j++) {
                        
                        screenBuffer[i + SCREEN_WIDTH + j] = screenBuffer[i + j];
                    }
                }
                
                // 새로 추가된 줄을 초기화.
                for(i = tempIndex; i < tempIndex + SCREEN_WIDTH; i++)
                    screenBuffer[i] = -1;
                
                // 밀린 문자 입력.
                screenBuffer[tempIndex] = '\n';
                
                // 마지막 열의 '\n'에 새로운 문자 삽입.
                if(data != '\n')
                    screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue] = data;
            }
            else { // 중간에 '\n'을 삽입하는 경우.
                
                
            }
        }
        else {
            
            // 중간에 '\n'이 아닌 임의의 데이터 삽입.
            // 1. 삽입 위치 기준으로 뒤의 모든 문자를 한 칸씩 미룸.
		i=1;

			tempBuffer[0]=	screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue];
			tempBuffer[1]=	screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue+1];	



			while(true){

				screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue+i]=tempBuffer[0];
				tempBuffer[0]=tempBuffer[1];

				if( tempBuffer[0]==-1 || tempBuffer[0] == '\n'){
					screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue+i+1]=tempBuffer[0];
					break;
				}

				tempBuffer[1]=screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue+i+1];


				i++;

			}	
			screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue] = data;	

			Display();
			Put_Cursor(temp_rowValue,temp_columnValue+1);


        }
    }
}

void GetBackspace() {
    
    Get_Cursor(&rowValue, &columnValue);
    
    if(pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue == 0)
        return;
    
    if(columnValue != 0) {
        
        Put_Cursor(rowValue, columnValue - 1);
    }
    else {
        
        MoveUp();
        End();
    }
    
    GetDelete();
}

void GetDelete() {
    
    int i, j;
    
    int temp;
    int index;
    
    int countCurrent = 0;
    int countFollows = 0;
    
    Get_Cursor(&rowValue, &columnValue);
    
    index = pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue;
    
    // 삭제할 문자가 없으면 바로 종료.
    if(index >= pageCount * SCREEN_SIZE)
        return;
    
    temp = pageIndex * SCREEN_SIZE + (rowValue + 1) * SCREEN_WIDTH;
    
    
    // 여유 공간을 다시 계산.
    for(i = index; i < temp; i++) {
        
        if(screenBuffer[i] == '\n' || screenBuffer[i] == -1)
            countCurrent++;
    }
    
    for(i = temp; i < temp + SCREEN_WIDTH; i++) {
        
        if(i >= pageCount * SCREEN_SIZE || screenBuffer[i] == -1)
            break;
        
        countFollows++;
    }
    
    // 마지막 줄에서 삭제하는 상황에 대응하는 코드 대처.
    if (countFollows == 0 && screenBuffer[temp] == -1) {
        
        screenBuffer[index] = -1;
        
        for(i = index; i < index + countCurrent; i++)
            screenBuffer[i] = screenBuffer[i + 1];
        
        Display();
        return;
    }
    
    // 현재 커서에 위치한 문자가 '\n'인지 아닌지에 따라 문제를 달리 해결.
    if(screenBuffer[index] == '\n') {
        
        // 여유 공간이 있을 경우, 빈 공간에 복사 후, 한 줄씩 당기고 마지막 줄을 -1로 초기화.
        if(countFollows <= countCurrent) {
            
            // 여유 공간에 모두 복사.
            for (i = 0; i < countFollows; i ++)
                screenBuffer[index + i] = screenBuffer[index + i + countCurrent];
            
            // 줄 단위로 당기는 코드.
            for (i = pageIndex * SCREEN_SIZE + (rowValue + 1) * SCREEN_WIDTH; i < pageCount * SCREEN_SIZE; i++) {
                
                for(j = 0; j < SCREEN_WIDTH; j++) {
                    
                    screenBuffer[i + j] = screenBuffer[i + SCREEN_WIDTH + j];
                }
            }
            
            // 빈 공간을 -1로 초기화.
            for (i = SCREEN_WIDTH; i >= 0; i--) {
                screenBuffer[pageCount * SCREEN_SIZE - i - 1] = -1;
            }
        }
        else { // 모든 문자를 당겨 복사하고 초기화함.
            
            for (i = index; i < index + countFollows; i++)
                screenBuffer[i] = screenBuffer[i + countCurrent];
            
            for (i = index + countFollows; i < temp + SCREEN_WIDTH; i++)
                screenBuffer[i] = -1;
        }
    }
    else {
        
        // 현재 커서가 위치한 줄에 있는 여유 공간의 크기에 따라 다음을 고려.
        // 1. 여유 공간이 없음.
        // 2. 여유 공간이 있음.
        
        if(countCurrent == 0) {
            
            // 다음 줄에 문자가 여러 개 있는 경우.
            if(countFollows > 1) {
                
                // 단순하게 한 글자씩 옮김.
                i = index;
                
                do {
                    
                    screenBuffer[i] = screenBuffer[i + 1];
                } while(screenBuffer[i++] != -1);
                
            }
            else {
                
                // 한 글자를 이동하게 되면 한 줄이 비어버리므로, 전체 데이터를 줄 단위로 당기고 마지막 줄을 초기화.
                
                // 한 글자씩 이동.
                for(i = index; i < index + countFollows; i++)
                    screenBuffer[i] = screenBuffer[i + 1];
                
                // 줄 단위로 당김.
                for (i = pageIndex * SCREEN_SIZE + (rowValue + 1) * SCREEN_WIDTH; i < pageCount * SCREEN_SIZE; i++) {
                    
                    for(j = 0; j < SCREEN_WIDTH; j++) {
                        
                        screenBuffer[i + j] = screenBuffer[i + SCREEN_WIDTH + j];
                    }
                }
                
                // 마지막 줄을 초기화.
                for (i = (rowValue + 1) * SCREEN_WIDTH; i < pageCount * SCREEN_SIZE; i++)
                    screenBuffer[i] = -1;
            }
        }
        else { // 현재 커서가 위치한 줄에 여유가 있는 경우.
            
            for(i = index; i < temp - 1; i++) {
                screenBuffer[i] = screenBuffer[i + 1];
            }
            
            screenBuffer[i] = -1;
        }
    }
    
    Display();
}

void Display() {
    
    int i, j;
    int tempIndex;
    
    char bufferedCharacter;
    
    Get_Cursor(&rowValue, &columnValue);
    
    for(i = 0; i < SCREEN_HEIGHT; i++) {
        
        for(j = 0; j < SCREEN_WIDTH; j++) {
            
            Put_Cursor(i, j);
            
            tempIndex = pageIndex * SCREEN_SIZE + i * SCREEN_WIDTH + j;
            
            bufferedCharacter = (screenBuffer[tempIndex] != -1) ? screenBuffer[tempIndex] : '-';
            Print("%c", bufferedCharacter);
        }
    }
    
    Put_Cursor(rowValue, columnValue);
}

void* Calloc(int size) {
    
    int i;
    char* allocator = (char *)Malloc(size);
    
    for (i = 0; i < size; i++)
        allocator[i] = -1;
    
    return (void *)allocator;
}

void MoveUp() {
    
    int i;
    char temp;
    
    Get_Cursor(&rowValue, &columnValue);
    
    if(rowValue == 0)
        return;
    
    for(i = columnValue; i >= 0; i--) {
        
        temp = screenBuffer[(rowValue - 1) * SCREEN_WIDTH + i];
        
        if(temp != -1) {
            Put_Cursor(rowValue - 1, i);
            break;
        }
    }
}

void MoveDown() {
    
    int i;
    char temp;
    
    Get_Cursor(&rowValue, &columnValue);
    
    if(rowValue == SCREEN_HEIGHT - 1)
        return;
    
    for(i = columnValue; i >= 0; i--) {
        
        temp = screenBuffer[(rowValue + 1) * SCREEN_WIDTH + i];
        
        if(temp != -1) {
            
            Put_Cursor(rowValue + 1, i);
            break;
        }
    }
}

void MoveLeft() {
    
    int i;
    char temp;
    
    Get_Cursor(&rowValue, &columnValue);
    
    for (i = rowValue * SCREEN_WIDTH + columnValue - 1; i >= 0; i--) {
        
        temp = screenBuffer[i];
        
        if(temp != -1) {
            
            rowValue = i / SCREEN_WIDTH;
            columnValue = i % SCREEN_WIDTH;
            
            Put_Cursor(rowValue, columnValue);
            
            break;
        }
    }
}

void MoveRight() {
    
    int i;
    char temp;
    
    Get_Cursor(&rowValue, &columnValue);
    
    if(rowValue == SCREEN_HEIGHT - 1 && columnValue == SCREEN_WIDTH - 1)
        return;
    
    temp = screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + columnValue];
    
    if(temp == -1 || temp == '\n' || columnValue == SCREEN_WIDTH - 1) {
        
        if(screenBuffer[pageIndex * SCREEN_SIZE + (rowValue + 1) * SCREEN_WIDTH] == -1)
            return;
        
        if(rowValue < SCREEN_HEIGHT - 1) {
            
            Put_Cursor(rowValue + 1, 0);
            return;
        }
    }
    
    Put_Cursor(rowValue, columnValue + 1);
}

void Home() {
    
    Get_Cursor(&rowValue, &columnValue);
    Put_Cursor(rowValue, 0);
}

void End() {
    
    int i;
    char temp;
    
    Get_Cursor(&rowValue, &columnValue);
    
    for(i = columnValue; i < SCREEN_WIDTH - 1; i++) {
        
        temp = screenBuffer[pageIndex * SCREEN_SIZE + rowValue * SCREEN_WIDTH + i];
        
        if(temp == -1 || temp == '\n')
            break;
    }
    
    Put_Cursor(rowValue, i);
}

void PgUp() {
    
    if(pageIndex > 0)
        pageIndex--;
    
    Display();
}

void PgDn() {
    
    if(pageIndex < pageCount)
        pageIndex++;
    
    Display();
}

void PageReallocation(char* target) {
    
    int i;
    char* temp;
    
    temp = target;
    target = (char *)Calloc((pageCount + 1) * SCREEN_SIZE);
    
    for (i = 0; i < pageCount * SCREEN_SIZE; i++)
        target[i] = temp[i];
    
    pageCount++;
    
    Free(temp);
}

void Save(int index) {
    
    int i;
    Storage* storage;
    
    storage = storages[index];
    
    Free(storage);
    
    storage = NULL;
    storage = (Storage *)Calloc(sizeof(Storage));
    
    storage->rowValue = rowValue;
    storage->columnValue = columnValue;
    
    storage->pageIndex = pageIndex;
    storage->pageCount = pageCount;
    
    storage->buffer = (char *)Calloc(pageCount * SCREEN_SIZE);
    
    for(i = 0; i < pageCount * SCREEN_SIZE; i++)
        storage->buffer[i] = screenBuffer[i];
}

void Load(int index) {
    
    int i;
    Storage* storage;
    
    storage = storages[index];
    
    rowValue = storage->rowValue;
    columnValue = storage->columnValue;
    
    pageIndex = storage->pageIndex;
    pageCount = storage->pageCount;
    
    Free(screenBuffer);
    screenBuffer = NULL;
    
    screenBuffer = (char *)Calloc(pageCount * SCREEN_SIZE);
    
    for(i = 0; i < pageCount * SCREEN_SIZE; i++) {
        
        screenBuffer[i] = storage->buffer[i];
    }
    
    Put_Cursor(rowValue, columnValue);
    
    Clear_Screen();
    Display();
}

void Close() {
    
    isKernelThreadValid = false;
    
    Clear_Screen();
    
    Put_Cursor(0, 0);
    Print("Text Editor terminated! You can restart it by reboot the GeekOS.");
}
