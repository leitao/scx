// Copyright (c) Meta Platforms, Inc. and affiliates.

// This software may be used and distributed according to the terms of the
// GNU General Public License version 2.
#ifndef __INTF_H
#define __INTF_H

#include <stdbool.h>
#ifndef __kptr
#ifdef __KERNEL__
#error "__kptr_ref not defined in the kernel"
#endif
#define __kptr
#endif

#ifndef __KERNEL__
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
#endif

#include <scx/ravg.bpf.h>

enum consts {
	MAX_CPUS		= 512,
	MAX_DOMS		= 64,	/* limited to avoid complex bitmask ops */
	MAX_NUMA_NODES		= MAX_DOMS,	/* Assume at least 1 domain per NUMA node */
	CACHELINE_SIZE		= 64,

	LB_DEFAULT_WEIGHT	= 100,
	LB_MIN_WEIGHT		= 1,
	LB_MAX_WEIGHT		= 10000,
	LB_LOAD_BUCKETS		= 100,	/* Must be a factor of LB_MAX_WEIGHT */
	LB_WEIGHT_PER_BUCKET	= LB_MAX_WEIGHT / LB_LOAD_BUCKETS,

	/*
	 * When userspace load balancer is trying to determine the tasks to push
	 * out from an overloaded domain, it looks at the the following number
	 * of recently active tasks of the domain. While this may lead to
	 * spurious migration victim selection failures in pathological cases,
	 * this isn't a practical problem as the LB rounds are best-effort
	 * anyway and will be retried until loads are balanced.
	 */
	MAX_DOM_ACTIVE_PIDS	= 1024,
};

/* Statistics */
enum stat_idx {
	/* The following fields add up to all dispatched tasks */
	RUSTY_STAT_WAKE_SYNC,
	RUSTY_STAT_PREV_IDLE,
	RUSTY_STAT_GREEDY_IDLE,
	RUSTY_STAT_PINNED,
	RUSTY_STAT_DIRECT_DISPATCH,
	RUSTY_STAT_DIRECT_GREEDY,
	RUSTY_STAT_DIRECT_GREEDY_FAR,
	RUSTY_STAT_DSQ_DISPATCH,
	RUSTY_STAT_GREEDY_LOCAL,
	RUSTY_STAT_GREEDY_XNUMA,

	/* Extra stats that don't contribute to total */
	RUSTY_STAT_REPATRIATE,
	RUSTY_STAT_KICK_GREEDY,
	RUSTY_STAT_LOAD_BALANCE,

	/* Errors */
	RUSTY_STAT_TASK_GET_ERR,

	RUSTY_NR_STATS,
};

struct task_ctx {
	/* The domains this task can run on */
	u64 dom_mask;

	struct bpf_cpumask __kptr *cpumask;
	struct bpf_cpumask __kptr *tmp_cpumask;
	u32 dom_id;
	u32 weight;
	bool runnable;
	u64 dom_active_pids_gen;
	u64 running_at;

	/* The task is a workqueue worker thread */
	bool is_kworker;

	/* Allowed on all CPUs and eligible for DIRECT_GREEDY optimization */
	bool all_cpus;

	/* select_cpu() telling enqueue() to queue directly on the DSQ */
	bool dispatch_local;

	struct ravg_data dcyc_rd;
};

struct bucket_ctx {
	u64 dcycle;
	struct ravg_data rd;
};

struct dom_ctx {
	u64 vtime_now;
	struct bpf_cpumask __kptr *cpumask;
	struct bpf_cpumask __kptr *direct_greedy_cpumask;
	struct bpf_cpumask __kptr *node_cpumask;
	u32 node_id;

	u64 dbg_dcycle_printed_at;
	struct bucket_ctx buckets[LB_LOAD_BUCKETS];
};

struct node_ctx {
	struct bpf_cpumask __kptr *cpumask;
};

#endif /* __INTF_H */
