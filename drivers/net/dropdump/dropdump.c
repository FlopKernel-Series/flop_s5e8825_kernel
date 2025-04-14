/*
 * Monitoring code for network dropped packet alerts
 *
 * Copyright (C) 2018 SAMSUNG Electronics, Co,LTD
 */

#include <net/ip.h>
#include <net/tcp.h>
#if defined(CONFIG_ANDROID_VENDOR_HOOKS)
#include <trace/hooks/net.h>
#endif
#include <trace/events/skb.h>
#include <net/dropdump.h>

int debug_drd = 0;
#define drd_info(fmt, ...) pr_info("drd: %s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define drd_dbg(flag, ...) \
do { \
	if (unlikely(debug_drd & flag)) { drd_info(__VA_ARGS__); } \
	else {} \
} while (0)

DEFINE_RATELIMIT_STATE(drd_ratelimit_print, 1 * HZ, 10);
#define drd_limit(...)				\
do {						\
	if (__ratelimit(&drd_ratelimit_print))	\
		drd_info(__VA_ARGS__);		\
} while (0)

DEFINE_RATELIMIT_STATE(drd_ratelimit_pkt, 1 * HZ, 32);

struct list_head ptype_log __read_mostly;
EXPORT_SYMBOL_GPL(ptype_log);

int support_dropdump;
EXPORT_SYMBOL_GPL(support_dropdump);

extern struct list_head ptype_all;

struct st_item hmap[DRD_HSIZE];
spinlock_t hlock;
u64 hmap_count;
u64 hdup_count;
uint hmax_depth;
u16 skip_count;
u64 dropped_count;

#ifdef DRD_WQ
struct _drd_worker drd_worker;

unsigned int budget_default = BUDGET_DEFAULT;
unsigned int budget_limit;

#define	BUDGET_MAX	(budget_default << 2)
#define	LIST_MAX	(BUDGET_MAX << 2)
#endif

void init_st_item(struct st_item *item)
{
	INIT_LIST_HEAD(&item->list);
	item->p = 0;
	item->matched = 0;
	item->st[0] = '\n';
}

int get_hkey(u64 *hvalue)
{
	u64 key = 0;
	u64 src = *hvalue & 0x00000000ffffffff;

	while (src) {
		key += src & 0x000000ff;
		src >>= 8;
	}
	key %= DRD_HSIZE;

	return (int)key;
}

char *get_hmap(u64 *hvalue)
{
	int hkey = get_hkey(hvalue);
	struct st_item *lookup = &hmap[hkey];
	uint depth = 1;

	do {
		drd_dbg(DEBUG_HASH, "hvalue search[%d]: <%pK|%pK|%pK> p:[%llx], hvalue:{%llx}\n",
				hkey, lookup, lookup->list.next, &hmap[hkey], lookup->p, *hvalue);
		if (lookup->p == *hvalue) {
			drd_dbg(DEBUG_HASH, "hvalue found: '%s'\n", lookup->st);
			if (lookup->matched < 0xffffffffffffffff)
				lookup->matched++;

			if (depth >=3 && lookup->matched > ((struct st_item *)hmap[hkey].list.next)->matched) {
				// simply reorder the list by matched count, except the hmap array head
				list_del(&lookup->list);
				__list_add(&lookup->list, &hmap[hkey].list, hmap[hkey].list.next);
			}

			return lookup->st;
		}
		lookup = (struct st_item *)lookup->list.next;
		if (hmax_depth < ++depth)
			hmax_depth = depth;
	} while (lookup != &hmap[hkey]);

	drd_dbg(DEBUG_HASH, "hvalue not found\n");
	return NULL;
}

