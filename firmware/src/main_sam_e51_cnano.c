/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

//DOM-IGNORE-BEGIN 
/*******************************************************************************
* Copyright (C) 2024 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
//DOM-IGNORE-END 

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <string.h>
#include "definitions.h"                // SYS function prototypes

/* RTC Time period match values for input clock of 1 KHz */
#define PERIOD_500MS                            512
#define PERIOD_1S                               1024
#define PERIOD_2S                               2048
#define PERIOD_4S                               4096

/* UART Tx & Rx Buffer Sizes */
#define UART_BUF_NUMBYTES_TX                    512
#define UART_BUF_NUMBYTES_RX                    256

typedef enum
{
    RTC_INTERRUPT_RATE_500MS = 0,
    RTC_INTERRUPT_RATE_1S = 1,
    RTC_INTERRUPT_RATE_2S = 2,
    RTC_INTERRUPT_RATE_4S = 3,
} RTC_INTERRUPT_RATE;

/* Standard identifier id[28:18]*/
#define WRITE_ID(id) (id << 18)
#define READ_ID(id)  (id >> 18)

/* Application's state machine enum */
typedef enum
{
    APP_CAN_STATE_RECEIVE,
    APP_CAN_STATE_TRANSMIT,
    APP_CAN_STATE_IDLE,
    APP_CAN_STATE_XFER_SUCCESSFUL,
    APP_CAN_STATE_XFER_ERROR,
    APP_CAN_STATE_USER_INPUT
} APP_CAN_STATES;

static RTC_INTERRUPT_RATE tempSampleRate = RTC_INTERRUPT_RATE_500MS;
static const char timeouts[4][20] = {"500 milliSeconds", "1 second",  "2 seconds",  "4 seconds"};

static uint8_t uartTxBuffer[UART_BUF_NUMBYTES_TX] = {0};
static uint8_t uartRxBuffer[UART_BUF_NUMBYTES_RX] = {0};

static volatile bool isRTCExpired = false;
static volatile bool changeTempSamplingRate = false;
static volatile bool isUSART0TxComplete = true;
static volatile bool isUSART5TxComplete = true;
static volatile bool APP_CAN_rxFifo0Callback = false;
static volatile bool APP_CAN_rxFifo1Callback = false;
static volatile bool APP_CAN_rxBufferCallback = false;

/* Variable to save Tx/Rx transfer status and context */
static uint32_t status = 0;
static uint32_t xferContext = 0;
/* Variable to save Tx/Rx message */
static uint8_t loop_count = 0;
//static uint8_t user_input = 0;
/* Variable to save application state */
volatile static APP_CAN_STATES state = APP_CAN_STATE_USER_INPUT;
uint8_t APP_CAN_numberOfMessage;

static uint8_t txFiFo[CAN1_TX_FIFO_BUFFER_SIZE];
static uint8_t rxFiFo0[CAN1_RX_FIFO0_SIZE];
static uint8_t rxFiFo1[CAN1_RX_FIFO1_SIZE];
static uint8_t rxBuffer[CAN1_RX_BUFFER_SIZE];

uint8_t Can1MessageRAM[CAN1_MESSAGE_RAM_CONFIG_SIZE] __attribute__((aligned (32)));
CAN_TX_BUFFER *txBuffer = NULL;

#define CAN_STD_FILTER_ID_MIN 0x0UL
#define CAN_STD_FILTER_ID_MAX (CAN_SIDFE_0_SFT(0UL)|CAN_SIDFE_0_SFID1(0x0UL)|CAN_SIDFE_0_SFID2(0x7ffUL)|CAN_SIDFE_0_SFEC(1UL))
#define CAN_EXT_FILTER_ID_MIN (CAN_XIDFE_0_EFID1(0x0UL)|CAN_XIDFE_0_EFEC(2UL))
#define CAN_EXT_FILTER_ID_MAX (CAN_XIDFE_1_EFID2(0x1fffffffUL)|CAN_XIDFE_1_EFT(0UL))

// *****************************************************************************
// *****************************************************************************
// Section: Interrupt Service Routines
// *****************************************************************************
// *****************************************************************************

static void EIC_User_Handler(uintptr_t context)
{
    changeTempSamplingRate = true;
}

static void rtcEventHandler (RTC_TIMER32_INT_MASK intCause, uintptr_t context)
{
    if (intCause & RTC_MODE0_INTENSET_CMP0_Msk)
    {            
        isRTCExpired = true;
    }
}

