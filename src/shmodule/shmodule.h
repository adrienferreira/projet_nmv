#ifndef SHMODULE_H_
#define SHMODULE_H_

static long perform_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static long perform_kill(unsigned long arg);

#endif