char *set_hmap(u64 *hvalue)
{
	int hkey = get_hkey(hvalue);
	struct st_item *newItem = NULL;
	bool first_hit = false;

	drd_dbg(DEBUG_HASH, "hvalue {%d}: <%llx> %llx\n", hkey, *hvalue, hmap[hkey].p);
	if (hmap[hkey].p == 0) {
		newItem = &hmap[hkey];
		first_hit = true;
	} else {
		newItem = kmalloc(sizeof(struct st_item), GFP_ATOMIC);
		if (newItem == NULL) {
			drd_dbg(DEBUG_HASH, "fail to alloc\n");
			spin_unlock_bh(&hlock);
			return NULL;
		}

		init_st_item(newItem);
		list_add_tail(&newItem->list, &hmap[hkey].list);
		hdup_count++;
	}

	newItem->p = *hvalue;
	hmap_count++;
	spin_unlock_bh(&hlock);

	snprintf(newItem->st, ST_SIZE, "%pS", (void *)*hvalue);
	drd_dbg(DEBUG_HASH, "{%d:%d} <%pK> '%s'\n", hkey, first_hit, hvalue, newItem->st);

	return newItem->st;
}

/* use direct call instead of recursive stack trace */
u64 __stack(int depth)
{
	u64 *func = NULL;
	switch (depth + ST_START) {
		case  3 :
			func = __builtin_return_address(3);
			break;
		case  4 :
			func = __builtin_return_address(4);
			break;
		case  5 :
			func = __builtin_return_address(5);
			break;
		case  6 :
			func = __builtin_return_address(6);
			break;
		case  7 :
			func = __builtin_return_address(7);
			break;
		case  8 :
			func = __builtin_return_address(8);
			break;
		case  9 :
			func = __builtin_return_address(9);
			break;
		case 10 :
			func = __builtin_return_address(10);
			break;
		case 11 :
			func = __builtin_return_address(11);
			break;
		case 12 :
			func = __builtin_return_address(12);
			break;
		case 13 :
			func = __builtin_return_address(13);
			break;
		case 14 :
			func = __builtin_return_address(14);
			break;
		case 15 :
			func = __builtin_return_address(15);
			break;
		case 16 :
			func = __builtin_return_address(16);
			break;
		case 17 :
			func = __builtin_return_address(17);
			break;
		case 18 :
			func = __builtin_return_address(18);
			break;
		case 19 :
			func = __builtin_return_address(19);
			break;
 		case 20 :
			func = __builtin_return_address(20);
			break;
		case 21 :
			func = __builtin_return_address(21);
			break;
		case 22 :
			func = __builtin_return_address(22);
			break;
		case 23 :
			func = __builtin_return_address(23);
			break;
		case 24 :
			func = __builtin_return_address(24);
			break;
		case 25 :
			func = __builtin_return_address(25);
			break;
		default :
			return 0;
	}

	return (u64)func;
}


#define NOT_TRACE	(0xDD)
#define FIN_TRACE	1
#define ACK_TRACE	2
#define GET_TRACE	3