static void usart0DmaChannelHandler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle)
{
    if (event == DMAC_TRANSFER_EVENT_COMPLETE)
    {
        isUSART0TxComplete = true;
    }
}

static void usart5DmaChannelHandler(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle)
{
    if (event == DMAC_TRANSFER_EVENT_COMPLETE)
    {
        isUSART5TxComplete = true;
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Debugger terminal functions
// Important: Do not call these functions during interrupt handling
// *****************************************************************************
// *****************************************************************************

void DEBUG_OUTPUT(char *buffer, char *mesg)
{
    isUSART5TxComplete = false;
    sprintf(buffer, mesg);
    DMAC_ChannelTransfer(DMAC_CHANNEL_0, buffer, \
            (const void *)&(SERCOM5_REGS->USART_INT.SERCOM_DATA), \
            strlen((const char*)buffer));
    while (isUSART5TxComplete == false);
}

void DEBUG_OUTPUT2(char *buffer)
{
    isUSART5TxComplete = false;
    DMAC_ChannelTransfer(DMAC_CHANNEL_0, buffer, \
            (const void *)&(SERCOM5_REGS->USART_INT.SERCOM_DATA), \
            strlen((const char*)buffer));
    while (isUSART5TxComplete == false);
}

void DEBUG_OUTPUT3(char *mesg)
{
    isUSART5TxComplete = false;
    DMAC_ChannelTransfer(DMAC_CHANNEL_0, mesg, \
            (const void *)&(SERCOM5_REGS->USART_INT.SERCOM_DATA), \
            strlen((const char*)mesg));
    while (isUSART5TxComplete == false);
}

// *****************************************************************************
// *****************************************************************************
// Section: BLE transmit functions
// Important: Do not call these functions during interrupt handling
// *****************************************************************************
// *****************************************************************************

void BLE_OUTPUT(char *buffer, char *mesg)
{
    isUSART0TxComplete = false;
    sprintf(buffer, mesg);
    DMAC_ChannelTransfer(DMAC_CHANNEL_1, buffer, \
            (const void *)&(SERCOM0_REGS->USART_INT.SERCOM_DATA), \
            strlen((const char*)buffer));
    while (isUSART0TxComplete == false);
}

void BLE_OUTPUT2(char *buffer)
{
    isUSART0TxComplete = false;
    DMAC_ChannelTransfer(DMAC_CHANNEL_1, buffer, \
            (const void *)&(SERCOM0_REGS->USART_INT.SERCOM_DATA), \
            strlen((const char*)buffer));
    while (isUSART0TxComplete == false);
}

void BLE_OUTPUT3(char *mesg)
{
    isUSART0TxComplete = false;
    DMAC_ChannelTransfer(DMAC_CHANNEL_1, mesg, \
            (const void *)&(SERCOM0_REGS->USART_INT.SERCOM_DATA), \
            strlen((const char*)mesg));
    while (isUSART0TxComplete == false);
}

// *****************************************************************************
// *****************************************************************************
// Section: Application functions
// *****************************************************************************
// *****************************************************************************

/* Message Length to Data length code */
static uint8_t CANLengthToDlcGet(uint8_t length)
{
    uint8_t dlc = 0;

    if (length <= 8U)
    {
        dlc = length;
    }
    else if (length <= 12U)
    {
        dlc = 0x9U;
    }
    else if (length <= 16U)
    {
        dlc = 0xAU;
    }
    else if (length <= 20U)
    {
        dlc = 0xBU;
    }
    else if (length <= 24U)
    {
        dlc = 0xCU;
    }
    else if (length <= 32U)
    {
        dlc = 0xDU;
    }
    else if (length <= 48U)
    {
        dlc = 0xEU;
    }
    else
    {
        dlc = 0xFU;
    }
    return dlc;
}

/* Data length code to Message Length */
static uint8_t CANDlcToLengthGet(uint8_t dlc)
{
    uint8_t msgLength[] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};
    return msgLength[dlc];
}

static void APP_CAN_menu(void)
{   
	DEBUG_OUTPUT3("\r\n\r\n[CAN] Demo Menu Options :\r\n"
	       "  --> Enter a key to select one of the following actions:\r\n"
	       "  [1] Send FD standard message with ID: 0x45A and 64 byte data 0 to 63 \r\n"
	       "  [2] Send FD standard message with ID: 0x469 and 64 byte data 128 to 191 \r\n"
	       "  [3] Send FD extended message with ID: 0x100000A5 and 64 byte data 0 to 63 \r\n"
	       "  [4] Send FD extended message with ID: 0x10000096 and 64 byte data 128 to 191 \r\n"
	       "  [5] Send normal standard message with ID: 0x469 and 8 byte data 0 to 7 \r\n"
	       "  [M/m] Display options in this menu \r\n"
	       "  [R/r] Reset MCU \r\n\r\n");
}

/* Print Rx message received by the CAN controller */
static void APP_CAN_outputMessage(uint8_t numberOfMessage, CAN_RX_BUFFER *rxBuf, uint8_t rxBufLen, uint8_t rxFifoBuf)
{
    uint8_t length = 0;
    uint8_t msgLength = 0;
    uint32_t id = 0;

    if (rxFifoBuf == 0)
    {
        sprintf((char*)uartTxBuffer, "[CAN] Rx FIFO0 (Standard Frames) >");
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);
    }
    else if (rxFifoBuf == 1)
    {
        sprintf((char*)uartTxBuffer, "[CAN] Rx FIFO1 (Extended Frames) >");
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);
    }
    else if (rxFifoBuf == 2)
    {
        sprintf((char*)uartTxBuffer, "[CAN] Rx Buffer >");
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);
    }
    
    for (uint8_t count = 0; count < numberOfMessage; count++)
    {
        /* Print message to Console */
        sprintf((char*)uartTxBuffer, " New Message Received: ");
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);
        id = rxBuf->xtd ? rxBuf->id : READ_ID(rxBuf->id);
        msgLength = CANDlcToLengthGet(rxBuf->dlc);
        length = msgLength;
        sprintf((char*)uartTxBuffer, "[ Timestamp = 0x%x | ID = 0x%x | Length = %d | Data : ", (unsigned int)rxBuf->rxts, (unsigned int)id, (unsigned int)msgLength);
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);
        while(length)
        {
            sprintf((char*)uartTxBuffer, "0x%x ", rxBuf->data[msgLength - length--]);
            DEBUG_OUTPUT2((char*)uartTxBuffer);
            BLE_OUTPUT2((char*)uartTxBuffer);
        }
        sprintf((char*)uartTxBuffer, " ]\r\n");
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);
        rxBuf += rxBufLen;
    }
}

