#ifndef _FPBENCH_H_
#define _FPBENCH_H_

#include "zns-tools.h"
#include <fcntl.h>
#include <sys/wait.h>

#define F_LINUX_SPECIFIC_BASE 1024

#if __linux__ && !defined(F_SET_RW_HINT)
#define F_GET_RW_HINT (F_LINUX_SPECIFIC_BASE + 11)
#define F_SET_RW_HINT (F_LINUX_SPECIFIC_BASE + 12)
#endif

#if __linux__ && !defined(F_SET_FILE_RW_HINT)
#define F_GET_FILE_RW_HINT (F_LINUX_SPECIFIC_BASE + 13)
#define F_SET_FILE_RW_HINT (F_LINUX_SPECIFIC_BASE + 14)
#endif

struct workload {
    char *filename;  /* file name to write */
    uint8_t rw_hint; /* read/write hint for the file in this workload */
    uint64_t bsize;  /* block size for writes to be submitted as */
    uint64_t fsize;  /* file size to write */
};

struct workload_manager {
    struct workload *wl; /* each of the workloads */
    uint16_t nr_wls;     /* total number of workloads in wls */
    uint16_t nr_jobs;    /* number of concurrent jobs to run */
    int data_fd;         /* fd to /dev/urandom to read data from */
    uint32_t segment_ctr;
    uint32_t cold_ctr;
    uint32_t warm_ctr;
    uint32_t hot_ctr;
};

#define FORMATTER                                                              \
    MSG("====================================================================" \
        "====================================================================" \
        "======================================\n");

extern struct workload_manager wl_man;

#endif
