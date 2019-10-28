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

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Led driver");

#define DRIVER_NAME "led"

/*FOPS Functions prototypes*/
static int led_probe(struct platform_device *pdev);
static int led_remove(struct platform_device *pdev);
static int led_open(struct inode *i, struct file *f);
static int led_close(struct inode *i, struct file *f);
static ssize_t led_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t led_write(struct file *f, const char __user *buf, size_t count, loff_t *off);
static int __init led_init(void);
static void __exit led_exit(void);

/*Helper Functions prototypes */
static int int_to_str(int val, char* pBuf, int buf_len, int base);
static unsigned long str_to_int(const char* pStr, int len, int base);
static char ch_to_upper(char ch);

/******************************************************/

static dev_t first;
static struct class *cl;
static struct cdev c_dev;
//static struct device *dev;


static int end_read = 0;

struct led_info {
  unsigned long mem_start;
  unsigned long mem_end;
  void __iomem *base_addr;
};


static struct led_info *lp = NULL;

/* Probe function for led
 * -------------------------------------------------------------------
 * Probe function called when led driver is recognized from device tree
 * after registering platform driver, using compatible string to 
 * search for compatible devices in device tree
 * -------------------------------------------------------------------
 */
static int led_probe(struct platform_device *pdev)
{
  struct resource *r_mem;
  int rc = 0;
  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "invalid address\n");
    return -ENODEV;
  }
  lp = (struct led_info *) kmalloc(sizeof(struct led_info), GFP_KERNEL);
  if (!lp) {
    printk(KERN_ALERT "Cound not allocate led device\n");
    return -ENOMEM;
  }

  lp->mem_start = r_mem->start;
  lp->mem_end = r_mem->end;
  printk(KERN_INFO "base address:%x \t high_address:%x", r_mem->start, r_mem->end);
  //request memory region for led driver, based on resources read from device tree
  if (!request_mem_region(lp->mem_start,lp->mem_end - lp->mem_start + 1,	DRIVER_NAME))
  {
    printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)lp->mem_start);
    rc = -EBUSY;
    goto error1;
  }

  //remap to virtual memory which will be used to access led from user space appplication
  lp->base_addr = ioremap(lp->mem_start, lp->mem_end - lp->mem_start + 1);
  if (!lp->base_addr) {
    printk(KERN_ALERT "led: Could not allocate iomem\n");
    rc = -EIO;
    goto error2;
  }

  return 0;

error2:
  release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
error1:
  return rc;
}


/* Remove function for led
 * -------------------------------------------------------
 *  When led module is removed, turn off all the leds first,
 *  release virtual address and the memory region requested.
 *  Also remove led_info struct allocated in led_probe.
 * --------------------------------------------------------
 */
static int led_remove(struct platform_device *pdev)
{
  printk(KERN_ALERT "led platform driver removed\n");
  iowrite32(0, lp->base_addr);
  iounmap(lp->base_addr);
  release_mem_region(lp->mem_start, lp->mem_end - lp->mem_start + 1);
  //release also memory allocated for led_info struct
  kfree(lp);
  return 0;
}

static struct of_device_id led_of_match[] = {
  { .compatible = "led_gpio", },
  { /* end of list */ },
};

MODULE_DEVICE_TABLE(of, led_of_match);

static int led_open(struct inode *i, struct file *f)
{
 
  return 0;
}

static int led_close(struct inode *i, struct file *f)
{
  
  return 0;
}

static ssize_t led_read(struct file *f, char __user *buf, size_t
                        len, loff_t *off)
{
  char *led_string = "0b0000";
  u32 led_value;
  char buffer[20];
  int i = 0;
  int length;
  int base = 2;
  if(end_read)
  {
    end_read = 0;
    return 0;
  }
  for (i = 2; i < strlen(led_string); i++)
    led_string[i] = '0';
  led_value = ioread32(lp->base_addr);
  length = int_to_str(led_value, buffer, 4, base);
  for(i = 0; i <= length - 1; i++)
  {
    led_string[strlen(led_string) - 1 - i] = buffer[length - 1 - i];
  }
  if (copy_to_user(buf, led_string, strlen(led_string)))
    return -EFAULT;
  end_read=1;
  
  return strlen(led_string);

}

