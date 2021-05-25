/* 
 * File:   UART1.h
 * Author: user
 *
 * Created on 2020年4月30日, 上午 9:00
 */

#ifndef serial_H
#define	serial_H

#ifdef	__cplusplus
extern "C" {
#endif

    typedef void * xComPortHandle;

    xComPortHandle xSerialPortInitMinimal(unsigned long ulWantedBaud, unsigned portBASE_TYPE uxQueueLength);
    void vSerialPutString(xComPortHandle pxPort, const signed char * const pcString, unsigned short usStringLength);
    signed portBASE_TYPE xSerialGetChar(xComPortHandle pxPort, signed char *pcRxedChar, TickType_t xBlockTime);
    signed portBASE_TYPE xSerialPutChar(xComPortHandle pxPort, signed char cOutChar, TickType_t xBlockTime);
    portBASE_TYPE xSerialWaitForSemaphore(xComPortHandle xPort);
    void vSerialClose(xComPortHandle xPort);


#ifdef	__cplusplus
}
#endif

#endif	/* UART1_H */

