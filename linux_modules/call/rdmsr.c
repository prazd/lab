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
    PPIN=0x4f, // unique processor id
    IA32_EFER=0xC0000080,
};

enum IA32_EFER_BIT_VAL {
    SYSCALL_SYSRET_ENABLE=0x0,
    LONG_MODE_ENABLE=0x8,
    LONG_MODE_ACTIVE=0xa,
    NX_ENABLE=0xb,
    AMD_SVM_ENABLE=0xc,
    FFXSR=0xe,
    AMD_TRANSLATION_CACHE_EXTENSION=0xf,
};

#define IA32_TABLE \
    X(SSE_M,      0x1,  SYSCALL_SYSRET_ENABLE,           "SYSCALL_SYSRET_ENABLE") \
    X(LME_M,      0x2,  LONG_MODE_ENABLE,                "LONG_MODE_ENABLE") \
    X(LMA_M,      0x4,  LONG_MODE_ACTIVE,                "LONG_MODE_ACTIVE") \
    X(NXE_M,      0x8,  NX_ENABLE,                       "NX_ENABLE") \
    X(AMD_SVME_M, 0x10, AMD_SVM_ENABLE,                  "AMD_SVM_ENABLE") \
    X(FFXSR_M,    0x20, FFXSR,                           "FFXSR") \
    X(AMD_TCE_M,  0x40, AMD_TRANSLATION_CACHE_EXTENSION, "AMD_TRANSLATION_CACHE_EXTENSION") \

typedef enum {
#define X(id, enable_pos, IA32_pos, str) id=enable_pos,
    IA32_TABLE
#undef X
} IA32_ENABLE;

int IA32_by_enable_pos(int enable_pos){
        switch(enable_pos){
#define X(id, enable_pos, IA32_pos,str)  case id: return IA32_pos;
    IA32_TABLE
#undef X
        default: return -1;
    }
}

const char* IA32_to_str(int n){
        switch(n){
#define X(id, enable_pos, IA32_pos,str)  case id: return str;
    IA32_TABLE
#undef X
        default: return "";
    }
}

#define IOCTL_CALL 'p'
#define IOCTL_RDMSR_HANDLE \
    _IOWR(IOCTL_CALL, 0, unsigned long)

unsigned long call(int msr){
   int fd = open("/dev/ioctl_call", O_RDONLY);
   unsigned long msr_req = msr;
   if (ioctl(fd, IOCTL_RDMSR_HANDLE, &msr_req) == -1){
      printf("ERROR\n");
      close(fd);
      abort();
    }
    close(fd);
    return msr_req;
}


#define PRINT_MSR(MSR_N) printf(#MSR_N":%lx\n", call(MSR_N));

void print_IA32(unsigned char mask) {
    printf("\nIA32_EFER:\n");
    unsigned res = call(IA32_EFER);
    for (int i = 0; i < 7; ++i){
        if (mask & 0x1) printf("%s:%u\n", IA32_to_str(1 << i), ((res >> (IA32_by_enable_pos(1 << i))) & 1));
        mask >>= 1;
    }
}

int main(){
    PRINT_MSR(IA32_THERM_STATUS)
    PRINT_MSR(MSR_TEMPERATURE_TARGET)
    PRINT_MSR(IA32_PACKAGE_THERM_STATUS)
    PRINT_MSR(MSR_PLATFORM_INFO)
    PRINT_MSR(IA32_PERF_STATUS)
    PRINT_MSR(IA32_MPERF)
    PRINT_MSR(IA32_APERF)
    PRINT_MSR(MSR_RAPL_POWER_UNIT)
    PRINT_MSR(MSR_PKG_ENERGY_STATUS)
    PRINT_MSR(MSR_DRAM_ENERGY_STATUS)
    PRINT_MSR(IA32_APIC_BASE)
    PRINT_MSR(IA32_SPEC_CTRL)
    PRINT_MSR(PMC1)
    PRINT_MSR(PPIN)
    print_IA32(0x7f);
    return 0;
}