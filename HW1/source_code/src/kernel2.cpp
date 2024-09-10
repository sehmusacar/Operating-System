
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>
#include <drivers/amd_am79c973.h>
#include <global.h>
#include <mylocked.h>
// #define GRAPHICSMODE


using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

GlobalDescriptorTable gdt;
TaskManager taskManager;
bool islocked;

// Ekrana yazı yazan fonksiyon
void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

// Integer'ı stringe çeviren fonksiyon
char* itoa(int value, char* result, int base) {
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

// Hexadecimal formatında yazı yazan fonksiyon
void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:
    
    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);        
    }
    
    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;
        y += yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);
    }
    
};

void exit(){
    taskManager.getCurrentTask()->isDone = true;
    for(int i=0;i<1000000;i++)
        for(int j=0;j<1000000;j++);
}

// Sisteme yazı yazdıran fonksiyon
void sysprintf(char* str)
{
    
    asm("int $0x80" : : "a" (4), "b" (str));
}

// Fork işlemi yapan fonksiyon
int forkk() {
    // Mevcut görevin ID'sini al
    int beforeFork = taskManager.getCurrentTask()->getId();
    
    // Mevcut görevin CPU durumunu al
    CPUState* cpustate = taskManager.getCurrentTask()->getCpuState();
    
    // Yazılım kesmesiyle fork işlemini başlat (sistem çağrısı)
    asm("int $0x80" : : "a" (2), "b" (cpustate));
    
    // Fork işleminden sonra yeni görevin ID'sini al
    int afterFork = taskManager.getCurrentTask()->getId();
    
    // Eğer mevcut görevin ID'si değişmemişse, bu ana görevdir
    if (beforeFork == afterFork) {
        return 1; // Ana görevden
    } else {
        return 0; // Yeni görevden
    }
}

void taskA()
{
    printf("NOT WORKING\n");
    forkk();
    // printf("************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************");
    printf("WORKING\n");
    // printfHex(taskManager.indexOfCurrentTask());
    // printf("\n");
    // printfHex(taskManager.getNumTasks());
    // printf("\n");
    while(true){
        for(int i=0;i<10000;i++)
            for(int j=0;j<10000;j++);
        //printf("HAHAHA");
        if(taskManager.indexOfCurrentTask() == 1){
            //sysprintf("A");
            // printfHex(taskManager.getNumTasks());
            // printf("\n");
        }
        else{
            // printfHex(taskManager.getNumTasks());
            // printf("\n");
        }
        
    }
}

void TestFork()
{
    printf("TASK B1 ");
    printfHex(taskManager.getNumTasks());
    printf(" ");
    for(int i=0;i<100000;i++)
        for(int j=0;j<100000;j++);
    forkk();
    for(int i=0;i<100000;i++)
        for(int j=0;j<100000;j++);
    if(taskManager.indexOfCurrentTask() == 0)
        printf("TASK B2 FIRST PROGRAM\n");
    else    
        printf("TASK B2 SECOND PROGRAM\n");
    exit();
}

void taskC()
{
    printf("TASK C1\n");
    for(int i=0;i<100000;i++)
        for(int j=0;j<100000;j++);
                
    printf("TASK C2\n");
    for(int i=0;i<100000;i++)
        for(int j=0;j<100000;j++);
}

void taskD(){
    while(true){
        printf("D");
        for(int i=0;i<10000;i++)
            for(int j=0;j<10000;j++);
    }
}

void binarySearch()
{
    int x = 1;
    for(int i=0;i<10;i++){        
        if(x>0){
            printf("\nFORKING Binary search");
            x = forkk();
        }
    }
    int n = 10;
    int low = 0, high = n - 1;
    int nums[10] = {10, 20, 30, 50, 60, 80, 100, 110, 130, 170};
    
    int target = 110;
    while (low <= high)
    {
        int mid = (low + high)/2;
        if (target == nums[mid]) {
            printf("\nTarget found in that index = ");
            printfHex(mid);
            printf("\n");
            if(taskManager.indexOfCurrentTask() == 0){
                while(true){

                }
            }
            exit();
        }
 
        else if (target < nums[mid]) {
            high = mid - 1;
        }

        else {
            low = mid + 1;
        }
    }
 
    printf("\nTarget can not found \n");
    exit();
    
}

