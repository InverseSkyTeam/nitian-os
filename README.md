# NiTianOS

> 一个从零编写的 x86 32 位保护模式操作系统内核学习项目。

## 简介

` NiTianOS` 是一个用于学习操作系统原理的教学型内核。它从 16 位实模式下的引导扇区起步，经加载器进入 32 位保护模式，并最终运行一个最小化的 C 内核。项目聚焦于理解底层机制：分段、中断、中断控制器与定时器。

当前内核已能在 QEMU 中引导、开启分页并稳定产生定时器中断（屏幕持续打印 `tick` 计数），证明 `IDT → PIC → PIT → IRQ` 的完整中断链路工作正常。内核运行于 **Higher Half**（虚拟地址 `0xC0000000+`），低端 3GB 地址空间留给未来的用户进程。

## 已实现功能

| 模块 | 说明 |
|------|------|
| 引导 (Boot) | `boot.asm`：512 字节主引导扇区，加载 `loader.bin` |
| 加载器 (Loader) | `loader.asm`：开启 VBE 图形模式、探测物理内存 (E820)、设置 GDT、进入保护模式、将内核拷贝到高位地址并跳入 |
| 全局描述符表 (GDT) | 在 loader 中建立代码段 / 数据段描述符 |
| 中断描述符表 (IDT) | `idt.c` 建立 256 个中断门；`stub.asm` 用宏统一生成全部入口 |
| CPU 异常 | `isr0–isr31`（区分带 / 不带错误码），由 `isr_handler` 打印异常名与现场寄存器后停机 |
| 硬件中断 (IRQ) | `irq0–irq15`，由 `irq_handler` 分发并发送 EOI |
| 中断控制器 (PIC) | `pic.c` 初始化 8259A 双片，将 IRQ 重映射至 `0x20–0x2F`，放开 IRQ0/IRQ1 |
| 定时器 (PIT) | `pit.c` 配置 8253/8254 通道 0 为 100 Hz 速率发生器，驱动 IRQ0 |
| 屏幕输出 | `io.c` 文本模式输出与 `printf`，支持 `%d %u %x %X %c %s` |
| 断言 (ASSERT) | `assert.h / assert.c`：失败时打印表达式、文件名、行号并停机（参考 OSDev Assertions） |
| 字符串库 | `lib/str`：strlen/strcpy/strcmp/strcat/strchr + memcpy/memset/memmove/memcmp（参考 OSDev C Library） |
| 位图 | `memory/bitmap`：位图分配器，高位在前（参考《操作系统真相还原》第5章） |
| 分页 + Higher Half | `entry.asm` 物理入口建页表（恒等 256MB + 高半区内核 16MB + VRAM）→ 开分页 → 跳转 `0xC0000000+` 运行；内核虚拟地址 `0xC0280000`，物理加载 `0x280000` |
| 内存池 | `memory/pool`：E820 探测物理内存，位图管理页框，`palloc`/`pfree`（参考《操作系统真相还原》第8章） |
| 线程与调度 | `thread`：任务表 + 就绪队列 + 时间片调度，`kernel_thread` 创建内核线程，`switch_to` 汇编上下文切换（参考第9章） |
| 同步机制 | `thread/sync`：信号量 `sema_down/up` 与可重入锁 `lock_acquire/release`，关中断保证原子性（参考第11章） |
| 环形缓冲区 | `device/ioqueue`：生产者-消费者模型，满/空时阻塞等待，`ioq_putchar`/`ioq_getchar`（参考第11章） |
| 键盘驱动 | `device/keyboard` + IRQ1 分发：8042 扫描码 → 双键位 keymap（shift/caps），字符写入键盘环形缓冲区 |

## 目录结构

```
NiTianOS/
├── Makefile                # 交叉编译构建（nasm + zig cc + ld.lld）
├── linker/
│   └── kernel.ld           # 内核链接脚本
├── scripts/
│   ├── mkfloppy.py         # 将 boot/loader/kernel 拼成软盘镜像
│   └── qemu_shot.py        # QEMU 无头运行并截图验证
├── src/
│   ├── boot/
│   │   ├── boot.asm        # Stage 1：引导扇区
│   │   └── loader.asm      # Stage 2：进入保护模式、建立 GDT/IDT
│   └── kernel/
│       ├── main.c          # 内核入口 KMain
│       ├── assert.c        # ASSERT 实现
│       ├── include/
│       │   ├── assert.h
│       │   ├── asmFunc.h   # C 调用汇编函数声明
│       │   └── asm/stub.h  # 中断栈帧结构
│       ├── asmCall/
│       │   ├── entry.asm   # 内核物理入口：建页表、开分页、跳转高半区
│       │   ├── func.asm    # 底层辅助（含 CR0/CR3 操作）
│       │   ├── io.asm      # 端口 I/O
│       │   └── stub.asm    # 中断入口桩
│       ├── lib/
│       │   ├── str/        # 字符串库
│       │   └── assets/     # 资源文件目录
│       ├── memory/
│       │   ├── bitmap/     # 位图分配器
│       │   └── pool/       # 物理内存池（palloc/pfree）
│       └── initer/
│           ├── io/         # 屏幕与 printf
│           ├── pic/        # 8259A PIC
│           ├── pit/        # 8253/8254 定时器
│           └── idt/        # IDT 与中断处理
└── build/                  # 构建产物（floppy.img 等）
```

## 构建与运行

### 工具链

- `nasm` —— 汇编（`elf32` / `bin`）
- `zig cc` —— C 编译（`-target x86-freestanding -ffreestanding`）
- `ld.lld` —— 链接
- `objcopy` —— 提取纯二进制
- `python3` —— 镜像打包脚本
- `qemu-system-i386` —— 运行验证

### 构建

```bash
make          
make clean    
```

### 运行

```bash
make run
# 或者手动指定
qemu-system-i386 -m 4G -fda build/floppy.img
```

内核启动后将在屏幕打印启动信息，并持续输出定时器 `tick` 计数。随后创建生产者/消费者演示线程（100 字符乒乓传输）与键盘回显线程，在 QEMU 中输入按键可在屏幕看到 `[KBD] line: ...` 行回显。

> `printf` 会同步镜像输出到 QEMU debugcon（端口 0xE9），因此 `make run` 的 `-debugcon stdio` 可在终端直接看到内核日志，便于无头调试。

## 参考资料与致谢

本项目的实现参考了以下资料：

- **内存管理**（分页机制、物理内存管理、内核堆分配等的设计与实现）参考自 **《操作系统真相还原》**（于渊 著）。
- **引导加载器（Loader）、可编程中断控制器（PIC）、中断描述符表（IDT）、全局描述符表（GDT）** 等底层机制参考自 **OSDev Wiki**：<https://wiki.osdev.org/Main_Page>。
  - GDT：<https://wiki.osdev.org/GDT>
  - IDT：<https://wiki.osdev.org/IDT>
  - PIC（8259A）：<https://wiki.osdev.org/PIC>
  - 断言：<https://wiki.osdev.org/Assertions>

## 路线图

- [x] 内存管理：物理内存探测（E820）、位图、分页、Higher Half 映射、物理内存池
- [x] 输入输出系统：内核线程与调度、信号量与锁、环形缓冲区（生产者-消费者）、键盘驱动（IRQ1）
- [ ] 内核堆分配器（基于内存池）
- [ ] 系统调用（int 0x80）
- [ ] 简单进程调度
- [ ] 文件系统
