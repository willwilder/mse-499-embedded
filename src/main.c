/**************************************************************************/
/*!
    @file     main.c

    @section LICENSE

    Software License Agreement (BSD License)
    Copyright (c) 2015, W. Wilder
    Copyright (c) 2013, K. Townsend (microBuilder.eu)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/
#include <stdio.h>
#include "LPC8xx.h"
#include "gpio.h"
#include "mrt.h"
#include "uart.h"


struct PID{
	int processRunning;
	int Kp;
	int Ki;
	int Kd;
	int error;
	int prevError;
	int integral;
	int derivative;
	int SP;
	int PV;
	int output;
};


#define LED_LOCATION    1	//pin 5
#define SPI_CLK			5	//pin 1
#define SPI_MISO		3 //pin 3
#define SPI_CS			2	//pin 4




/*
 * contents of swm.c from switch matrix tool
 */
void configurePins()
{
    /* Enable SWM clock */
    LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7);

    /* Pin Assign 8 bit Configuration */
    /* U0_TXD */
    /* U0_RXD */
    LPC_SWM->PINASSIGN0 = 0xffff0004UL;
    /* SPI0_SCK */
    //LPC_SWM->PINASSIGN3 = 0x05ffffffUL;
    /* SPI0_MISO */
    /* SPI0_SSEL */
    //LPC_SWM->PINASSIGN4 = 0xff0203ffUL;

    /* Pin Assign 1 bit Configuration */
    LPC_SWM->PINENABLE0 = 0xffffffffUL;

    LPC_GPIO_PORT->DIR0 |= (1 << LED_LOCATION);
    LPC_GPIO_PORT->DIR0 |= (1 << SPI_CS);
    LPC_GPIO_PORT->DIR0 |= (1 << SPI_CLK);
    LPC_GPIO_PORT->DIR0 |= (0 << SPI_MISO);
}

/*
void showbits(uint32_t x){
	int i;
	//for(i=31; i>=0; i--)
	for(i=sizeof(uint32_t)*8-1; i>=0; i--)
		(x&(1<<i))?printf("1"):printf("0");
	printf("\n");
}
*/


int readCelsius() {

	int i;
	int MISO_pinValue;
	uint32_t rawTemperature = 0;

	gpioSetValue(0,SPI_CLK,0);
	mrtDelay(1);
	gpioSetValue(0,SPI_CS,0);
	mrtDelay(1);

	for (i=31; i>=0; i--)
	{
		gpioSetValue(0,SPI_CLK,0);
		mrtDelay(1);
		MISO_pinValue = gpioGetPinValue(0,SPI_MISO);
		if (MISO_pinValue) {
			rawTemperature |= (1<<i);
		}
		gpioSetValue(0,SPI_CLK,1);
		mrtDelay(1);
	}
	gpioSetValue(0,SPI_CS,1);

  /*
   * Killed Thermocouple fault handling
   *
  if (rawTemperature & 0x1) {
    // uh oh, a serious problem!
    printf("Thermocouple Connection Open Connection\r\n");
    return;
  }
  if (rawTemperature & 0x2)
  {
		// uh oh, a serious problem!
		printf("Thermocouple Ground Fault\r\n");
		return;
  }
  if (rawTemperature & 0x4)
  {
		// uh oh, a serious problem!
		printf("Thermocouple VCC Fault\r\n");
		return;
  }
  */

  // get rid of internal temp data, and any fault bits
  rawTemperature >>= 18;
  int sign = rawTemperature & 0x2000;
  //get raw temperature, mask the sign bit
  int currentTemp = (rawTemperature)>>2;

  // check sign bit
  if (sign)
    currentTemp *= -1;


  //printf("t=%d\n",currentTemp);

  return currentTemp;
}

/*
void printFloat(){
	float testFloat = 1.23;
	uint8_t *p = (uint8_t*)&testFloat;
	printf("T");
	for(i = sizeof(testFloat)-1;i>=0;i--) {
	  //printf("\\x");
	  printf("%c",p[i]);
}*/

int setOutput(int state){
	gpioSetValue(0,LED_LOCATION,state);
	return state;
}

