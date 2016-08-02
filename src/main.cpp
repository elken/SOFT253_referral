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

#define DEBUG 0
#define MAX_MESSAGES 16
#define MAX_ITEMS 10
#define CAPACITY (MAX_ITEMS + 1)

/* Instantiate the expansion board */
static X_NUCLEO_IKS01A1 *mems_expansion_board = X_NUCLEO_IKS01A1::Instance(D14, D15);

/* Retrieve the composing elements of the expansion board */
static MotionSensor *accelerometer = mems_expansion_board->GetAccelerometer();

Serial pc(USBTX, USBRX);

// Message struct for the message queue
struct Message {
  const char* message;
};

Ticker ticker;
Mail<Data, MAX_ITEMS> dataMailBox;
Mail<Message, MAX_MESSAGES> messageBox;
Data samples[CAPACITY];
int32_t sampleCount = 0;
Mutex* averageLock = new Mutex();
Semaphore* logSemaphore = new Semaphore(MAX_MESSAGES);

// Send a message to the message box
void sendMessage(const char* msg) {
  logSemaphore->wait();
  Message* m = messageBox.calloc();
  m->message = msg;
  messageBox.put(m);
}

// Print all messages in the mailbox
void printMessages(void const*) {
  while (true) {
    osEvent evt = messageBox.get(1000);
    if (evt.status == osEventMail) {
      Message* m = (Message*) evt.value.p;
      pc.printf(m->message);
      messageBox.free(m);
    }
    logSemaphore->release();
  }
}

// Sample data every 100ms, send error to log thread where applicable
void sampleData() {
    int32_t axes[3];
    averageLock->lock();
#if DEBUG
    sendMessage("Sample data got lock\r\n");
#endif
    accelerometer->Get_X_Axes(axes);
    Data* accelData = dataMailBox.alloc();

    accelData = new Data(axes[0], axes[1], axes[2]);
    osStatus status = dataMailBox.put(accelData);
    averageLock->unlock();
#if DEBUG
    sendMessage("Sample data lost lock\r\n");
#endif

    if (status == osErrorResource) {
#if DEBUG
      char message[50];
      sprintf(message, "Resource not available (%4Xh)\r\n", status);
      sendMessage(message);
#endif
    }
}

/* Simple main function */
int main() {
#if DEBUG
  sendMessage("\r\n--- Starting new debug run---\r\n");
#else
  sendMessage("\r\n--- Starting new run---\r\n");
#endif


#if DEBUG
  uint8_t id;
  accelerometer->ReadID(&id);
  char message[50];
  sprintf(message, "LSM6DS0 Accelerometer             = 0x%X\r\n", id);
  sendMessage(message);
#endif

  Thread logging(printMessages);
  ticker.attach(&sampleData, 0.1);

  while(1) {
    osEvent evt = dataMailBox.get();
    if (evt.status == osEventMail) {
      Data* mailData = (Data*) evt.value.p;
      samples[sampleCount] = *mailData;
      sampleCount = (sampleCount + 1) % CAPACITY;
      if (sampleCount == MAX_ITEMS) {
        averageLock->lock();
        Data averages;
#if DEBUG
        sendMessage("Main got lock\r\n");
#endif
        for (int i = 0; i < MAX_ITEMS; i++) {
          averages = averages + samples[i];
        }
        averages = averages / 10;
        char message[64];
        sprintf(message, "Average: \tx: %ld\t y: %ld\t z: %ld\r\n", averages.x(), averages.y(), averages.z());
        sendMessage(message);
        averages = Data();
        averageLock->unlock();
#if DEBUG
        sendMessage("Main lost lock\r\n");
#endif
      }
      dataMailBox.free(mailData);
      sleep();
    }
  }
}
