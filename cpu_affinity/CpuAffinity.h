#ifndef ZL_CPU_AFFINITY_H
#define ZL_CPU_AFFINITY_H

namespace zl
{

int getCpuCores();

int setCurThreadCpuAffinity(int cpu_id);

int setCurProccessCpuAffinity(int cpu_id);

int getCpuIdOfCurThread();

int getCpuIdOfCurProccess();

}

#endif  /// ZL_CPU_AFFINITY_H