void linearSearch()
{
    int y = 1;
    for(int i=0;i<4;i++){        
        if(y>0){
            printf("FORKING linear searchS\n");
            y = forkk();
        }
    }
    int arr[10] = {10, 20, 80, 30, 60, 50, 110, 100, 130, 170};
    int n = 10;
    int x = 175;
    int i;
    for (i = 0; i < n; i++){
        if (arr[i] == x){
            printf("Found at ");
            printfHex(i);
            printf(" index\n");
            if(taskManager.indexOfCurrentTask() == 0){
                while(true){
                    
                }
            }
            exit();
        }
    }
    printf("Can not found\n");
    exit();
}

void printfInt(int num) {
    char buffer[12]; 
    itoa(num, buffer, 10); 
    printf(buffer);
}

void collatz() {
    for (int i = 1; i < 100; ++i) {
        int n = i;
        printf("Collatz sequence starting from: ");
        printfInt(n);
        printf("\n");

        while (n != 1) {
            if (n % 2 == 0) {
                n = n / 2;
            } else {
                n = 3 * n + 1;
            }
            printfInt(n);
            printf("-");
        }

        printf("\nCollatz sequence completed for: ");
        printfInt(i);
        printf("\n");
    }

    exit();
}

void long_running_program() {
    int n = 1000;  
    long long result = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            result += i * j;
        }
    }

    printf("Result of long running program: ");
    char buffer[20];  
    itoa(result, buffer, 10);
    printf(buffer);
    printf("\n");

    while (true);  
}

void _fork(void (*entrypoint)()) {
    Task* temp = new Task(&gdt, entrypoint, 0);
    taskManager.AddTask(temp);
}

// Basit bir rastgele sayı üretici fonksiyon
unsigned int rand_seed = 1;

void srand(unsigned int seed) {
    rand_seed = seed;
}

int rand() {// Linear Congruential Generator (LCG) algoritmasını kullanarak rastgele bir sayı üretir
    rand_seed = rand_seed * 1103515245 + 12345;
    return (rand_seed / 65536) % 32768; // Sonucu 0 ile 32767 arasında sınırlandır
}

void partA_firstStrategy() {

    _fork(collatz);
    _fork(long_running_program);
    _fork(collatz);
    _fork(long_running_program);
    _fork(collatz);
    _fork(long_running_program);

    while (taskManager.getNumTasks() > 2) {
        for (int i = 0; i < 100000; i++)
            for (int j = 0; j < 100000; j++);
    }
    exit();
}

void partB_firstStrategy() {
    srand(12345);  // Rastgele sayı üretici fonksiyonu başlatmak için bir seed kullan

    void (*programs[4])() = {binarySearch, collatz, linearSearch, long_running_program};

    // Rastgele seçilen programları 10 kez yükle
    for (int i = 0; i < 10; ++i) {
        int programIndex = rand() % 4;  // Her seferinde rastgele bir program seç
        _fork(programs[programIndex]);
    }

    // Tüm süreçler bitene kadar sonsuz döngüye gir
    while (taskManager.getNumTasks() > 2) {
        for (int i = 0; i < 100000; i++)
            for (int j = 0; j < 100000; j++);
    }
    exit();
}



void partB_secondStrategy() {
    srand(12345);  // Rastgele sayı üretici fonksiyonu başlatmak için bir seed kullan

    // Rastgele iki program seç
    int programIndex1 = rand() % 4;
    int programIndex2;
    do {
        programIndex2 = rand() % 4;
    } while (programIndex2 == programIndex1);

    void (*programs[4])() = {collatz, binarySearch, linearSearch, long_running_program};

    // Her seçilen programı 3 kez yükle
    for (int i = 0; i < 3; ++i) {
        _fork(programs[programIndex1]);
        _fork(programs[programIndex2]);
    }

    // Tüm süreçler bitene kadar sonsuz döngüye gir
    while (taskManager.getNumTasks() > 1) {
        for (int i = 0; i < 100000; i++)
            for (int j = 0; j < 100000; j++);
    }
    exit();
}


