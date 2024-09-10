#ifndef __MYOS__GLOBAL_H
#define __MYOS__GLOBAL_H

#include <gdt.h>
using namespace myos;

extern GlobalDescriptorTable gdt;
extern TaskManager taskManager;
    
#endif