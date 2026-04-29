#ifndef _LINUX_ZEROMOUNT_H
#define _LINUX_ZEROMOUNT_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/spinlock.h>
#include <linux/limits.h>
#include <linux/atomic.h>
#include <linux/uidgid.h>
#include <linux/stat.h>
#include <linux/ioctl.h>
#include <linux/rcupdate.h>
#include <linux/printk.h>
#include <linux/sched.h>
#include <linux/bitops.h>

#define ZM_BLOOM_BITS     4096
#define ZM_BLOOM_SHIFT    12
#define ZM_BLOOM_MASK     (ZM_BLOOM_BITS - 1)

/* Per-task recursion guard using android_oem_data1 bit 0.
 * Survives CPU migration -- no preemption constraints needed. */
#ifdef CONFIG_ANDROID_VENDOR_OEM_DATA
#define ZM_RECURSIVE_BIT 0

static inline void zm_enter(void)
{
	set_bit(ZM_RECURSIVE_BIT,
		(unsigned long *)&current->android_oem_data1);
}

static inline void zm_exit(void)
{
	clear_bit(ZM_RECURSIVE_BIT,
		  (unsigned long *)&current->android_oem_data1);
}

static inline bool zm_is_recursive(void)
{
	return test_bit(ZM_RECURSIVE_BIT,
			(unsigned long *)&current->android_oem_data1);
}
#else
#define ZM_RECURSIVE_MARKER ((void *)0x5A4D)

/* Only claim journal_info if it is free — avoids clobbering jbd2 handles
 * held by user processes doing ext4 I/O. If already occupied, we treat
 * the call as a no-op; zm_is_recursive() returns false so ZeroMount
 * proceeds normally, which is safe since the occupied handle means we
 * are in a filesystem transaction context where redirection is unwanted. */
static inline void zm_enter(void)
{
	if (!current->journal_info)
		current->journal_info = ZM_RECURSIVE_MARKER;
}

static inline void zm_exit(void)
{
	if (current->journal_info == ZM_RECURSIVE_MARKER)
		current->journal_info = NULL;
}

static inline bool zm_is_recursive(void)
{
	return current->journal_info == ZM_RECURSIVE_MARKER;
}
#endif

extern int zeromount_debug_level;

