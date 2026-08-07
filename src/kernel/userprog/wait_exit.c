// 参考: 《操作系统真相还原》(于渊) 第15章 wait与exit系统调用
#include "./wait_exit.h"
#include "../thread/thread.h"
#include "../lib/list/list.h"
#include "../memory/pool/pool.h"
#include "../memory/bitmap/bitmap.h"
#include "./process.h"
#include "../fs/file.h"
#include "../include/assert.h"

static int vaddr_owned_by_current(struct task_struct* t, uint32_t vaddr) {
    if (vaddr < USER_VADDR_START || vaddr >= 0xc0000000) {
        return 0;
    }
    uint32_t bit_idx = (vaddr - USER_VADDR_START) / PAGE_SIZE;
    if (bit_idx >= t->userprog_v_addr.vaddr_bitmap.btmp_bytes_len * 8) {
        return 0;
    }
    return bitmap_scan_test(&t->userprog_v_addr.vaddr_bitmap, bit_idx) == 1;
}

static void release_prog_resource(struct task_struct* release_thread) {
    if (release_thread->pgdir != 0) {
        uint32_t* pgdir_vaddr = (uint32_t*)release_thread->pgdir;
        for (uint32_t pde_idx = 0; pde_idx < 768; pde_idx++) {
            uint32_t pde = pgdir_vaddr[pde_idx];
            if (!(pde & 1)) continue;
            if (pde & 0x80) continue;

            uint32_t* first_pte = pte_ptr(pde_idx * 0x400000);
            uint32_t remaining = 0;
            for (uint32_t pte_idx = 0; pte_idx < 1024; pte_idx++) {
                if (!(first_pte[pte_idx] & 1)) continue;
                uint32_t vaddr = pde_idx * 0x400000 + pte_idx * PAGE_SIZE;
                if (!vaddr_owned_by_current(release_thread, vaddr)) {
                    remaining++;
                    continue;
                }
                pfree(&kernel_pool, first_pte[pte_idx] & 0xfffff000);
                first_pte[pte_idx] = 0;
            }
            if (remaining == 0) {
                pfree(&kernel_pool, pde & 0xfffff000);
                pgdir_vaddr[pde_idx] = 0;
            }
        }
        if (release_thread->userprog_v_addr.vaddr_bitmap.bits != NULL) {
            uint32_t bitmap_bytes = release_thread->userprog_v_addr.vaddr_bitmap.btmp_bytes_len;
            uint32_t bitmap_pg_cnt = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
            for (uint32_t i = 0; i < bitmap_pg_cnt; i++) {
                free_kernel_page((uint32_t)release_thread->userprog_v_addr.vaddr_bitmap.bits + i * PAGE_SIZE);
            }
            release_thread->userprog_v_addr.vaddr_bitmap.bits = NULL;
        }
    }
    for (uint32_t fd_idx = 3; fd_idx < MAX_FILES_OPEN_PER_PROC; fd_idx++) {
        if (current_task->fd_table[fd_idx] != (uint32_t)-1) {
            close_file((int)fd_idx);
        }
    }
}

static int find_hanging_child(struct list_elem* pelem, int32_t ppid) {
    struct task_struct* t = list_entry(pelem, struct task_struct, all_list_tag);
    return (t->parent_pid == ppid && t->status == TASK_HANGING);
}

static int find_child(struct list_elem* pelem, int32_t ppid) {
    struct task_struct* t = list_entry(pelem, struct task_struct, all_list_tag);
    return (t->parent_pid == ppid);
}

pid_t sys_wait(int32_t* status) {
    struct task_struct* parent = current_task;
    for (;;) {
        struct list_elem* e = g_thread_all_list.head.next;
        while (e != &g_thread_all_list.tail) {
            struct list_elem* next = e->next;
            if (find_hanging_child(e, (int32_t)parent->pid)) {
                struct task_struct* child = list_entry(e, struct task_struct, all_list_tag);
                *status = child->exit_status;
                uint32_t child_pid = child->pid;
                thread_exit(child, 0);
                return child_pid;
            }
            e = next;
        }

        struct list_elem* child = g_thread_all_list.head.next;
        while (child != &g_thread_all_list.tail) {
            if (find_child(child, (int32_t)parent->pid)) {
                break;
            }
            child = child->next;
        }
        if (child == &g_thread_all_list.tail) {
            return -1;
        }
        thread_block_with_status(TASK_WAITING);
    }
}

void sys_exit(int32_t status) {
    struct task_struct* cur = current_task;
    cur->exit_status = (int8_t)status;

    struct list_elem* e = g_thread_all_list.head.next;
    while (e != &g_thread_all_list.tail) {
        struct task_struct* t = list_entry(e, struct task_struct, all_list_tag);
        if (t->parent_pid == (int32_t)cur->pid) {
            t->parent_pid = (int32_t)g_init_pid;
        }
        e = e->next;
    }
    release_prog_resource(cur);
    struct task_struct* parent = pid2thread(cur->parent_pid);
    if (parent && parent->status == TASK_WAITING) {
        thread_unblock(parent);
    }
    thread_block_with_status(TASK_HANGING);
}
