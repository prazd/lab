#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

volatile static int started = 0;

#define LOG 1

static void call(const char* str, void(*f)()){
#if LOG
    printk("%s:", str);
#endif

    f();

#if LOG
    printk(" OK\n");
#endif
}

#define CALL(x) call(#x, x);

// start() jumps here in supervisor mode on all CPUs.
void
main()
{
  if (cpuid() == 0) {
    consoleinit();
    printkinit();
    printk("\n");
    printk("xv6 kernel is booting\n");
    printk("\n");
    CALL(kinit)            // physical page allocator
    CALL(kvminit)          // create kernel page table
    CALL(kvminithart)      // turn on paging
    CALL(procinit)         // process table
    CALL(trapinit)         // trap vectors
    CALL(trapinithart)     // install kernel trap vector
    CALL(plicinit)         // set up interrupt controller
    CALL(plicinithart)     // ask PLIC for device interrupts
    CALL(binit)            // buffer cache
    CALL(iinit)            // inode table
    CALL(fileinit)         // file table
    CALL(virtio_disk_init) // emulated hard disk
    CALL(userinit)         // first user process
    __atomic_store_n(&started, 1, __ATOMIC_RELEASE);
  } else {
    while (__atomic_load_n(&started, __ATOMIC_ACQUIRE) == 0)
      ;

    printk("hart %d starting\n", cpuid());
    kvminithart();  // turn on paging
    trapinithart(); // install kernel trap vector
    plicinithart(); // ask PLIC for device interrupts
  }

  scheduler();
}
