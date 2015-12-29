static long perform_meminfo(unsigned long arg);

static long perform_meminfo(unsigned long arg)
{
	struct sysinfo kmeminfo;

	si_meminfo(&kmeminfo);
	si_swapinfo(&kmeminfo);

	copy_to_user((struct sysinfo *) arg, &kmeminfo, sizeof(struct sysinfo));

	return 0;
}
