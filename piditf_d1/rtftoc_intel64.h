#ifndef _RTFTOC_INTEL_H
#define _RTFTOC_INTEL_H
#ifdef __cplusplus   
extern   "C"   {   
#endif   
void create_rt_thread_(void *func);
void create_rt_simplethread_(void *func);
/*
 * Locks process' current and future pages in memory.
 */
void lock_pages_(void);
void  fortran_rtclock_gettime_ (double *curr_time);

void fortran_clock_sleep_(int *usec);

void fortran_clock_sleepto_(double *sleepto);
void fortran_rt_finilize_(void);

void fortran_fifo_open_(char *devname, int *fno);

void fortran_fifo_truncate_(int *fno,int *length);

// FIFO operation
void fortran_fifo_write_(int *fno,char *sline, int *nwrite, int *nret);
// FIFO read
void fortran_fifo_read_(int *fno,char *sline,int *maxlen, int *nread);

// FIFO close
void fortran_fifo_close_(char *devname,int *fno,int *nret);
void fortran_rtprints_(char * s);
#ifdef __cplusplus
}
#endif
#endif
