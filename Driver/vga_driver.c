// Copyright [2018] <nikola>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <linux/init.h>
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
#include <linux/dma-mapping.h>  // dma access
#include <linux/mm.h>  // dma access


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver for VGA ouvput");
#define DEVICE_NAME "vga"
#define DRIVER_NAME "vga_dma_driver"
#define MAX_PKT_LEN 640*480*4
//*************************************************************************
static int vga_dma_probe(struct platform_device *pdev);
static int vga_dma_open(struct inode *i, struct file *f);
static int vga_dma_close(struct inode *i, struct file *f);
static ssize_t vga_dma_read(struct file *f, char __user *buf, size_t len, loff_t *off);
static ssize_t vga_dma_write(struct file *f, const char __user *buf, size_t count, loff_t *off);
static ssize_t vga_dma_mmap(struct file *f, struct vm_area_struct *vma_s);
static int __init vga_dma_init(void);
static void __exit vga_dma_exit(void);
static int vga_dma_remove(struct platform_device *pdev);

static irqreturn_t dma_isr(int irq,void*dev_id);
int dma_init(void __iomem *base_address);
u32 dma_simple_write(dma_addr_t TxBufferPtr, u32 max_pkt_len, void __iomem *base_address); // helper function, defined later

static char chToUpper(char ch);
static unsigned long strToInt(const char* pStr, int len, int base);

// *********************GLOBAL VARIABLES*************************************
static struct file_operations vga_dma_fops = {
  .owner = THIS_MODULE,
  .open = vga_dma_open,
  .release = vga_dma_close,
  .read = vga_dma_read,
  .write = vga_dma_write,
  .mmap = vga_dma_mmap
};
static struct of_device_id vga_dma_of_match[] = {
  { .compatible = "xlnx,axi-dma-mm2s-vga-channel", },
  {.compatible = "xlnx,axi-dma-vga-1.00.a"},
  { /* end of list */ },
};

static struct platform_driver vga_dma_driver = {
  .driver = {
    .name = DRIVER_NAME,
    .owner = THIS_MODULE,
    .of_match_table = vga_dma_of_match,
  },
  .probe = vga_dma_probe,
  .remove = vga_dma_remove,
};

struct vga_dma_info {
  unsigned long mem_start;
  unsigned long mem_end;
  void __iomem *base_addr;
  int irq_num;
  
};

static struct vga_dma_info *vp = NULL;

MODULE_DEVICE_TABLE(of, vga_dma_of_match);

static struct cdev c_dev;
static dev_t first;
static struct class *cl;
static int int_cnt;

dma_addr_t tx_phy_buffer;
u32 *tx_vir_buffer;
//***************************************************
// PROBE AND REMOVE

static int vga_dma_probe(struct platform_device *pdev) {
  struct resource *r_mem;
  int rc = 0;

  printk(KERN_INFO "Probing\n");

  r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (!r_mem) {
    printk(KERN_ALERT "invalid address\n");
    return -ENODEV;
  }
  else
    printk(KERN_INFO "platform get resource success\n");
  vp = (struct vga_dma_info *) kmalloc(sizeof(struct vga_dma_info), GFP_KERNEL);
  if (!vp) {
    printk(KERN_ALERT "Cound not allocate vga device\n");
    return -ENOMEM;
  }
  else
  {
    printk(KERN_INFO "allocation of space for vp was a success\n");
  }
  vp->mem_start = r_mem->start;
  vp->mem_end = r_mem->end;
  

  if (!request_mem_region(vp->mem_start,
                          vp->mem_end - vp->mem_start + 1, DRIVER_NAME)) {
    printk(KERN_ALERT "Couldn't lock memory region at %p\n",(void *)vp->mem_start);
    rc = -EBUSY;
    goto error1;
  }
  else {
    printk(KERN_INFO "vga_dma_init: Successfully allocated memory region for vga\n");
  }
  /* 
   * Map Physical address to Virtual address
   */

  vp->base_addr = ioremap(vp->mem_start, vp->mem_end - vp->mem_start + 1);
  if (!vp->base_addr) {
    printk(KERN_ALERT "vga: Could not allocate iomem\n");
    rc = -EIO;
    goto error2;
  }
  else
    printk(KERN_INFO "ioremap was a success\n");
  
  /* 
   * Geting irq num 
   */
  vp->irq_num = platform_get_irq(pdev, 0);
  printk("irq number is: %d\n", vp->irq_num);
  
  if (request_irq(vp->irq_num, dma_isr, 0, DEVICE_NAME, NULL)) {
    printk(KERN_ERR "vga_dmai_init: Cannot register IRQ %d\n", vp->irq_num);
    return -EIO;
  }
  else {
    printk(KERN_INFO "vga_dma_init: Registered IRQ %d\n", vp->irq_num);
  }
  
  
  /* INIT DMA */
  dma_init(vp->base_addr);
  dma_simple_write(tx_phy_buffer, MAX_PKT_LEN, vp->base_addr); // helper function, defined later
  
  printk("probing done");
error2:
  release_mem_region(vp->mem_start, vp->mem_end - vp->mem_start + 1);
error1:
  return rc;

}

