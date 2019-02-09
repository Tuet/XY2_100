/*  XY2_100 library
    Copyright (c) 2018 Lutz Lisseck

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <string.h>
#include "XY2_100.h"

uint16_t XY2_100::lastX;
uint16_t XY2_100::lastY;
void * XY2_100::pingBuffer;
void * XY2_100::pongBuffer;
DMAChannel XY2_100::dma;

static DMAMEM int pingMemory[10];
static DMAMEM int pongMemory[10];

// Bit0: 0=Ping buffer content is beeing transmitted
static volatile uint8_t txPing = 0;

XY2_100::XY2_100()
{
	pingBuffer = pingMemory;
	pongBuffer = pongMemory;
	txPing = 0;
}


void XY2_100::begin(void)
{
	uint32_t bufsize, frequency;
	bufsize = 40;

	// set up the buffers
	memset(pingBuffer, 0, bufsize);
	memset(pongBuffer, 0, bufsize);

	// configure the 8 output pins
	GPIOD_PCOR = 0xFF;
  GPIOD_PDOR = 0x0F;
	pinMode(2, OUTPUT);	 // bit 0
	pinMode(14, OUTPUT); // bit 1
	pinMode(7, OUTPUT);  // bit 2
	pinMode(8, OUTPUT);  // bit 3
	pinMode(6, OUTPUT);  // bit 4
	pinMode(20, OUTPUT); // bit 5
	pinMode(21, OUTPUT); // bit 6
	pinMode(5, OUTPUT);  // bit 7

	frequency = 4000000;
  
	// DMA channel writes the data
	dma.sourceBuffer((uint8_t *)pingBuffer, bufsize);
	dma.destination(GPIOD_PDOR);
	dma.transferSize(1);
	dma.transferCount(bufsize);
	dma.disableOnCompletion();
  dma.interruptAtCompletion();
  
	pinMode(9, OUTPUT); // testing: oscilloscope trigger

#if defined(__MK20DX256__)  
  // TEENSY 3.1/3.2
	FTM2_SC = 0;
	FTM2_CNT = 0;
	uint32_t mod = (F_BUS + frequency / 2) / frequency;
	FTM2_MOD = mod - 1; // 11 @96Mhz, 8 @72MHz
	FTM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0); // increment on every TPM clock, prescaler 1
  
  // need ISR also
  FTM2_C0SC = 0x69;  // MSB:MSA 10, ELSB:ELSA 10, DMA on
	FTM2_C0V = (mod * 128) >> 8;  // 256 = 100% of the time

  // route the timer interrupt to trigger the dma channel
  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_CH0);
  // enable a done interrupts when channel completes
	dma.attachInterrupt(isr);

	FTM2_C0SC = 0x28;
  noInterrupts();
	FTM2_SC = 0;             // stop FTM2 timer (hopefully before it rolls over)
	FTM2_CNT = 0;

	//PORTB_ISFR = (1<<18);    // clear any prior rising edge
	uint32_t tmp __attribute__((unused));
	FTM2_C0SC = 0x28;
	tmp = FTM2_C0SC;         // clear any prior timer DMA triggers
	FTM2_C0SC = 0x69;
  dma.enable();
	FTM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0); // restart FTM2 timer
  
#elif defined(__MKL26Z64__)
  // TEENSY LC
	FTM2_SC = 0;
	FTM2_CNT = 0;
	uint32_t mod = F_CPU / frequency;
	FTM2_MOD = mod - 1;
	FTM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0); // increment on every TPM clock, prescaler 1
  
	// route the timer interrupt to trigger the dma channel
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_FTM2_OV);
  // enable a done interrupts when channel completes
	dma.attachInterrupt(isr);

	uint32_t sc __attribute__((unused)) = FTM2_SC;  
	noInterrupts();
	FTM2_SC = 0;		// stop FTM2 timer (hopefully before it rolls over)
	dma.clearComplete();
	dma.transferCount(bufsize);
	dma.sourceBuffer((uint8_t *)pingBuffer, bufsize);
	// clear any pending event flags
	FTM2_SC = FTM_SC_TOF;
	dma.enable();		// enable DMA channel
	FTM2_CNT = 0; // writing any value resets counter
	FTM2_SC = FTM_SC_DMA | FTM_SC_CLKS(1) | FTM_SC_PS(0);
#endif  

	//digitalWriteFast(9, LOW);
	interrupts();
}

void XY2_100::isr(void)
{
	//digitalWriteFast(9, LOW);
  
	dma.clearInterrupt();
  if(txPing & 2) {
    txPing &= ~2;
    if(txPing & 1) {
      dma.sourceBuffer((uint8_t *)pongBuffer, 40);
    } else {
      dma.sourceBuffer((uint8_t *)pingBuffer, 40);
    }
  }
  //txPing |= 128;

#if defined(__MK20DX256__)  
  FTM2_SC = 0;
  FTM2_SC = FTM_SC_TOF;
  uint32_t tmp __attribute__((unused));
	FTM2_C0SC = 0x28;
	tmp = FTM2_C0SC;         // clear any prior timer DMA triggers
	FTM2_C0SC = 0x69;
  FTM2_CNT = 0;
	dma.enable();		// enable DMA channel
  FTM2_SC = FTM_SC_CLKS(1) | FTM_SC_PS(0); // restart FTM2 timer
#elif defined(__MKL26Z64__)
  FTM2_SC = 0;
  FTM2_SC = FTM_SC_TOF;
	dma.enable();		// enable DMA channel
	FTM2_CNT = 0; // writing any value resets counter 
	FTM2_SC = FTM_SC_DMA | FTM_SC_CLKS(1) | FTM_SC_PS(0);  
#endif  

 	//digitalWriteFast(9, HIGH); // oscilloscope trigger 
}

uint8_t XY2_100::stat(void) { 
  uint8_t ret = txPing;
  txPing &= ~128;
  return ret; 
}

void XY2_100::setSignedXY(int16_t X, int16_t Y) 
{
  // -32768 => 0; 32767 => 65535;
  int32_t xu = (int32_t)X + 32768L, yu = (int32_t)Y + 32768L;
  setXY((uint16_t)xu, (uint16_t)yu);
}

void XY2_100::setXY(uint16_t X, uint16_t Y)
{
	uint32_t *p;
  uint32_t Ch1 = (((uint32_t)X << 1) | 0x20000ul) & 0x3fffeul;
  uint32_t Ch2 = (((uint32_t)Y << 1) | 0x20000ul) & 0x3fffeul;
  uint8_t parity1 = 0;
  uint8_t parity2 = 0;
  
  // 0123456789abcdef
  // fedcba9876543210
  // every 16-bit word generates a clock pulse also
  const uint16_t Sync1[4] = { 0xd2c3, 0x9687, 0x5a4b, 0x1e0f  };
  const uint16_t Sync0[4] = { 0xf0e1, 0xb4a5, 0x7869, 0x3c2d  };
  
  lastX = X;
  lastY = Y;
  
  for(int i=0; i<20; i++) {
    if(Ch1 & (1 << i)) parity1++;
    if(Ch2 & (1 << i)) parity2++;
  }  
  if(parity1 & 1) Ch1 |= 1;
  if(parity2 & 1) Ch2 |= 1;

	if(txPing & 1) {
    p = ((uint32_t *) pingBuffer);
  } else {
    p = ((uint32_t *) pongBuffer);
  }
  
  // Clock cycle: 1-0, Sync 111...0
  for(int i=19; i>=0; i--) {
    int j = 0;
    uint32_t d;
    if(Ch1 & (1 << i)) j = 1;
    if(Ch2 & (1 << i)) j |= 2;
    d = Sync1[j];
    i--;
    j = 0;
    if(Ch1 & (1 << i)) j = 1;
    if(Ch2 & (1 << i)) j |= 2;
    if(i != 0) d |= (uint32_t)Sync1[j] << 16; else d |= (uint32_t)Sync0[j] << 16;
    *p++ = d;
  }
  
  noInterrupts();
  txPing ^= 1;
  txPing |= 2;
  interrupts();
}


