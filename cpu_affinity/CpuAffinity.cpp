#include "CpuAffinity.h"
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/sysinfo.h>

namespace zl
{

    int getCpuCores()
    {
        return ::get_nprocs();
    }

    int setCurThreadCpuAffinity(int cpu_id)
    {
        cpu_set_t  mask;
        CPU_ZERO(&mask);
        CPU_SET(cpu_id, &mask);
        pthread_t tid = pthread_self();

        int ret = pthread_setaffinity_np(tid, sizeof(mask), &mask);
        if(ret != 0)
        {
            printf("set tid(%d) to cpu(%d) affinity failed, return value(%d)\n", tid, cpu_id, ret);
        }

        return ret;
    }

    int setCurProccessCpuAffinity(int cpu_id)
    {
        cpu_set_t  mask;
        CPU_ZERO(&mask);
        CPU_SET(cpu_id, &mask);
        pid_t pid = getpid();

        int ret = sched_setaffinity(pid, sizeof(mask), &mask);
        if(ret != 0)
        {
            printf("set pid(%d) to cpu(%d) affinity failed, return value(%d)\n", pid, cpu_id, ret);
        }

        return ret;
    }


    int getCpuIdOfCurThread()
    {
        int cpuCores = getCpuCores();

        cpu_set_t mask;
        CPU_ZERO(&mask);
        pthread_t tid = pthread_self();

        int ret = pthread_getaffinity_np (tid, sizeof(mask), &mask);
        if(ret != 0)
        {
            printf("get cpuid on tid(%d) failed, return value(%d)\n", tid, ret);
            return -1;
        }

        for(int i = 0; i < cpuCores; i++)
        {
            if(CPU_ISSET(i, &mask))
            {
                return i;
            }
        }

        return -1;
    }

    int getCpuIdOfCurProccess()
    {
        int cpuCores = getCpuCores();

        cpu_set_t mask;
        CPU_ZERO(&mask);
        if(sched_getaffinity(0, sizeof(mask), &mask) == -1)
        {
            printf("warning: cound not get cpu affinity\n");
            return -1;
        }

        for(int i = 0; i < cpuCores; i++)
        {
            if(CPU_ISSET(i, &mask))
            {
                return i;
            }
        }

        return -1;
    }
}