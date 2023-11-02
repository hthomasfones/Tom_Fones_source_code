
/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : Krash                                                       */
/*  COPYRIGHT    : (c) 2023 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by Dual/GPL license to the Linux open source   */ 
/* community                                                                   */
/*                                                                             */ 
/*******************************************************************************/
/*                                                                             */
/*  Exe files   : kcrash.ko                                                    */ 
/*                                                                             */
/*  Module NAME : kcrash.c                                                     */
/*                                                                             */
/*  DESCRIPTION : Properties & Definitions for the kernel-mode crash module    */
/*                                                                             */
/*  MODULE_AUTHOR("HTF Consulting");                                           */
/*  MODULE_LICENSE("Dual/GPL");                                                */
/*                                                                             */
/* The source code in this file can be freely used, adapted, and redistributed */
/* in source or binary form, so long as an acknowledgment appears in derived   */
/* source files.  The citation should state that the source code comes from    */
/* a set of source files developed by HTF Consulting                           */
/* http://www.htfconsulting.com                                                */
/*                                                                             */
/* No warranty is attached.                                                    */
/* HTF Consulting takes no responsibility for errors or fitness of use         */
/*                                                                             */
/*                                                                             */
/* $Id: kcrash.c, v 1.0 2023/01/01 00:00:00 htf $                              */
/*                                                                             */
/*******************************************************************************/

/*
===============================================================================
Driver Name		:		krash
Author			:		TOM FONES
License		    :		GPL
Description		:		LINUX DEVICE DRIVER PROJECT
===============================================================================
*/

#include"krash.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TOM FONES");

static int krashnum;

int krash_major=99;

dev_t krash_device_num;

struct class *krash_class;
struct timer_list ktimer1;

krash_private devices[KRASH_N_MINORS];

struct platform_device *krash_devs[KRASH_N_MINORS] = {};

struct platform_driver krash_driver =
{
		.driver = {
			.name	= DRIVER_NAME,
			.owner	= THIS_MODULE,
		},
		//.probe 		= krash_probe,
		//.remove		= krash_remove,
        .shutdown       = NULL,
};

struct platform_private 
{
	struct platform_device *pdev;
	int irq;
	krash_private charpriv;
};

atomic_t dev_cnt = ATOMIC_INIT(KRASH_FIRST_MINOR - 1);

int KrashFaultThread(void* pvoid)
{
krash_private *priv = (krash_private *)pvoid;
int arg = priv->arg;
void* pMem;

    PINFO("KrashFaultThread...  priv=%p arg=%d\n", priv, arg);
	switch (arg)
	    {
        case 1:
            spin_lock(&priv->spinlock1);
            break;
        case 2:
            spin_lock(&priv->spinlock2);
            //
            spin_lock(&priv->spinlock1);
            break;
        default:;
        }
              
/* 
    while (priv->arg == arg) 
    {
        if (arg != 4)
            schedule();
        if (arg == 3)
            pMem = kmalloc(0x1000, GFP_KERNEL);
    } */

    PINFO("KrashFaultThread() exit\n");
    return 0;
}


void freeSomething(void *ptr)
{
    PINFO("freeSomething() ptr=%p\n", ptr);

    kfree(ptr);
}

int zeroDivide(void)
{
    int nDivider = 5;
    int nRes = 0;
    PINFO("zeroDivide()\n");

    while(nDivider > 0)
        {
        nDivider--;
        nRes = 5 / nDivider;
        }

    return nRes;
}

void hardRun(void)
{

    int X = 0;
    PINFO("hardRun()\n");
    while (true)
        {
        X += 5;
        X -= 5;
        }

    return;
}

void exhaustMemory(void)
{
    void* ptr = (void*)0;
    PINFO("exhaustMemory()\n");
    while (true)
        {
        ptr = kmalloc(0x4000, GFP_KERNEL);
        if (ptr==NULL)
            break;
        schedule();
        }

    ptr = kmalloc(0x4000, GFP_KERNEL);

    return;
}

void leakMemory(void)
{
    void* ptr = (void*)0;
    int j;
    PINFO("leaktMemory()\n");
    for (j=0; j < 12; j+=1)
        {
        ptr = kmalloc(0x50000, GFP_KERNEL);
        }
    kfree(ptr);

    return;
}

void deadLock(krash_private *priv)
{
    struct task_struct *pThread;
    //char ThreadName[] = "KTdeadlock";

    priv->arg = 2;
    spin_lock(&priv->spinlock1);

    pThread = kthread_create(KrashFaultThread, (void *)priv, "%s", "KTdeadlock");
    if (!IS_ERR(pThread))
        wake_up_process(pThread);

    schedule(); schedule(); 
    spin_lock(&priv->spinlock2);

    return;
}

