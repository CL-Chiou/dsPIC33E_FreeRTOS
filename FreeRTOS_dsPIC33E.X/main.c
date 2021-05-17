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


/*
 * The check task as described at the top of this file.
 */
void OSCILLATOR_Initialize(void);
static void vToggleLED1Task(void *pvParameters);
static void vToggleLED2Task(void *pvParameters);
static void vToggleLED3Task(void *pvParameters);
static void vToggleLED4Task(void *pvParameters);

/*
 * 
 */
int main(void) {
    OSCILLATOR_Initialize();
    /* Create the test tasks defined within this file. */
    xTaskCreate(vToggleLED1Task, "ToggleLED1", 105, NULL, 1, NULL);
    /* Finally start the scheduler. */
    vTaskStartScheduler();
    // Will not get here unless there is insufficient RAM.
    return 0;
}

static void vToggleLED1Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT(((uint32_t) pvParameters) == 1);
    vTaskDelay((100 / portTICK_PERIOD_MS));
    xTaskCreate(vToggleLED2Task, "ToggleLED2", 105, NULL, 1, NULL);
    TRISAbits.TRISA5 = 0;
    for (;;) {
        LATAbits.LATA5 ^= 1;
        vTaskDelayUntil(&xLastWakeTime, (500 / portTICK_PERIOD_MS));
    }
    vTaskDelete(NULL);
}

static void vToggleLED2Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT(((uint32_t) pvParameters) == 1);
    vTaskDelay((100 / portTICK_PERIOD_MS));
    xTaskCreate(vToggleLED3Task, "ToggleLED3", 105, NULL, 1, NULL);
    TRISAbits.TRISA4 = 0;
    for (;;) {
        LATAbits.LATA4 ^= 1;
        vTaskDelayUntil(&xLastWakeTime, (500 / portTICK_PERIOD_MS));
    }
    vTaskDelete(NULL);
}

static void vToggleLED3Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT(((uint32_t) pvParameters) == 1);
    vTaskDelay((100 / portTICK_PERIOD_MS));
    xTaskCreate(vToggleLED4Task, "ToggleLED4", 105, NULL, 1, NULL);
    TRISAbits.TRISA3 = 0;
    for (;;) {
        LATAbits.LATA3 ^= 1;
        vTaskDelayUntil(&xLastWakeTime, (500 / portTICK_PERIOD_MS));
    }
    vTaskDelete(NULL);
}

static void vToggleLED4Task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* The parameter value is expected to be 1 as 1 is passed in the
    pvParameters value in the call to xTaskCreate() below. */
    configASSERT(((uint32_t) pvParameters) == 1);
    TRISAbits.TRISA2 = 0;
    vTaskDelay((100 / portTICK_PERIOD_MS));
    for (;;) {
        LATAbits.LATA2 ^= 1;
        vTaskDelayUntil(&xLastWakeTime, (500 / portTICK_PERIOD_MS));
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
    PLLFBD = 38; /* M=40 */
    CLKDIVbits.PLLPOST = 0; /* N1=2 */
    CLKDIVbits.PLLPRE = 0; /* N2=2 */
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
    while (OSCCONbits.LOCK != 1) {
    };
}

