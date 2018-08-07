#include <sys/rt_types.h>
#include <sys/rt_stat.h>
#include <rt_fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rt_stdio.h>
#include <rt_pthread.h>
#include <sys/mman.h>
//#include <time.h>
#include <rt_time.h>


/*
 * Fortran passes arguments by reference whereas C passes arguments by value. When you put a 
 * variable name into a function call from Fortran, the corresponding C function receives a 
 * pointer to that variable. Variable's value can be accessed by dereferencing it.
 */

rt_pthread_t thread;

/* 
 * Creates a RT thread with parameter "func" as address of function to be 
 * executed as RT thread
 */
#if 0
void create_rt_thread_(void * func)
{
	int ret = rt_pthread_create(&thread, NULL, (void *)func, NULL);
	//ret = rt_pthread_setfp_np(thread, 1);

	printf("rt_pthread_create returns %d\n", ret);
}
#else
void create_rt_thread_(void * func)
{
	rt_pthread_attr_t attr;
    struct rt_sched_param sched_param;
	int ret, fd;
	void *ptr;
	int Maxstacksize;
	Maxstacksize=1<<26;
	ptr = malloc(Maxstacksize);
	if (!ptr) {
		printf("Error, unable to allocate stack for thread \n");
		exit(1);
	}
	rt_growstack(1<<20);
	rt_pthread_attr_init(&attr);
	rt_pthread_attr_setstacksize(&attr,Maxstacksize);
	rt_pthread_attr_setstackaddr(&attr,ptr);

	fd = rt_open("/dev/rtcpu", 0);
	ret = rt_ioctl(fd, 1, 0);
	if (0==ret) ret = rt_ioctl(fd, 1, 0);    //get cpu again, aviod 0
	printf("PHY: rtcpu returns %d\n", ret);
	rt_close(fd);
	rt_pthread_attr_setcpu_np(&attr, ret);

	sched_param.sched_priority = rt_sched_get_priority_max(RT_SCHED_DEFAULT);
	rt_pthread_attr_setschedparam( &attr, &sched_param );

	ret = rt_pthread_create(&thread, &attr, (void *)func, NULL);
	if (0==ret) {
		int np;
		rt_pthread_attr_getcpu_np(&attr, &np);
		printf("PHY: rt_pthread_create returns %d for thread id %d, and its cpu no. is %d\n", ret,thread,np);
   }else{
		printf("PHY: rt_pthread_create returns %d for thread id %d\n", ret,thread);
	}
}
#endif

/*
 * Locks process' current and future pages in memory.
 */
void lock_pages_(void)
{
	mlockall(MCL_CURRENT | MCL_FUTURE);
}

void unlock_pages_(void)
{
    munlockall();
}

void  fortran_rtclock_gettime_ (double *curr_time)
{
        struct rt_timespec curr;

        rt_clock_gettime( RT_CLOCK_REALTIME, &curr);
        *curr_time = curr.tv_sec+ (curr.tv_nsec*1.0)/NSECS_PER_SEC;
}

void fortran_clock_sleep_(int *usec)
{
      struct rt_timespec begin,add;
      add.tv_sec=*usec/1000000;
      add.tv_nsec=(*usec-add.tv_sec*1000000)*1000;
      rt_clock_gettime( RT_CLOCK_REALTIME, &begin );
      rt_timespec_add(&begin, &add);
      rt_clock_nanosleep(RT_CLOCK_REALTIME, RT_TIMER_ABSTIME, &begin, NULL);   
}

void fortran_clock_sleepto_(double *sleepto)
{
      struct rt_timespec end,tmtp;
      
      end.tv_sec=*sleepto;
      end.tv_nsec=(*sleepto-end.tv_sec)*NSECS_PER_SEC;
      tmtp.tv_sec=0;
      tmtp.tv_nsec=10e3;
//      rt_clock_nanosleep(RT_CLOCK_REALTIME, RT_TIMER_ABSTIME|RT_TIMER_ADVANCE , &end, &tmtp);
      rt_clock_nanosleep(RT_CLOCK_REALTIME, RT_TIMER_ABSTIME , &end, NULL);
}

void fortran_rt_finilize_(void)
{
//        rt_main_wait();

        rt_pthread_cancel( thread );
        rt_pthread_join( thread, NULL );
}

void fortran_rtprints_(char * s)
{
	rt_printf("%s",s);
}

void fortran_prints_(char * s)
{
	printf("%s",s);
}

void fortran_usleep_(int * us)
{
	usleep(*us);
}

// FIFO operation
//void fortran_fifo_open_(int *chno, int *fno)
//{
//   char fname[20];
//   sprintf(fname,"/dev/rtf%d",*chno);
//	*fno=rt_open(fname, RT_O_NONBLOCK | RT_O_RDONLY | RT_O_WRONLY | RT_O_CREAT);
//}

void fortran_fifo_open_(char *devname, int *fno)
{
   rt_unlink(devname);
//   rt_mkfifo(devname, 0755);
   rt_mkfifo(devname, 0);
   *fno=rt_open(devname,RT_O_RDWR | RT_O_NONBLOCK);
//  rt_ftruncate(*fno, 1024*10);
   rt_ftruncate(*fno, 1024*100);   // 2008-5-2
   //printf("fortran_fifo_open %s, return %d.\n",devname,*fno);
}

void fortran_fifo_truncate_(int *fno,int *length)
{
   rt_ftruncate(*fno, *length);
}

// FIFO operation
void fortran_fifo_write_(int *fno,char *sline, int *nwrite, int *nret)
{
   *nret=rt_write(*fno,sline,*nwrite);
}
// FIFO read
void fortran_fifo_read_(int *fno,char *sline,int *maxlen, int *nread)
{
   *nread = rt_read(*fno,sline,*maxlen);
}

// FIFO close
void fortran_fifo_close_(char *devname,int *fno,int *nret)
{
   *nret = rt_close(*fno);
   *nret = rt_unlink(devname);
}