static int vga_dma_remove(struct platform_device *pdev)
{
  // Exit Device Module
  u32 reset;
  reset = 0x00000004;
  iowrite32(reset, vp->base_addr); // writing to MM2S_DMACR register. Seting reset bit (3. bit)
  iounmap(vp->base_addr);
  free_irq(vp->irq_num, NULL);
  //release_mem_region(vp->mem_start, vp->mem_end - vp->mem_start + 1);
  return 0;
}

//***************************************************
// IMPLEMENTATION OF FILE OPERATION FUNCTIONS

static int vga_dma_open(struct inode *i, struct file *f)
{
  //printk("vga opened\n");
  return 0;
}
static int vga_dma_close(struct inode *i, struct file *f)
{
  //printk("vga closed\n");
  return 0;
}
static ssize_t vga_dma_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
  //printk("vga read\n");
  return 0;
}
static ssize_t vga_dma_write(struct file *f, const char __user *buf, size_t count, loff_t *off)
{
	
  char buffer[count];
  char *lp='\0';
  char *rp='\0';
  int i = 0;
  unsigned int xpos=0,ypos=0,rgb=0;
  i = copy_from_user(buffer, buf, count);
  buffer[count - 1] = '\0';

  //extract position on x axis 
  lp = buffer;
  rp = strchr(lp,',');
  if(!rp)
  {
    printk("Invalid input, expected format: xpos,ypos,rgb\n");
    return count;
  }
  *rp = '\0';
  rp++;

  if(lp[0]=='0' && lp[1]=='x')
  {
    lp=lp+2;
    xpos = strToInt(lp, strlen(lp), 16);
  }
  else
    xpos = strToInt(lp, strlen(lp), 10);

  //extract position on y axis 
  lp = rp;
  rp = strchr(lp,',');
  if(!rp)
  {
    printk("Invalid input, expected format: xpos,ypos,rgb\n");
    return count;
  }
  *rp = '\0';
  rp++;

  if(lp[0]=='0' && lp[1]=='x')
  {
    lp=lp+2;
    ypos = strToInt(lp, strlen(lp), 16);
  }
  else
    ypos = strToInt(lp, strlen(lp), 10);

  //extract rgb(red,green,blue) value of pixel 
  lp = rp;
  if(!lp)
  {
    printk("Invalid input, expected format: xpos,ypos,rgb\n");
    return count;
  }
  if(lp[0]=='0' && lp[1]=='x')
  {
    lp=lp+2;
    rgb = strToInt(lp, strlen(lp), 16);
  }
  else
    rgb = strToInt(lp, strlen(lp), 10);

	
  if (xpos>=640 || ypos>=480)
  {
    printk("position of pixel is out of bounds\n");
    return count;
  }


  tx_vir_buffer[640*ypos + xpos] = (u32)rgb;


  //printk("Sucessfull write \n");
  return count;

}

static ssize_t vga_dma_mmap(struct file *f, struct vm_area_struct *vma_s)
{
	int ret = 0;
	long length = vma_s->vm_end - vma_s->vm_start;
	//printk(KERN_INFO "DMA TX Buffer is being memory mapped\n");

	if(length > MAX_PKT_LEN)
	{
		return -EIO;
		printk(KERN_ERR "Trying to mmap more space than it's allocated\n");
	}

	ret = dma_mmap_coherent(NULL, vma_s, tx_vir_buffer, tx_phy_buffer, length);
	if(ret<0)
	{
		printk(KERN_ERR "memory map failed\n");
		return ret;
	}
	return 0;
}

/****************************************************/
// IMPLEMENTATION OF DMA related functions

static irqreturn_t dma_isr(int irq,void*dev_id)
{
  u32 IrqStatus;  
  /* Read pending interrupts */
  IrqStatus = ioread32(vp->base_addr + 4);//read irq status from MM2S_DMASR register
  iowrite32(IrqStatus | 0x00007000, vp->base_addr + 4);//clear irq status in MM2S_DMASR register
  //(clearing is done by writing 1 on 13. bit in MM2S_DMASR (IOC_Irq)

  /*Send a transaction*/
  dma_simple_write(tx_phy_buffer, MAX_PKT_LEN, vp->base_addr); //My function that starts a DMA transaction
  return IRQ_HANDLED;;
}

int dma_init(void __iomem *base_address)
{
  u32 reset = 0x00000004;
  u32 IOC_IRQ_EN; 
  u32 ERR_IRQ_EN;
  u32 MM2S_DMACR_reg;
  u32 en_interrupt;
    
  IOC_IRQ_EN = 1 << 12; // this is IOC_IrqEn bit in MM2S_DMACR register
  ERR_IRQ_EN = 1 << 14; // this is Err_IrqEn bit in MM2S_DMACR register

  iowrite32(reset, base_address); // writing to MM2S_DMACR register. Seting reset bit (3. bit)
  
  MM2S_DMACR_reg = ioread32(base_address); // Reading from MM2S_DMACR register inside DMA
  en_interrupt = MM2S_DMACR_reg | IOC_IRQ_EN | ERR_IRQ_EN;// seting 13. and 15.th bit in MM2S_DMACR
  iowrite32(en_interrupt, base_address); // writing to MM2S_DMACR register  
  return 0;
}

