#define CHECK_FREQUENCY (1 * HZ)

static long perform_gen_wait(unsigned long arg, int is_waitall);
static void gen_wait_work(struct work_struct*param_ws);

struct gen_wait_struct 
{
	//pids to wait
	int nb_pid;
	struct task_struct **tasks;

	//generic wait/waitall
	atomic_t nb_finished;
	int nb_to_wait;

	//re-schecdule autonomously
	unsigned long check_freq;
	struct delayed_work dws;
};

static struct gen_wait_struct gws;

static long perform_gen_wait(unsigned long arg, int is_waitall)
{
	struct gen_wait_usr_struct usr_struct;
	unsigned int icp;//ind cur pid
	pid_t *usr_pids_ptr;

	if(copy_from_user(&usr_struct, (void*)arg, sizeof(struct gen_wait_usr_struct))){
		pr_warn("Impossible to retrieve struct to kernel space\n");
		goto copy_waitall_struct_fail;
	}

	usr_pids_ptr = usr_struct.pids;//do not override user pointer
	usr_struct.pids = (pid_t*)kmalloc((usr_struct.nb_pid * sizeof(pid_t)), GFP_KERNEL);

	if(!(usr_struct.pids)){//TODO <==kmalloc nb_pid?=0
		pr_warn("Impossible to allocate space for PIDs in kernel space\n");
		goto alloc_pids_fail;
	}

	if(copy_from_user(usr_struct.pids, (void*)usr_pids_ptr, (usr_struct.nb_pid * sizeof(pid_t)))){
		pr_warn("Impossible to retrieve PIDs to kernel space\n");
		goto copy_pids_fail;
	}

	gws.tasks = (struct task_struct**)kmalloc((usr_struct.nb_pid * sizeof(struct task_struct*)), GFP_KERNEL);
	if(!(gws.tasks)){
		pr_warn("Impossible to allocate space for PIDs in kernel space\n");
		goto alloc_gws_fail;
	}

	for(icp=0; icp<(usr_struct.nb_pid); icp++)
	{
		//TODO get_* obligé pour incrémenter compteur de références
		gws.tasks[icp] = get_pid_task(find_vpid(usr_struct.pids[icp]), PIDTYPE_PID);
		if(!(gws.tasks[icp]))
		{
			pr_warn("No process with the given PID (%d)\n",(int)usr_struct.pids[icp]);
			goto pid_not_found_fail;
		}
	}

	atomic_set(&(gws.nb_finished), 0);
	gws.nb_to_wait = 0;
	gws.nb_pid = usr_struct.nb_pid;
	gws.check_freq = CHECK_FREQUENCY;
	gws.nb_to_wait = is_waitall ? gws.nb_pid : 1;

	INIT_DELAYED_WORK(&(gws.dws), gen_wait_work);
	schedule_delayed_work(&(gws.dws), gws.check_freq);

	while(atomic_read(&(gws.nb_finished)) < gws.nb_to_wait)
		schedule();

	pid_not_found_fail:
	//TODO ???
	//for(; icp>=0 && (gws.tasks[icp]); icp--)
	//	put_task_struct(gws.tasks[icp]);
	kfree(gws.tasks);
	alloc_gws_fail:
	copy_pids_fail:
	kfree(usr_struct.pids);
	alloc_pids_fail:
	copy_waitall_struct_fail:
	return 1;
}

static void gen_wait_work(struct work_struct*param_ws)
{
	struct gen_wait_struct *param_gws;
	int cur_fin, cur_nb_wait;
	int icp;

	param_gws = container_of(param_ws, struct gen_wait_struct, dws.work);
	cur_nb_wait=param_gws->nb_to_wait;
	cur_fin=0;

	for (icp = 0; icp <(param_gws->nb_pid); icp++)
		if(! (pid_alive(param_gws->tasks[icp])) )
			cur_fin++;

	pr_warn("Process finished %d/%d\n", cur_fin, cur_nb_wait);
	atomic_set(&(param_gws->nb_finished), cur_fin);

	if(cur_fin < cur_nb_wait)
		schedule_delayed_work(&(param_gws->dws), param_gws->check_freq);	
}