int chk_stack(char *pos, int net_pkt)
{
	/* stop tracing */
	if (!strncmp(pos + 4, "f_nbu", 5))// __qdf_nbuf_free
		return NOT_TRACE;
	if (!strncmp(pos, "unix", 4))	  // unix_xxx
		return NOT_TRACE;
	if (!strncmp(pos + 2, "tlin", 4)) // netlink_xxx
		return NOT_TRACE;
	if (!strncmp(pos, "tpac", 4))	  // tpacket_rcv
		return NOT_TRACE;
	if (!strncmp(pos, "drd", 3))	  // drd_xxx
		return NOT_TRACE;
	if (!strncmp(pos + 1, "_sk_d", 5))// __sk_destruct
		return NOT_TRACE;
#ifdef EXTENDED_DROPDUMP 
	/* ignore normally consumed packets on TX path */
	if (!strncmp(pos + 2, "it_on", 5))// xmit_one
		return NOT_TRACE;
	if (!strncmp(pos + 2, "t_tx_", 5))// net_tx_action
		return NOT_TRACE;
	if (!strncmp(pos, "dp_tx", 5))    //dp_tx_comp_xxx
		return NOT_TRACE;
	/* prevent recursive call by __kfree_skb() */
	if (!strncmp(pos + 4, "ree_s", 5))// __kfree_skb
		return NOT_TRACE;
#endif

	/* end of callstack */
	if (!strncmp(pos, "loc", 3))// local_*
		return FIN_TRACE;
	if (!strncmp(pos + 7, "ftir", 4))// __do_softirq
		return FIN_TRACE;
	if (!strncmp(pos + 7, "rk_r", 4))// task_work_run
		return FIN_TRACE;
	if (!strncmp(pos, "SyS", 3))	 // SyS_xxx
		return FIN_TRACE;
	if (!strncmp(pos, "ret_", 4))	 // ret_from_xxx
		return FIN_TRACE;
	if (!strncmp(pos, "el", 2))	 // el*
		return FIN_TRACE;
	if (!strncmp(pos, "gic", 3))	 // gic_xxx
		return FIN_TRACE;
	if (!strncmp(pos + 3, "rt_ke", 5))// start_kernel
		return FIN_TRACE;
	if (!strncmp(pos + 13, "rt_ke", 5))// secondary_start_kernel
		return FIN_TRACE;

	/* network pkt */
	if (!net_pkt) {
		if (!strncmp(pos, "net", 3))
			return GET_TRACE;
		if (!strncmp(pos, "tcp", 3)) {
			// packet from tcp_drop() could be normal operation.
			// don't logging pure ack.
			if (!strncmp(pos, "tcp_drop", 8))
				return ACK_TRACE;
			return GET_TRACE;
		}
		if (!strncmp(pos, "ip",  2))
			return GET_TRACE;
		if (!strncmp(pos, "icmp",  4))
			return GET_TRACE;
		if (!strncmp(pos, "xfr", 3))
			return GET_TRACE;
	}

	return 0;
}


static bool _is_tcp_ack(struct sk_buff *skb)
{
	switch (skb->protocol) {
	/* TCPv4 ACKs */
	case htons(ETH_P_IP):
		if ((ip_hdr(skb)->protocol == IPPROTO_TCP) &&
			(ntohs(ip_hdr(skb)->tot_len) - (ip_hdr(skb)->ihl << 2) ==
			 tcp_hdr(skb)->doff << 2) &&
		    ((tcp_flag_word(tcp_hdr(skb)) &
		      cpu_to_be32(0x00FF0000)) == TCP_FLAG_ACK))
			return true;
		break;

	/* TCPv6 ACKs */
	case htons(ETH_P_IPV6):
		if ((ipv6_hdr(skb)->nexthdr == IPPROTO_TCP) &&
			(ntohs(ipv6_hdr(skb)->payload_len) ==
			 (tcp_hdr(skb)->doff) << 2) &&
		    ((tcp_flag_word(tcp_hdr(skb)) &
		      cpu_to_be32(0x00FF0000)) == TCP_FLAG_ACK))
			return true;
		break;
	}

	return false;
}

static inline bool is_tcp_ack(struct sk_buff *skb)
{
	if (skb_is_tcp_pure_ack(skb))
		return true;

	if (unlikely(_is_tcp_ack(skb)))
		return true;

	return false;
}

int symbol_lookup(u64 *addr, int net_pkt) {
	char *symbol = NULL;

	spin_lock_bh(&hlock);
	symbol = get_hmap(addr);

	if (symbol != NULL)
		spin_unlock_bh(&hlock);
	else
		symbol = set_hmap(addr);

	memcpy((char *)addr, symbol, strlen(symbol));
	return chk_stack(symbol, net_pkt);
}