u32 dma_simple_write(dma_addr_t TxBufferPtr, u32 max_pkt_len, void __iomem *base_address) {
  u32 MM2S_DMACR_reg;

  MM2S_DMACR_reg = ioread32(base_address); // READ from MM2S_DMACR register

  iowrite32(0x1 |  MM2S_DMACR_reg, base_address); // set RS bit in MM2S_DMACR register (this bit starts the DMA)

  iowrite32((u32)TxBufferPtr, base_address + 24); // Write into MM2S_SA register the value of TxBufferPtr.
  // With this, the DMA knows from where to start.

  iowrite32(max_pkt_len, base_address + 40); // Write into MM2S_LENGTH register. This is the length of a tranaction.
  // In our case this is the size of the image (640*480*4)
  return 0;
}



//***************************************************
// HELPER FUNCTIONS (STRING TO INTEGER)


static char chToUpper(char ch)
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

static unsigned long strToInt(const char* pStr, int len, int base)
{
  //                      0,1,2,3,4,5,6,7,8,9,:,;,<,=,>,?,@,A ,B ,C ,D ,E ,F
  static const int v[] = {0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,10,11,12,13,14,15};
  int i   = 0;
  unsigned long val = 0;
  int dec = 1;
  int idx = 0;

  for(i = len; i > 0; i--)
  {
    idx = chToUpper(pStr[i-1]) - '0';

    if(idx > sizeof(v)/sizeof(int))
    {
      printk("strToInt: illegal character %c\n", pStr[i-1]);
      continue;
    }

    val += (v[idx]) * dec;
    dec *= base;
  }

  return val;
}


//***************************************************
// INIT AND EXIT FUNCTIONS OF THE DRIVER

static int __init vga_dma_init(void)
{
  int i = 0;
  int_cnt = 0;

  printk(KERN_INFO "vga_dma_init: Initialize Module \"%s\"\n", DEVICE_NAME);

  if (alloc_chrdev_region(&first, 0, 1, "VGA_DMA_region") < 0)
  {
    printk(KERN_ALERT "<1>Failed CHRDEV!.\n");
    return -1;
  }
  printk(KERN_INFO "Succ CHRDEV!.\n");

  if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
  {
    printk(KERN_ALERT "<1>Failed class create!.\n");
    goto fail_0;
  }
  printk(KERN_INFO "Succ class chardev1 create!.\n");
  if (device_create(cl, NULL, MKDEV(MAJOR(first),0), NULL, "vga_dma") == NULL)
  {
    goto fail_1;
  }

  printk(KERN_INFO "Device created.\n");

  cdev_init(&c_dev, &vga_dma_fops);
  if (cdev_add(&c_dev, first, 1) == -1)
  {
    goto fail_2;
  }

  printk(KERN_INFO "Device init.\n");
  
  /*ALLOC COHERENT MEMOERY FOR DMA*/
  tx_vir_buffer = dma_alloc_coherent(NULL, MAX_PKT_LEN, &tx_phy_buffer, GFP_DMA | GFP_KERNEL); //GFP_KERNEL
  if(!tx_vir_buffer){
    printk(KERN_ALERT "Could not allocate dma_alloc_coherent for img");
    goto fail_3;
  }
  else
    printk("dma_alloc_coherent success img\n");
  for (i = 0; i < MAX_PKT_LEN/4;i++)
    tx_vir_buffer[i] = 0x00000000;
  printk(KERN_INFO "DMA memory reset.\n");
  return platform_driver_register(&vga_dma_driver);
 fail_3:
  cdev_del(&c_dev);
 fail_2:
  device_destroy(cl, MKDEV(MAJOR(first),0));
 fail_1:
  class_destroy(cl);
 fail_0:
  unregister_chrdev_region(first, 1);
  return -1;

} 

static void __exit vga_dma_exit(void)  		
{
  
  platform_driver_unregister(&vga_dma_driver);
  cdev_del(&c_dev);
  device_destroy(cl, MKDEV(MAJOR(first),0));
  class_destroy(cl);
  unregister_chrdev_region(first, 1);
  dma_free_coherent(NULL, MAX_PKT_LEN, tx_vir_buffer, tx_phy_buffer);
  printk(KERN_INFO "vga_dma_exit: Exit Device Module \"%s\".\n", DEVICE_NAME);
}

module_init(vga_dma_init);
module_exit(vga_dma_exit);

MODULE_AUTHOR ("FTN");
MODULE_DESCRIPTION("Test Driver for VGA output.");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("custom:vga_dma");
