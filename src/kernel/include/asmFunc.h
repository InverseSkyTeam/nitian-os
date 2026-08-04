#include <stdint.h>

extern void asm_hlt(void);
extern void asm_cli(void);
extern void asm_sti(void);
extern void asm_stihlt(void);

extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);

extern uint32_t asm_read_cr0(void);
extern void asm_write_cr0(uint32_t cr0);
extern void asm_write_cr3(uint32_t cr3);