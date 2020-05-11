/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

#define N_LORD_FLOWERKILLER 8
#define NROPES 16
static int ropes_left = NROPES;

struct rope{ // rope structure 
	int my_stake;
	int my_hook;
	bool cut;
};

struct stake{
	int my_rope;
	bool have_rope;
};



static struct lock* rope_lock[NROPES];// single lock for each rope
static struct lock* stake_lock[NROPES];
static struct lock* rope_remain_lock; // lock protect ropes left
// static struct lock* first_lock;
// static struct lock* second_lock;
static struct lock* swap_lock;
/* Data structures for rope mappings */
static struct rope one_rope[NROPES];//store each rope struct in array
static struct stake one_stake[NROPES];

/* Implement this! */

/* Synchronization primitives */
static struct semaphore *balloon_done;
static struct semaphore *airballoon_done;
/* Implement this! */

/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 */

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Dandelion thread starting\n");
   
	/* Implement this function */
	while(true){
		// check rope number
		if(ropes_left <= 0){
			break;
		}

		int hook_num = random() % NROPES;
		for(int i = 0; i < NROPES; i++){
			if(one_rope[i].my_hook == hook_num && one_rope[i].cut == false){
				lock_acquire(rope_lock[i]);//protect rope
				if(one_rope[i].my_hook == hook_num && one_rope[i].cut == false){// for safe...
					//start cut hahaha
					one_rope[i].cut = true;
					//reduce the ropes left
					lock_acquire(rope_remain_lock);
					ropes_left --;
					lock_release(rope_remain_lock);
					//release lock for single rope
					kprintf("Dandelion severed rope %d.\n",i);
					lock_release(rope_lock[i]);
					
					thread_yield();
					break; //get out of for loop
				}
				else{
					lock_release(rope_lock[i]);
					break;
				}
			}
		}
	
	}
		//something to let thread end here
		kprintf("Dandelion thread done\n");
		 V(balloon_done);
		V(airballoon_done);
}

static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Marigold thread starting\n");
	
    while(true){
		if(ropes_left <= 0){
			break;
		}
        int stake_num = random()%NROPES;
		for(int i = 0; i < NROPES; i++){
			if(one_rope[i].my_stake == stake_num && one_rope[i].cut == false && one_stake[stake_num].my_rope == i && one_stake[stake_num].have_rope == true){
				lock_acquire(rope_lock[i]);//protect rope
				lock_acquire(stake_lock[stake_num]);
				//aviod change by flowerkillers
				if(one_rope[i].my_stake == stake_num && one_rope[i].cut == false && one_stake[stake_num].my_rope == i && one_stake[stake_num].have_rope == true){
					one_rope[i].cut = true;
					one_rope[i].my_stake = -1;//be cut
					one_stake[stake_num].my_rope = -1;
					one_stake[stake_num].have_rope = false;
					lock_acquire(rope_remain_lock);
					ropes_left --;
					lock_release(rope_remain_lock);
					//release lock for single rope
					
					kprintf("Marigold severed rope %d from stake %d\n",i,stake_num);
					lock_release(rope_lock[i]);
					lock_release(stake_lock[stake_num]);
					thread_yield();
					break; //get out of for loop
				}
				else{
					lock_release(rope_lock[i]);
					break;
				}
			}

		}		
	}
	/* Implement this function */
    //something to let thread end here
	kprintf("Marigold thread done\n");
	V(balloon_done);
	V(airballoon_done);
}

 static
 void
 flowerkiller(void *p, unsigned long arg)
 {
 	(void)p;
 	(void)arg;
   
 	kprintf("Lord FlowerKiller thread starting\n");
    while(true){
		lock_acquire(swap_lock);
		if(ropes_left <= 0 ){
			lock_release(swap_lock);
			break;
		}
		int stake_1 = random()%NROPES;
		lock_acquire(stake_lock[stake_1]);

		//stake have rope
		if(one_stake[stake_1].have_rope == true){
			int rope_1 = one_stake[stake_1].my_rope;
			int stake_2 = random() % NROPES;

			if(stake_1 == stake_2){
				lock_release(stake_lock[stake_1]);
				lock_release(swap_lock);
				continue;
			}
			lock_acquire(stake_lock[stake_2]);
			if(one_stake[stake_2].have_rope == true){
				int rope_2 = one_stake[stake_2].my_rope;
				//swap
				one_stake[stake_1].my_rope = rope_2;
				one_stake[stake_2].my_rope = rope_1;
				kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n",rope_1,stake_1,stake_2);
	 			kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n",rope_2,stake_2,stake_1);
				lock_release(stake_lock[stake_1]);
				lock_release(stake_lock[stake_2]);
				lock_release(swap_lock);
				thread_yield();
			}else{
				lock_release(stake_lock[stake_1]);
				lock_release(stake_lock[stake_2]);
				lock_release(swap_lock);
				continue;
			}
			
		}else{
			lock_release(stake_lock[stake_1]);
			lock_release(swap_lock);
			continue;
		}
	}

	/* Implement this function */
  	kprintf("Lord FlowerKiller thread done\n");
 	V(balloon_done);
 	V(airballoon_done);
 }
   
	
static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Balloon thread starting\n");

    
	for(int i = 0; i < 2+N_LORD_FLOWERKILLER; i++){ //+N_LORD_FLOWERKILLER
		P(balloon_done);
	}
	kprintf("Balloon freed and Prince Dandelion escapes!\n");
	kprintf("Balloon thread done\n");
	V(airballoon_done);
	/* Implement this function */
}


// Change this function as necessary
int
airballoon(int nargs, char **args)
{

	int err = 0, i; //here need to uncomment

	(void)nargs;
	(void)args;
	(void)ropes_left;

    //initialize rope struct array btw create locks

	for(int j = 0; j < NROPES; j++){
		one_rope[j].my_stake = j;
		one_rope[j].my_hook = j;
		one_rope[j].cut = false; // all ropes are not cut
		rope_lock[j] = lock_create("rope");//create rope lock
		stake_lock[j] = lock_create("stake");
		one_stake[j].my_rope = j;
		one_stake[j].have_rope = true;
	}
    //lock initialize
	rope_remain_lock = lock_create("ropes left");
	swap_lock = lock_create("swap lock");
	//semaphore initialize
	balloon_done = sem_create("ballon_done",0);
	airballoon_done = sem_create("airballon_done",0);
	
	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;

	for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
		err = thread_fork("Lord FlowerKiller Thread",
				  NULL, flowerkiller, NULL, 0);
		if(err)
			goto panic;
	}

	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;

	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:
    for(int k = 0; k < N_LORD_FLOWERKILLER+3; k++){ //N_LORD_FLOWERKILLER+
		P(airballoon_done);
	}
	kprintf("Main thread done");
    lock_destroy(rope_remain_lock);
	lock_destroy(swap_lock);
	
	int cnt = 0;
	while(cnt < NROPES){
		lock_destroy(rope_lock[cnt]);
		lock_destroy(stake_lock[cnt]);
		cnt++;
	}
	sem_destroy(balloon_done);
	sem_destroy(airballoon_done);
	ropes_left = NROPES;
	return 0;
}
