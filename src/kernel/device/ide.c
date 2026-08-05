// 参考: 《操作系统真相还原》(于渊) 第13章 硬盘驱动
#include "ide.h"
#include "../initer/io/io.h"
#include "../include/asmFunc.h"
#include "../include/assert.h"
#include "../initer/pit/pit.h"
#include "../memory/pool/pool.h"
#include "../lib/str/str.h"
#include "../lib/user/stdio.h"

#define reg_data(channel) ((channel)->port_base + 0)     
#define reg_error(channel) ((channel)->port_base + 1)    
#define reg_sect_cnt(channel) ((channel)->port_base + 2) 
#define reg_lba_l(channel) ((channel)->port_base + 3)    
#define reg_lba_m(channel) ((channel)->port_base + 4)    
#define reg_lba_h(channel) ((channel)->port_base + 5)    
#define reg_dev(channel) ((channel)->port_base + 6)      
#define reg_status(channel) ((channel)->port_base + 7)   
#define reg_cmd(channel) (reg_status(channel))           
#define reg_alt_status(channel) ((channel)->port_base + 0x206) 
#define reg_ctl(channel) (reg_alt_status(channel))       

#define BIT_ALT_STAT_BSY 0x80  
#define BIT_ALT_STAT_DRDY 0x40 
#define BIT_ALT_STAT_DRQ 0x8   

#define BIT_DEV_MBS 0xa0 
#define BIT_DEV_LBA 0x40 
#define BIT_DEV_DEV 0x10 

#define CMD_IDENTIFY 0xec     
#define CMD_READ_SECTOR 0x20  
#define CMD_WRITE_SECTOR 0x30 

#define MAX_LBA ((80 * 1024 * 1024 / 512) - 1)

#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / STEP)

uint8_t channel_cnt;
struct ide_channel channels[2];

int32_t ext_lba_base = 0;   
uint8_t p_no = 0, l_no = 0; 
struct list partition_list; 

struct partition_table_entry {
    uint8_t bootable;   
    uint8_t start_head; 
    uint8_t start_sec;  
    uint8_t start_chs;  
    uint8_t fs_type;    
    uint8_t end_head;   
    uint8_t end_sec;    
    uint8_t end_chs;    
    uint32_t start_lba; 
    uint32_t sec_cnt;   
} __attribute__((packed));

struct boot_sector {
    uint8_t other[446];
    struct partition_table_entry partition_table[4];
    uint16_t signature; 
} __attribute__((packed));

static void ide_panic(const char* msg) {
    setTextColor(12);
    kprintf("IDE PANIC: %s\n", msg);
    asm_cli();
    for (;;) {
        asm_hlt();
    }
}

static void select_disk(struct disk* hd) {
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
    if (hd->dev_no == 1) {
        reg_device |= BIT_DEV_DEV;
    }
    outb(reg_dev(hd->my_channel), reg_device);
}

static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt) {
    ASSERT(lba <= MAX_LBA);
    struct ide_channel* channel = hd->my_channel;
    outb(reg_sect_cnt(channel), sec_cnt);
    outb(reg_lba_l(channel), lba);
    outb(reg_lba_m(channel), lba >> 8);
    outb(reg_lba_h(channel), lba >> 16);
    
    outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV : 0) | (uint8_t)(lba >> 24));
}

static void wait_bsy_clear(struct ide_channel* channel) {
    while (inb(reg_status(channel)) & BIT_ALT_STAT_BSY) {
    }
}

static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
    wait_bsy_clear(channel);
    channel->expecting_intr = 1;
    outb(reg_cmd(channel), cmd);
}

static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
    uint32_t byte;
    if (sec_cnt == 0) { 
        byte = 256 * 512;
    } else {
        byte = sec_cnt * 512;
    }
    insw(reg_data(hd->my_channel), buf, byte / 2);
}

static void write_to_sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
    uint32_t byte;
    if (sec_cnt == 0) {
        byte = 256 * 512;
    } else {
        byte = sec_cnt * 512;
    }
    outsw(reg_data(hd->my_channel), buf, byte / 2);
}

static int busy_wait(struct disk* hd) {
    struct ide_channel* channel = hd->my_channel;
    uint32_t timeout = 30 * 1000 * 100;
    while (timeout--) {
        uint8_t st = inb(reg_status(channel));
        if (!(st & BIT_ALT_STAT_BSY)) {
            return (st & BIT_ALT_STAT_DRQ) != 0;
        }
    }
    return 0;
}

void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
    ASSERT(lba <= MAX_LBA);
    ASSERT(sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);

    select_disk(hd);

    uint32_t secs_done = 0;
    while (secs_done < sec_cnt) {
        select_sector(hd, lba + secs_done, 1);
        cmd_out(hd->my_channel, CMD_READ_SECTOR);

        if (!busy_wait(hd)) {
            char error[64];
            sprintf(error, "%s read sector %d failed!!!!!!", hd->name, (int)(lba + secs_done));
            ide_panic(error);
        }
        read_from_sector(hd, (void*)((uint32_t)buf + secs_done * 512), 1);

        secs_done++;
    }
    lock_release(&hd->my_channel->lock);
}