void resetPID(struct PID *myPID){

	myPID->error = 0;
	myPID->prevError = 0;
	myPID->integral = 0;
	myPID->derivative = 0;
	myPID->output = 0;

}

void printStatus(struct PID *myPID){
	printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		myPID->processRunning,
		myPID->SP,
		myPID->PV,
		myPID->Kp,
		myPID->Ki,
		myPID->Kd,
		myPID->error,
		myPID->prevError,
		myPID->integral,
		myPID->derivative,
		myPID->output);

}

int main(void)
{
  /* Initialise the GPIO block */
  gpioInit();

  /* Initialise the UART0 block for printf output */
  uart0Init(115200);

  /* Configure the multi-rate timer for 1ms ticks */
  mrtInit(__SYSTEM_CLOCK/1000);

  /* Configure the switch matrix (setup pins for UART0 and GPIO) */
  configurePins();


  int printOn = 0;
  int digOut = 0;
  int index = 0;
  char c = 0;
  int i = 0;
  char buffer[10];
  char command = 0;
  int printInterval = 500;
  int lastPrintTime=0;

  int lastPIDTime=0;
  int PIDInterval = 100;

  struct PID myPID = {0,0,0,0,0,0,0,0,0,0,0};

  setOutput(0);


  while(1)
  {

	  if(printOn && ((mrt_counter-lastPrintTime)>printInterval))
	  {
		  myPID.PV = readCelsius();
		  printStatus(&myPID);
		  lastPrintTime=mrt_counter;
	  }

	  if(myPID.processRunning && ((mrt_counter-lastPIDTime)>PIDInterval)){

		  myPID.PV = readCelsius();
		  myPID.error = myPID.SP - myPID.PV;
		  myPID.integral += myPID.error;// * PIDInterval;
		  myPID.derivative = (myPID.error - myPID.prevError);

		  myPID.output =
				  myPID.Kp * myPID.error +
				  myPID.Kd * myPID.derivative;

		  if(myPID.Ki!=0) myPID.output += (myPID.integral / myPID.Ki);

		  myPID.prevError = myPID.error;

		  if(myPID.output > 0 ) setOutput(1);
		  else setOutput(0);

		  printStatus(&myPID);

	  }


	  while(uart0DataPresent())
	  {
		  c = uart0GetChar();

		  if(c==0x0a)
		  {
		  	  /*printf("command=[");
		  	  for(i=0;i<index-1;i++)
		  	  {
		  		  printf("%c",buffer[i]);
		  	  }
		  	  mrtDelay(1);
		  	  printf("]\n");
			  */
		  	  command = buffer[0];

		  	  switch(command)
		  	  {
		  	  	  case 'd':
		  	  		  digOut = !digOut;
		  	  		  setOutput(digOut);
		  	  		  printf("digOut = %d\n",digOut);
		  	  		  break;

		  	  	  case 'p':
		  	  		  if(buffer[1]=='1')
		  	  			  printOn = 1;
		  	  		  else
		  	  			  printOn = 0;
		  	  		  //printf("printOn = %d\n",printOn);
		  	  		  break;
		  	  	  case 'r':
					  if(buffer[1]=='1')
						  myPID.processRunning = 1;
					  else{
						  myPID.processRunning = 0;
						  resetPID(&myPID);
					  }
					  if(!myPID.processRunning){
						  setOutput(0);
					  }
					  break;
		  	  	  case 'P':
		  	  		  myPID.Kp = buffer[1];
		  	  		  break;

		  	  	  case 'I':
		  	  		  myPID.Ki = buffer[1];
		  	  		  break;
		  	  	  case 'D':
		  	  		  myPID.Kd = buffer[1];
		  	  		  break;
		  	  	  /*case 'R':
		  	  		myPID.processRunning = 0;
		  	  		setOutput(0);
					*/

		  	  	  case 'S':
					  myPID.SP = buffer[1];
					  break;
		  	  	  default:
		  	  		  break;
		  	  }
		  	  index = 0;
		  	  for(i=0;i<sizeof(buffer);i++){
		  		  buffer[i]=0;
		  	  }
		  	  c = 0;
		  	  break;
		  }

		  buffer[index]=c;
		  index++;

	  }
  }
}