static int process_krash(int knum, krash_private *priv)
{
    //struct task_struct *pThread;
    int rc=0;
    PINFO("process_krash() knum=%d\n", knum);

    switch(knum)
        {
        case 0: //divided by zero
            if (priv != NULL) 
                spin_lock(&priv->spinlock1);
            zeroDivide(); 
            if (priv != NULL) 
                spin_unlock(&priv->spinlock1);
            break;

        case 1: //nullptr
            {
            void* ptr = NULL;
            freeSomething(ptr);
            memset(ptr, 0x00, 10);
            }
            break;

        case 2: //invalid ptr
            {
            void* ptr = &krashnum;
            freeSomething(ptr);
            }
            break;

        case 3: //double free
            {                                
            void* ptr = kmalloc(0x1000, GFP_KERNEL);
            freeSomething(ptr);
            freeSomething(ptr);
            }
            break;

        case 4: //reference executable memory
            {
            void* ptr = (void *)freeSomething; 
            memset(ptr, 0x00, 10);
            }
            break;

        case 5:
            hardRun();
            break; 

        case 6:
            exhaustMemory();
            break; 

        case 7: //Memory leak
            leakMemory();
            rc=7;
            break;

        case 8: //Deadlock
            if (priv != NULL)
                deadLock(priv);
            break;

        /*case 8: //System stall
            if (priv == NULL)
                break;
            {
            priv->arg = 4;
            pThread =  
            kthread_create(KrashFaultThread, (void *)priv, "KrFltThrd4a");
            pThread =  
            kthread_create(KrashFaultThread, (void *)priv, "KrFltThrd4b");
            pThread =  
            kthread_create(KrashFaultThread, (void *)priv, "KrFltThrd4c");
            pThread =  
            kthread_create(KrashFaultThread, (void *)priv, "KrFltThrd4d");
            if (!IS_ERR(pThread))
                wake_up_process(pThread);
            }
            break;*/

        default:
            PINFO("process_krash() invalid krash #\n");
            break;
        }

    PINFO("process_krash() exit *!*\n");

    return rc;
}

/* static void krash_timer1(unsigned long data)
{
	PINFO("krash_timer executing\n");

	ktimer1.expires = jiffies + KRASH_DELAY_MS * HZ / 1000;
	add_timer(&ktimer1);
} */

static long krash_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    krash_private *priv = filp->private_data;
    PINFO("krash_dev_ioctl()... cmd=x%X, arg=%d\n", cmd, (int)arg);

    if (cmd != KRASH_DEV_IOC_KRASH_NUM)
        return -EINVAL;

    return (long)process_krash((int)arg, priv);
   	//PINFO("krash_dev_ioctl() exit *!*\n");
}

static int krash_dev_open(struct inode *inode,struct file *filp)
{
    krash_private *priv = container_of(inode->i_cdev, krash_private, cdev);

  	PINFO("krash_dev_open()... priv=%p\n", priv);

    filp->private_data = priv;

//	PINFO("krash_dev_open() exit \n");

	return 0;
}					

static int krash_dev_release(struct inode *inode, struct file *filp)
{
    krash_private *priv = filp->private_data;

	PINFO("krash_dev_release()... priv=%p\n", priv);
	//PINFO("krash_dev_release() exit\n");

	return 0;
}

static const struct file_operations krash_fops =
{
	.owner				= THIS_MODULE,
	.open				= krash_dev_open,
	.release			= krash_dev_release,
	.unlocked_ioctl		= krash_dev_ioctl,
};

static struct platform_private *krash_create_device(int minor)
{
	dev_t curr_dev;

	struct platform_private *priv = NULL;
	krash_private *charpriv;

	PINFO("krash_create_device()... minor=%d\n", minor);

	priv = kzalloc(sizeof(struct platform_private), GFP_KERNEL);
	if (!priv)
    {
		PERR("Failed to allocate memory for the private data structure\n");
		return (void *)-ENOMEM;
	}

	charpriv = &priv->charpriv;

	cdev_init(&priv->charpriv.cdev, &krash_fops);

	curr_dev = MKDEV(MAJOR(krash_device_num), MINOR(krash_device_num) + minor);
	cdev_add(&priv->charpriv.cdev, curr_dev, 1);

	device_create(krash_class, NULL, curr_dev, priv,
                  KRASH_NODE_NAME/*"%d", minor*/);

	//init_timer(&charpriv->krash_timer3);
	//charpriv->krash_timer3.data = &charpriv; //TODO
	//charpriv->krash_timer3.function = ktimer1;
	//charpriv->krash_timer3.expires = jiffies + KRASH_DELAY_MS * HZ / 1000;

	//tasklet_init(&charpriv->krash_tasklet , thread1 , 0);

	spin_lock_init(&charpriv->spinlock1);

	//spin_lock_init(&charpriv->spinlock2);

	//platform_set_drvdata(pdev, priv);

    PINFO("krash_create_device() exit priv=%p\n",priv);

	return priv;
}

