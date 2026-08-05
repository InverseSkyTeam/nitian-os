#include <stdint.h>

extern void asm_hlt(void);
extern void asm_cli(void);
extern void asm_sti(void);
extern void asm_stihlt(void);

extern void outb(uint16_t port, uint8_t value);
extern uint8_t inb(uint16_t port);
extern void insw(uint16_t port, void* buf, int words);
extern void outsw(uint16_t port, const void* buf, int words);

extern uint32_t asm_read_cr0(void);
extern void asm_write_cr0(uint32_t cr0);
extern void asm_write_cr3(uint32_t cr3);

extern uint32_t asm_save_eflags(void);
extern void asm_restore_eflags(uint32_t eflags);

extern void asm_lgdt(uint32_t gdtr_ptr);
extern void asm_ltr(uint16_t sel);
extern uint16_t asm_str(void);