u8 get_stack(struct sk_buff *skb, struct sk_buff *dmy, unsigned int offset, unsigned int reason)
{
	u8 depth = 0, max_depth = ST_MAX;
	struct _dmy_info *dmy_info = (struct _dmy_info *)(dmy->data + offset);
	u64 *stack_base = &dmy_info->stack;

#ifdef DRD_WQ
	// sometimes __builtin_return_address() returns invalid address for deep stack of
	// ksoftirq or kworker, and migration task. limit the maximun depth for them.
	if ((current->comm[0] == 'k' && (current->comm[4] == 't' || current->comm[4] == 'k')) ||
	    (current->comm[0] == 'm' &&  current->comm[4] == 'a')) {
		dmy_info->flag |= LIMIT_DEPTH_BIT;
		max_depth >>= 1;
	}
#else
	int chk = 0, net_pkt = 0;
#endif

	if (skb->tstamp >> 48 < 5000) {
		// packet has kernel timestamp, not utc.
		// using a zero-value for updating to utc at tpacket_rcv()
		dmy_info->flag |= UPDATE_TIME_BIT;
		dmy->tstamp = 0;
	} else {
		// using utc of original packet
		dmy->tstamp = skb->tstamp;
	}

	drd_dbg(DEBUG_SAVE, "trace <%pK>\n", skb);
	for (depth = 0; depth < max_depth; depth++) {
		*stack_base = __stack(depth);
#ifdef DRD_WQ
		drd_dbg(DEBUG_SAVE, "%02d: <%pK>\n", depth, (u64 *)*stack_base);
		if (*stack_base == 0) {
			// __builtin_return_address() returned root stack
			depth--;
			break;
		}
#else
		/* functions that instead of when set_stack_work not used */
		chk = symbol_lookup(stack_base, net_pkt);
		drd_dbg(DEBUG_SAVE, "[%2d:%d] <%s>\n", depth, chk, (char *)stack_base);

		if (chk == NOT_TRACE) {
			drd_dbg(DEBUG_TRACE, "not target stack\n");
			return NOT_TRACE;
		}

		if (chk == FIN_TRACE)
			break;

		if (chk == ACK_TRACE) {
			if (is_tcp_ack(skb)) {
				drd_dbg(DEBUG_TRACE, "don't trace tcp ack\n");
				return NOT_TRACE;
			} else {
				net_pkt = 1;
			}
		}

		if (chk == GET_TRACE)
			net_pkt = 1;
#endif

		stack_base += (ST_SIZE / sizeof(u64));
	}

	memcpy(dmy_info->magic, "DRD", 3);
	dmy_info->depth = depth;
	if (skip_count > 0) {
		dmy_info->skip_count = skip_count;
		skip_count = 0;
	}
	dmy_info->count = ++dropped_count;
	dmy_info->reason_id = reason;
	if (reason < DRD_REASON_MAX) {
		memcpy(dmy_info->reason_str, drd_reasons[reason], min(16, (int)strlen(drd_reasons[reason])));
	} else {
		memcpy(dmy_info->reason_str, "UNDEFINED_REASON", 16);
	}

	drd_dbg(DEBUG_RAW, "<%pK:%pK> %*ph\n", dmy, dmy_info, 16, dmy_info);

	return depth;
}

int set_stack_work(struct sk_buff *skb, struct _dmy_info *dmy_info)
{
	int chk = 0, net_pkt = 0;
	u8 depth;
	u64 *stack_base;

	drd_dbg(DEBUG_RAW, "<%pK:%pK> %*ph\n", skb, dmy_info, 16, dmy_info);

	if (strncmp(dmy_info->magic, "DRD", 3)) {
		drd_dbg(DEBUG_TRACE, "invalid magic <%pK>\n", skb);
		return -1;
	}

	stack_base = &dmy_info->stack;
	for (depth = 0; depth < dmy_info->depth; depth++) {
		chk = symbol_lookup(stack_base, net_pkt);
		drd_dbg(DEBUG_RESTORE, "[%2d:%d] <%s>\n", depth, chk, (char *)stack_base);

		if (chk == NOT_TRACE) {
			drd_dbg(DEBUG_TRACE, "not target stack\n");
			return NOT_TRACE;
		}

		if (chk == FIN_TRACE)
			break;

		if (chk == ACK_TRACE) {
			if (is_tcp_ack(skb)) {
				drd_dbg(DEBUG_TRACE, "don't trace tcp ack\n");
				return NOT_TRACE;
			} else {
				net_pkt = 1;
			}
		}

		if (chk == GET_TRACE)
			net_pkt = 1;

		stack_base += (ST_SIZE / sizeof(u64));
	}

	if (net_pkt == 0) {
		drd_dbg(DEBUG_TRACE, "not defined packet\n");
		return -3;
	}

	return depth;
}

