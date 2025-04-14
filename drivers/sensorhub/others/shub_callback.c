#include "shub_callback.h"
#include "shub_hall_ic.h"

static LIST_HEAD(shub_cb_list);

static struct shub_callback hall_ic_callback = {
	.init = init_hall_ic_callback,
	.remove = remove_hall_ic_callback,
	.sync = sync_hall_ic_state,
};

void initialize_shub_callback(void)
{
	shub_register_callback(&hall_ic_callback);
	shub_init_callback();
}


void shub_register_callback(struct shub_callback *cb)
{
	list_add_tail(&cb->list, &shub_cb_list);
}

void shub_unregister_callback(struct shub_callback *cb)
{
	list_del(&cb->list);
}

void shub_init_callback(void)
{
	struct shub_callback *cb;
	list_for_each_entry(cb, &shub_cb_list, list) {
		if (cb->init) {
			cb->init();
		}
	}
}

void shub_remove_callback(void)
{
	struct shub_callback *cb;
	list_for_each_entry(cb, &shub_cb_list, list) {
		if (cb->remove) {
			cb->remove();
		}
	}
}

void shub_sync_callback(void)
{
	struct shub_callback *cb;
	list_for_each_entry(cb, &shub_cb_list, list) {
		if (cb->sync) {
			cb->sync();
		}
	}
}