static int krash_remove_device(struct platform_private *priv)
{
    PINFO("krash_remove_devie()...\n");

    //struct platform_private *priv = platform_get_drvdata(pdev);
	//atomic_dec(&dev_cnt);

	device_destroy(krash_class, priv->charpriv.cdev.dev);

	cdev_del(&priv->charpriv.cdev);

	//tasklet_kill(&priv->charpriv.krash_tasklet);
	//del_timer(&priv->charpriv.krash_timer3);

	//platform_set_drvdata(pdev, NULL);

	/* Free the device specific structure */
	kfree(priv);

	PINFO("krash_remove() exit\n");

	return 0;
}

#if 0
static void thread1(unsigned long data)
{
	PINFO("thread1()... \n");
	PINFO("thread1() exit\n");
}

static int krash_probe(struct platform_device *pdev)
{
	dev_t curr_dev;

    PINFO("krash_probe()...\n");

	unsigned int minor = atomic_inc_return(&dev_cnt);
	struct platform_private *priv;
	krash_private *charpriv;

	if (minor == KRASH_N_MINORS + KRASH_FIRST_MINOR)
		return -EAGAIN;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
    {
		PERR("Failed to allocate memory for the private data structure\n");
		return -ENOMEM;
	}

	charpriv = &priv->charpriv;

	cdev_init(&priv->charpriv.cdev, &krash_fops);

	curr_dev = MKDEV(MAJOR(krash_device_num), MINOR(krash_device_num) + minor);
	cdev_add(&priv->charpriv.cdev, curr_dev, 1);

	device_create(krash_class, NULL, curr_dev, priv, KRASH_NODE_NAME"%d", minor);

	//init_timer(&charpriv->krash_timer3);
	//charpriv->krash_timer3.data = &charpriv; //TODO
	//charpriv->krash_timer3.function = ktimer1;
	//charpriv->krash_timer3.expires = jiffies + KRASH_DELAY_MS * HZ / 1000;

	tasklet_init(&charpriv->krash_tasklet , thread1 , 0);

	spin_lock_init(&charpriv->spinlock1);

	spin_lock_init(&charpriv->spinlock2);

	platform_set_drvdata(pdev, priv);

    PINFO("krash_probe()...\n");

	return 0;
}

static int krash_remove(struct platform_device *pdev)
{
	PINFO("krash_remove()...\n");

    struct platform_private *priv = platform_get_drvdata(pdev);

	atomic_dec(&dev_cnt);

	device_destroy(krash_class, priv->charpriv.cdev.dev);

	cdev_del(&priv->charpriv.cdev);

	tasklet_kill(&priv->charpriv.krash_tasklet);

	//del_timer(&priv->charpriv.krash_timer3);

	platform_set_drvdata(pdev, NULL);

	/* Free the device specific structure */
	kfree(priv);

	PINFO("krash_remove() exit\n");

	return 0;
}
#endif

/******************************* *****************************/
static int __init krash_init(void)
{
	int res;
    struct platform_private *priv = NULL;

    PINFO("krash_init()... \n");

    if (krashnum != 0)
       res = process_krash(krashnum, NULL);

	krash_device_num = MKDEV(krash_major, KRASH_FIRST_MINOR);
	res = register_chrdev_region(krash_device_num, KRASH_N_MINORS, DRIVER_NAME);
	if( res < 0)
    {
		PINFO("register device failed res=%d\n", res);
        if (res != -16)
		    return -1;
	}

	krash_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (!krash_class)
    {
		PERR("Failed to create the class\n");
		res = PTR_ERR(krash_class);
		return res;
	}

	/*res = platform_add_devices(krash_devs, ARRAY_SIZE(krash_devs));
	if (res)
    {
		PERR("Failed to register the platform device\n");
		return res;
	} */

    priv = krash_create_device(1);
	if (IS_ERR(priv))
        {
		res = PTR_ERR(priv);
        PERR("krash_create_device returned %d\n", res);
        return res;
        }
		
    krash_driver.shutdown = (void*)priv;
    res = platform_driver_register(&krash_driver);
	if (res)
    {
		PERR("Failed to register the platform driver\n");
		return res;
	}

	//init_timer(&krash_timer);
	//krash_timer.data = 100; //TODO
	//krash_timer.function = ktimer1;
	//krash_timer.expires = jiffies + KRASH_DELAY_MS * HZ / 1000;

	/* TODO : Initialise tasklet argument */
	//tasklet_init(&krash_tasklet1 , thread1 , 0);

    PINFO("krash_init() exiting priv=%p\n", priv);

	return 0;
}

static void __exit krash_exit(void)
{	
	//int i;
    struct platform_private *priv = (struct platform_private *)krash_driver.shutdown;
	PINFO("krash_exit()... priv=%p\n", priv);

    krash_remove_device(priv);

    //tasklet_kill(&krash_tasklet1);
	//del_timer(&ktimer1);

	//for (i = 0; i < ARRAY_SIZE(krash_devs); i++)
	//	platform_device_del(krash_devs[i]);

	platform_driver_unregister(&krash_driver);

	class_destroy(krash_class);

	unregister_chrdev_region(krash_device_num, KRASH_N_MINORS);

	PINFO("krash_exit() exit\n");
}

module_init(krash_init);
module_exit(krash_exit);

