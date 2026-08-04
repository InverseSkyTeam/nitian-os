#!/usr/bin/env python3
# 参考: 《操作系统真相还原》第10章 输入输出系统 / 第11章 用户进程
import subprocess
import sys
import time

QEMU = "D:/Software/qemu/qemu-system-i386.exe"
FLOPPY = "build/floppy.img"
LOGFILE = "build/dbg_check.txt"


def run_qemu(sendkeys):
    import os
    if os.path.exists(LOGFILE):
        os.remove(LOGFILE)
    p = subprocess.Popen(
        [QEMU, "-m", "4G", "-fda", FLOPPY, "-display", "none",
         "-vga", "vmware", "-no-reboot", "-monitor", "stdio",
         "-debugcon", "file:" + LOGFILE],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def cmd(s):
        p.stdin.write((s + "\n").encode())
        p.stdin.flush()

    time.sleep(4)
    for k in sendkeys:
        cmd("sendkey " + k)
        time.sleep(0.1)
    time.sleep(1.5)
    cmd("quit")
    try:
        p.communicate(timeout=25)
    except subprocess.TimeoutExpired:
        p.kill()
        p.communicate()
    with open(LOGFILE, "rb") as f:
        return f.read().decode("utf-8", "replace")


def check(name, ok):
    print("[%s] %s" % ("PASS" if ok else "FAIL", name))
    return ok


def main():
    print("===== 输入输出系统 + 用户进程验证 =====")
    results = []

    text = run_qemu(["a", "b", "c", "ret"])

    results.append(check("内核自检(str)", "Kernel Inited" in text and "[OK] str" in text))
    results.append(check("内核自检(bitmap)", "[OK] bitmap" in text))
    results.append(check("内核自检(palloc)", "[OK] palloc" in text and "[OK] pfree/realloc" in text))
    results.append(check("TSS 加载(TR=0x18)", "[OK] TSS loaded, TR=0x18" in text))
    results.append(check("线程就绪", "thread mgr ready" in text))
    results.append(check("生产者完成", "producer done (100 chars)" in text))
    results.append(check("消费者完成", "consumer done (100 chars)" in text))
    results.append(check("键盘回显", "[KBD] line: abc" in text))
    results.append(check("用户进程无异常", "EXCEPTION" not in text and "ASSERT FAILED" not in text))

    print()
    if all(results):
        print("结果: ALL PASS")
        return 0
    print("结果: %d/%d PASS" % (sum(results), len(results)))
    return 1


if __name__ == "__main__":
    sys.exit(main())
