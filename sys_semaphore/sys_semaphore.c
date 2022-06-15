#include <asm/barrier.h>
#include <linux/linkage.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>

#define set_task_state(tsk, state_value)		\
	do { (tsk)->state = (state_value); smp_mb(); } while (0)

typedef struct {
	int* sem_id;
	int value;
	wait_queue_t* wait;
} semaphore;

static unsigned long count = 0;
static semaphore semaphores[] = {};

semaphore* linear_search(semaphore* semaphores, size_t size, int* sem_id) {
  for (size_t i = 0; i < size; i++) {
    if (*semaphores[i].sem_id == *sem_id) {
      return &semaphores[i];
    }
  }
  return NULL;
}

DECLARE_WAIT_QUEUE_HEAD(queue);

static int autoremove_autofree_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
	int ret = default_wake_function(wait, mode, sync, key);

	if (ret) {
		list_del_init(&wait->task_list);
		kfree(wait);
	}
	return ret;
}

static long stop_process(struct task_struct *p, int sem_id)
{
	
	
	unsigned long flags;
	wait_queue_t *wait = kmalloc(sizeof(*wait), GFP_KERNEL);
	if (!wait)
		return -1;
	init_wait(wait);
	wait->private = p;
	wait->func = autoremove_autofree_wake_function;
	wait->flags |= WQ_FLAG_EXCLUSIVE;

	spin_lock_irqsave(&queue.lock, flags);
	__add_wait_queue_tail(&queue, wait);
	set_task_state(p, TASK_STOPPED);
	spin_unlock_irqrestore(&queue.lock, flags);

	return 0;
}

asmlinkage long sys_stop_process(int pid)
{
	struct task_struct *p = find_task_by_vpid(pid);
	return p ? stop_process(p) : -1;
}

asmlinkage long sys_continue_process(void)
{ 
	__wake_up(&queue, TASK_STOPPED, 1, NULL);
	return 0;
}

asmlinkage long sys_init_semaphore(int initial_value) {
  if (initial_value < 0) return -1;

  semaphore *new_semaphore;
  new_semaphore = kmalloc(sizeof(*new_semaphore), GFP_KERNEL);

  new_semaphore->value = initial_value;
  new_semaphore->sem_id = count;
  count++;

  semaphore semaphores[new_semaphore->sem_id] = {&new_semaphore->sem_id, new_semaphore->value};

  return new_semaphore->sem_id
}

asmlinkage long sys_down(int *sem_id) {
  size_t num_semaphores = sizeof(semaphores) / sizeof(semaphore);
  semaphore* found = linear_search(semaphores, num_semaphores, &sem_id);
  if (!found)return;
	if(found->value == 0) {
		sys_stop_process(p);
	}
	found->value--;
  return;
}

asmlinkage void sys_up(long *sem_id){
	size_t num_semaphores = sizeof(semaphores) / sizeof(semaphore);
  semaphore* found = linear_search(semaphores, num_semaphores, &sem_id);
  if (!found)	return;	
	found->value++;
	if(found->waitlist) {
		sys_continue_process()
	}
}