#define ZM_LOG(level, fmt, ...) \
	do { \
		if (zeromount_debug_level >= (level) && printk_ratelimit()) \
			pr_info("ZeroMount: " fmt, ##__VA_ARGS__); \
	} while (0)

#define ZM_TRACE(fmt, ...) ZM_LOG(3, fmt, ##__VA_ARGS__)
#define ZM_DBG(fmt, ...)  ZM_LOG(2, fmt, ##__VA_ARGS__)
#define ZM_INFO(fmt, ...) ZM_LOG(1, fmt, ##__VA_ARGS__)
#define ZM_ERR(fmt, ...)  pr_err("ZeroMount: " fmt, ##__VA_ARGS__)

#define ZEROMOUNT_MAGIC_CODE	0x5A
#define ZEROMOUNT_VERSION	1
#define ZEROMOUNT_HASH_BITS	10
#define ZM_FLAG_ACTIVE		(1 << 0)
#define ZM_FLAG_IS_DIR		(1 << 7)
#define ZEROMOUNT_MAGIC_POS	0x7000000000000000ULL

#define ZEROMOUNT_IOC_MAGIC	ZEROMOUNT_MAGIC_CODE
#define ZEROMOUNT_IOC_ADD_RULE	_IOW(ZEROMOUNT_IOC_MAGIC, 1, struct zeromount_ioctl_data)
#define ZEROMOUNT_IOC_DEL_RULE	_IOW(ZEROMOUNT_IOC_MAGIC, 2, struct zeromount_ioctl_data)
#define ZEROMOUNT_IOC_CLEAR_ALL	_IO(ZEROMOUNT_IOC_MAGIC, 3)
#define ZEROMOUNT_IOC_GET_VERSION _IOR(ZEROMOUNT_IOC_MAGIC, 4, int)
#define ZEROMOUNT_IOC_ADD_UID	_IOW(ZEROMOUNT_IOC_MAGIC, 5, unsigned int)
#define ZEROMOUNT_IOC_DEL_UID	_IOW(ZEROMOUNT_IOC_MAGIC, 6, unsigned int)
#define ZEROMOUNT_IOC_GET_LIST	_IOR(ZEROMOUNT_IOC_MAGIC, 7, int)
#define ZEROMOUNT_IOC_ENABLE	_IO(ZEROMOUNT_IOC_MAGIC, 8)
#define ZEROMOUNT_IOC_DISABLE	_IO(ZEROMOUNT_IOC_MAGIC, 9)
#define ZEROMOUNT_IOC_REFRESH	_IO(ZEROMOUNT_IOC_MAGIC, 10)
#define ZEROMOUNT_IOC_GET_STATUS _IOR(ZEROMOUNT_IOC_MAGIC, 11, int)
#define MAX_LIST_BUFFER_SIZE	(64 * 1024)

struct zeromount_ioctl_data {
	char __user *virtual_path;
	char __user *real_path;
	unsigned int flags;
};

struct zeromount_rule {
	struct hlist_node node;
	struct hlist_node ino_node;
	struct list_head list;
	size_t vp_len;
	char *virtual_path;
	char *real_path;
	unsigned long real_ino;
	dev_t real_dev;
	unsigned long v_ino;
	dev_t v_dev;
	bool is_new;
	u32 flags;
	struct rcu_head rcu;
};

struct zeromount_dir_node {
	struct hlist_node node;
	char *dir_path;
	struct list_head children_names;
	struct rcu_head rcu;
};

struct zeromount_child_name {
	struct list_head list;
	char *name;
	unsigned char d_type;
	struct rcu_head rcu;
};

struct zeromount_uid_node {
	uid_t uid;
	struct hlist_node node;
	struct rcu_head rcu;
};

extern DECLARE_HASHTABLE(zeromount_rules_ht, ZEROMOUNT_HASH_BITS);
extern DECLARE_HASHTABLE(zeromount_dirs_ht, ZEROMOUNT_HASH_BITS);
extern DECLARE_HASHTABLE(zeromount_uid_ht, ZEROMOUNT_HASH_BITS);
extern DECLARE_HASHTABLE(zeromount_ino_ht, ZEROMOUNT_HASH_BITS);
extern struct list_head zeromount_rules_list;
extern spinlock_t zeromount_lock;

#ifdef CONFIG_ZEROMOUNT
extern atomic_t zeromount_enabled;

bool zeromount_should_skip(void);
char *zeromount_resolve_path(const char *pathname);
char *zeromount_build_absolute_path(int dfd, const char *name);
struct filename *zeromount_getname_hook(struct filename *name);
void zeromount_inject_dents_common(struct file *file, void __user **dirent,
				   int *count, loff_t *pos, int compat);
void zeromount_inject_dents64(struct file *file, void __user **dirent,
			      int *count, loff_t *pos);
void zeromount_inject_dents(struct file *file, void __user **dirent,
			    int *count, loff_t *pos);
char *zeromount_get_virtual_path_for_inode(struct inode *inode);
char *zeromount_get_static_vpath(struct inode *inode);
void zeromount_spoof_mmap_metadata(struct inode *inode, dev_t *dev,
				   unsigned long *ino);
bool zeromount_is_traversal_allowed(struct inode *inode, int mask);
bool zeromount_is_injected_file(struct inode *inode);
bool zeromount_is_uid_blocked(uid_t uid);
int zeromount_spoof_statfs(const char __user *pathname, struct kstatfs *buf);
ssize_t zeromount_spoof_xattr(struct dentry *dentry, const char *name,
			      void *value, size_t size);
#else
static inline bool zeromount_should_skip(void) { return true; }
static inline char *zeromount_resolve_path(const char *p) { return NULL; }
static inline char *zeromount_build_absolute_path(int dfd, const char *name) { return NULL; }
static inline struct filename *zeromount_getname_hook(struct filename *name) { return name; }
static inline void zeromount_inject_dents_common(struct file *f, void __user **d,
						 int *c, loff_t *p, int compat) {}
static inline void zeromount_inject_dents64(struct file *f, void __user **d,
					    int *c, loff_t *p) {}
static inline void zeromount_inject_dents(struct file *f, void __user **d,
					  int *c, loff_t *p) {}
static inline char *zeromount_get_virtual_path_for_inode(struct inode *inode) { return NULL; }
static inline char *zeromount_get_static_vpath(struct inode *inode) { return NULL; }
static inline void zeromount_spoof_mmap_metadata(struct inode *inode,
						 dev_t *dev,
						 unsigned long *ino) {}
static inline bool zeromount_is_traversal_allowed(struct inode *inode, int mask) { return false; }
static inline bool zeromount_is_injected_file(struct inode *inode) { return false; }
static inline bool zeromount_is_uid_blocked(uid_t uid) { return false; }
static inline int zeromount_spoof_statfs(const char __user *p, struct kstatfs *b) { return 0; }
static inline ssize_t zeromount_spoof_xattr(struct dentry *d, const char *n,
					    void *v, size_t s) { return -EOPNOTSUPP; }
#endif

#endif /* _LINUX_ZEROMOUNT_H */
