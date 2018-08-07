#ifndef __WINDRIVER_H__
#define __WINDRIVER_H__

#ifdef __cplusplus
extern   "C"   {
#endif

struct ether_hw_addr {
	uint8_t addr_bytes[6]; /**< in transmission order */
} __attribute__((__packed__));

struct hw_ether_hdr {
	struct ether_hw_addr d_addr; /**< Destination address. */
	struct ether_hw_addr s_addr; /**< Source address. */
	uint16_t ether_type;      /**< Frame type. */
} __attribute__((__packed__));

#define pktmbuf_mtod(m, t) ((t)((m)->pkt.data))
#define pktmbuf_pkt_len(m) ((m)->pkt.pkt_len)
#define pktmbuf_data_len(m) ((m)->pkt.data_len)

int log_msg(const char *fmt, ...);
int log_out();
int log_init();
int log_exit();
int log_release();

unsigned long long get_nanosecond(void);

extern void *rt_malloc(size_t size);
extern void rt_free(void* pv);
extern void *rt_calloc(size_t n, size_t size);
extern void *rt_realloc(void *mem_address, size_t size);

#define TAILQ_ENTRY(type)                                               \
struct {                                                                \
        struct type *tqe_next;  /* next element */                      \
        struct type **tqe_prev; /* address of previous next element */  \
}

/*
 * Tail queue declarations.
 */
#define TAILQ_HEAD(name, type)                                          \
struct name {                                                           \
        struct type *tqh_first; /* first element */                     \
        struct type **tqh_last; /* addr of last next element */         \
}


#include <rte_errno.h>
#include <rte_config.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#define MAX_PKT_BURST 128

int dpdk_pkt_receive(unsigned char port_id,struct rte_mbuf **pkts);
int dpdk_env_init(unsigned char core_id, unsigned char port);
int dpdk_env_exit(unsigned char core_id, unsigned char port_id);
unsigned int get_local_core_id();
void ether_hw_addr_copy(const struct ether_hw_addr *ea_from, struct ether_hw_addr *ea_to);
void dpdk_start_launch_on_core(int (*f)(void *), void * arg, unsigned char core);
void dpdk_wait_core();
struct rte_mbuf *dpdk_combine_raw_pkt(unsigned char *data, unsigned int data_len, unsigned char *dstaddr, unsigned char *srcaddr, int protocol_type);
void dpdk_get_macaddr(unsigned char portid, struct ether_addr *mac_addr);
unsigned long long get_rtcore_status(void);

#ifdef __cplusplus
}
#endif

#endif