/* This function will be called by CAN PLIB when transfer is completed from Tx FIFO */
void APP_CAN_TxFifoCallback(uintptr_t context)
{
    xferContext = context;

    /* Check CAN Status */
    status = CAN1_ErrorGet();

    if (((status & CAN_PSR_LEC_Msk) == CAN_ERROR_NONE) || ((status & CAN_PSR_LEC_Msk) == CAN_ERROR_LEC_NC))
    {
        switch ((APP_CAN_STATES)context)
        {
            case APP_CAN_STATE_TRANSMIT:
            {
                state = APP_CAN_STATE_XFER_SUCCESSFUL;
                break;
            }
            default:
                break;
        }
    }
    else
    {
        state = APP_CAN_STATE_XFER_ERROR;
    }
}

void APP_CAN_RxBufferCallback(uint8_t numberOfMessage, uintptr_t context)
{
    APP_CAN_numberOfMessage = numberOfMessage;
    xferContext = context;
    APP_CAN_rxBufferCallback = true;
}

/* This function will be called by CAN PLIB when Message received in Rx Buffer */
void APP_CAN_RxBufferProcess(uint8_t bufferNumber, uintptr_t context)
{
    xferContext = context;

    /* Check CAN Status */
    status = CAN1_ErrorGet();

    if (((status & CAN_PSR_LEC_Msk) == CAN_ERROR_NONE) || ((status & CAN_PSR_LEC_Msk) == CAN_ERROR_LEC_NC))
    {
        switch ((APP_CAN_STATES)context)
        {
            case APP_CAN_STATE_RECEIVE:
            {
                memset(rxBuffer, 0x00, CAN1_RX_BUFFER_ELEMENT_SIZE);
                if (CAN1_MessageReceive(bufferNumber, (CAN_RX_BUFFER *)rxBuffer) == true)
                {
                    APP_CAN_outputMessage(1, (CAN_RX_BUFFER *)rxBuffer, CAN1_RX_BUFFER_ELEMENT_SIZE, 2);
                    state = APP_CAN_STATE_XFER_SUCCESSFUL;
                }
                else
                {
                    state = APP_CAN_STATE_XFER_ERROR;
                }
                break;
            }
            default:
                break;
        }
    }
    else
    {
        state = APP_CAN_STATE_XFER_ERROR;
    }
}

