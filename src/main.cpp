/**
 ******************************************************************************
 * @file    main.cpp
 * @author  AST / EST
 * @version V0.0.1
 * @date    14-August-2015
 * @brief   Simple Example application for using the X_NUCLEO_IKS01A1
 *          MEMS Inertial & Environmental Sensor Nucleo expansion board.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************


 HINTS:

 Use a Ticker for accurate sampling, but do NOT use printf or locks inside an ISR. Instead, use a MailBox to safely move data across from an ISR to a Thread
 Many functions in MBED are thread-safe - check the online docs

 For buffering, use an Array (of structures) and the producer-consumer pattern (or a variant of it).
 DO NOT use a mailbox or queue to perform the buffering

 Perform serial comms on another thread

 Beware of a thread running out of stack space. If you have to use a lot of local variable data, consider increasing the size of the stack for the respective thread. See the constructor for Thread in the docs

 In terms of diagnostics, consider the following type of information:

 An indication that the sampling is running (not every sample maybe, but a heart-beat type indication)
 An error if the buffer is full
 An warning if the buffer is empty
 Anything that helps diagnose a deadlock (e.g. output a message / toggle an LED before a lock is taken and after it is released)

 For high marks in the logging aspect, remember that although printf is thread safe (not interrupt safe), printf from multiple threads will result in interleaved text.
 To solve this, have a logging thread that queues up whole messages and write them to the serial interface one at a time - this is ambitious but can be done
*/

/* Includes */
#include "mbed.h"
#include "rtos.h"
#include "x_nucleo_iks01a1.h"
#include "cmsis_os.h"
#include "data.hpp"

#define MAX_ITEMS 10

/* Instantiate the expansion board */
static X_NUCLEO_IKS01A1 *mems_expansion_board = X_NUCLEO_IKS01A1::Instance(D14, D15);

/* Retrieve the composing elements of the expansion board */
static MotionSensor *accelerometer = mems_expansion_board->GetAccelerometer();

Ticker ticker;
Mail<Data, MAX_ITEMS> dataMailBox;
osThreadId mainThreadId;
Data averages;

void initSensors() {
  uint8_t id;
  printf("\r\n--- Starting new run ---\r\n");

  accelerometer->ReadID(&id);
  printf("LSM6DS0 Accelerometer             = 0x%X\r\n", id);
}

void sampleData() {
    int32_t axes[3];
    accelerometer->Get_X_Axes(axes);
    Data* accelData = dataMailBox.alloc();
    if (dataMailBox.alloc() == NULL) {
      osSignalSet(mainThreadId, 0x1);
      averages = averages / 10;
    }

    accelData = new Data(axes[0], axes[1], axes[2]);
    averages = averages + *accelData;

    osStatus status = dataMailBox.put(accelData);

    if (status == osErrorResource) {
      printf("Resource not available (%4Xh)", status);
    }
}

/* Simple main function */
int main() {
  mainThreadId = osThreadGetId();
  initSensors();
  ticker.attach(&sampleData, 0.1);

  while(1) {
    osSignalWait(0x1, osWaitForever);
    printf("Averages: \tx: %ld\t y: %ld\t z: %ld\r\n", averages.x(), averages.y(), averages.z());
    dataMailBox = Mail<Data, MAX_ITEMS>();
    averages = {0, 0, 0};
    sleep();
  }
}
