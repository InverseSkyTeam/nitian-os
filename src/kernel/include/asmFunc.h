#include <stdint.h>

extern void asm_hlt(void);
extern void asm_cli(void);
extern void asm_sti(void);
extern void asm_stihlt(void);

// IO
extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);