static ssize_t led_write(struct file *f, const char __user *buf,
                         size_t count, loff_t *off)
{
  char buffer[16];
  u32 led_value;
  int str_length;
  
  if (copy_from_user(buffer, buf, count))
    return -EFAULT;
  if (buffer[count - 1]=='\n')
    str_length = count - 1;
  else
    str_length = count;  
  if(buffer[1] == 'b')
    led_value = str_to_int(buffer+2, str_length - 2, 2);  
  else if(buffer[1] == 'x')
    led_value = str_to_int(buffer+2,str_length - 2, 16);  
  else
    led_value = str_to_int(buffer, str_length, 10);  
      
  iowrite32(led_value, lp->base_addr);
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

static unsigned long str_to_int(const char* pStr, int len, int base)
{
  //                      0,1,2,3,4,5,6,7,8,9,:,;,<,=,>,?,@,A ,B ,C ,D ,E ,F
  static const int v[] = {0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,10,11,12,13,14,15};
  int i   = 0;
  unsigned long val = 0;
  int dec = 1;
  int idx = 0;

  for(i = len; i > 0; i--)
  {
    idx = ch_to_upper(pStr[i-1]) - '0';

    if(idx > sizeof(v)/sizeof(int))
    {
      printk("str_to_int: illegal character %c\n", pStr[i-1]);
      continue;
    }

    val += (v[idx]) * dec;
    dec *= base;
  }

  return val;
}

static char ch_to_upper(char ch)
{
  if((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
  {
    return ch;
  }
  else
  {
    return ch - ('a'-'A');
  }
}
static struct file_operations led_fops =
{
  .owner = THIS_MODULE,
  .open = led_open,
  .release = led_close,
  .read = led_read,
  .write = led_write
};

static struct platform_driver led_driver = {
  .driver = {
    .name = DRIVER_NAME,
    .owner = THIS_MODULE,
    .of_match_table	= led_of_match,
  },
  .probe		= led_probe,
  .remove		= led_remove,
};

static int __init led_init(void)
{
  /* unsigned long addr; */
  /* int ret; */

  printk(KERN_INFO "Led init.\n");

  if (alloc_chrdev_region(&first, 0, 1, "Led_region") < 0)
  {
    printk(KERN_ALERT "<1>Failed CHRDEV!.\n");
    return -1;
  }
  printk(KERN_INFO "Succ CHRDEV!.\n");

  if ((cl = class_create(THIS_MODULE, "led_chardrv")) == NULL)
  {
    printk(KERN_ALERT "<1>Failed class create!.\n");
    goto fail_0;
  }
  printk(KERN_INFO "Succ class chardev1 create!.\n");

  if (device_create(cl, NULL, MKDEV(MAJOR(first),0), NULL, "led") == NULL)
  {
    goto fail_1;
  }

  printk(KERN_INFO "Device created.\n");

  cdev_init(&c_dev, &led_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    goto fail_2;
  }

  printk(KERN_INFO "Device init.\n");

  return platform_driver_register(&led_driver);

fail_2:
  device_destroy(cl, MKDEV(MAJOR(first),0));
fail_1:
  class_destroy(cl);
fail_0:
  unregister_chrdev_region(first, 1);
  return -1;
}


static void __exit led_exit(void)
{
  platform_driver_unregister(&led_driver);
  cdev_del(&c_dev);
  device_destroy(cl, MKDEV(MAJOR(first),0));
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
  printk(KERN_ALERT "Led exit.\n");
}

module_init(led_init);
module_exit(led_exit);
