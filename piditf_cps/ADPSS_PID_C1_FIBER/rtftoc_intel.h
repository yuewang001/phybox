#ifndef _RTFTOC_INTEL_H
#define _RTFTOC_INTEL_H
#ifdef __cplusplus   
extern   "C"   {   
#endif   

void *create_rt_thread_(void *func);

void fortran_rtclock_gettime_ (double *curr_time);

void fortran_clock_sleep_(int *usec);

void fortran_clock_sleepto_(double *sleepto);

void fortran_fifo_open_(char *devname, int *fno);

void fortran_fifo_close_(char *devname,int *fno,int *nret);

//void init_rt_env_();

//int dpdk_init();

//void dpdk_release();

#ifdef __cplusplus
}
#endif
#endif
