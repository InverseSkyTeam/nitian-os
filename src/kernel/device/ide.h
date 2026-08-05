// 参考: 《操作系统真相还原》(于渊) 第13章 硬盘驱动
#ifndef IDE_H
#define IDE_H

#include <stdint.h>
#include "../lib/list/list.h"
#include "../thread/sync.h"

struct partition {
    uint32_t start_lba;        
    uint32_t sec_cnt;          
    struct disk* my_disk;      
    struct list_elem part_tag; 
    char name[8];              
};

struct disk {
    char name[8];                    
    struct ide_channel* my_channel;  
    uint8_t dev_no;                  
    struct partition prim_parts[4];  
    struct partition logic_parts[8]; 
};

struct ide_channel {
    char name[8];               
    uint16_t port_base;         
    uint8_t irq_no;             
    struct lock lock;           
    int expecting_intr;         
    struct semaphore disk_done; 
    struct disk devices[2];     
};

void ide_read(struct disk* hd, uint32_t lba, void* buf, uint8_t sec_cnt);
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint8_t sec_cnt);
void intr_hd_handler(uint8_t irq_no);
void ide_init(void);

extern struct ide_channel channels[2];
extern struct list partition_list;

#endif
