#include <stdint.h>

struct IDTEntry {
    uint16_t offset_low;  
    uint16_t selector;     
    uint8_t  ist;         
    uint8_t  type;         
    uint16_t offset_high;   
};

#define IDT_TYPE_INT_GATE32   0x8E  
#define IDT_TYPE_TRAP_GATE32  0x8F 
#define IDT_TYPE_TRAP_GATE3   0xEF  

extern struct IDTEntry idt[256];

void initIDT(void);
