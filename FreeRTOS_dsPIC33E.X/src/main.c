/* 
 * File:   main.c
 * Author: Charlie
 *
 * Created on 2021年5月2日, 下午 8:26
 */


/* Standard includes. */
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "timers.h"

/* application includes. */
#include "comport.h"

#define mainAUTO_RELOAD_TIMER_PERIOD pdMS_TO_TICKS (250)
#define mainCOM_PRIORITY                       2

/* Baud rate used by the comtest tasks. */
#define mainCOM_BAUD_RATE                      115200

/* The LED used by the comtest tasks.  mainCOM_TEST_LED + 1 is also used.
See the comtest.c file for more information. */
#define mainCOM_TEST_LED                            2

uint8_t U1RxBuffer[32], U1RxBufferLength = 0;

/*
 * The check task as described at the top of this file.
 */
void OSCILLATOR_Initialize(void);
static void vWaitToU1ReceiveCompletedTask(void *pvParameters);

/*
 * 
 */

static void prvAutoReloadTimerCallback(TimerHandle_t xTimer) {
    LATAbits.LATA5 ^= 1;
}

int main(void) {
    TimerHandle_t xAutoReloadTimer, xOneShotTimer;
    BaseType_t xTimer1Started, xTimer2Started;
    
    OSCILLATOR_Initialize();
    
    /* Configure any hardware required for this demo. */
    vParInitialise();
    /* Create the auto-reload timer, storing the handle to the created timer in xAutoReloadTimer. */
    xAutoReloadTimer = xTimerCreate(
            /* Text name for the software timer - not used by FreeRTOS. */
            "AutoReload",
            /* The software timer's period in ticks. */
            mainAUTO_RELOAD_TIMER_PERIOD,
            /* Setting uxAutoRealod to pdTRUE creates an auto-reload timer. */
            pdTRUE,
            /* This example does not use the timer id. */
            0,
            /* The callback function to be used by the software timer being created. */
            prvAutoReloadTimerCallback);

    vAltStartComTestTasks(mainCOM_PRIORITY, mainCOM_BAUD_RATE, mainCOM_TEST_LED);
    /* Create the test tasks defined within this file. */
    xTaskCreate(
            /* Function that implements the task. */
            vWaitToU1ReceiveCompletedTask,
            /* Text name for the task. */
            "CopyRxCharFromQueue",
            /* Stack size in words, not bytes. */
            configMINIMAL_STACK_SIZE,
            /* Parameter passed into the task. */
            (void*) &U1RxBuffer,
            /* Priority at which the task is created. */
            1,
            /* Used to pass out the created task's handle. */
            NULL);

    /* Finally start the scheduler. */
    /* Check the software timers were created. */
    if ((xAutoReloadTimer != NULL)) {
        /* Start the software timers, using a block time of 0 (no block time). The scheduler has
        not been started yet so any block time specified here would be ignored anyway. */
        xTimer1Started = xTimerStart(xAutoReloadTimer, 0);
        /* The implementation of xTimerStart() uses the timer command queue, and xTimerStart()
        will fail if the timer command queue gets full. The timer service task does not get
        created until the scheduler is started, so all commands sent to the command queue will
        stay in the queue until after the scheduler has been started. Check both calls to
        xTimerStart() passed. */
        if ((xTimer1Started == pdPASS)) {
            /* Start the scheduler. */
            vTaskStartScheduler();
        }
    }
    // Will not get here unless there is insufficient RAM.
    return 0;
}

static void vWaitToU1ReceiveCompletedTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    extern uint16_t serNumberOfReceiveFromU1Rx;
    uint16_t PreviousNumberOfReceiveFromU1Rx = 0;
    uint8_t cCopyCharToBuffer = 0;
    uint8_t cReceiveCharFromQueue;
    uint8_t *RxBuffer;
    extern QueueHandle_t xRxedChars;
    
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT(((uint32_t) pvParameters) == 1);
    
    for (;;) {
        if ((PreviousNumberOfReceiveFromU1Rx == serNumberOfReceiveFromU1Rx)) {
            if ((serNumberOfReceiveFromU1Rx != 0)) ++U1RxBufferLength;
        } else {
            RxBuffer = (uint8_t*) pvParameters;
            U1RxBufferLength = 0;
        }
        if (U1RxBufferLength >= 5) {
            for (cCopyCharToBuffer = 0; cCopyCharToBuffer < serNumberOfReceiveFromU1Rx; cCopyCharToBuffer++) {
                if (xQueueReceive(xRxedChars, &cReceiveCharFromQueue, (TickType_t) 0x0000) == pdTRUE) {
                    *RxBuffer++ = cReceiveCharFromQueue;
                }
            }
            serNumberOfReceiveFromU1Rx = 0;
            U1RxBufferLength = 0;
        }
        PreviousNumberOfReceiveFromU1Rx = serNumberOfReceiveFromU1Rx;
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    vTaskDelete(NULL);
}

void vApplicationIdleHook(void) {
    /* Schedule the co-routines from within the idle task hook. */
    vCoRoutineSchedule();
}

void OSCILLATOR_Initialize(void) {
    /*  Configure Oscillator to operate the device at 40Mhz
     Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
     Fosc= 8M*40/(2*2)=80Mhz for 8M input clock */
    PLLFBD = 38; //M=40
    CLKDIVbits.PLLPOST = 0; // N1=2
    CLKDIVbits.PLLPRE = 0; // N2=2
    OSCTUN = 0; /* Tune FRC oscillator, if FRC is used */

    /* Disable Watch Dog Timer */
    RCONbits.SWDTEN = 0;

    /* Clock switch to incorporate PLL*/
    __builtin_write_OSCCONH(0x03); // Initiate Clock Switch to Primary

    // Oscillator with PLL (NOSC=0b011)
    __builtin_write_OSCCONL(OSCCON || 0x01); // Start clock switching

    while (OSCCONbits.COSC != 0b011);
    // Wait for Clock switch to occur
    /* Wait for PLL to lock */
    while (OSCCONbits.LOCK != 1);
}

