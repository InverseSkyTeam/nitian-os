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

$(BUILD_DIR)/ioc.o: $(SRC_DIR)/kernel/initer/io/io.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pic.o: $(SRC_DIR)/kernel/initer/pic/pic.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pit.o: $(SRC_DIR)/kernel/initer/pit/pit.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idt.o: $(SRC_DIR)/kernel/initer/idt/idt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/interrupt.o: $(SRC_DIR)/kernel/initer/idt/interrupt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.o: $(SRC_DIR)/kernel/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/assert.o: $(SRC_DIR)/kernel/assert.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/str.o: $(SRC_DIR)/kernel/lib/str/str.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/bitmap.o: $(SRC_DIR)/kernel/memory/bitmap/bitmap.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/paging.o: $(SRC_DIR)/kernel/memory/paging/paging.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pool.o: $(SRC_DIR)/kernel/memory/pool/pool.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(BUILD_DIR)/kernel.o \
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
						 $(BUILD_DIR)/paging.o \
						 $(BUILD_DIR)/pool.o \
                         $(LINKER_DIR)/kernel.ld | $(BUILD_DIR)
	$(LD) -T $(LINKER_DIR)/kernel.ld -o $@ \
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
		  $(BUILD_DIR)/paging.o \
		  $(BUILD_DIR)/pool.o

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