void APP_CAN_RxFifo0Callback(uint8_t numberOfMessage, uintptr_t context)
{
    APP_CAN_numberOfMessage = numberOfMessage;
    xferContext = context;
    APP_CAN_rxFifo0Callback = true;
}

/* This function will be called by CAN PLIB when Message received in Rx FIFO0 */
void APP_CAN_RxFifo0Process(uint8_t numberOfMessage, uintptr_t context)
{
    xferContext = context;

    /* Check CAN Status */
    status = CAN1_ErrorGet();

    if (((status & CAN_PSR_LEC_Msk) == CAN_ERROR_NONE) || ((status & CAN_PSR_LEC_Msk) == CAN_ERROR_LEC_NC))
    {
        switch ((APP_CAN_STATES)context)
        {
            case APP_CAN_STATE_RECEIVE:
            {
                memset(rxFiFo0, 0x00, (numberOfMessage * CAN1_RX_FIFO0_ELEMENT_SIZE));
                if (CAN1_MessageReceiveFifo(CAN_RX_FIFO_0, numberOfMessage, (CAN_RX_BUFFER *)rxFiFo0) == true)
                {
                    APP_CAN_outputMessage(numberOfMessage, (CAN_RX_BUFFER *)rxFiFo0, CAN1_RX_FIFO0_ELEMENT_SIZE, 0);
                    state = APP_CAN_STATE_XFER_SUCCESSFUL;
                }
                else
                {
                    state = APP_CAN_STATE_XFER_ERROR;
                }
                break;
            }
            default:
                break;
        }
    }
    else
    {
        state = APP_CAN_STATE_XFER_ERROR;
    }
}

void APP_CAN_RxFifo1Callback(uint8_t numberOfMessage, uintptr_t context)
{
    APP_CAN_numberOfMessage = numberOfMessage;
    xferContext = context;
    APP_CAN_rxFifo1Callback = true;
}

/* This function will be called by CAN PLIB when Message received in Rx FIFO1 */
void APP_CAN_RxFifo1Process(uint8_t numberOfMessage, uintptr_t context)
{
    xferContext = context;

    /* Check CAN Status */
    status = CAN1_ErrorGet();

    if (((status & CAN_PSR_LEC_Msk) == CAN_ERROR_NONE) || ((status & CAN_PSR_LEC_Msk) == CAN_ERROR_LEC_NC))
    {
        switch ((APP_CAN_STATES)context)
        {
            case APP_CAN_STATE_RECEIVE:
            {
                memset(rxFiFo1, 0x00, (numberOfMessage * CAN1_RX_FIFO1_ELEMENT_SIZE));
                if (CAN1_MessageReceiveFifo(CAN_RX_FIFO_1, numberOfMessage, (CAN_RX_BUFFER *)rxFiFo1) == true)
                {
                    APP_CAN_outputMessage(numberOfMessage, (CAN_RX_BUFFER *)rxFiFo1, CAN1_RX_FIFO1_ELEMENT_SIZE, 1);
                    state = APP_CAN_STATE_XFER_SUCCESSFUL;
                }
                else
                {
                    state = APP_CAN_STATE_XFER_ERROR;
                }
                break;
            }
            default:
                break;
        }
    }
    else
    {
        state = APP_CAN_STATE_XFER_ERROR;
    }
}

