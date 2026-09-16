;
;/**
; * @file       startup_apm32f105xc.s
; *
; * @brief      CMSIS Cortex-M3 based Core Device Startup File for Device startup_apm32f105xc.s
; *
; * @version    V1.0.0
; *
; * @date       2026-03-01
; *
; * @attention
; *
; *  Copyright (C) 2026 Geehy Semiconductor
; *
; *  You may not use this file except in compliance with the
; *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
; *
; *  The program is only for reference, which is distributed in the hope
; *  that it will be useful and instructional for customers to develop
; *  their software. Unless required by applicable law or agreed to in
; *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
; *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
; *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
; *  and limitations under the License.
; */


        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)

        SECTION .intvec:CODE:NOROOT(2)

        EXTERN  __iar_program_start
        EXTERN  SystemInit
        PUBLIC  __vector_table

        DATA
__vector_table
        DCD     sfe(CSTACK)
        DCD     Reset_Handler                     ; Reset Handler
        DCD     NMI_Handler                       ; < Non Maskable Interrupt
        DCD     HardFault_Handler                 ; < Hard Fault, all classes of Fault
        DCD     MemManage_Handler                 ; < Memory Management Interrupt
        DCD     BusFault_Handler                  ; < Bus Fault Interrupt
        DCD     UsageFault_Handler                ; < Usage Fault Interrupt
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     SVC_Handler                       ; < SV Call Interrupt
        DCD     DebugMon_Handler                  ; < Debug Monitor Interrupt
        DCD     0                                 ; Reserved
        DCD     PendSV_Handler                    ; < Pend SV Interrupt
        DCD     SysTick_Handler                   ; < System Tick Interrupt

        ; External Interrupts
        DCD     WWDT_IRQHandler                   ; < Window WatchDog Interrupt
        DCD     PVD_IRQHandler                    ; < PVD through EINT Line detection Interrupt
        DCD     TAMPER_IRQHandler                 ; < Tamper Interrupt
        DCD     RTC_IRQHandler                    ; < RTC global Interrupt
        DCD     FLASH_IRQHandler                  ; < FLASH global Interrupt
        DCD     RCM_IRQHandler                    ; < RCM global Interrupt
        DCD     EINT0_IRQHandler                  ; < EINT Line0 Interrupt
        DCD     EINT1_IRQHandler                  ; < EINT Line1 Interrupt
        DCD     EINT2_IRQHandler                  ; < EINT Line2 Interrupt
        DCD     EINT3_IRQHandler                  ; < EINT Line3 Interrupt
        DCD     EINT4_IRQHandler                  ; < EINT Line4 Interrupt
        DCD     DMA1_Channel1_IRQHandler          ; < DMA1 Channel 1 global Interrupt
        DCD     DMA1_Channel2_IRQHandler          ; < DMA1 Channel 2 global Interrupt
        DCD     DMA1_Channel3_IRQHandler          ; < DMA1 Channel 3 global Interrupt
        DCD     DMA1_Channel4_IRQHandler          ; < DMA1 Channel 4 global Interrupt
        DCD     DMA1_Channel5_IRQHandler          ; < DMA1 Channel 5 global Interrupt
        DCD     DMA1_Channel6_IRQHandler          ; < DMA1 Channel 6 global Interrupt
        DCD     DMA1_Channel7_IRQHandler          ; < DMA1 Channel 7 global Interrupt
        DCD     ADC1_2_IRQHandler                 ; < ADC1 and ADC2 global Interrupt
        DCD     CAN1_TX_IRQHandler                ; < CAN1 TX Interrupts
        DCD     CAN1_RX0_IRQHandler               ; < CAN1 RX0 Interrupts
        DCD     CAN1_RX1_IRQHandler               ; < CAN1 RX1 Interrupt
        DCD     CAN1_SCE_IRQHandler               ; < CAN1 SCE Interrupt
        DCD     EINT9_5_IRQHandler                ; < External Line[9:5] Interrupts
        DCD     TMR1_BRK_IRQHandler               ; < TMR1 Break Interrupt
        DCD     TMR1_UP_IRQHandler                ; < TMR1 Update Interrupt
        DCD     TMR1_TRG_COM_IRQHandler           ; < TMR1 Trigger and Commutation Interrupt
        DCD     TMR1_CC_IRQHandler                ; < TMR1 Capture Compare Interrupt
        DCD     TMR2_IRQHandler                   ; < TMR2 global Interrupt
        DCD     TMR3_IRQHandler                   ; < TMR3 global Interrupt
        DCD     TMR4_IRQHandler                   ; < TMR4 global Interrupt
        DCD     I2C1_EV_IRQHandler                ; < I2C1 Event Interrupt
        DCD     I2C1_ER_IRQHandler                ; < I2C1 Error Interrupt
        DCD     I2C2_EV_IRQHandler                ; < I2C2 Event Interrupt
        DCD     I2C2_ER_IRQHandler                ; < I2C2 Error Interrupt
        DCD     SPI1_IRQHandler                   ; < SPI1 global Interrupt
        DCD     SPI2_IRQHandler                   ; < SPI2 global Interrupt
        DCD     USART1_IRQHandler                 ; < USART1 global Interrupt
        DCD     USART2_IRQHandler                 ; < USART2 global Interrupt
        DCD     USART3_IRQHandler                 ; < USART3 global Interrupt
        DCD     EINT15_10_IRQHandler              ; < External Line[15:10] Interrupts
        DCD     RTCAlarm_IRQHandler               ; < RTC Alarm through EINT Line Interrupt
        DCD     OTG_FS_WKUP_IRQHandler            ; < USB Device WakeUp from suspend through EINT Line Interrupt
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     TMR5_IRQHandler                   ; < TMR5 global Interrupt
        DCD     SPI3_IRQHandler                   ; < SPI3 global Interrupt
        DCD     UART4_IRQHandler                  ; < UART4 global Interrupt
        DCD     UART5_IRQHandler                  ; < UART5 global Interrupt
        DCD     TMR6_IRQHandler                   ; < TMR6 global Interrupt
        DCD     TMR7_IRQHandler                   ; < TMR7 global Interrupt
        DCD     DMA2_Channel1_IRQHandler          ; < DMA2 Channel 1 global Interrupt
        DCD     DMA2_Channel2_IRQHandler          ; < DMA2 Channel 2 global Interrupt
        DCD     DMA2_Channel3_IRQHandler          ; < DMA2 Channel 3 global Interrupt
        DCD     DMA2_Channel4_IRQHandler          ; < DMA2 Channel 4 global Interrupt
        DCD     DMA2_Channel5_IRQHandler          ; < DMA2 Channel 5 global Interrupt
        DCD     0                                 ; Reserved
        DCD     0                                 ; Reserved
        DCD     CAN2_TX_IRQHandler                ; < CAN2 TX Interrupts
        DCD     CAN2_RX0_IRQHandler               ; < CAN2 RX0 Interrupts
        DCD     CAN2_RX1_IRQHandler               ; < CAN2 RX1 Interrupts
        DCD     CAN2_SCE_IRQHandler               ; < CAN2 SCE Interrupts
        DCD     OTG_FS_IRQHandler                 ; < OTG FS Interrupts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
        THUMB
        PUBWEAK Reset_Handler
        SECTION .text:CODE:REORDER:NOROOT(2)
