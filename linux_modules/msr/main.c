#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

enum MSR {
    IA32_THERM_STATUS=0x19c,
    MSR_TEMPERATURE_TARGET=0x1a2,
    IA32_PACKAGE_THERM_STATUS=0x1b1,
    MSR_PLATFORM_INFO=0xCE,
    IA32_PERF_STATUS=0x198,
    IA32_MPERF=0xe7,
    IA32_APERF=0xe8,
    MSR_RAPL_POWER_UNIT=0x606,
    MSR_PKG_ENERGY_STATUS=0x611,
    MSR_DRAM_ENERGY_STATUS=0x619,
    IA32_APIC_BASE=0x1b,
    IA32_SPEC_CTRL=0x48,
    PMC1=0xc1, // 0xc1-0xc8
    PPIN=0x4f // unique processor id
};


#define IOCTL_READ_MSR 'p'
#define IOCTL_READ_MSR_HANDLE \
    _IOWR(IOCTL_READ_MSR, 1, unsigned long)


void call(const char* str, int msr){
   int fd = open("/dev/ioctl_read_msr", O_RDONLY);
   unsigned long msr_req = msr;
   if (ioctl(fd, IOCTL_READ_MSR_HANDLE, &msr_req) == -1){
      printf("ERROR\n");
      close(fd);
      abort();
    }
    printf("%s:%lx\n", str, msr_req);
    close(fd);
}

#define CALL(MSR_N) call(#MSR_N, MSR_N);

int main(){
    CALL(IA32_THERM_STATUS)
    CALL(MSR_TEMPERATURE_TARGET)
    CALL(IA32_PACKAGE_THERM_STATUS)
    CALL(MSR_PLATFORM_INFO)
    CALL(IA32_PERF_STATUS)
    CALL(IA32_MPERF)
    CALL(IA32_APERF)
    CALL(MSR_RAPL_POWER_UNIT)
    CALL(MSR_PKG_ENERGY_STATUS)
    CALL(MSR_DRAM_ENERGY_STATUS)
    CALL(IA32_APIC_BASE)
    CALL(IA32_SPEC_CTRL)
    CALL(PMC1)
    CALL(PPIN)

   return 0;
}