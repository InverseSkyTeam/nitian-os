BUILD_DIR   = build
SRC_DIR     = src
LINKER_DIR  = linker
SCRIPTS_DIR = scripts

.DEFAULT_GOAL := all

ASM     = nasm
CC      = zig cc
LD      = ld.lld        
OBJCOPY = objcopy
PYTHON  = python3       

CFLAGS = -target x86-freestanding -ffreestanding -fno-builtin -fno-sanitize=all

UP_CFLAGS = $(CFLAGS) -I $(SRC_DIR)/kernel/lib/user -I $(SRC_DIR)/kernel/lib/str -I $(SRC_DIR)/kernel/lib
UP_LDFLAGS = -s -m elf_i386 -Ttext 0x8048000 -e main

KERNEL_HDRS := $(wildcard $(SRC_DIR)/kernel/include/*.h \
                         $(SRC_DIR)/kernel/include/asm/*.h \
                         $(SRC_DIR)/kernel/lib/*/*.h \
                         $(SRC_DIR)/kernel/lib/user/*.h \
                         $(SRC_DIR)/kernel/memory/*/*.h \
                         $(SRC_DIR)/kernel/thread/*.h \
                         $(SRC_DIR)/kernel/device/*.h \
                         $(SRC_DIR)/kernel/fs/*.h \
                         $(SRC_DIR)/kernel/userprog/*.h \
                         $(SRC_DIR)/kernel/syscall/*.h \
                         $(SRC_DIR)/kernel/shell/*.h \
                         $(SRC_DIR)/kernel/initer/*/*.h)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.bin: $(SRC_DIR)/boot/boot.asm | $(BUILD_DIR)
	$(ASM) -f bin $< -o $@

$(BUILD_DIR)/loader.bin: $(SRC_DIR)/boot/loader.asm | $(BUILD_DIR)
	$(ASM) -f bin $< -o $@