Reset_Handler

        LDR     R0, =SystemInit
        BLX     R0
        LDR     R0, =__iar_program_start
        BX      R0

        PUBWEAK NMI_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
NMI_Handler
        B       NMI_Handler

        PUBWEAK HardFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
HardFault_Handler
        B       HardFault_Handler

        PUBWEAK MemManage_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
MemManage_Handler
        B       MemManage_Handler

        PUBWEAK BusFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
BusFault_Handler
        B       BusFault_Handler

        PUBWEAK UsageFault_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
UsageFault_Handler
        B       UsageFault_Handler

        PUBWEAK SVC_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SVC_Handler
        B       SVC_Handler

        PUBWEAK DebugMon_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
DebugMon_Handler
        B       DebugMon_Handler

        PUBWEAK PendSV_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
PendSV_Handler
        B       PendSV_Handler

        PUBWEAK SysTick_Handler
        SECTION .text:CODE:REORDER:NOROOT(1)
SysTick_Handler
        B       SysTick_Handler

        PUBWEAK WWDT_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
WWDT_IRQHandler
        B       WWDT_IRQHandler

        PUBWEAK PVD_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
PVD_IRQHandler
        B       PVD_IRQHandler

        PUBWEAK TAMPER_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TAMPER_IRQHandler
        B       TAMPER_IRQHandler

        PUBWEAK RTC_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
RTC_IRQHandler
        B       RTC_IRQHandler

        PUBWEAK FLASH_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
FLASH_IRQHandler
        B       FLASH_IRQHandler

        PUBWEAK RCM_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
RCM_IRQHandler
        B       RCM_IRQHandler

        PUBWEAK EINT0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EINT0_IRQHandler
        B       EINT0_IRQHandler

        PUBWEAK EINT1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EINT1_IRQHandler
        B       EINT1_IRQHandler

        PUBWEAK EINT2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EINT2_IRQHandler
        B       EINT2_IRQHandler

        PUBWEAK EINT3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EINT3_IRQHandler
        B       EINT3_IRQHandler

        PUBWEAK EINT4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EINT4_IRQHandler
        B       EINT4_IRQHandler

        PUBWEAK DMA1_Channel1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA1_Channel1_IRQHandler
        B       DMA1_Channel1_IRQHandler

        PUBWEAK DMA1_Channel2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA1_Channel2_IRQHandler
        B       DMA1_Channel2_IRQHandler

        PUBWEAK DMA1_Channel3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA1_Channel3_IRQHandler
        B       DMA1_Channel3_IRQHandler

        PUBWEAK DMA1_Channel4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA1_Channel4_IRQHandler
        B       DMA1_Channel4_IRQHandler

        PUBWEAK DMA1_Channel5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA1_Channel5_IRQHandler
        B       DMA1_Channel5_IRQHandler

        PUBWEAK DMA1_Channel6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA1_Channel6_IRQHandler
        B       DMA1_Channel6_IRQHandler

        PUBWEAK DMA1_Channel7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA1_Channel7_IRQHandler
        B       DMA1_Channel7_IRQHandler

        PUBWEAK ADC1_2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
