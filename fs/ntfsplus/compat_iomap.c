// SPDX-License-Identifier: GPL-2.0
/*
 * Compatibility writeback implementation for iomap on Linux < 5.5.0
 * Based on the Linux 5.4 XFS / 5.5 generic iomap writeback infrastructure.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/iomap.h>
#include <linux/pagemap.h>
#include <linux/writeback.h>
#include <linux/bio.h>
#include <linux/sched/signal.h>

#include "ntfs.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)

static struct bio_set ntfs_ioend_bioset;

static void ntfs_finish_page_writeback(struct inode *inode,
				       struct bio_vec *bvec, int error)
{
	struct iomap_page *iop = to_iomap_page(bvec->bv_page);

	if (error) {
		SetPageError(bvec->bv_page);
		mapping_set_error(inode->i_mapping, -EIO);
	}

	if (!iop || atomic_dec_and_test(&iop->write_count))
		end_page_writeback(bvec->bv_page);
}

static void ntfs_destroy_ioend(struct iomap_ioend *ioend, int error)
{
	struct inode *inode = ioend->io_inode;
	struct bio *bio = &ioend->io_inline_bio;
	struct bio *last = ioend->io_bio, *next;

	for (bio = &ioend->io_inline_bio; bio; bio = next) {
		struct bio_vec *bvec;
		struct bvec_iter_all iter_all;

		if (bio == last)
			next = NULL;
		else
			next = bio->bi_private;

		bio_for_each_segment_all(bvec, bio, iter_all)
			ntfs_finish_page_writeback(inode, bvec, error);
		bio_put(bio);
	}
}

static void ntfs_end_bio(struct bio *bio)
{
	struct iomap_ioend *ioend = bio->bi_private;

	ntfs_destroy_ioend(ioend, blk_status_to_errno(bio->bi_status));
}

static int ntfs_submit_ioend(struct writeback_control *wbc,
			     struct iomap_ioend *ioend, int status)
{
	ioend->io_bio->bi_private = ioend;
	ioend->io_bio->bi_end_io = ntfs_end_bio;

	if (status) {
		ioend->io_bio->bi_status = errno_to_blk_status(status);
		bio_endio(ioend->io_bio);
		return status;
	}

	submit_bio(ioend->io_bio);
	return 0;
}

static struct iomap_ioend *
ntfs_alloc_ioend(struct inode *inode, struct iomap_writepage_ctx *wpc,
		 loff_t offset, sector_t sector, struct writeback_control *wbc)
{
	struct iomap_ioend *ioend;
	struct bio *bio;

	bio = bio_alloc_bioset(GFP_NOFS, BIO_MAX_PAGES, &ntfs_ioend_bioset);
	bio_set_dev(bio, wpc->iomap.bdev);
	bio->bi_iter.bi_sector = sector;
	bio->bi_opf = REQ_OP_WRITE | wbc_to_write_flags(wbc);
	bio->bi_write_hint = inode->i_write_hint;
	wbc_init_bio(wbc, bio);

	ioend = container_of(bio, struct iomap_ioend, io_inline_bio);
	INIT_LIST_HEAD(&ioend->io_list);
	ioend->io_type = wpc->iomap.type;
	ioend->io_flags = wpc->iomap.flags;
	ioend->io_inode = inode;
	ioend->io_size = 0;
	ioend->io_offset = offset;
	ioend->io_private = NULL;
	ioend->io_bio = bio;
	return ioend;
}

static struct bio *ntfs_chain_bio(struct bio *prev)
{
	struct bio *new;

	new = bio_alloc(GFP_NOFS, BIO_MAX_PAGES);
	bio_copy_dev(new, prev);
	new->bi_iter.bi_sector = bio_end_sector(prev);
	new->bi_opf = prev->bi_opf;
	new->bi_write_hint = prev->bi_write_hint;

	bio_chain(prev, new);
	bio_get(prev);
	submit_bio(prev);
	return new;
}

static void ntfs_add_to_ioend(struct inode *inode, loff_t offset,
			      struct page *page, struct iomap_page *iop,
			      struct iomap_writepage_ctx *wpc,
			      struct writeback_control *wbc,
			      struct list_head *iolist)
{
	sector_t sector = iomap_sector(&wpc->iomap, offset);
	unsigned int len = i_blocksize(inode);
	unsigned int poff = offset & (PAGE_SIZE - 1);
	bool merged, same_page = false;

	if (!wpc->ioend ||
	    wpc->iomap.type != wpc->ioend->io_type ||
	    offset != wpc->ioend->io_offset + wpc->ioend->io_size ||
	    sector != bio_end_sector(wpc->ioend->io_bio)) {
		if (wpc->ioend)
			list_add(&wpc->ioend->io_list, iolist);
		wpc->ioend = ntfs_alloc_ioend(inode, wpc, offset, sector, wbc);
	}

	merged = __bio_try_merge_page(wpc->ioend->io_bio, page, len, poff,
				      &same_page);
	if (iop && !same_page)
		atomic_inc(&iop->write_count);

	if (!merged) {
		if (bio_full(wpc->ioend->io_bio, len))
			wpc->ioend->io_bio = ntfs_chain_bio(wpc->ioend->io_bio);
		bio_add_page(wpc->ioend->io_bio, page, len, poff);
	}

	wpc->ioend->io_size += len;
	wbc_account_cgroup_owner(wbc, page, len);
}

static int ntfs_compat_writepage_map(struct iomap_writepage_ctx *wpc,
				     struct writeback_control *wbc,
				     struct inode *inode,
				     struct page *page,
				     u64 end_offset)
{
	struct iomap_page *iop = to_iomap_page(page);
	struct iomap_ioend *ioend, *next;
	unsigned int len = i_blocksize(inode);
	u64 file_offset;
	int error = 0, count = 0, i;
	LIST_HEAD(submit_list);

	for (i = 0, file_offset = page_offset(page);
	     i < (PAGE_SIZE >> inode->i_blkbits) && file_offset < end_offset;
	     i++, file_offset += len) {
		if (iop && !test_bit(i, iop->uptodate))
			continue;

		error = wpc->ops->map_blocks(wpc, inode, file_offset);
		if (error)
			break;
		if (wpc->iomap.type == IOMAP_HOLE)
			continue;
		ntfs_add_to_ioend(inode, file_offset, page, iop, wpc, wbc,
				  &submit_list);
		count++;
	}

	if (unlikely(error)) {
		if (!count) {
			iomap_invalidatepage(page, 0, PAGE_SIZE);
			ClearPageUptodate(page);
			unlock_page(page);
			goto done;
		}
		set_page_writeback_keepwrite(page);
	} else {
		clear_page_dirty_for_io(page);
		set_page_writeback(page);
	}

	unlock_page(page);

	list_for_each_entry_safe(ioend, next, &submit_list, io_list) {
		int error2;

		list_del_init(&ioend->io_list);
		error2 = ntfs_submit_ioend(wbc, ioend, error);
		if (error2 && !error)
			error = error2;
	}

	if (!count)
		end_page_writeback(page);
done:
	mapping_set_error(page->mapping, error);
	return error;
}

static int ntfs_compat_do_writepage(struct page *page,
				    struct writeback_control *wbc,
				    void *data)
{
	struct iomap_writepage_ctx *wpc = data;
	struct inode *inode = page->mapping->host;
	pgoff_t end_index;
	u64 end_offset;
	loff_t offset;

	if (WARN_ON_ONCE((current->flags & (PF_MEMALLOC | PF_KSWAPD)) == PF_MEMALLOC))
		goto redirty;
	if (WARN_ON_ONCE(current->flags & PF_MEMALLOC_NOFS))
		goto redirty;

	offset = i_size_read(inode);
	end_index = offset >> PAGE_SHIFT;
	if (page->index < end_index) {
		end_offset = (loff_t)(page->index + 1) << PAGE_SHIFT;
	} else {
		unsigned int offset_into_page = offset & (PAGE_SIZE - 1);

		if (page->index > end_index ||
		    (page->index == end_index && offset_into_page == 0))
			goto redirty;
		zero_user_segment(page, offset_into_page, PAGE_SIZE);
		end_offset = offset;
	}

	return ntfs_compat_writepage_map(wpc, wbc, inode, page, end_offset);

redirty:
	redirty_page_for_writepage(wbc, page);
	unlock_page(page);
	return 0;
}

int iomap_writepage(struct page *page, struct writeback_control *wbc,
		    struct iomap_writepage_ctx *wpc,
		    const struct iomap_writeback_ops *ops)
{
	int ret;

	wpc->ops = ops;
	ret = ntfs_compat_do_writepage(page, wbc, wpc);
	if (!wpc->ioend)
		return ret;
	return ntfs_submit_ioend(wbc, wpc->ioend, ret);
}

int iomap_writepages(struct address_space *mapping,
		     struct writeback_control *wbc,
		     struct iomap_writepage_ctx *wpc,
		     const struct iomap_writeback_ops *ops)
{
	int ret;

	wpc->ops = ops;
	ret = write_cache_pages(mapping, wbc, ntfs_compat_do_writepage, wpc);
	if (!wpc->ioend)
		return ret;
	return ntfs_submit_ioend(wbc, wpc->ioend, ret);
}

int ntfs_compat_iomap_init(void)
{
	return bioset_init(&ntfs_ioend_bioset, 4 * (PAGE_SIZE / SECTOR_SIZE),
			   offsetof(struct iomap_ioend, io_inline_bio),
			   BIOSET_NEED_BVECS);
}

void ntfs_compat_iomap_exit(void)
{
	bioset_exit(&ntfs_ioend_bioset);
}

#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0) */
