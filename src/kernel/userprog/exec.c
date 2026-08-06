// 参考: 《操作系统真相还原》(于渊) 第15章 加载用户进程
#include "./exec.h"
#include "../thread/thread.h"
#include "../userprog/process.h"
#include "../initer/gdt/gdt.h"
#include "../initer/io/io.h"
#include "../include/asm/stub.h"
#include "../memory/pool/pool.h"
#include "../lib/str/str.h"
#include "../fs/fs.h"
#include "../include/assert.h"

#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / STEP)
#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF_1 (1 << 9)
#define EFLAGS_IOPL_0 0
#define MAX_ARG_NR 16

typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

struct Elf32_Ehdr {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
};

struct Elf32_Phdr {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
};

enum segment_type {
    PT_NULL,
    PT_LOAD,
    PT_DYNAMIC,
    PT_INTERP,
    PT_NOTE,
    PT_SHLIB,
    PT_PHDR
};

static int32_t segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr) {
    uint32_t vaddr_first_page = vaddr & 0xfffff000;
    uint32_t size_in_first_page = PAGE_SIZE - (vaddr & 0x00000fff);
    uint32_t occupy_pages = (filesz > size_in_first_page)
                            ? DIV_ROUND_UP(filesz - size_in_first_page, PAGE_SIZE) + 1
                            : 1;

    uint32_t vaddr_page = vaddr_first_page;
    for (uint32_t i = 0; i < occupy_pages; i++) {
        uint32_t* pde = pde_ptr(vaddr_page);
        uint32_t* pte = pte_ptr(vaddr_page);

        if (!(*pde & 1) || (*pde & 0x80) || !(*pte & 1)) {
            if (get_a_page(vaddr_page) == 0) {
                return -1;
            }
        }
        vaddr_page += PAGE_SIZE;
    }
    sys_lseek(fd, offset, SEEK_SET);
    read_file(fd, (void*)vaddr, filesz);
    return 0;
}

static int32_t load(const char* pathname) {
    int32_t ret = -1;
    struct Elf32_Ehdr elf_header;
    struct Elf32_Phdr prog_header;
    memset(&elf_header, 0, sizeof(elf_header));

    int32_t fd = open_file(pathname, O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    if (read_file(fd, &elf_header, sizeof(elf_header)) != sizeof(elf_header)) {
        goto done;
    }
    if (memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7) ||
        elf_header.e_type != 2 ||
        elf_header.e_machine != 3 ||
        elf_header.e_version != 1 ||
        elf_header.e_phnum > 1024 ||
        elf_header.e_phentsize != sizeof(struct Elf32_Phdr)) {
        goto done;
    }

    Elf32_Off prog_header_offset = elf_header.e_phoff;
    for (uint32_t prog_idx = 0; prog_idx < elf_header.e_phnum; prog_idx++) {
        memset(&prog_header, 0, sizeof(prog_header));
        sys_lseek(fd, prog_header_offset, SEEK_SET);
        if (read_file(fd, &prog_header, sizeof(prog_header)) != sizeof(prog_header)) {
            goto done;
        }
        if (prog_header.p_type == PT_LOAD &&
            prog_header.p_vaddr >= USER_VADDR_START &&
            prog_header.p_vaddr < USER_STACK3_VADDR) {
            if (segment_load(fd, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr) == -1) {
                goto done;
            }
        }
        prog_header_offset += elf_header.e_phentsize;
    }
    ret = elf_header.e_entry;
done:
    close_file(fd);
    return ret;
}

int32_t sys_execv(const char* path, const char* argv[]) {
    uint32_t argc;
    int32_t entry_point;
    struct task_struct* cur;
    uint32_t old_pgdir;
    uint32_t ustack_ptr;
    uint32_t argv_user_addrs[MAX_ARG_NR];                  
    uint32_t argv_user_base;
    int32_t i;
    uint32_t slen;
    struct Registers* ps;

    cur = current_task;

    old_pgdir = cur->pgdir;
    if (cur->pgdir == 0) {
        create_user_vaddr_bitmap(cur);
        cur->pgdir = (uint32_t)create_page_dir();
        process_activate(cur);
    }

    argc = 0;
    while (argv && argv[argc] && argc < MAX_ARG_NR) {
        ++argc;
    }

    entry_point = load(path);
    if (entry_point == -1) {

        if (cur->pgdir != old_pgdir) {
            cur->pgdir = old_pgdir;
            page_dir_activate(cur);
        }
        return -1;
    }

    memcpy(cur->name, path, 15);
    cur->name[15] = 0;

    for (uint32_t sp = USER_STACK3_VADDR - PAGE_SIZE; sp <= USER_STACK3_VADDR; sp += PAGE_SIZE) {
        uint32_t* pde = pde_ptr(sp);
        uint32_t* pte = pte_ptr(sp);
        if (!(*pde & 1) || !(*pte & 1)) {
            if (get_a_page(sp) == 0) {
                kprintf("[exec] get_a_page for user stack failed\n");
                return -1;
            }
        }
    }

    ustack_ptr = USER_STACK3_VADDR + PAGE_SIZE;             

    for (i = 0; i < MAX_ARG_NR; ++i) {
        argv_user_addrs[i] = 0;
    }

    if (argc > 0) {

        for (i = (int32_t)argc - 1; i >= 0; --i) {
            slen = strlen(argv[i]) + 1;                   
            ustack_ptr -= slen;

            ustack_ptr &= 0xfffffffc;

            if (ustack_ptr < USER_STACK3_VADDR) {
                kprintf("[exec] argv too large for user stack\n");
                return -1;
            }

            memcpy((void*)ustack_ptr, argv[i], slen);

            argv_user_addrs[i] = ustack_ptr;
        }
    }

    ustack_ptr &= 0xfffffffc;

    ustack_ptr -= sizeof(uint32_t);
    *((uint32_t*)ustack_ptr) = 0;

    for (i = (int32_t)argc - 1; i >= 0; --i) {
        ustack_ptr -= sizeof(uint32_t);
        *((uint32_t*)ustack_ptr) = argv_user_addrs[i];
    }

    argv_user_base = ustack_ptr;

    ps = (struct Registers*)(cur->kernel_stack_top - THREAD_STACK_SIZE + 0x100);
    ps->edi = 0;
    ps->esi = 0;
    ps->ebp = 0;
    ps->esp = 0;
    ps->ebx = argv_user_base;                              
    ps->ecx = argc;                             
    ps->edx = 0;
    ps->eax = 0;
    ps->gs = SELECTOR_U_DATA;
    ps->fs = SELECTOR_U_DATA;
    ps->es = SELECTOR_U_DATA;
    ps->ds = SELECTOR_U_DATA;
    ps->int_no = 0;
    ps->err_code = 0;
    ps->eip = (uint32_t)entry_point;
    ps->cs = SELECTOR_U_CODE;
    ps->eflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;
    ps->user_esp = ustack_ptr;               
    ps->ss = SELECTOR_U_DATA;
    __asm__ volatile("movl %0, %%esp; jmp intr_exit" : : "g"(ps) : "memory");
    return 0;
}
