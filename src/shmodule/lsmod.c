static long perform_lsmod(unsigned long arg);

struct lsmod_list {
	struct lsmod_struct mod;
	struct list_head list;
};

static long perform_lsmod(unsigned long arg)
{
	int counter = 1;
	struct lsmod_struct *buf = (struct lsmod_struct *) arg;
	struct lsmod_struct me;
        struct list_head *head = &(THIS_MODULE->list);
	struct module *curs = THIS_MODULE;
	struct lsmod_list *elt, *tmp;
	LIST_HEAD(klist);

	/* gather inserted modules informations */
	mutex_lock(&module_mutex);
	list_for_each_entry_continue(curs, head, list) {
		if (curs->name[0] != '\0') {
			elt = kmalloc(sizeof(struct lsmod_list), GFP_KERNEL);
			strncpy(elt->mod.name, curs->name, MODULE_NAME_LEN);
			elt->mod.ref = module_refcount(curs);
			elt->mod.size = curs->init_size + curs->core_size;
			list_add(&(elt->list), &klist);
			counter++;
		}
	}
	mutex_unlock(&module_mutex);

	/* copy to user-space and destroy */
	strncpy(me.name, THIS_MODULE->name, MODULE_NAME_LEN);
	me.size = THIS_MODULE->init_size + THIS_MODULE->core_size;
	me.ref = module_refcount(THIS_MODULE);
	copy_to_user(buf, &me, sizeof(me));
	buf++;
	list_for_each_entry_safe_reverse(elt, tmp, &klist, list) {
		copy_to_user(buf, &(elt->mod), sizeof(struct lsmod_struct));
		buf++;
		list_del(&(elt->list));
		kfree(elt);
	}
	copy_to_user(buf->name, "", sizeof(char));

	return 0;
}
