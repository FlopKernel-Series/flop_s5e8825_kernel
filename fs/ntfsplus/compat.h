/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Compatibility definitions for NTFSPLUS on older Linux kernels (< 6.1 / 5.4).
 */

#ifndef _NTFS_COMPAT_H
#define _NTFS_COMPAT_H

#include <linux/version.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/blkdev.h>
#include <linux/uio.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/mount.h>
#include <linux/iomap.h>

#ifndef FMODE_CAN_ODIRECT
#define FMODE_CAN_ODIRECT 0
#endif

/*
 * filemap_invalidate_lock / unlock shims.
 * mapping->invalidate_lock rwsem was introduced in Linux 5.15.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static inline void filemap_invalidate_lock(struct address_space *mapping)
{
}

static inline void filemap_invalidate_unlock(struct address_space *mapping)
{
}

static inline void filemap_invalidate_lock_shared(struct address_space *mapping)
{
}

static inline void filemap_invalidate_unlock_shared(struct address_space *mapping)
{
}
#endif

/*
 * Block device discard helpers.
 * Introduced in Linux 5.17+.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
static inline unsigned int bdev_max_discard_sectors(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);
	return q ? q->limits.max_discard_sectors : 0;
}

static inline unsigned int bdev_discard_granularity(struct block_device *bdev)
{
	struct request_queue *q = bdev_get_queue(bdev);
	return q ? q->limits.discard_granularity : 0;
}
#endif


/*
 * u64_to_fsid helper (upstream in 5.9).
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
static inline __kernel_fsid_t u64_to_fsid(u64 val)
{
	__kernel_fsid_t fsid;
	fsid.val[0] = (u32)val;
	fsid.val[1] = (u32)(val >> 32);
	return fsid;
}
#endif

/*
 * kvrealloc with 4 parameters: p, oldsize, newsize, flags.
 * kvrealloc was introduced in Linux 5.8+.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
static inline void *ntfs_kvrealloc(const void *p, size_t oldsize, size_t newsize, gfp_t flags)
{
	void *newp;

	if (oldsize >= newsize)
		return (void *)p;
	newp = kvmalloc(newsize, flags);
	if (!newp)
		return NULL;
	if (p) {
		memcpy(newp, p, oldsize);
		kvfree(p);
	}
	return newp;
}
#define kvrealloc ntfs_kvrealloc
#endif

/*
 * On Linux < 5.9.0, kvmalloc_node() does not fall back to vmalloc when
 * GFP_NOFS or GFP_NOIO is used. Provide wrappers that fall back to __vmalloc().
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
#include <linux/vmalloc.h>

static inline void *ntfs_compat_kvmalloc(size_t size, gfp_t flags)
{
	void *ret;

	if ((flags & GFP_KERNEL) != GFP_KERNEL) {
		ret = kmalloc(size, flags | __GFP_NOWARN);
		if (!ret)
			ret = __vmalloc(size, flags);
		return ret;
	}
	return kvmalloc(size, flags);
}

static inline void *ntfs_compat_kvzalloc(size_t size, gfp_t flags)
{
	return ntfs_compat_kvmalloc(size, flags | __GFP_ZERO);
}

static inline void *ntfs_compat_kvcalloc(size_t n, size_t size, gfp_t flags)
{
	size_t total;

	if (check_mul_overflow(n, size, &total))
		return NULL;
	return ntfs_compat_kvzalloc(total, flags);
}

static inline void *ntfs_compat_kvmalloc_array(size_t n, size_t size, gfp_t flags)
{
	size_t total;

	if (check_mul_overflow(n, size, &total))
		return NULL;
	return ntfs_compat_kvmalloc(total, flags);
}

#define kvmalloc(size, flags) ntfs_compat_kvmalloc(size, flags)
#define kvzalloc(size, flags) ntfs_compat_kvzalloc(size, flags)
#define kvcalloc(n, size, flags) ntfs_compat_kvcalloc(n, size, flags)
#define kvmalloc_array(n, size, flags) ntfs_compat_kvmalloc_array(n, size, flags)
#endif

/*
 * iov_iter copy / fault helpers.
 * fault_in_iov_iter_readable was introduced in Linux 5.15+ (replacing iov_iter_fault_in_readable).
 * copy_page_from_iter_atomic was introduced in Linux 5.15+.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static inline int fault_in_iov_iter_readable(struct iov_iter *i, size_t size)
{
	return iov_iter_fault_in_readable(i, size);
}

static inline size_t copy_page_from_iter_atomic(struct page *page, unsigned offset,
						size_t bytes, struct iov_iter *i)
{
	return iov_iter_copy_from_user_atomic(page, i, offset, bytes);
}
#endif

/*
 * Basic single-page folio shims for < 5.16.0 kernels.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)
struct folio {
	struct page page;
};

static inline struct folio *page_folio(struct page *page)
{
	return (struct folio *)page;
}

static inline struct page *folio_page(struct folio *folio, size_t n)
{
	return ((struct page *)folio) + n;
}

static inline void folio_put(struct folio *folio)
{
	put_page((struct page *)folio);
}

static inline void folio_lock(struct folio *folio)
{
	lock_page((struct page *)folio);
}

static inline void folio_unlock(struct folio *folio)
{
	unlock_page((struct page *)folio);
}

static inline void folio_mark_uptodate(struct folio *folio)
{
	SetPageUptodate((struct page *)folio);
}

static inline void folio_clear_uptodate(struct folio *folio)
{
	ClearPageUptodate((struct page *)folio);
}

static inline void folio_mark_dirty(struct folio *folio)
{
	set_page_dirty((struct page *)folio);
}

static inline void folio_clear_dirty(struct folio *folio)
{
	ClearPageDirty((struct page *)folio);
}

static inline struct folio *read_mapping_folio(struct address_space *mapping,
					       pgoff_t index, struct file *file)
{
	return (struct folio *)read_mapping_page(mapping, index, file);
}

static inline void *kmap_local_folio(struct folio *folio, size_t offset)
{
	return kmap_atomic((struct page *)folio) + offset;
}

static inline void kunmap_local(const void *addr)
{
	kunmap_atomic((void *)addr);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
static inline void *kmap_local_page(struct page *page)
{
	return kmap_atomic(page);
}
#endif

static inline size_t folio_size(struct folio *folio)
{
	return PAGE_SIZE;
}

static inline unsigned int folio_shift(struct folio *folio)
{
	return PAGE_SHIFT;
}

static inline loff_t folio_pos(struct folio *folio)
{
	return (loff_t)((struct page *)folio)->index << PAGE_SHIFT;
}

static inline size_t offset_in_folio(struct folio *folio, loff_t pos)
{
	return offset_in_page(pos);
}

static inline void flush_dcache_folio(struct folio *folio)
{
	flush_dcache_page((struct page *)folio);
}

static inline void memcpy_to_folio(struct folio *folio, size_t offset,
				   const void *from, size_t len)
{
	void *kaddr = kmap_atomic((struct page *)folio);
	memcpy(kaddr + offset, from, len);
	kunmap_atomic(kaddr);
}

static inline void memcpy_from_folio(void *to, struct folio *folio,
				     size_t offset, size_t len)
{
	void *kaddr = kmap_atomic((struct page *)folio);
	memcpy(to, kaddr + offset, len);
	kunmap_atomic(kaddr);
}

static inline bool folio_contains(struct folio *folio, pgoff_t index)
{
	return ((struct page *)folio)->index == index;
}

static inline unsigned int folio_nr_pages(struct folio *folio)
{
	return 1;
}

static inline loff_t folio_next_pos(struct folio *folio)
{
	return ((loff_t)((struct page *)folio)->index + 1) << PAGE_SHIFT;
}

static inline struct folio *filemap_grab_folio(struct address_space *mapping, pgoff_t index)
{
	struct page *page = grab_cache_page_write_begin(mapping, index, AOP_FLAG_NOFS);
	return (struct folio *)page;
}

static inline struct folio *filemap_lock_folio(struct address_space *mapping, pgoff_t index)
{
	struct page *page = find_lock_page(mapping, index);
	if (!page)
		return ERR_PTR(-ENOENT);
	return (struct folio *)page;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0) */