#ifdef DRD_WQ
static void save_pkt_work(struct work_struct *ws)
{
	struct sk_buff *skb, *next;
	struct packet_type *ptype = NULL;
	struct _dmy_info *dmy_info = NULL;
	int st_depth = 0;
	u16 budget = 0;

	list_for_each_entry_safe(skb, next, &drd_worker.list, list) {
		spin_lock_bh(&drd_worker.lock);
		if (support_dropdump) {
			list_for_each_entry_rcu(ptype, &ptype_log, list) {
				if (ptype != NULL)
					break;
			}

			drd_dbg(DEBUG_LOG, "del %u:%llu <%llx>\n", budget, drd_worker.num, (u64)(skb));
			skb_list_del_init(skb);
			drd_worker.num--;
		} else {
			spin_unlock_bh(&drd_worker.lock);
			return;
		}
		spin_unlock_bh(&drd_worker.lock);

		if (ptype == NULL || list_empty(&ptype->list)) {
			drd_dbg(DEBUG_LOG,"pt list not ready\n");
			__kfree_skb(skb);
			continue;
		}

		dmy_info = (struct _dmy_info *)(skb->data + PKTINFO_OFFSET(skb));

		st_depth = set_stack_work(skb, dmy_info);
		if (st_depth != NOT_TRACE) {
			ptype->func(skb, skb->dev, ptype, skb->dev);
		} else {
			__kfree_skb(skb);
		}

		if (++budget >= budget_limit)
			break;
	}

	if (!list_empty(&drd_worker.list)) {
		if (budget_limit < BUDGET_MAX)
			budget_limit <<= 1;
		queue_delayed_work(drd_worker.wq, &drd_worker.dwork, msecs_to_jiffies(1));
		drd_dbg(DEBUG_LOG, "pkt remained(%llu), trigger again. budget:%d\n", drd_worker.num, budget_limit);
	} else {
		drd_worker.num = 0;
	}

	return;
}
#else
void save_pkt(struct sk_buff *skb)
{
	struct packet_type *ptype = NULL;

	rcu_read_lock();
	list_for_each_entry_rcu(ptype, &ptype_log, list) {
		if (ptype != NULL)
			break;
	}

	if (ptype == NULL || list_empty(&ptype->list)) {
		drd_dbg(DEBUG_LOG,"pt list not ready\n");
		__kfree_skb(skb);
		goto out;
	}

	drd_dbg(DEBUG_LOG, "%llu <%llx>\n", dropped_count, (u64)(skb));
	ptype->func(skb, skb->dev, ptype, skb->dev);
out:
	rcu_read_unlock();
}
#endif

int skb_validate(struct sk_buff *skb)
{
	if (virt_addr_valid(skb) && virt_addr_valid(skb->dev)) {
		struct iphdr *ip4hdr = (struct iphdr *)skb_network_header(skb);

		if (skb->protocol != htons(ETH_P_IPV6)
		    && skb->protocol != htons(ETH_P_IP))
			return -1;

		switch (skb->dev->name[0]) {
			case 'r': // rmnet*
			case 'v': // v4-rmnet*
			case 't': // tun
			case 'e': // epdg
				break;
			case 'l': // lo
			case 'b': // bt*
			case 'w': // wlan
			case 's': // swlan
				if (__ratelimit(&drd_ratelimit_pkt))
					break;
				if (skip_count < 0xffff)
					skip_count++;
				dropped_count++;
				return -9;
			default:
				drd_dbg(DEBUG_LOG, "invalid dev: %s\n", skb->dev->name);
				return -2;
		}

		if (unlikely((ip4hdr->version != 4 && ip4hdr->version != 6)
				|| ip4hdr->id == 0x6b6b))
			return -3;

		if (unlikely(!skb->len))
			return -4;

		if (unlikely(skb->len > skb->tail))
			return -5;

		if (unlikely(skb->data <= skb->head))
			return -6;

		if (unlikely(skb->tail > skb->end))
			return -7;

		if (unlikely(skb->pkt_type == PACKET_LOOPBACK))
			return -8;

		drd_dbg(DEBUG_RAW, "ndev: %s\n", skb->dev->name);
		return 0;
	}

	return -255;
}


