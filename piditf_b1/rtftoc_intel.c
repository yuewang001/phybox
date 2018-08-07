#include <sys/rtl_types.h>
#include <sys/rtl_stat.h>
#include <rtl_fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rtl_stdio.h>
#include <rtl_pthread.h>
#include <sys/mman.h>
//#include <time.h>
#include <rtl_time.h>


/*
 * Fortran passes arguments by reference whereas C passes arguments by value. When you put a 
 * variable name into a function call from Fortran, the corresponding C function receives a 
 * pointer to that variable. Variable's value can be accessed by dereferencing it.
 */

rtl_pthread_t thread;

/* 
 * Creates a RT thread with parameter "func" as address of function to be 
 * executed as RT thread
 */
#if 0
void create_rt_thread_(void * func)
{
   int ret = rtl_pthread_create(&thread, NULL, (void *)func, NULL);
   //ret = rtl_pthread_setfp_np(thread, 1);

   printf("rtl_pthread_create returns %d\n", ret);
}
#else
void create_rt_thread_(void * func)
{
	rtl_pthread_attr_t attr;
	struct rtl_sched_param sched_param;
	int ret, fd;
	void *ptr;
	int Maxstacksize;
	Maxstacksize=1<<26;
	ptr = malloc(Maxstacksize);
	if (!ptr) {
		printf("Error, unable to allocate stack for thread \n");
		exit(1);
	}
	rtl_growstack(1<<20);
	rtl_pthread_attr_init(&attr);
	rtl_pthread_attr_setstacksize(&attr,Maxstacksize);
	rtl_pthread_attr_setstackaddr(&attr,ptr);

	fd = rtl_open("/dev/rtcpu", 0);
	ret = rtl_ioctl(fd, 1, 0);
	if (0==ret) ret = rtl_ioctl(fd, 1, 0);    //get cpu again, aviod 0
	printf("PHY: rtcpu returns %d\n", ret);
	rtl_close(fd);
	rtl_pthread_attr_setcpu_np(&attr, ret);

	sched_param.sched_priority = rtl_sched_get_priority_max(RTL_SCHED_DEFAULT);
	rtl_pthread_attr_setschedparam( &attr, &sched_param );

	ret = rtl_pthread_create(&thread, &attr, (void *)func, NULL);
	if (0==ret) {
		int np;
		rtl_pthread_attr_getcpu_np(&attr, &np);
		printf("PHY: rtl_pthread_create returns %d for thread id %d, and its cpu no. is %d\n", ret,thread,np);
	}else{
		printf("PHY: rtl_pthread_create returns %d for thread id %d\n", ret,thread);
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
        struct rtl_timespec curr;

        rtl_clock_gettime( RTL_CLOCK_REALTIME, &curr);
        *curr_time = curr.tv_sec+ (curr.tv_nsec*1.0)/NSECS_PER_SEC;
}

void fortran_clock_sleep_(int *usec)
{
      struct rtl_timespec begin,add;
      add.tv_sec=*usec/1000000;
      add.tv_nsec=(*usec-add.tv_sec*1000000)*1000;
      rtl_clock_gettime( RTL_CLOCK_REALTIME, &begin );
      rtl_timespec_add(&begin, &add);
      rtl_clock_nanosleep(RTL_CLOCK_REALTIME, RTL_TIMER_ABSTIME, &begin, NULL);   
}

void fortran_clock_sleepto_(double *sleepto)
{
      struct rtl_timespec end,tmtp;
      
      end.tv_sec=*sleepto;
      end.tv_nsec=(*sleepto-end.tv_sec)*NSECS_PER_SEC;
      tmtp.tv_sec=0;
      tmtp.tv_nsec=10e3;
//      rtl_clock_nanosleep(RTL_CLOCK_REALTIME, RTL_TIMER_ABSTIME|RTL_TIMER_ADVANCE , &end, &tmtp);
      rtl_clock_nanosleep(RTL_CLOCK_REALTIME, RTL_TIMER_ABSTIME , &end, NULL);
}

void fortran_rt_finilize_(void)
{
//        rtl_main_wait();

        rtl_pthread_cancel( thread );
        rtl_pthread_join( thread, NULL );
}

void fortran_rtprints_(char * s)
{
   rtl_printf("%s\n",s);
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
// *fno=rtl_open(fname, RTL_O_NONBLOCK | RTL_O_RDONLY | RTL_O_WRONLY | RTL_O_CREAT);
//}

void fortran_fifo_open_(char *devname, int *fno)
{
   rtl_unlink(devname);
   //rtl_printf("after rtl_unlink %s:0\n",devname);
//   rtl_mkfifo(devname, 0755);
   rtl_mkfifo(devname, 0);
   //rtl_printf("after rtl_mkfifo:1\n");
   *fno=rtl_open(devname,RTL_O_RDWR | RTL_O_NONBLOCK);
   
    //rtl_printf("after rtl_open:2\n");
    
//  rtl_ftruncate(*fno, 1024*10);
   rtl_ftruncate(*fno, 1024*100);   // 2008-5-2
   //printf("fortran_fifo_open %s, return %d.\n",devname,*fno);
   
   //rtl_printf("after rtl_ftruncate:3\n");
}

void fortran_fifo_truncate_(int *fno,int *length)
{
   rtl_ftruncate(*fno, *length);
}

// FIFO operation
void fortran_fifo_write_(int *fno,char *sline, int *nwrite, int *nret)
{
   *nret=rtl_write(*fno,sline,*nwrite);
}
// FIFO read
void fortran_fifo_read_(int *fno,char *sline,int *maxlen, int *nread)
{
   *nread = rtl_read(*fno,sline,*maxlen);
}

// FIFO close
void fortran_fifo_close_(char *devname,int *fno,int *nret)
{
   *nret = rtl_close(*fno);
   *nret = rtl_unlink(devname);
}
