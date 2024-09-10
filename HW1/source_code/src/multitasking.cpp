
#include <multitasking.h>
#include <global.h>
using namespace myos;
using namespace myos::common;

void printf(char* str);
void printfHex(uint8_t key);


int Task::id = 0;

Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
    parent_id = -1;
    secret_id = id++;
    printf("IM HERRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRE\n");
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    
    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    // cpustate -> error = 0;    
   
    // cpustate -> esp = ;
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate -> eflags = 0x202;
    int x = 0;
    // printf("Eip = ");
    // printfHex(cpustate->eip);
    // printf("\n");
    isDone = false;
    settedEntryPoint = true;
}
Task::Task(GlobalDescriptorTable *gdt, void (*entrypoint)(), int ret_value)
{
    secret_id = id++;
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    
    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    // cpustate -> error = 0;    
   
    // cpustate -> esp = ;
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate -> eflags = 0x202;
    int x = 0;
    // printf("Eip = ");
    // printfHex(cpustate->eip);
    // printf("\n");
    while(x<4096){
        if(stack[x] != 0)
            printfHex(stack[x]);
        printf(" ");
        x++;
    }
    isDone = false;
    settedEntryPoint = true;
}
Task::Task(Task& task, GlobalDescriptorTable *gdt, uint32_t esp){
    parent_id = task.getId();
    secret_id = id++;
    int x = 0;
    while(x<4096){
        stack[x] = task.stack[x];
        x++;
    }
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = task.cpustate->eax;
    cpustate -> ebx = task.cpustate->ebx;
    cpustate -> ecx = task.cpustate->ecx;
    cpustate -> edx = task.cpustate->edx;

    cpustate -> esi = task.cpustate->esi;
    cpustate -> edi = task.cpustate->edi;
    cpustate -> ebp = task.cpustate->ebp;
    
    cpustate -> error = task.cpustate->error;
    cpustate -> esp = task.cpustate->esp;
    cpustate -> ss = task.cpustate->ss;




    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    // cpustate -> error = 0;    
   
    // cpustate -> esp = ;
    //cpustate -> eip = task.cpustate->eip;
    
    //cpustate -> eip = task.cpustate->eip;
    // printf("Forked Eip = ");
    // printfHex(task.getCpuState()->eip);
    // printf("\n");
    cpustate -> eip = task.cpustate->eip;
    // cpustate -> cs = task.cpustate->cs;
    cpustate -> cs = task.cpustate->cs;
    // cpustate -> ss = ;
    cpustate -> eflags = task.cpustate->eflags;
    isDone = false;
    settedEntryPoint = false;
    
}

Task::~Task()
{
}

        
TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
}
CPUState* Task::getCpuState(){
    return cpustate;
}
TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256)
        return false;
    tasks[numTasks++] = task;
    return true;
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0)
        tasks[currentTask]->cpustate = cpustate;
    
    if(++currentTask >= numTasks)
        currentTask %= numTasks;
    if(tasks[currentTask]->settedEntryPoint == false){
        // printf("Scheduling for ");
        // printfHex(currentTask);
        // printf(" ");
        // printfHex(tasks[currentTask]->getId());
        // printf("\n");
        tasks[currentTask] = new Task(*tasks[0],&gdt,-1);
        tasks[currentTask]->settedEntryPoint = true;
        tasks[currentTask]->isDone = false;
    }
    return tasks[currentTask]->cpustate;
}
CPUState* TaskManager::ScheduleToSpecialTask(CPUState* cpustate,int index)
{
    currentTask = index;
    return tasks[currentTask]->cpustate;
}

void Task::setCpuState(CPUState* _cpustate, GlobalDescriptorTable *gdt){
    
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = _cpustate->eax;
    cpustate -> ebx = _cpustate->ebx;
    cpustate -> ecx = _cpustate->ecx;
    cpustate -> edx = _cpustate->edx;

    cpustate -> esi = _cpustate->esi;
    cpustate -> edi = _cpustate->edi;
    cpustate -> ebp = _cpustate->ebp;
    
    cpustate -> error = _cpustate->error;
    cpustate -> esp = _cpustate->esp;
    cpustate -> ss = _cpustate->ss;




    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    // cpustate -> error = 0;    
   
    // cpustate -> esp = ;
    //cpustate -> eip = task.cpustate->eip;
    
    //cpustate -> eip = task.cpustate->eip;
    // printf("Forked Eip = ");
    // printfHex(cpustate->eip);
    // printf("\n");
    cpustate -> eip = _cpustate->eip;
    // cpustate -> cs = task.cpustate->cs;
    cpustate -> cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    cpustate -> eflags = _cpustate->eflags;
    // printf("Forked cpu state eip = ");
    // printfHex(cpustate->eip);
    // printf("\n");
}

Task* TaskManager::getCurrentTask(){
    return tasks[currentTask];
}


bool TaskManager::deleteCurrentTask(){
    int index = currentTask;
    int upperBound = numTasks-1;
    bool success = false;
    while(upperBound > index){
        tasks[upperBound-1] = tasks[upperBound];
        upperBound--;
        success = true;
    }
    if(success)
        numTasks = numTasks - 1;
    currentTask = -1;
    return true;

}


void Task::assign(const Task& complex){
    //printf("ASSINGMENT OPERATOR\n");
    cpustate -> eax = complex.cpustate->eax;
    cpustate -> ebx = complex.cpustate->ebx;
    cpustate -> ecx = complex.cpustate->ecx;
    cpustate -> edx = complex.cpustate->edx;

    cpustate -> esi = complex.cpustate->esi;
    cpustate -> edi = complex.cpustate->edi;
    cpustate -> ebp = complex.cpustate->ebp;
    
    cpustate -> error = complex.cpustate->error;
    cpustate -> esp = complex.cpustate->esp;
    cpustate -> ss = complex.cpustate->ss;

    secret_id = complex.secret_id;
    parent_id = complex.parent_id;
    isDone = complex.isDone;
    settedEntryPoint = complex.settedEntryPoint;
    based_eip = complex.based_eip;
    id = complex.id;
}




bool TaskManager::deleteTask(int index){
    int upperBound = numTasks-1;
    bool success = false;
    if(upperBound == index){
        numTasks--;
        return true;
    }
    while(upperBound >= index){
        tasks[upperBound-1]->assign(*tasks[upperBound]);
        upperBound--;
        success = true;
    }
    if(success)
        numTasks = numTasks - 1;
    return true;

}