struct sk_buff *get_dummy(struct sk_buff *skb, unsigned int reason)//, char *pos, int st_depth)
{
	struct sk_buff *dummy = NULL;
	struct skb_shared_info *shinfo;
	unsigned int copy_len     = PKTINFO_COPYLEN_MAX;
	unsigned int copy_buf_len = PKTINFO_COPYLEN_MAX;
	unsigned int org_len, dummy_len;
	u8 ret = 0;

	struct iphdr *ip4hdr = (struct iphdr *)(skb_network_header(skb));
	struct ipv6hdr *ip6hdr;

	if (ip4hdr->version == 4) {
		org_len = ntohs(ip4hdr->tot_len);
	} else {
		ip6hdr  = (struct ipv6hdr *)ip4hdr;
		org_len = skb_network_header_len(skb) + ntohs(ip6hdr->payload_len);
	}

	if (org_len < PKTINFO_COPYLEN_MAX) {
		copy_len     = org_len;
		copy_buf_len = round_up(org_len, 0x10);
	}

	dummy_len = copy_buf_len + sizeof(struct _dmy_info) + ST_BUF_SIZE;

	dummy = alloc_skb(dummy_len, GFP_ATOMIC);
	if (unlikely(!dummy)) {
		drd_dbg(DEBUG_LOG, "alloc fail, %u\n", dummy_len);
		return NULL;
	}

	drd_dbg(DEBUG_SAVE, "skb->len:%u org_len:%u copy_len:%u copy_buf_len:%u dummy_len:%u\n",
		 skb->len, org_len, copy_len, copy_buf_len, dummy_len);

	dummy->dev = skb->dev;
	dummy->protocol = skb->protocol;
	dummy->ip_summed = CHECKSUM_UNNECESSARY;

	refcount_set(&skb->users, 1);

	skb_put(dummy, dummy_len);
	skb_reset_mac_header(dummy);
	skb_reset_network_header(dummy);
	skb_set_transport_header(dummy, skb_network_header_len(skb));

	shinfo = skb_shinfo(dummy);
	memset(shinfo, 0, sizeof(struct skb_shared_info));
	atomic_set(&shinfo->dataref, 1);

	INIT_LIST_HEAD(&dummy->list);

	memcpy(dummy->data, skb_network_header(skb), copy_len);
	memset((void *)((u64)dummy->data + (u64)copy_len), 0,
		0x10 - (copy_len % 0x10) + sizeof(struct _dmy_info) + ST_BUF_SIZE);

	ret = get_stack(skb, dummy, copy_buf_len, reason);
	if (ret != NOT_TRACE) {
		PKTINFO_OFFSET(dummy) = copy_buf_len;
	} else {
		drd_dbg(DEBUG_SAVE, "not saving pkt\n");
		__kfree_skb(dummy);
		return NULL;
	}

	return dummy;
}

