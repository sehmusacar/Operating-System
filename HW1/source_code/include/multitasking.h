 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

using namespace myos::common;

namespace myos
{
    
    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));
    
    
    class Task
    {
    friend class TaskManager;
    private:
        common::uint8_t stack[4096]; // 4 KiB
        CPUState* cpustate;
        int secret_id;
        int parent_id;
    public:
        Task(GlobalDescriptorTable *gdt, void entrypoint());
        Task(GlobalDescriptorTable *gdt, void (*entrypoint)(), int ret_value);
        Task(Task& task, GlobalDescriptorTable *gdt, uint32_t esp);
        void setCpuState(CPUState* cpustate, GlobalDescriptorTable *gdt);
        void assign(const Task& complex);
        ~Task();
        CPUState* getCpuState();
        bool isDone;
        bool settedEntryPoint;
        uint32_t based_eip;
        static int id;
        inline int getId(){return secret_id;}
        inline int getParentId(){return parent_id;}
    };
    
    
    class TaskManager
    {
    private:
        Task* tasks[256];
        int numTasks;
        int currentTask;
    public:
        TaskManager();
        ~TaskManager();
        bool AddTask(Task* task);
        CPUState* Schedule(CPUState* cpustate);
        CPUState* ScheduleToSpecialTask(CPUState* cpustate, int index);
        Task* getCurrentTask();
        inline int getNumTasks(){return numTasks;}
        inline int indexOfCurrentTask(){return currentTask;}
        bool deleteCurrentTask();
        bool deleteTask(int index);
        inline Task* getTask(int index){return tasks[index];}
    };
    
    
    
}


#endif