void emptyTask(){
    while(true){

    }
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello World! --- http://www.AlgorithMan.de\n");

    
    
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
    
    printf("heap: 0x");
    printfHex((heap >> 24) & 0xFF);
    printfHex((heap >> 16) & 0xFF);
    printfHex((heap >> 8 ) & 0xFF);
    printfHex((heap      ) & 0xFF);
    
    void* allocated = memoryManager.malloc(1024);
    printf("\nallocated: 0x");
    printfHex(((size_t)allocated >> 24) & 0xFF);
    printfHex(((size_t)allocated >> 16) & 0xFF);
    printfHex(((size_t)allocated >> 8 ) & 0xFF);
    printfHex(((size_t)allocated      ) & 0xFF);
    printf("\n");

    //Task task6(&gdt, partA_firstStrategy);
    //taskManager.AddTask(&task6);

    //Task task7(&gdt, partB_firstStrategy);
    //taskManager.AddTask(&task7);

    Task task8(&gdt, partB_secondStrategy);
    taskManager.AddTask(&task8);

    //Task task9(&gdt, TestFork);
    //taskManager.AddTask(&task9);

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);
    




    printf("Initializing Hardware, Stage 1\n");
    #ifdef GRAPHICSMODE
        Desktop desktop(320,200, 0x00,0x00,0xA8);
    #endif
    
    DriverManager drvManager;
    
        #ifdef GRAPHICSMODE
            KeyboardDriver keyboard(&interrupts, &desktop);
        #else
            PrintfKeyboardEventHandler kbhandler;
            KeyboardDriver keyboard(&interrupts, &kbhandler,&taskManager);
        #endif
        drvManager.AddDriver(&keyboard);
        
    
        #ifdef GRAPHICSMODE
            MouseDriver mouse(&interrupts, &desktop);
        #else
            MouseToConsole mousehandler;
            MouseDriver mouse(&interrupts, &mousehandler);
        #endif
        drvManager.AddDriver(&mouse);
        
        PeripheralComponentInterconnectController PCIController;
        PCIController.SelectDrivers(&drvManager, &interrupts);

        #ifdef GRAPHICSMODE
            VideoGraphicsArray vga;
        #endif
        
    printf("Initializing Hardware, Stage 2\n");
        drvManager.ActivateAll();
        
    printf("Initializing Hardware, Stage 3\n");

    #ifdef GRAPHICSMODE
        vga.SetMode(320,200,8);
        Window win1(&desktop, 10,10,20,20, 0xA8,0x00,0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40,15,30,30, 0x00,0xA8,0x00);
        desktop.AddChild(&win2);
    #endif


    /*
    printf("\nS-ATA primary master: ");
    AdvancedTechnologyAttachment ata0m(true, 0x1F0);
    ata0m.Identify();
    
    printf("\nS-ATA primary slave: ");
    AdvancedTechnologyAttachment ata0s(false, 0x1F0);
    ata0s.Identify();
    ata0s.Write28(0, (uint8_t*)"http://www.AlgorithMan.de", 25);
    ata0s.Flush();
    ata0s.Read28(0, 25);
    
    printf("\nS-ATA secondary master: ");
    AdvancedTechnologyAttachment ata1m(true, 0x170);
    ata1m.Identify();
    
    printf("\nS-ATA secondary slave: ");
    AdvancedTechnologyAttachment ata1s(false, 0x170);
    ata1s.Identify();
    // third: 0x1E8
    // fourth: 0x168
    */
    
    
    amd_am79c973* eth0 = (amd_am79c973*)(drvManager.drivers[2]);
    eth0->Send((uint8_t*)"Hello Network", 13);
        

    interrupts.Activate();


    while(1)
    {
        #ifdef GRAPHICSMODE
            desktop.Draw(&vga);
        #endif
    }
}