void drd_kfree_skb(struct sk_buff *skb, unsigned int reason)
{
	struct sk_buff *dmy;
#ifdef DRD_WQ
	struct sk_buff *next;
#endif

	if (support_dropdump < 2) {
#ifdef DRD_WQ
		if (drd_worker.num) {
			drd_dbg(DEBUG_LOG, "purge drd list\n");
			cancel_delayed_work(&drd_worker.dwork);
			spin_lock_bh(&drd_worker.lock);
			list_for_each_entry_safe(dmy, next, &drd_worker.list, list) {
				skb_list_del_init(dmy);
				__kfree_skb(dmy);
			}
			drd_worker.num = 0;
			spin_unlock_bh(&drd_worker.lock);
		}
#endif
		return;
	}

	if (skb_validate(skb))
		return;

#ifdef DRD_WQ
	if (unlikely(drd_worker.num >= LIST_MAX - 1)) {
		drd_dbg(DEBUG_LOG, "drd list full\n");
		return;
	}
#endif

	dmy = get_dummy(skb, reason);
	if (dmy == NULL)
		return;

#ifdef DRD_WQ
	spin_lock_bh(&drd_worker.lock);

	if (support_dropdump) {
		list_add_tail(&dmy->list, &drd_worker.list);
		drd_worker.num++;
		drd_dbg(DEBUG_LOG, "add %llu <%pK>\n", drd_worker.num, dmy);
	}
	spin_unlock_bh(&drd_worker.lock);

	budget_limit = budget_default;
	queue_delayed_work(drd_worker.wq, &drd_worker.dwork, 0);
#else
	save_pkt(dmy);
#endif

}
EXPORT_SYMBOL_GPL(drd_kfree_skb);

void drd_ptype_head(const struct packet_type *pt, struct list_head *vendor_pt)
{
	if (pt->type == htons(ETH_P_LOG))
		vendor_pt->next = &ptype_log;
}
EXPORT_SYMBOL_GPL(drd_ptype_head);

#if defined(CONFIG_ANDROID_VENDOR_HOOKS)
static void drd_ptype_head_handler(void *data, const struct packet_type *pt, struct list_head *vendor_pt)
{
	drd_ptype_head(pt, vendor_pt);
}
#else
/* can't use macro directing the drd_xxx functions instead of lapper. *
 * because of have to use EXPORT_SYMBOL macro for module parts.       *
 * it should to be used at here with it's definition                  */
void trace_android_vh_ptype_head(const struct packet_type *pt, struct list_head *vendor_pt)
{
	drd_ptype_head(pt, vendor_pt);
}
EXPORT_SYMBOL_GPL(trace_android_vh_ptype_head);
#endif

#if defined(TRACE_SKB_DROP_REASON) || defined(DEFINE_DROP_REASON)
static void drd_kfree_skb_handler(void *data, struct sk_buff *skb,
				  void *location, enum skb_drop_reason reason)
{
#else
static void drd_kfree_skb_handler(void *data, struct sk_buff *skb, void *location)
{
	unsigned int reason = 0;
#endif
	drd_kfree_skb(skb, (unsigned int)reason);
}

struct kobject *drd_kobj;
int get_attr_input(const char *buf, int *val)
{
	int ival;
	int err = kstrtoint(buf, 0, &ival);
	if (err >= 0)
		*val = ival;
	else
		drd_info("invalid input: %s\n", buf);
	return err;
}

static ssize_t hstat_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
		       "stack : total %d, used %lld, dupplicated %llu, max_depth %u, dropped %llu\n",
		       DRD_HSIZE, hmap_count, hdup_count, hmax_depth, dropped_count);
}


static struct kobj_attribute hstat_attribute = {
	.attr = {.name = "hstat", .mode = 0660},
	.show = hstat_show,
};

static ssize_t hmap_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int i;
	struct st_item *lookup;

	for (i = 0; i < DRD_HSIZE; i++) {
		lookup = &hmap[i];
		drd_info("---------------------------------------------------------------------\n");
		do {
			drd_info("%03d <%llx:%llu> '%s'\n", i, lookup->p, lookup->matched, lookup->st);
			lookup = (struct st_item *)lookup->list.next;
		} while (lookup != &hmap[i]);
	}
	drd_info("---------------------------------------------------------------------\n");

	return sprintf(buf, "hmap checked\n");
}

static struct kobj_attribute hmap_attribute = {
	.attr = {.name = "hmap", .mode = 0660},
	.show = hmap_show,
};

