
#include <syscalls.h>
#include <global.h>
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 


 
SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}


void printf(char*);
void printfHex(uint8_t key);
uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    
    

    switch(cpu->eax)
    {
        case 2:
            fork(cpu->eip);
            break;
        case 4:
            printf((char*)cpu->esp);
            break;
        default:
            break;
    }

    
    return esp;
}





void myos::fork(uint32_t esp){
    Task* temp = taskManager.getCurrentTask();
    Task* task3 = new Task(*temp, &gdt, esp);
    taskManager.AddTask(task3);
}