#ifndef IOMAP_F_STALE
#define IOMAP_F_STALE 0
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static inline void *iomap_inline_data(const struct iomap *iomap, loff_t pos)
{
	return (void *)((char *)iomap->inline_data + pos - iomap->offset);
}
#endif

/*
 * iomap writeback compatibility declarations for < 5.5.0 kernels.
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
#include <linux/iomap.h>

struct iomap_ioend {
	struct list_head	io_list;
	u16			io_type;
	u16			io_flags;
	struct inode		*io_inode;
	size_t			io_size;
	loff_t			io_offset;
	void			*io_private;
	struct bio		*io_bio;
	struct bio		io_inline_bio;
};

struct iomap_writepage_ctx;

struct iomap_writeback_ops {
	int (*map_blocks)(struct iomap_writepage_ctx *wpc, struct inode *inode,
				loff_t offset);
	int (*prepare_ioend)(struct iomap_ioend *ioend, int status);
	void (*discard_page)(struct page *page, loff_t fileoff);
};

struct iomap_writepage_ctx {
	struct iomap		iomap;
	struct iomap_ioend	*ioend;
	const struct iomap_writeback_ops *ops;
};

int iomap_writepage(struct page *page, struct writeback_control *wbc,
		    struct iomap_writepage_ctx *wpc,
		    const struct iomap_writeback_ops *ops);
int iomap_writepages(struct address_space *mapping,
		     struct writeback_control *wbc,
		     struct iomap_writepage_ctx *wpc,
		     const struct iomap_writeback_ops *ops);

int ntfs_compat_iomap_init(void);
void ntfs_compat_iomap_exit(void);
#else
static inline int ntfs_compat_iomap_init(void) { return 0; }
static inline void ntfs_compat_iomap_exit(void) {}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0) */

#endif /* _NTFS_COMPAT_H */