void APP_CAN_command(char user_input)
{       
    /* Check for user input on the CAN FD demo terminal window and process command */
    if (state == APP_CAN_STATE_USER_INPUT) {
        /* Read user input */
        //scanf("%c", (char *) &user_input);

        switch (user_input) {
            case '1':
                memset(txFiFo, 0x00, CAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *) txFiFo;
                txBuffer->id = WRITE_ID(0x45A);
                txBuffer->dlc = CANLengthToDlcGet(64);
                txBuffer->fdf = 1;
                txBuffer->brs = 1;
                for (loop_count = 0; loop_count < 64; loop_count++) {
                    txBuffer->data[loop_count] = loop_count;
                }
                DEBUG_OUTPUT3("\r\n[CAN] Send FD standard message with ID: 0x45A and 64 byte data 0 to 63.\r\n");
                CAN1_TxFifoCallbackRegister(APP_CAN_TxFifoCallback, (uintptr_t) APP_CAN_STATE_TRANSMIT);
                state = APP_CAN_STATE_IDLE;
                if (CAN1_MessageTransmitFifo(1, txBuffer) == false) {
                    DEBUG_OUTPUT3("[CAN] Message send failed!!! \r\n");
                }
                break;
            case '2':
                memset(txFiFo, 0x00, CAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *) txFiFo;
                txBuffer->id = WRITE_ID(0x469);
                txBuffer->dlc = CANLengthToDlcGet(64);
                txBuffer->fdf = 1;
                txBuffer->brs = 1;
                for (loop_count = 128; loop_count < 192; loop_count++) {
                    txBuffer->data[loop_count - 128] = loop_count;
                }
                DEBUG_OUTPUT3("\r\n[CAN] Send FD standard message with ID: 0x469 and 64 byte data 128 to 191.\r\n");
                CAN1_TxFifoCallbackRegister(APP_CAN_TxFifoCallback, (uintptr_t) APP_CAN_STATE_TRANSMIT);
                state = APP_CAN_STATE_IDLE;
                if (CAN1_MessageTransmitFifo(1, txBuffer) == false) {
                    DEBUG_OUTPUT3("[CAN] Message send failed!!! \r\n");
                }
                break;
            case '3':
                memset(txFiFo, 0x00, CAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *) txFiFo;
                txBuffer->id = 0x100000A5;
                txBuffer->dlc = CANLengthToDlcGet(64);
                txBuffer->xtd = 1;
                txBuffer->fdf = 1;
                txBuffer->brs = 1;
                for (loop_count = 0; loop_count < 64; loop_count++) {
                    txBuffer->data[loop_count] = loop_count;
                }
                DEBUG_OUTPUT3("\r\n[CAN] Send FD extended message with ID: 0x100000A5 and 64 byte data 0 to 63.\r\n");
                CAN1_TxFifoCallbackRegister(APP_CAN_TxFifoCallback, (uintptr_t) APP_CAN_STATE_TRANSMIT);
                state = APP_CAN_STATE_IDLE;
                if (CAN1_MessageTransmitFifo(1, txBuffer) == false) {
                    DEBUG_OUTPUT3("[CAN] Message send failed!!! \r\n");
                }
                break;
            case '4':
                memset(txFiFo, 0x00, CAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *) txFiFo;
                txBuffer->id = 0x10000096;
                txBuffer->dlc = CANLengthToDlcGet(64);
                txBuffer->xtd = 1;
                txBuffer->fdf = 1;
                txBuffer->brs = 1;
                for (loop_count = 128; loop_count < 192; loop_count++) {
                    txBuffer->data[loop_count - 128] = loop_count;
                }
                DEBUG_OUTPUT3("\r\n[CAN] Send FD extended message with ID: 0x10000096 and 64 byte data 128 to 191.\r\n");
                CAN1_TxFifoCallbackRegister(APP_CAN_TxFifoCallback, (uintptr_t) APP_CAN_STATE_TRANSMIT);
                state = APP_CAN_STATE_IDLE;
                if (CAN1_MessageTransmitFifo(1, txBuffer) == false) {
                    DEBUG_OUTPUT3("[CAN] Message send failed!!! \r\n");
                }
                break;
            case '5':
                memset(txFiFo, 0x00, CAN1_TX_FIFO_BUFFER_ELEMENT_SIZE);
                txBuffer = (CAN_TX_BUFFER *) txFiFo;
                txBuffer->id = WRITE_ID(0x469);
                txBuffer->dlc = 8;
                for (loop_count = 0; loop_count < 8; loop_count++) {
                    txBuffer->data[loop_count] = loop_count;
                }
                DEBUG_OUTPUT3("\r\n[CAN] Send normal standard message with ID: 0x469 and 8 byte data 0 to 7.\r\n");
                CAN1_TxFifoCallbackRegister(APP_CAN_TxFifoCallback, (uintptr_t) APP_CAN_STATE_TRANSMIT);
                state = APP_CAN_STATE_IDLE;
                if (CAN1_MessageTransmitFifo(1, txBuffer) == false) {
                    DEBUG_OUTPUT3("[CAN] Message send failed!!! \r\n");
                }
                break;
            case 'm': case 'M':
                APP_CAN_menu();
                break;
            case 'r': case 'R':
                NVIC_SystemReset();
                break;
            default:
                DEBUG_OUTPUT3("\r\n[***ERROR***] An invalid menu item was selected... \r\n");
                break;
        }
    }
}

