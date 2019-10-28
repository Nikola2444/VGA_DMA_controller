/*
 *	 This program is free software; you can redistribute it and/or modify
 *	 it under the terms of the GNU General Public License as published by
 *	 the Free Software Foundation; either version 2 of the License, or
 *	 (at your option) any later version.

 *	 This program is distributed in the hope that it will be useful,
 *	 but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	 GNU General Public License for more details.
 *
 *	 You should have received a copy of the GNU General Public License along
 *	 with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>

#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Switch driver");

#define DRIVER_NAME "switch"

/*FOPS Functions prototypes*/
static int switch_probe(struct platform_device *pdev);
static int switch_remove(struct platform_device *pdev);
static int switch_open(struct inode *i, struct file *f);
static int switch_close(struct inode *i, struct file *f);
static ssize_t switch_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t switch_write(struct file *f, const char __user *buf, size_t count, loff_t *off);
static int __init switch_init(void);
static void __exit switch_exit(void);

/*Helper Functions prototypes */
static int int_to_str(int val, char* pBuf, int buf_len, int base);


/******************************************************/

static dev_t first;
static struct class *cl;
static struct cdev c_dev;
//static struct device *dev;


static int end_read = 0;

struct switch_info {
  unsigned long mem_start;
  unsigned long mem_end;
  void __iomem *base_addr;
};


static struct switch_info *lp = NULL;

/* Probe function for switch
 * -------------------------------------------------------------------
 * Probe function called when switch driver is recognized from device tree
 * after registering platform driver, using compatible string to 
 * search for compatible devices in device tree
 * -------------------------------------------------------------------
 */
static int switch_probe(struct platform_device *pdev)
{
  struct resource *r_mem;
  int rc = 0;
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "invalid address\n");
    return -ENODEV;
  }
  lp = (struct switch_info *) kmalloc(sizeof(struct switch_info), GFP_KERNEL);
  if (!lp) {
    printk(KERN_ALERT "Cound not allocate switch device\n");
    return -ENOMEM;
  }

  lp->mem_start = r_mem->start;
  lp->mem_end = r_mem->end;
  //printk(KERN_INFO "base address:%x \t high_address:%x\n", r_mem->start, r_mem->end);
  //request memory region for switch driver, based on resources read from device tree
  if (!request_mem_region(lp->mem_start,lp->mem_end - lp->mem_start + 1,	DRIVER_NAME))
  {
    printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)lp->mem_start);
    rc = -EBUSY;
    goto error1;
  }

  //remap to virtual memory which will be used to access switch from user space appplication
  lp->base_addr = ioremap(lp->mem_start, lp->mem_end - lp->mem_start + 1);
  if (!lp->base_addr) {
    printk(KERN_ALERT "switch: Could not allocate iomem\n");
    rc = -EIO;
    goto error2;
  }

  return 0;

error2:
  release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
error1:
  return rc;
}


/* Remove function for switch
 * -------------------------------------------------------
 *  When switch module is removed, turn off all the switchs first,
 *  release virtual address and the memory region requested.
 *  Also remove switch_info struct allocated in switch_probe.
 * --------------------------------------------------------
 */
static int switch_remove(struct platform_device *pdev)
{
  printk(KERN_ALERT "switch platform driver removed\n");
  //iowrite32(0, lp->base_addr);
  iounmap(lp->base_addr);
  release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
  //release also memory allocated for switch_info struct
  kfree(lp);
  return 0;
}

static struct of_device_id switch_of_match[] = {
  { .compatible = "switch_gpio", },
  { /* end of list */ },
};

MODULE_DEVICE_TABLE(of, switch_of_match);

static int switch_open(struct inode *i, struct file *f)
{
  return 0;
}

static int switch_close(struct inode *i, struct file *f)
{  
  return 0;
}

static ssize_t switch_read(struct file *f, char __user *buf, size_t
                        len, loff_t *off)
{
  char *switch_string = "0b0000";
  u32 switch_value;
  char buffer[20];
  int i = 0;
  int length;
  int base = 2;
  if(end_read)
  {
    end_read = 0;
    return 0;
  }
  for (i = 2; i < strlen(switch_string); i++)
    switch_string[i] = '0';
  switch_value = ioread32(lp->base_addr);
  length = int_to_str(switch_value, buffer, 4, base);
  for(i = 0; i <= length - 1; i++)
  {
    switch_string[strlen(switch_string) - 1 - i] = buffer[length - 1 - i];
  }
  if (copy_to_user(buf, switch_string, strlen(switch_string)))
    return -EFAULT;
  end_read=1;
  
  return strlen(switch_string);

}

static ssize_t switch_write(struct file *f, const char __user *buf,
                         size_t count, loff_t *off)
{
  printk(KERN_INFO "switches do not support write operation\n");
  return count;
}

static int int_to_str(int val, char* pBuf, int buf_len, int base)
{
  static const char* p_conv = "0123456789ABCDEF";
  int num = val;
  int len = 0;
  int pos = 0;

  while(num > 0)
  {
    len++;
    num /= base;
  }

  if(val == 0)
  {
    len = 1;
  }

  pos = len-1;
  num = val;

  if(pos > buf_len-1)
  {
    pos = buf_len-1;
  }

  for(; pos >= 0; pos--)
  {
    pBuf[pos] = p_conv[num % base];
    num /= base;
  }
  
  return len;
}


static struct file_operations switch_fops =
{
  .owner = THIS_MODULE,
  .open = switch_open,
  .release = switch_close,
  .read = switch_read,
  .write = switch_write
};

static struct platform_driver switch_driver = {
  .driver = {
    .name = DRIVER_NAME,
    .owner = THIS_MODULE,
    .of_match_table	= switch_of_match,
  },
  .probe		= switch_probe,
  .remove		= switch_remove,
};

static int __init switch_init(void)
{
  /* unsigned long addr; */
  /* int ret; */

  printk(KERN_INFO "Switch init.\n");

  if (alloc_chrdev_region(&first, 0, 1, "Switch_region") < 0)
  {
    printk(KERN_ALERT "<1>Failed CHRDEV!.\n");
    return -1;
  }
  printk(KERN_INFO "Succ CHRDEV!.\n");

  if ((cl = class_create(THIS_MODULE, "switch_chardrv")) == NULL)
  {
    printk(KERN_ALERT "<1>Failed class create!.\n");
    goto fail_0;
  }
  printk(KERN_INFO "Succ class chardev1 create!.\n");

  if (device_create(cl, NULL, MKDEV(MAJOR(first),0), NULL, "switch") == NULL)
  {
    goto fail_1;
  }

  printk(KERN_INFO "Device created.\n");

  cdev_init(&c_dev, &switch_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    goto fail_2;
  }

  printk(KERN_INFO "Device init.\n");

  return platform_driver_register(&switch_driver);

fail_2:
  device_destroy(cl, MKDEV(MAJOR(first),0));
fail_1:
  class_destroy(cl);
fail_0:
  unregister_chrdev_region(first, 1);
  return -1;
}


static void __exit switch_exit(void)
{
  platform_driver_unregister(&switch_driver);
  cdev_del(&c_dev);
  device_destroy(cl, MKDEV(MAJOR(first),0));
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
  printk(KERN_ALERT "Switch exit.\n");
}

module_init(switch_init);
module_exit(switch_exit);