$(BUILD_DIR)/func.o: $(SRC_DIR)/kernel/asmCall/func.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/io.o: $(SRC_DIR)/kernel/asmCall/io.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/stub.o: $(SRC_DIR)/kernel/asmCall/stub.asm | $(BUILD_DIR)	
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/entry.o: $(SRC_DIR)/kernel/asmCall/entry.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/switch.o: $(SRC_DIR)/kernel/asmCall/switch.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/ioc.o: $(SRC_DIR)/kernel/initer/io/io.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pic.o: $(SRC_DIR)/kernel/initer/pic/pic.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pit.o: $(SRC_DIR)/kernel/initer/pit/pit.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: $(SRC_DIR)/kernel/initer/idt/idt.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupt.o: $(SRC_DIR)/kernel/initer/idt/interrupt.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel/main.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/assert.o: $(SRC_DIR)/kernel/assert.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/str.o: $(SRC_DIR)/kernel/lib/str/str.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bitmap.o: $(SRC_DIR)/kernel/memory/bitmap/bitmap.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pool.o: $(SRC_DIR)/kernel/memory/pool/pool.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/list.o: $(SRC_DIR)/kernel/lib/list/list.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/thread.o: $(SRC_DIR)/kernel/thread/thread.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/sync.o: $(SRC_DIR)/kernel/thread/sync.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ioqueue.o: $(SRC_DIR)/kernel/device/ioqueue.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: $(SRC_DIR)/kernel/device/keyboard.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ide.o: $(SRC_DIR)/kernel/device/ide.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fs.o: $(SRC_DIR)/kernel/fs/fs.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/inode.o: $(SRC_DIR)/kernel/fs/inode.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/dir.o: $(SRC_DIR)/kernel/fs/dir.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/file.o: $(SRC_DIR)/kernel/fs/file.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt.o: $(SRC_DIR)/kernel/initer/gdt/gdt.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tss.o: $(SRC_DIR)/kernel/initer/tss/tss.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/process.o: $(SRC_DIR)/kernel/userprog/process.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/shell.o: $(SRC_DIR)/kernel/shell/shell.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/buildin_cmd.o: $(SRC_DIR)/kernel/shell/buildin_cmd.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pipe.o: $(SRC_DIR)/kernel/shell/pipe.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ksyscall.o: $(SRC_DIR)/kernel/syscall/syscall.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/usyscall.o: $(SRC_DIR)/kernel/lib/user/syscall.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ustdio.o: $(SRC_DIR)/kernel/lib/user/stdio.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/exec.o: $(SRC_DIR)/kernel/userprog/exec.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/wait_exit.o: $(SRC_DIR)/kernel/userprog/wait_exit.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fork.o: $(SRC_DIR)/kernel/userprog/fork.c $(KERNEL_HDRS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/prog_no_arg.elf: $(SRC_DIR)/command/prog_no_arg.c \
                               $(SRC_DIR)/kernel/lib/user/stdio.c \
                               $(SRC_DIR)/kernel/lib/user/syscall.c \
                               $(SRC_DIR)/kernel/lib/str/str.c | $(BUILD_DIR)
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/command/prog_no_arg.c -o $(BUILD_DIR)/up_no_arg.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/stdio.c -o $(BUILD_DIR)/up_stdio.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/syscall.c -o $(BUILD_DIR)/up_syscall.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/str/str.c -o $(BUILD_DIR)/up_str.o
	$(LD) $(UP_LDFLAGS) -o $@ \
	      $(BUILD_DIR)/up_no_arg.o $(BUILD_DIR)/up_stdio.o $(BUILD_DIR)/up_syscall.o $(BUILD_DIR)/up_str.o

$(BUILD_DIR)/prog_no_arg_data.o: $(BUILD_DIR)/prog_no_arg.elf | $(BUILD_DIR)
	cd $(BUILD_DIR) && $(OBJCOPY) -I binary -O elf32-i386 -B i386 prog_no_arg.elf prog_no_arg_data.o

$(BUILD_DIR)/up_start.o: $(SRC_DIR)/command/start.asm | $(BUILD_DIR)
	$(ASM) -f elf32 $< -o $@

$(BUILD_DIR)/prog_arg.elf: $(BUILD_DIR)/up_start.o \
                           $(SRC_DIR)/command/prog_arg.c \
                           $(SRC_DIR)/command/start.asm \
                           $(SRC_DIR)/kernel/lib/user/stdio.c \
                           $(SRC_DIR)/kernel/lib/user/syscall.c \
                           $(SRC_DIR)/kernel/lib/str/str.c | $(BUILD_DIR)
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/command/prog_arg.c -o $(BUILD_DIR)/up_arg.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/stdio.c -o $(BUILD_DIR)/up_stdio.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/syscall.c -o $(BUILD_DIR)/up_syscall.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/str/str.c -o $(BUILD_DIR)/up_str.o
	$(LD) -s -m elf_i386 -Ttext 0x8048000 -e _start -o $@ \
	      $(BUILD_DIR)/up_start.o $(BUILD_DIR)/up_arg.o $(BUILD_DIR)/up_stdio.o $(BUILD_DIR)/up_syscall.o $(BUILD_DIR)/up_str.o

$(BUILD_DIR)/prog_arg_data.o: $(BUILD_DIR)/prog_arg.elf | $(BUILD_DIR)
	cd $(BUILD_DIR) && $(OBJCOPY) -I binary -O elf32-i386 -B i386 prog_arg.elf prog_arg_data.o

$(BUILD_DIR)/cat.elf: $(BUILD_DIR)/up_start.o \
                      $(SRC_DIR)/command/cat.c \
                      $(SRC_DIR)/command/start.asm \
                      $(SRC_DIR)/kernel/lib/user/stdio.c \
                      $(SRC_DIR)/kernel/lib/user/syscall.c \
                      $(SRC_DIR)/kernel/lib/str/str.c | $(BUILD_DIR)
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/command/cat.c -o $(BUILD_DIR)/up_cat.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/stdio.c -o $(BUILD_DIR)/up_stdio.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/syscall.c -o $(BUILD_DIR)/up_syscall.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/str/str.c -o $(BUILD_DIR)/up_str.o
	$(LD) -s -m elf_i386 -Ttext 0x8048000 -e _start -o $@ \
	      $(BUILD_DIR)/up_start.o $(BUILD_DIR)/up_cat.o $(BUILD_DIR)/up_stdio.o $(BUILD_DIR)/up_syscall.o $(BUILD_DIR)/up_str.o

$(BUILD_DIR)/cat_data.o: $(BUILD_DIR)/cat.elf | $(BUILD_DIR)
	cd $(BUILD_DIR) && $(OBJCOPY) -I binary -O elf32-i386 -B i386 cat.elf cat_data.o

$(BUILD_DIR)/fork_demo.elf: $(BUILD_DIR)/up_start.o \
                            $(SRC_DIR)/command/fork_demo.c \
                            $(SRC_DIR)/command/start.asm \
                            $(SRC_DIR)/kernel/lib/user/stdio.c \
                            $(SRC_DIR)/kernel/lib/user/syscall.c \
                            $(SRC_DIR)/kernel/lib/str/str.c | $(BUILD_DIR)
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/command/fork_demo.c -o $(BUILD_DIR)/up_fork.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/stdio.c -o $(BUILD_DIR)/up_stdio.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/syscall.c -o $(BUILD_DIR)/up_syscall.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/str/str.c -o $(BUILD_DIR)/up_str.o
	$(LD) -s -m elf_i386 -Ttext 0x8048000 -e _start -o $@ \
	      $(BUILD_DIR)/up_start.o $(BUILD_DIR)/up_fork.o $(BUILD_DIR)/up_stdio.o $(BUILD_DIR)/up_syscall.o $(BUILD_DIR)/up_str.o

$(BUILD_DIR)/fork_demo_data.o: $(BUILD_DIR)/fork_demo.elf | $(BUILD_DIR)
	cd $(BUILD_DIR) && $(OBJCOPY) -I binary -O elf32-i386 -B i386 fork_demo.elf fork_demo_data.o

$(BUILD_DIR)/prog_pipe.elf: $(BUILD_DIR)/up_start.o \
                            $(SRC_DIR)/command/prog_pipe.c \
                            $(SRC_DIR)/command/start.asm \
                            $(SRC_DIR)/kernel/lib/user/stdio.c \
                            $(SRC_DIR)/kernel/lib/user/syscall.c \
                            $(SRC_DIR)/kernel/lib/str/str.c | $(BUILD_DIR)
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/command/prog_pipe.c -o $(BUILD_DIR)/up_pipe.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/stdio.c -o $(BUILD_DIR)/up_stdio.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/syscall.c -o $(BUILD_DIR)/up_syscall.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/str/str.c -o $(BUILD_DIR)/up_str.o
	$(LD) -s -m elf_i386 -Ttext 0x8048000 -e _start -o $@ \
	      $(BUILD_DIR)/up_start.o $(BUILD_DIR)/up_pipe.o $(BUILD_DIR)/up_stdio.o $(BUILD_DIR)/up_syscall.o $(BUILD_DIR)/up_str.o

$(BUILD_DIR)/prog_pipe_data.o: $(BUILD_DIR)/prog_pipe.elf | $(BUILD_DIR)
	cd $(BUILD_DIR) && $(OBJCOPY) -I binary -O elf32-i386 -B i386 prog_pipe.elf prog_pipe_data.o
 
$(BUILD_DIR)/font_subset.ttf: $(SCRIPTS_DIR)/make_font_subset.py \
                               $(SRC_DIR)/kernel/lib/assets/font.ttf | $(BUILD_DIR)
	$(PYTHON) $(SCRIPTS_DIR)/make_font_subset.py \
	          $(SRC_DIR)/kernel/lib/assets/font.ttf $@

$(BUILD_DIR)/font_subset_ttf_data.o: $(BUILD_DIR)/font_subset.ttf | $(BUILD_DIR)
	cd $(BUILD_DIR) && $(OBJCOPY) -I binary -O elf32-i386 -B i386 font_subset.ttf font_subset_ttf_data.o

$(BUILD_DIR)/font_demo.elf: $(BUILD_DIR)/up_start.o \
                            $(SRC_DIR)/command/font_demo.c \
                            $(SRC_DIR)/command/start.asm \
                            $(SRC_DIR)/kernel/lib/stb_truetype.h \
                            $(SRC_DIR)/kernel/lib/user/stdio.c \
                            $(SRC_DIR)/kernel/lib/user/syscall.c \
                            $(SRC_DIR)/kernel/lib/str/str.c | $(BUILD_DIR)
	$(CC) $(UP_CFLAGS) -Os -c $(SRC_DIR)/command/font_demo.c -o $(BUILD_DIR)/up_font.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/stdio.c -o $(BUILD_DIR)/up_stdio.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/user/syscall.c -o $(BUILD_DIR)/up_syscall.o
	$(CC) $(UP_CFLAGS) -c $(SRC_DIR)/kernel/lib/str/str.c -o $(BUILD_DIR)/up_str.o
	$(LD) -s -m elf_i386 -Ttext 0x8048000 -e _start -o $@ \
	      $(BUILD_DIR)/up_start.o $(BUILD_DIR)/up_font.o $(BUILD_DIR)/up_stdio.o $(BUILD_DIR)/up_syscall.o $(BUILD_DIR)/up_str.o

$(BUILD_DIR)/font_demo_data.o: $(BUILD_DIR)/font_demo.elf | $(BUILD_DIR)
	cd $(BUILD_DIR) && $(OBJCOPY) -I binary -O elf32-i386 -B i386 font_demo.elf font_demo_data.o

$(BUILD_DIR)/kernel.elf: $(BUILD_DIR)/entry.o \
                         $(BUILD_DIR)/kernel.o \
                         $(BUILD_DIR)/func.o \
                         $(BUILD_DIR)/ioc.o \
						 $(BUILD_DIR)/io.o \
                         $(BUILD_DIR)/pic.o \
						 $(BUILD_DIR)/pit.o \
						 $(BUILD_DIR)/stub.o \
						 $(BUILD_DIR)/idt.o \
						 $(BUILD_DIR)/interrupt.o \
						 $(BUILD_DIR)/assert.o \
						 $(BUILD_DIR)/str.o \
						 $(BUILD_DIR)/bitmap.o \
						 $(BUILD_DIR)/pool.o \
						 $(BUILD_DIR)/list.o \
						 $(BUILD_DIR)/switch.o \
						 $(BUILD_DIR)/thread.o \
						 $(BUILD_DIR)/sync.o \
						 $(BUILD_DIR)/ioqueue.o \
						 $(BUILD_DIR)/keyboard.o \
						 $(BUILD_DIR)/ide.o \
						 $(BUILD_DIR)/fs.o \
						 $(BUILD_DIR)/inode.o \
						 $(BUILD_DIR)/dir.o \
						 $(BUILD_DIR)/file.o \
						 $(BUILD_DIR)/gdt.o \
						 $(BUILD_DIR)/tss.o \
						 $(BUILD_DIR)/process.o \
						 $(BUILD_DIR)/exec.o \
						 $(BUILD_DIR)/shell.o \
						 $(BUILD_DIR)/buildin_cmd.o \
						 $(BUILD_DIR)/ksyscall.o \
						 $(BUILD_DIR)/usyscall.o \
						 $(BUILD_DIR)/ustdio.o \
						 $(BUILD_DIR)/prog_no_arg_data.o \
						 $(BUILD_DIR)/prog_arg_data.o \
						 $(BUILD_DIR)/cat_data.o \
						 $(BUILD_DIR)/fork_demo_data.o \
						 $(BUILD_DIR)/prog_pipe_data.o \
						 $(BUILD_DIR)/font_demo_data.o \
						 $(BUILD_DIR)/font_subset_ttf_data.o \
						 $(BUILD_DIR)/wait_exit.o \
						 $(BUILD_DIR)/fork.o \
						 $(BUILD_DIR)/pipe.o \
                         $(LINKER_DIR)/kernel.ld | $(BUILD_DIR)
	$(LD) -T $(LINKER_DIR)/kernel.ld -o $@ \
	      $(BUILD_DIR)/entry.o \
	      $(BUILD_DIR)/kernel.o \
	      $(BUILD_DIR)/func.o \
	      $(BUILD_DIR)/ioc.o \
		  $(BUILD_DIR)/io.o \
	      $(BUILD_DIR)/pic.o \
		  $(BUILD_DIR)/pit.o \
		  $(BUILD_DIR)/stub.o \
		  $(BUILD_DIR)/idt.o \
	      $(BUILD_DIR)/interrupt.o \
		  $(BUILD_DIR)/assert.o \
		  $(BUILD_DIR)/str.o \
		  $(BUILD_DIR)/bitmap.o \
		  $(BUILD_DIR)/pool.o \
		  $(BUILD_DIR)/list.o \
		  $(BUILD_DIR)/switch.o \
		  $(BUILD_DIR)/thread.o \
		  $(BUILD_DIR)/sync.o \
		  $(BUILD_DIR)/ioqueue.o \
		  $(BUILD_DIR)/keyboard.o \
		  $(BUILD_DIR)/ide.o \
		  $(BUILD_DIR)/fs.o \
		  $(BUILD_DIR)/inode.o \
		  $(BUILD_DIR)/dir.o \
		  $(BUILD_DIR)/file.o \
		  $(BUILD_DIR)/gdt.o \
		  $(BUILD_DIR)/tss.o \
		  $(BUILD_DIR)/process.o \
		  $(BUILD_DIR)/exec.o \
		  $(BUILD_DIR)/shell.o \
		  $(BUILD_DIR)/buildin_cmd.o \
		  $(BUILD_DIR)/ksyscall.o \
		  $(BUILD_DIR)/usyscall.o \
		  $(BUILD_DIR)/ustdio.o \
		  $(BUILD_DIR)/prog_no_arg_data.o \
		  $(BUILD_DIR)/prog_arg_data.o \
		  $(BUILD_DIR)/cat_data.o \
		  $(BUILD_DIR)/fork_demo_data.o \
		  $(BUILD_DIR)/prog_pipe_data.o \
		  $(BUILD_DIR)/font_demo_data.o \
		  $(BUILD_DIR)/font_subset_ttf_data.o \
		  $(BUILD_DIR)/wait_exit.o \
		  $(BUILD_DIR)/fork.o \
		  $(BUILD_DIR)/pipe.o

$(BUILD_DIR)/kernel.bin: $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@

FLOPPY_DEPS = $(BUILD_DIR)/boot.bin \
              $(BUILD_DIR)/loader.bin \
              $(BUILD_DIR)/kernel.bin \
              $(SCRIPTS_DIR)/mkfloppy.py

$(BUILD_DIR)/floppy.img: $(FLOPPY_DEPS) | $(BUILD_DIR)
	$(PYTHON) $(SCRIPTS_DIR)/mkfloppy.py \
	          $(BUILD_DIR)/boot.bin \
	          $(BUILD_DIR)/loader.bin \
	          $(BUILD_DIR)/kernel.bin \
	          $@

$(BUILD_DIR)/test_hd.img: $(SCRIPTS_DIR)/make_disk.py | $(BUILD_DIR)
	$(PYTHON) $(SCRIPTS_DIR)/make_disk.py $@

.PHONY: all floppy run clean

all: floppy

floppy: $(BUILD_DIR)/floppy.img

run: floppy $(BUILD_DIR)/test_hd.img
	qemu-system-i386 -accel tcg,tb-size=256 -m 1G -fda $(BUILD_DIR)/floppy.img \
	                 -hda $(BUILD_DIR)/test_hd.img -debugcon stdio

clean:
	rm -rf $(BUILD_DIR)

-include $(BUILD_DIR)/*.d