ADC1_2_IRQHandler
        B       ADC1_2_IRQHandler

        PUBWEAK CAN1_TX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CAN1_TX_IRQHandler
        B       CAN1_TX_IRQHandler

        PUBWEAK CAN1_RX0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CAN1_RX0_IRQHandler
        B       CAN1_RX0_IRQHandler

        PUBWEAK CAN1_RX1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CAN1_RX1_IRQHandler
        B       CAN1_RX1_IRQHandler

        PUBWEAK CAN1_SCE_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CAN1_SCE_IRQHandler
        B       CAN1_SCE_IRQHandler

        PUBWEAK EINT9_5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EINT9_5_IRQHandler
        B       EINT9_5_IRQHandler

        PUBWEAK TMR1_BRK_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR1_BRK_IRQHandler
        B       TMR1_BRK_IRQHandler

        PUBWEAK TMR1_UP_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR1_UP_IRQHandler
        B       TMR1_UP_IRQHandler

        PUBWEAK TMR1_TRG_COM_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR1_TRG_COM_IRQHandler
        B       TMR1_TRG_COM_IRQHandler

        PUBWEAK TMR1_CC_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR1_CC_IRQHandler
        B       TMR1_CC_IRQHandler

        PUBWEAK TMR2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR2_IRQHandler
        B       TMR2_IRQHandler

        PUBWEAK TMR3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR3_IRQHandler
        B       TMR3_IRQHandler

        PUBWEAK TMR4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR4_IRQHandler
        B       TMR4_IRQHandler

        PUBWEAK I2C1_EV_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
I2C1_EV_IRQHandler
        B       I2C1_EV_IRQHandler

        PUBWEAK I2C1_ER_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
I2C1_ER_IRQHandler
        B       I2C1_ER_IRQHandler

        PUBWEAK I2C2_EV_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
I2C2_EV_IRQHandler
        B       I2C2_EV_IRQHandler

        PUBWEAK I2C2_ER_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
I2C2_ER_IRQHandler
        B       I2C2_ER_IRQHandler

        PUBWEAK SPI1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
SPI1_IRQHandler
        B       SPI1_IRQHandler

        PUBWEAK SPI2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
SPI2_IRQHandler
        B       SPI2_IRQHandler

        PUBWEAK USART1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART1_IRQHandler
        B       USART1_IRQHandler

        PUBWEAK USART2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART2_IRQHandler
        B       USART2_IRQHandler

        PUBWEAK USART3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
USART3_IRQHandler
        B       USART3_IRQHandler

        PUBWEAK EINT15_10_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
EINT15_10_IRQHandler
        B       EINT15_10_IRQHandler

        PUBWEAK RTCAlarm_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
RTCAlarm_IRQHandler
        B       RTCAlarm_IRQHandler

        PUBWEAK OTG_FS_WKUP_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
OTG_FS_WKUP_IRQHandler
        B       OTG_FS_WKUP_IRQHandler

        PUBWEAK TMR5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR5_IRQHandler
        B       TMR5_IRQHandler

        PUBWEAK SPI3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
SPI3_IRQHandler
        B       SPI3_IRQHandler

        PUBWEAK UART4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
UART4_IRQHandler
        B       UART4_IRQHandler

        PUBWEAK UART5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
UART5_IRQHandler
        B       UART5_IRQHandler

        PUBWEAK TMR6_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR6_IRQHandler
        B       TMR6_IRQHandler

        PUBWEAK TMR7_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
TMR7_IRQHandler
        B       TMR7_IRQHandler

        PUBWEAK DMA2_Channel1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA2_Channel1_IRQHandler
        B       DMA2_Channel1_IRQHandler

        PUBWEAK DMA2_Channel2_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA2_Channel2_IRQHandler
        B       DMA2_Channel2_IRQHandler

        PUBWEAK DMA2_Channel3_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA2_Channel3_IRQHandler
        B       DMA2_Channel3_IRQHandler

        PUBWEAK DMA2_Channel4_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA2_Channel4_IRQHandler
        B       DMA2_Channel4_IRQHandler

        PUBWEAK DMA2_Channel5_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
DMA2_Channel5_IRQHandler
        B       DMA2_Channel5_IRQHandler

        PUBWEAK CAN2_TX_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CAN2_TX_IRQHandler
        B       CAN2_TX_IRQHandler

        PUBWEAK CAN2_RX0_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CAN2_RX0_IRQHandler
        B       CAN2_RX0_IRQHandler

        PUBWEAK CAN2_RX1_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CAN2_RX1_IRQHandler
        B       CAN2_RX1_IRQHandler

        PUBWEAK CAN2_SCE_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
CAN2_SCE_IRQHandler
        B       CAN2_SCE_IRQHandler

        PUBWEAK OTG_FS_IRQHandler
        SECTION .text:CODE:REORDER:NOROOT(1)
OTG_FS_IRQHandler
        B       OTG_FS_IRQHandler

        END