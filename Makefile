BUILD_DIR   = build
SRC_DIR     = src
LINKER_DIR  = linker
SCRIPTS_DIR = scripts

ASM     = nasm
CC      = zig cc
LD      = ld.lld        
OBJCOPY = objcopy
PYTHON  = python3       

CFLAGS = -target x86-freestanding -ffreestanding -fno-builtin -fno-sanitize=all

KERNEL_HDRS := $(wildcard $(SRC_DIR)/kernel/include/*.h \
                         $(SRC_DIR)/kernel/include/asm/*.h \
                         $(SRC_DIR)/kernel/lib/*/*.h \
                         $(SRC_DIR)/kernel/memory/*/*.h \
                         $(SRC_DIR)/kernel/thread/*.h \
                         $(SRC_DIR)/kernel/device/*.h \
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
		  $(BUILD_DIR)/keyboard.o

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

.PHONY: all floppy run clean

all: floppy

floppy: $(BUILD_DIR)/floppy.img

run: floppy
	qemu-system-i386 -m 4G -fda $(BUILD_DIR)/floppy.img -debugcon stdio

clean:
	rm -rf $(BUILD_DIR)

# 头文件依赖(由 -MMD 生成), 保证修改 .h 后对应 .o 自动重编
-include $(BUILD_DIR)/*.d