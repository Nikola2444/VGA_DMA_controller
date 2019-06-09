/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
//#include "printf.h"
#include "xparameters.h"
#include "xaxidma.h"
#include "xil_io.h"
#include "xbasic_types.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "xil_cache.h"
#include "lena_hex.h"
#include "sleep.h"


#include "xdebug.h"


#define DMA_DEV_ID		XPAR_AXIDMA_0_DEVICE_ID

#ifdef XPAR_AXI_7SDDR_0_S_AXI_BASEADDR
#define DDR_BASE_ADDR		XPAR_AXI_7SDDR_0_S_AXI_BASEADDR
#elif XPAR_MIG7SERIES_0_BASEADDR
#define DDR_BASE_ADDR	XPAR_MIG7SERIES_0_BASEADDR
#elif XPAR_MIG_0_BASEADDR
#define DDR_BASE_ADDR	XPAR_MIG_0_BASEADDR
#elif XPAR_PSU_DDR_0_S_AXI_BASEADDR
#define DDR_BASE_ADDR	XPAR_PSU_DDR_0_S_AXI_BASEADDR
#endif

#ifndef DDR_BASE_ADDR
#warning CHECK FOR THE VALID DDR ADDRESS IN XPARAMETERS.H, \
		DEFAULT SET TO 0x01000000
#define MEM_BASE_ADDR		0x01000000
#else
#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x1000000)
#endif

#define TX_INTR_ID			XPAR_FABRIC_AXI_DMA_0_MM2S_INTROUT_INTR
#define TX_BUFFER_BASE		(MEM_BASE_ADDR + 0x00001000)

#define INTC_DEVICE_ID 		XPAR_PS7_SCUGIC_0_DEVICE_ID

#define RESET_TIMEOUT_COUNTER	10000
#define MAX_PKT_LEN 307200*4
static void TxIntrHandler(void *Callback);
static void DisableIntrSystem();
static void TxIntrHandler(void *Callback);

u32 Init_Function(u32 DeviceId);
u32 DMA_init();
u32 dma_simple_write(u32 *TxBufferPtr, u32 max_pkt_len, u32 base_address); // my function, go and have a look of her body

volatile int Error;
volatile int tx_intr_done;

static XScuGic INTCInst;
static XAxiDma AxiDma;		/* Instance of the XAxiDma */
u32 *TxBufferPtr = (u32 *)TX_BUFFER_BASE;


int main()
{
	int status;
	u32 MM2S_DMACR_reg;
	Xil_DCacheDisable();
	Xil_ICacheDisable();
    init_platform();

    xil_printf("\r\nStarting simulation");
    status = Init_Function(INTC_DEVICE_ID);
    u32 j = 0;
    //Copy image to TxBuffer
    for(int i=0; i<MAX_PKT_LEN/4; i++){
		TxBufferPtr[i] = image[i];
	}
	//Start first DMA transaction
	dma_simple_write(TxBufferPtr, MAX_PKT_LEN, XPAR_AXI_DMA_0_BASEADDR); //My function that starts a DMA transaction

    sleep(10);//10 sec delay

    //Change values of TxBuffer
	for(int i=0; i<MAX_PKT_LEN/4; i++){
		if(i < MAX_PKT_LEN/8)
			TxBufferPtr[i] = 0xff00;
		else
			TxBufferPtr[i] = 0x00ff;
	}

	sleep(10);//10 sec delay
    cleanup_platform();
    Xil_DCacheDisable();
	Xil_ICacheDisable();

	DisableIntrSystem();
    return 0;
}

// Interrupt Handler
static void TxIntrHandler(void *Callback)
{
	//In every interrupt handler send the next transaction to continue the cycle
	u32 IrqStatus;
	int TimeOut, status;
	XAxiDma *AxiDmaInst = (XAxiDma *)Callback;
	u32 MM2S_DMACR_reg;
	/* Read pending interrupts */

	//Read irq status from MM2S_DMASR register
	IrqStatus = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + 4);

	//Clear irq status in MM2S_DMASR register
	//(clearing is done by writing 1 on 13. bit in MM2S_DMASR (IOC_Irq)
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR + 4, IrqStatus | 0x00007000);

	/*Send a transaction*/
	dma_simple_write(TxBufferPtr, MAX_PKT_LEN, XPAR_AXI_DMA_0_BASEADDR); //My function that starts a DMA transaction

	tx_intr_done = 1;

}

// Initialize System  function
u32 Init_Function(u32 DeviceId)
{
	XScuGic_Config *IntcConfig;
	int status;
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
	if(status != XST_SUCCESS) return XST_FAILURE;
	status = XScuGic_SelfTest(&INTCInst);
	if (status != XST_SUCCESS)
	{
	  return XST_FAILURE;
	  printf("error");
	}

	//DMA enable and connect interrupt
	DMA_init();

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
		(Xil_ExceptionHandler)XScuGic_InterruptHandler,&INTCInst);
	Xil_ExceptionEnable();

	return XST_SUCCESS;

}


u32 DMA_init()
{
	int Status;
	u32 reset = 0x00000004;
	u32 MM2S_DMACR_reg;

	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR,  reset); // writing to MM2S_DMACR register

	XScuGic_SetPriorityTriggerType(&INTCInst, TX_INTR_ID, 0xA8, 0x3);

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(&INTCInst, TX_INTR_ID, (Xil_InterruptHandler)TxIntrHandler, NULL);
	if (Status != XST_SUCCESS) {
		return Status;
	}
	XScuGic_Enable(&INTCInst, TX_INTR_ID);

	/* THIS HERE IS NEEDED TO CONFIGURE DMA*/
	//enable interrupts
	 u32 IOC_IRQ_EN = 1 << 12; // this is IOC_IrqEn bit in MM2S_DMACR register
     u32 ERR_IRQ_EN = 1 << 14; // this is Err_IrqEn bit in MM2S_DMACR register

     MM2S_DMACR_reg = Xil_In32(XPAR_AXI_DMA_0_BASEADDR); // Reading from MM2S_DMACR register inside DMA
	 u32 en_interrupt = MM2S_DMACR_reg | IOC_IRQ_EN | ERR_IRQ_EN;// seting 13. and 15.th bit in MM2S_DMACR
	 Xil_Out32(XPAR_AXI_DMA_0_BASEADDR,  en_interrupt); // writing to MM2S_DMACR register
	 /************************************************************/


	/* Initialize flags before start transfer test  */
	tx_intr_done = 0;
	Error = 0;
	return 0;
}

u32 dma_simple_write(u32 *TxBufferPtr, u32 max_pkt_len, u32 base_address) {
	u32 MM2S_DMACR_reg;

	MM2S_DMACR_reg = Xil_In32(base_address); // READ from MM2S_DMACR register

	Xil_Out32(base_address, 0x1 |  MM2S_DMACR_reg); // set RS bit in MM2S_DMACR register (this bit starts the DMA)

	Xil_Out32(base_address + 24,  (UINTPTR)TxBufferPtr); // Write into MM2S_SA register the value of TxBufferPtr.
														 // With this, the DMA knows from where to start.

	Xil_Out32(base_address + 40,  max_pkt_len); // Write into MM2S_LENGTH register. This is the length of a tranaction.
												// In our case this is the size of the image (640*480*4)
	return 0;
}
static void DisableIntrSystem()
{

	XScuGic_Disconnect(&INTCInst, TX_INTR_ID);

}