void APP_CAN_state(void)
{
    /* Check the application's current state. */
    switch (state) {
        case APP_CAN_STATE_IDLE:
        {
            /* Application can do other task here */
            break;
        }
        case APP_CAN_STATE_XFER_SUCCESSFUL:
        {
            if ((APP_CAN_STATES) xferContext == APP_CAN_STATE_TRANSMIT) {
                DEBUG_OUTPUT3("[CAN] Message sent successfully!\r\n");
            }
            state = APP_CAN_STATE_USER_INPUT;
            break;
        }
        case APP_CAN_STATE_XFER_ERROR:
        {
            if ((APP_CAN_STATES) xferContext == APP_CAN_STATE_RECEIVE) {
                DEBUG_OUTPUT3("[CAN] Error in received message!\r\n");
            } else {
                DEBUG_OUTPUT3("[CAN] Message send failed!\r\n");
            }
            state = APP_CAN_STATE_USER_INPUT;
            break;
        }
        default:
        {
            break;
        }
    }
    
    if (APP_CAN_rxFifo0Callback == true)
    {
        APP_CAN_rxFifo0Callback = false;
        APP_CAN_RxFifo0Process(APP_CAN_numberOfMessage, xferContext);
    }
    if (APP_CAN_rxFifo1Callback == true)
    {
        APP_CAN_rxFifo1Callback = false;
        APP_CAN_RxFifo1Process(APP_CAN_numberOfMessage, xferContext);
    }
    if (APP_CAN_rxBufferCallback == true)
    {
        APP_CAN_rxBufferCallback = false;
        APP_CAN_RxBufferProcess(APP_CAN_numberOfMessage, xferContext);
    }
}

void APP_LED_toggle(void)
{
    if (isRTCExpired == true) {
        isRTCExpired = false;
        LED0_Toggle();       
    }
    /* Maintain state machines of all polled MPLAB Harmony modules */
    if (changeTempSamplingRate == true) {
        changeTempSamplingRate = false;
        if (tempSampleRate == RTC_INTERRUPT_RATE_500MS) {
            tempSampleRate = RTC_INTERRUPT_RATE_1S;
            RTC_Timer32Compare0Set(PERIOD_1S);
        } else if (tempSampleRate == RTC_INTERRUPT_RATE_1S) {
            tempSampleRate = RTC_INTERRUPT_RATE_2S;
            RTC_Timer32Compare0Set(PERIOD_2S);
        } else if (tempSampleRate == RTC_INTERRUPT_RATE_2S) {
            tempSampleRate = RTC_INTERRUPT_RATE_4S;
            RTC_Timer32Compare0Set(PERIOD_4S);
        }
        else if (tempSampleRate == RTC_INTERRUPT_RATE_4S) {
            tempSampleRate = RTC_INTERRUPT_RATE_500MS;
            RTC_Timer32Compare0Set(PERIOD_500MS);
        } else {
            ;
        }
        RTC_Timer32CounterSet(0);
        sprintf((char*)uartTxBuffer, "\r\n[SAM_E51_CNANO] LED0 toggle rate is now %s\r\n", &timeouts[(uint8_t) tempSampleRate][0]);
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        /* Send message to BLE module for Tx */
        //SERCOM0_USART_Write(uartTxBuffer, strlen((const char*) uartTxBuffer));
        BLE_OUTPUT2((char*)uartTxBuffer);
        
    }
}

