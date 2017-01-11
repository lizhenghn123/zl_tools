#include <stdio.h>
#include "CpuAffinity.h"
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/sysinfo.h>

int main()
{
    int cpuCores = zl::getCpuCores();
    printf("CPU has %d core\n", cpuCores);

    for(int cpuid = 0; cpuid < cpuCores; cpuid++)
    {
        if(zl::setCurThreadCpuAffinity(cpuid) != 0)      
        {
            printf("warning: could not set CPU affinity, continuing\n");
        }

        int cpu_id = zl::getCpuIdOfCurThread();
        if(cpu_id >= 0)
        {
            printf("now this process(%d) is running on processor : %d\n", ::getpid(), cpu_id);
            assert(cpu_id == cpuid);
        }
        else
        {
            printf("error, cannot get cpu id on this process, return value: %d\n", cpu_id);
        }

    }

    return 0;
}