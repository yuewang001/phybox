#include "mpi.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <time.h>
#include "rtftoc_intel.h"
#include <windriver.h>
#include <i_malloc.h>

#define _GNU_SOURCE
/*
 * Fortran passes arguments by reference whereas C passes arguments by value. When you put a 
 * variable name into a function call from Fortran, the corresponding C function receives a 
 * pointer to that variable. Variable's value can be accessed by dereferencing it.
 */

static int rt_core_id = 0;

#if 1
#define RT_TIMER_ABSTIME 0x1
#define NSECS_PER_SEC 1000000000L

#define rt_timespec_normalize(t) {\
        if ((t)->tv_nsec >= NSECS_PER_SEC) { \
                (t)->tv_nsec -= NSECS_PER_SEC; \
                (t)->tv_sec++; \
        } else if ((t)->tv_nsec < 0) { \
                (t)->tv_nsec += NSECS_PER_SEC; \
                (t)->tv_sec--; \
        } \
}
#define timespec_add(t1, t2) do { \
        (t1)->tv_nsec += (t2)->tv_nsec;  \
        (t1)->tv_sec += (t2)->tv_sec; \
        rt_timespec_normalize(t1);\
} while (0)

#endif

static void sig_usr(int signo)
{
	if (signo == SIGINT)
	{
		if (rt_core_id > 0)
		{
			release_rtcore(rt_core_id);
		}
		exit(0);
	}
}

void init_rt_env_()
{
	i_malloc =  rt_malloc;
	i_calloc =  rt_calloc;
	i_realloc = rt_realloc;
	i_free = rt_free;

	if (signal(SIGINT, sig_usr) == SIG_ERR)
	{
		printf("Cannot catch SIGINT\n");
	}
}

void *rt_thread(void *func)
{
	void (*pfun)();
	pfun = func;

	set_thread_affinity(rt_core_id);

	pfun();
}

/* 
 * Creates a RT thread with parameter "func" as address of function to be 
 * executed as RT thread
 */
void *create_rt_thread_(void * func)
{
	dpdk_start_launch_on_core(func, NULL, rt_core_id);

	return NULL;	
}

int dpdk_init()
{
	if(0)
	{
		test_nic();
		return 0;
	}
	printf("dpdk_init start:\n");
	int ret;
	int core;
	
	core = allocate_rtcore();

	if (core < 0)
	{
		printf("RtEmtPhy allocate_rtcore failed,core = %d\n", core);
		return -1;
	}
	rt_core_id = core;

	printf("RtEmtPhy allocate_rtcore,core = %d\n", rt_core_id);

	ret = dpdk_env_init((unsigned char)rt_core_id);

	if (ret != 0) 
	{
		release_rtcore(rt_core_id);
		return -1;
	}
	printf("dpdk_init rt_core_id:%d done:\n",rt_core_id);
	return 0;
}

extern void debug_reg_content(int port, int channel, int val);
extern uint32_t
rte_fiber_generic_pkg(uint8_t port_id, uint16_t queue_id, void *data, uint32_t len);


extern  uint32_t
rte_fiber_tx_burst(uint8_t port_id, uint16_t queue_id,
		uint16_t pkt_len);

extern rte_fiber_rx_burst(uint8_t port_id, uint16_t queue_id,
		struct rte_mbuf **rx_pkts, uint16_t nb_pkts);

#define DATA_OFS  50

int test_nic()
{
	int core,i;
	core = allocate_rtcore();
	unsigned portid = 0;
	uint8_t channel=0;
	int retval = 0;

	struct rte_mbuf *pkts_burst[10];
	struct rte_mbuf *m;


	rte_fiber_debug_open();
	retval = dpdk_env_init((unsigned char)core);
	if (retval != 0)
	{
		printf("dpdk init failed with error[%d]\n", retval);
		return -1;
	}

//	debug_reg_content(portid, channel,  5);

	{
		uint8_t buffer[128];
		int i,j,ret;
		char *pdata;

		/*
		 * send / receive packages test.
		 */

		//rte_fiber_debug_open();
		debug_reg_content(portid, channel, 10);
		usleep(100);

		for( i = 1; i < 20; i ++ ){
			memset(&buffer[0], i, 128);
			rte_fiber_generic_pkg(portid, channel, &buffer[0], 64);
			rte_wmb();
			rte_fiber_tx_burst(portid, channel, 64);

			/*Have to wait for ready of receiving.*/
			usleep(50);

			/* receive test.*/

			//rte_fiber_debug_open();
			debug_reg_content(portid, channel, 10);
#if 1
			ret = rte_fiber_rx_burst(portid, channel, pkts_burst, 1);
			if (ret != 0 ){
				m = pkts_burst[0];
				pdata = m->pkt.data;

				if ( (pdata[DATA_OFS]&0xff) != (i & 0xff) ){
					printf("====        error         =======pdata[DATA_OFS] = 0x%x=======i=0x%x===j=%x=========\n",pdata[DATA_OFS]&0xff, i, j);
					rte_fiber_debug_open();
					debug_reg_content(portid, channel, 10);

					rte_fiber_statistics_dump(portid, channel);

					while(1);
				}
				rte_pktmbuf_free(m);
			}
			else
			{
				printf("------------Do not get the packages in receive queue!-----[%d]----\n", i);
				rte_fiber_debug_open();
				debug_reg_content(portid, channel, 10);

				rte_fiber_statistics_dump(portid, channel);

				while(1);
			}
#endif
		}
		printf("Finished test!\n");
	}
	//while(1);

	return 0;
}

void dpdk_release()
{
	dpdk_wait_core();
	dpdk_env_exit();
	release_rtcore(rt_core_id);
	printf("RtEmtPhy release_rtcore,core = %d\n", rt_core_id);

	rt_core_id = 0;
}

void  fortran_rtclock_gettime_ (double *curr_time)
{
        struct timespec curr;

        clock_gettime( CLOCK_REALTIME, &curr);
        *curr_time = curr.tv_sec+ (curr.tv_nsec*1.0)/NSECS_PER_SEC;
}

void fortran_clock_sleep_(int *usec)
{
      struct timespec begin,add;
      add.tv_sec=*usec/1000000;
      add.tv_nsec=(*usec-add.tv_sec*1000000)*1000;
      clock_gettime( CLOCK_REALTIME, &begin );
      timespec_add(&begin, &add);
      clock_nanosleep(CLOCK_REALTIME, RT_TIMER_ABSTIME, &begin, NULL);   
}

void fortran_clock_sleepto_(double *sleepto)
{
      struct timespec end,tmtp;
      
      end.tv_sec=*sleepto;
      end.tv_nsec=(*sleepto-end.tv_sec)*NSECS_PER_SEC;
      tmtp.tv_sec=0;
      tmtp.tv_nsec=10e3;
      clock_nanosleep(CLOCK_REALTIME, RT_TIMER_ABSTIME , &end, NULL);
}

void fortran_fifo_open_(char *devname, int *fno)
{
   unlink(devname);
   mkfifo(devname, 0);
   *fno=open(devname,O_RDWR | O_NONBLOCK);
   ftruncate(*fno, 1024*10);
}

// FIFO close
void fortran_fifo_close_(char *devname,int *fno,int *nret)
{
   *nret = close(*fno);
   *nret = unlink(devname);
}