void APP_BLE_demo(void)
{
    /* Check for user input typed in the debug terminal window */
    if (SERCOM5_USART_Read(uartRxBuffer, 1)) // USART read should be non-blocking
    {
        SERCOM0_USART_Write(uartRxBuffer, 1); // Send to BLE module for Tx
        APP_CAN_command(uartRxBuffer[0]); // See if need to execute CAN command
    }
    /* Check for incoming received character from the BLE module */
    if (SERCOM0_USART_Read(uartRxBuffer, 1)) // USART read should be non-blocking
    {
        SERCOM5_USART_Write(uartRxBuffer, 1); // Display received character on terminal
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    //can_sidfe_registers_t stdMsgIDFilterElement;
    //can_xidfe_registers_t extMsgIDFilterElement;

    /* Initialize all modules */
    SYS_Initialize ( NULL );

    DMAC_ChannelCallbackRegister(DMAC_CHANNEL_0, usart5DmaChannelHandler, 0);
    DMAC_ChannelCallbackRegister(DMAC_CHANNEL_1, usart0DmaChannelHandler, 0);
    EIC_CallbackRegister(EIC_PIN_15, EIC_User_Handler, 0);
    RTC_Timer32CallbackRegister(rtcEventHandler, 0);
    RTC_Timer32Start();
    
    /* Set CAN Message RAM Configuration */
    CAN1_MessageRAMConfigSet(Can1MessageRAM);

    CAN1_RxFifoCallbackRegister(CAN_RX_FIFO_0, APP_CAN_RxFifo0Callback, APP_CAN_STATE_RECEIVE);
    CAN1_RxFifoCallbackRegister(CAN_RX_FIFO_1, APP_CAN_RxFifo1Callback, APP_CAN_STATE_RECEIVE);
    CAN1_RxBuffersCallbackRegister(APP_CAN_RxBufferCallback, APP_CAN_STATE_RECEIVE);

    sprintf((char*)uartTxBuffer, "\r\n ------------------------------------------------ \r\n");
    DEBUG_OUTPUT2((char*)uartTxBuffer);
    BLE_OUTPUT2((char*)uartTxBuffer);
    sprintf((char*)uartTxBuffer, "     SAME51 CAN & BLE Demo               \r\n");
    DEBUG_OUTPUT2((char*)uartTxBuffer);
    BLE_OUTPUT2((char*)uartTxBuffer);
    sprintf((char*)uartTxBuffer, " ------------------------------------------------ \r\n\r\n");
    DEBUG_OUTPUT2((char*)uartTxBuffer);
    BLE_OUTPUT2((char*)uartTxBuffer);
#ifdef _ELIMINATE
    /* Set Filter Address Range for Standard Filter */
    stdMsgIDFilterElement.CAN_SIDFE_0 = CAN_STD_FILTER_ID_MAX;
    if (CAN1_StandardFilterElementSet(0, &stdMsgIDFilterElement) == false)
    {
        sprintf((char*)uartTxBuffer, "[CAN] Failed to set standard filter ID range!!!\r\n");
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);        
    }
    if (CAN1_StandardFilterElementGet(0, &stdMsgIDFilterElement) == true)
    {
        sprintf((char*)uartTxBuffer, "[CAN] Standard Filter ID Range  = 0x0 to 0x%x\r\n", 
                (unsigned int)stdMsgIDFilterElement.CAN_SIDFE_0);
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);
    }

    /* Set Filter Address Range for Extended Filter */
    extMsgIDFilterElement.CAN_XIDFE_0 = CAN_EXT_FILTER_ID_MIN;
    extMsgIDFilterElement.CAN_XIDFE_1 = CAN_EXT_FILTER_ID_MAX;
    if (CAN1_ExtendedFilterElementSet(0, &extMsgIDFilterElement) == false)
    {
        sprintf((char*)uartTxBuffer, "[CAN] Failed to set extended filter ID range!!!\r\n");
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);        
    }
    if (CAN1_ExtendedFilterElementGet(0, &extMsgIDFilterElement) == true)
    {
        sprintf((char*)uartTxBuffer, "[CAN] Extended Filter ID Range = 0x%x to 0x%x\r\n", 
                (unsigned int)extMsgIDFilterElement.CAN_XIDFE_0, (unsigned int)extMsgIDFilterElement.CAN_XIDFE_1);
        DEBUG_OUTPUT2((char*)uartTxBuffer);
        BLE_OUTPUT2((char*)uartTxBuffer);
    }
#endif
    APP_CAN_menu();
    
    while ( true )
    {
        /* Update CAN demo state machine */
        APP_CAN_state();
        /* Check if BLE demo needs to be serviced */
        APP_BLE_demo();
        /* Check if LED needs to be toggled */
        APP_LED_toggle();
    }
            
    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