static ssize_t debug_drd_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "current debug_drd: %d (0x%x)\n", debug_drd, debug_drd);
}

ssize_t debug_drd_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	if (get_attr_input(buf, &debug_drd) >= 0)
		drd_info("debug_drd = %d\n", debug_drd);
	return count;
}

static struct kobj_attribute debug_drd_attribute = {
	.attr = {.name = "debug_drd", .mode = 0660},
	.show = debug_drd_show,
	.store = debug_drd_store,
};

#ifdef DRD_WQ
static ssize_t budget_default_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "current budget_default: %u\n", budget_default);
}

ssize_t budget_default_store(struct kobject *kobj, struct kobj_attribute *attr,
			const char *buf, size_t count)
{
	if (get_attr_input(buf, &budget_default) >= 0)
		drd_info("budget_default = %u\n", budget_default);
	return count;
}

static struct kobj_attribute budget_default_attribute = {
	.attr = {.name = "budget_default", .mode = 0660},
	.show = budget_default_show,
	.store = budget_default_store,
};
#endif

static struct attribute *dropdump_attrs[] = {
	&hstat_attribute.attr,
	&hmap_attribute.attr,
	&debug_drd_attribute.attr,
#ifdef DRD_WQ
	&budget_default_attribute.attr,
#endif
	NULL,
};
ATTRIBUTE_GROUPS(dropdump);

static struct ctl_table drd_proc_table[] = {
	{
		.procname	= "support_dropdump",
		.data		= &support_dropdump,
		.maxlen 	= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
#ifdef EXTENDED_DROPDUMP
	{
		.procname	= "support_dropdump_ext",
		.data		= &support_dropdump,
		.maxlen 	= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
#endif
	{ }
};

static int __init init_net_drop_dump(void)
{
	int rc = 0, i;

	drd_info("\n");

	INIT_LIST_HEAD(&ptype_log);

	init_net.core.sysctl_hdr = register_net_sysctl(&init_net, "net/core", drd_proc_table);
	if (init_net.core.sysctl_hdr == NULL) {
		drd_info("init sysctrl failed\n");
		return -ENODEV;
	}

#if defined(CONFIG_ANDROID_VENDOR_HOOKS)
	rc  = register_trace_android_vh_ptype_head(drd_ptype_head_handler, NULL);
#endif
	rc += register_trace_kfree_skb(drd_kfree_skb_handler, NULL);
	if (rc) {
		drd_info("fail to register android_trace\n");
		return -EIO;
	}

#ifdef DRD_WQ
	drd_worker.wq = create_workqueue("drd_work");
	if (!drd_worker.wq) {
		drd_info("fail to create wq\n");
		return -ENOMEM;
	}
	INIT_DELAYED_WORK(&drd_worker.dwork, save_pkt_work);
	INIT_LIST_HEAD(&drd_worker.list);
	spin_lock_init(&drd_worker.lock);
	drd_worker.num = 0;
#endif

	drd_kobj = kobject_create_and_add("dropdump", kernel_kobj);
	if (!drd_kobj) {
		drd_info("fail to create kobj\n");
		rc = -ENOMEM;
		goto kobj_error;
	}
	rc = sysfs_create_groups(drd_kobj, dropdump_groups);
	if (rc) {
		drd_info("fail to create attr\n");
		goto attr_error;
	}

	for (i = 0; i < DRD_HSIZE; i++) {
		init_st_item(&hmap[i]);
	}
	spin_lock_init(&hlock);

	support_dropdump = 0;
	goto out;

attr_error:
	kobject_put(drd_kobj);

kobj_error:
#ifdef DRD_WQ
	destroy_workqueue(drd_worker.wq);
#endif

out:
	return rc;
}

static void exit_net_drop_dump(void)
{
	drd_info("\n");

	support_dropdump = 0;
}

module_init(init_net_drop_dump);
module_exit(exit_net_drop_dump);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung dropdump module");

