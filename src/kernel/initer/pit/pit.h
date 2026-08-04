#include <stdint.h>

#define PIT_CTRL   0x43  
#define PIT_CNT0   0x40   

#define PIT_HZ     100

void initPIT(uint32_t hz);