void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
    ASSERT(lba <= MAX_LBA);
    ASSERT(sec_cnt > 0);
    lock_acquire(&hd->my_channel->lock);

    select_disk(hd);

    uint32_t secs_done = 0;
    while (secs_done < sec_cnt) {
        select_sector(hd, lba + secs_done, 1);
        cmd_out(hd->my_channel, CMD_WRITE_SECTOR);
        if (!busy_wait(hd)) {
            char error[64];
            sprintf(error, "%s write sector %d failed!!!!!!", hd->name, (int)(lba + secs_done));
            ide_panic(error);
        }
        write_to_sector(hd, (void*)((uint32_t)buf + secs_done * 512), 1);
        wait_bsy_clear(hd->my_channel);

        secs_done++;
    }
    lock_release(&hd->my_channel->lock);
}

void intr_hd_handler(uint8_t irq_no) {
    ASSERT(irq_no == 0x2e || irq_no == 0x2f);
    uint32_t no = irq_no - 0x2e;
    struct ide_channel* channel = &channels[no];
    ASSERT(channel->irq_no == irq_no);
    channel->expecting_intr = 0;
    inb(reg_status(channel));
}

static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len) {
    uint8_t idx;
    for (idx = 0; idx < len; idx += 2) {
        buf[idx + 1] = *dst;
        dst++;
        buf[idx] = *dst;
        dst++;
    }
    buf[idx] = '\0';
}

static void identify_disk(struct disk* hd) {
    char id_info[512];
    select_disk(hd);
    cmd_out(hd->my_channel, CMD_IDENTIFY);

    if (!busy_wait(hd)) {
        char error[64];
        sprintf(error, "%s identify failed!!!!!!", hd->name);
        ide_panic(error);
    }
    read_from_sector(hd, id_info, 1);

    char buf[64];
    uint8_t sn_start = 10 * 2, sn_len = 20; 
    uint8_t md_start = 27 * 2, md_len = 40; 
    swap_pairs_bytes(&id_info[sn_start], buf, sn_len);
    kprintf("  disk %s info:\n", hd->name);
    kprintf("    SN: %s\n", buf);
    memset(buf, 0, sizeof(buf));
    swap_pairs_bytes(&id_info[md_start], buf, md_len);
    kprintf("    MODULE: %s\n", buf);
    uint32_t sector = *(uint32_t*)&id_info[60 * 2]; 
    kprintf("    SECTORS: %d\n", (int)sector);
    kprintf("    CAPACITY: %dMB\n", (int)(sector * 512 / 1024 / 1024));
}

static void partition_scan(struct disk* hd, uint32_t ext_lba) {
    struct boot_sector* bs = (struct boot_sector*)get_kernel_pages(1);
    ide_read(hd, ext_lba, bs, 1);
    uint8_t part_idx = 0;
    struct partition_table_entry* p = bs->partition_table;

    while (part_idx++ < 4) {
        if (p->fs_type == 0x5) { 
            if (ext_lba == 0) {
                ext_lba_base = p->start_lba;
                partition_scan(hd, p->start_lba);
            } else {
                partition_scan(hd, p->start_lba + ext_lba_base);
            }
        } else if (p->fs_type != 0) { 
            if (ext_lba == 0) {       
                hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
                hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
                hd->prim_parts[p_no].my_disk = hd;
                list_append(&partition_list, &hd->prim_parts[p_no].part_tag);
                sprintf(hd->prim_parts[p_no].name, "%s%d", hd->name, p_no + 1);
                p_no++;
                ASSERT(p_no < 4);
            } else { 
                hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
                hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
                hd->logic_parts[l_no].my_disk = hd;
                list_append(&partition_list, &hd->logic_parts[l_no].part_tag);
                sprintf(hd->logic_parts[l_no].name, "%s%d", hd->name, l_no + 5);
                l_no++;
                if (l_no >= 8) {
                    return;
                }
            }
        }
        p++;
    }
}

static void print_partition_info(void) {
    struct list_elem* e = partition_list.head.next;
    while (e != &partition_list.tail) {
        struct partition* part = list_entry(e, struct partition, part_tag);
        kprintf("    %s start_lba:0x%x, sec_cnt:0x%x\n",
                part->name, part->start_lba, part->sec_cnt);
        e = e->next;
    }
}

void ide_init(void) {
    kprintf("ide_init start\n");
    
    uint8_t hd_cnt = *((uint8_t*)(0x475));
    if (hd_cnt == 0) {
        kprintf("  no hard disk detected (0x475=0), skip ide_init\n");
        return;
    }

    channel_cnt = (uint8_t)DIV_ROUND_UP(hd_cnt, 2);
    struct ide_channel* channel;
    uint8_t channel_no = 0, dev_no = 0;
    uint8_t global_dev = 0;
    list_init(&partition_list);

    while (channel_no < channel_cnt) {
        channel = &channels[channel_no];
        sprintf(channel->name, "ide%d", channel_no);
        if (channel_no == 0) {
            channel->port_base = 0x1f0;
            channel->irq_no = 0x20 + 14; 
        } else {
            channel->port_base = 0x170;
            channel->irq_no = 0x20 + 15; 
        }
        channel->expecting_intr = 0;
        lock_init(&channel->lock);
        sema_init(&channel->disk_done, 0);

        while (dev_no < 2 && global_dev < hd_cnt) {
            struct disk* hd = &channel->devices[dev_no];
            hd->my_channel = channel;
            hd->dev_no = dev_no;
            sprintf(hd->name, "sd%c", 'a' + channel_no * 2 + dev_no);
            identify_disk(hd);
            p_no = 0;
            l_no = 0;
            ext_lba_base = 0;
            partition_scan(hd, 0);
            dev_no++;
            global_dev++;
        }
        dev_no = 0;
        channel_no++;
    }
    kprintf("\n  all partition info\n");
    print_partition_info();
    kprintf("ide_init done\n");
}
