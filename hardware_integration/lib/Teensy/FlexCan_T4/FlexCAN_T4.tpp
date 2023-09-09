/*
  MIT License

  Copyright (c) 2018 Antonio Alexander Brewer (tonton81) - https://github.com/tonton81

  Designed and tested for PJRC Teensy 4.0.

  Forum link : https://forum.pjrc.com/threads/56035-FlexCAN_T4-FlexCAN-for-Teensy-4?highlight=flexcan_t4

  Thanks goes to skpang, mjs513, and collin for tech/testing support

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#pragma once
#include <FlexCAN_T4.hpp>
#include "imxrt_flexcan.hpp"
#include "Arduino.h"

#if defined(__IMXRT1062__)
static void flexcan_isr_can3();
static void flexcan_isr_can2();
static void flexcan_isr_can1();
#endif
#if defined(__MK20DX256__) || defined(__MK64FX512__)
static void flexcan_isr_can0();
#endif
#if defined(__MK66FX1M0__)
static void flexcan_isr_can0();
static void flexcan_isr_can1();
#endif

FCTP_FUNC FCTP_OPT::FlexCAN_T4() {
#if defined(__IMXRT1062__)
  if ( _bus == CAN3 ) _CAN3 = this;
  if ( _bus == CAN2 ) _CAN2 = this;
  if ( _bus == CAN1 ) _CAN1 = this;
#endif
#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
  if ( _bus == CAN1 ) _CAN1 = this;
  if ( _bus == CAN0 ) _CAN0 = this;
#endif

#if defined(__MK20DX256__) || defined(__MK64FX512__)
  static_assert(_bus == CAN0, "Only CAN0 works on Teensy 3.2/3.5");
#endif
#if defined(__MK66FX1M0__)
  static_assert(_bus == CAN0 || _bus == CAN1, "Only CAN0 & CAN1 works on Teensy 3.6");
#endif
#if defined(__IMXRT1062__)
  static_assert(_bus == CAN1 || _bus == CAN2 || _bus == CAN3, "Only CAN1 & CAN2 & CAN3 works on Teensy 4.0/4.1");
#endif

}

#if defined(__IMXRT1062__)
FCTP_FUNC void FCTP_OPT::setClock(FLEXCAN_CLOCK clock) {
  if ( clock == CLK_OFF ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(3) | CCM_CSCMR2_CAN_CLK_PODF(0);
  if ( clock == CLK_8MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(9);
  if ( clock == CLK_16MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(4);
  if ( clock == CLK_24MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(1) | CCM_CSCMR2_CAN_CLK_PODF(0);
  if ( clock == CLK_20MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(3);
  if ( clock == CLK_30MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(0) | CCM_CSCMR2_CAN_CLK_PODF(1);
  if ( clock == CLK_40MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(1);
  if ( clock == CLK_60MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(0) | CCM_CSCMR2_CAN_CLK_PODF(0);
  if ( clock == CLK_80MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(0);

  if ( _CAN1 ) _CAN1->setBaudRate(currentBitrate, (( FLEXCANb_CTRL1(_bus) & FLEXCAN_CTRL_LOM ) ? LISTEN_ONLY : TX));
  if ( _CAN2 ) _CAN2->setBaudRate(currentBitrate, (( FLEXCANb_CTRL1(_bus) & FLEXCAN_CTRL_LOM ) ? LISTEN_ONLY : TX));
  if ( _CAN3 ) _CAN3->setBaudRate(currentBitrate, (( FLEXCANb_CTRL1(_bus) & FLEXCAN_CTRL_LOM ) ? LISTEN_ONLY : TX));
}

FCTP_FUNC uint32_t FCTP_OPT::getClock() {
  const uint8_t clocksrc[4] = {60, 24, 80, 0};
  return clocksrc[(CCM_CSCMR2 & 0x300) >> 8];
}
#endif

FCTP_FUNC void FCTP_OPT::begin() {

  for (uint8_t i = 0; i < SIZE_LISTENERS; i++) listener[i] = nullptr;

#if defined(__IMXRT1062__)
  if ( !getClock() ) setClock(CLK_24MHz); /* no clock enabled, enable osc clock */

  if ( _bus == CAN3 ) {
    nvicIrq = IRQ_CAN3;
    _VectorsRam[16 + nvicIrq] = flexcan_isr_can3;
    CCM_CCGR7 |= 0x3C0;
    busNumber = 3;
  }
  if ( _bus == CAN2 ) {
    nvicIrq = IRQ_CAN2;
    _VectorsRam[16 + nvicIrq] = flexcan_isr_can2;
    CCM_CCGR0 |= 0x3C0000;
    busNumber = 2;
  }
  if ( _bus == CAN1 ) {
    nvicIrq = IRQ_CAN1;
    _VectorsRam[16 + nvicIrq] = flexcan_isr_can1;
    CCM_CCGR0 |= 0x3C000;
    busNumber = 1;
  }
#endif



#if defined(__MK20DX256__)
  if ( _bus == CAN0 ) {
    nvicIrq = IRQ_CAN_MESSAGE;
    _VectorsRam[16 + nvicIrq] = flexcan_isr_can0;
    busNumber = 0;
  }
#endif

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
  if ( _bus == CAN0 ) {
    nvicIrq = IRQ_CAN0_MESSAGE;
    _VectorsRam[16 + nvicIrq] = flexcan_isr_can0;
    busNumber = 0;
  }
#endif

#if defined(__MK66FX1M0__)
  else if ( _bus == CAN1 ) {
    nvicIrq = IRQ_CAN1_MESSAGE;
    _VectorsRam[16 + nvicIrq] = flexcan_isr_can1;
    busNumber = 1;
  }
#endif

#if defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
  OSC0_CR |= OSC_ERCLKEN;
  if ( _bus == CAN0 ) SIM_SCGC6 |= SIM_SCGC6_FLEXCAN0;
#if defined(__MK66FX1M0__)
  else if ( _bus == CAN1 ) SIM_SCGC3 |= SIM_SCGC3_FLEXCAN1;
#endif
  FLEXCANb_CTRL1(_bus) &= ~FLEXCAN_CTRL_CLK_SRC;
#endif

  setTX(); setRX();

  FLEXCANb_MCR(_bus) &= ~FLEXCAN_MCR_MDIS; /* enable module */
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_CTRL1(_bus) |= FLEXCAN_CTRL_LOM; /* listen only mode */
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_FRZ; /* enable freeze bit */
  while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_LPM_ACK);
  softReset(); /* reset bus */
  while (!(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK));
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_SRX_DIS; /* Disable self-reception */
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_IRMQ; // individual mailbox masking
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_AEN; // TX ABORT FEATURE
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_LPRIO_EN; // TX PRIORITY FEATURE
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_SLF_WAK; // SELF-WAKE UP FEATURE	
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_WAK_SRC; // WAKE-UP LOW-PASS FILTER
  FLEXCANb_MCR(_bus) &= ~0x8800; // disable DMA and FD (valid bits are reserved in legacy controllers)
  FLEXCANb_CTRL2(_bus) |= FLEXCAN_CTRL2_RRS | // store remote frames
                                  FLEXCAN_CTRL2_EACEN | /* handles the way filtering works. Library adjusts to whether you use this or not */ 
                                  FLEXCAN_CTRL2_MRP; // mailbox > FIFO priority.
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_WRN_EN;
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_WAK_MSK;

  disableFIFO(); /* clears all data and layout to legacy mailbox mode */
  FLEXCAN_ExitFreezeMode();
  NVIC_ENABLE_IRQ(nvicIrq);
}

FCTP_FUNC void FCTP_OPT::enableFIFO(bool status) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_MCR(_bus) &= ~FLEXCAN_MCR_FEN; // Disable FIFO if already enabled for cleanup.
  writeIMASK(0ULL); // disable all FIFO/MB Interrupts

  for (uint8_t i = 0; i < FLEXCANb_MAXMB_SIZE(_bus); i++ ) { // clear all mailboxes
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80 + (i * 0x10)));
    mbxAddr[0] = mbxAddr[1] = mbxAddr[2] = mbxAddr[3] = 0; // code, id, word0, word1
    FLEXCANb_RXIMR(_bus, i) = 0UL; // CLEAR MAILBOX MASKS (RXIMR)
  }

  FLEXCANb_RXMGMASK(_bus) = FLEXCANb_RXFGMASK(_bus) = 0;
  writeIFLAG(readIFLAG()); // (all bits reset when written back)

  if ( status ) {
    FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_FEN;
    for (uint8_t i = mailboxOffset(); i < FLEXCANb_MAXMB_SIZE(_bus); i++) {
      FLEXCANb_MBn_CS(_bus,i) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
      enableMBInterrupt((FLEXCAN_MAILBOX)i); /* enable TX interrupt */
    } 
  } 
  else { // FIFO disabled default setup of mailboxes, 0-7 RX, 8-15 TX
    for (uint8_t i = 0; i < FLEXCANb_MAXMB_SIZE(_bus); i++ ) { // clear all mailboxes
      volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80 + (i * 0x10)));
      if ( i < (FLEXCANb_MAXMB_SIZE(_bus) / 2) ) {
        mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | ((i < (FLEXCANb_MAXMB_SIZE(_bus) / 4)) ? 0 : FLEXCAN_MB_CS_IDE | FLEXCAN_MB_CS_SRR);
        FLEXCANb_RXIMR(_bus, i) = 0UL | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0); // (RXIMR)
      }
      else {
        mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
        enableMBInterrupt((FLEXCAN_MAILBOX)i); /* enable TX interrupt */
      }
    }
  }
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::enableFIFOInterrupt(bool status) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN) ) return; /* FIFO must be enabled first */
  if ( FLEXCANb_IMASK1(_bus) & FLEXCAN_IMASK1_BUF5M ) return; /* FIFO interrupts already enabled */
  FLEXCANb_IMASK1(_bus) &= ~0xFF; /* disable FIFO interrupt flags */
  if ( status ) FLEXCANb_IMASK1(_bus) |= FLEXCAN_IMASK1_BUF5M; /* enable FIFO interrupt */
}

FCTP_FUNC void FCTP_OPT::enableMBInterrupts(bool status) {
  FLEXCAN_EnterFreezeMode();
  for ( uint8_t mb_num = mailboxOffset(); mb_num < FLEXCANb_MAXMB_SIZE(_bus); mb_num++ ) {
    enableMBInterrupt((FLEXCAN_MAILBOX)mb_num, status);
  }
  FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::enableMBInterrupt(const FLEXCAN_MAILBOX &mb_num, bool status) {
  if ( mb_num < mailboxOffset() ) return; /* mailbox not available */
  if ( status ) writeIMASKBit(mb_num); /* enable mailbox interrupt */
  else if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) writeIMASKBit(mb_num); /* transmit interrupt keeper */
  else writeIMASKBit(mb_num, 0); /* disable mailbox interrupt */
}

FCTP_FUNC bool FCTP_OPT::setMB(const FLEXCAN_MAILBOX &mb_num, const FLEXCAN_RXTX &mb_rx_tx, const FLEXCAN_IDE &ide) {
  if ( mb_num < mailboxOffset() ) return 0; /* mailbox not available */
  writeIMASKBit(mb_num, 0); /* immediately disable mailbox interrupt */
  FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)); // Reading Control Status atomically locks mailbox (if it is RX mode).
  FLEXCANb_MBn_ID(_bus, mb_num) = 0UL;
  FLEXCANb_MBn_WORD0(_bus, mb_num) = 0UL;
  FLEXCANb_MBn_WORD1(_bus, mb_num) = 0UL;
  if ( mb_rx_tx == RX ) {
    if ( ide != EXT ) FLEXCANb_MBn_CS(_bus, mb_num) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY);
    else FLEXCANb_MBn_CS(_bus, mb_num) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | FLEXCAN_MB_CS_SRR | FLEXCAN_MB_CS_IDE;
  }
  if ( mb_rx_tx == TX ) {
    FLEXCANb_MBn_CS(_bus, mb_num) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
    writeIMASKBit(mb_num); /* keep transmit interrupt enabled */
  }
  if ( ide == INACTIVE ) {
    FLEXCANb_MBn_CS(_bus, mb_num) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_INACTIVE);
  }
  (void)FLEXCANb_TIMER(_bus);
  writeIFLAGBit(mb_num); /* clear mailbox reception flag */
  mb_filter_table[mb_num][0] = ( ((FLEXCANb_MBn_CS(_bus, mb_num) & 0x600000) ? 1UL : 0UL) << 27); /* extended flag check */
  return 1;
}

FCTP_FUNC void FCTP_OPT::mailboxStatus() {
  if ( FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN ) {
    Serial.print("FIFO Enabled --> "); ( FLEXCANb_IMASK1(_bus) & FLEXCAN_IFLAG1_BUF5I ) ? Serial.println("Interrupt Enabled") : Serial.println("Interrupt Disabled");
    Serial.print("\tFIFO Filters in use: ");
    uint32_t remaining_mailboxes = FLEXCANb_MAXMB_SIZE(_bus) - 6 /* MAXMB - FIFO */ - ((((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 2);
    if ( FLEXCANb_MAXMB_SIZE(_bus) < (6 + ((((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 2))) remaining_mailboxes = 0;
    Serial.println(constrain((uint8_t)(FLEXCANb_MAXMB_SIZE(_bus) - remaining_mailboxes), 0, 32));
    Serial.print("\tRemaining Mailboxes: ");
    if ( FLEXCANb_MAXMB_SIZE(_bus) < (6 + ((((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 2))) remaining_mailboxes = 0;
    Serial.println(remaining_mailboxes); // 8 filters per 2 mailboxes
    for ( uint8_t i = FLEXCANb_MAXMB_SIZE(_bus) - remaining_mailboxes; i < FLEXCANb_MAXMB_SIZE(_bus); i++ ) {
      switch ( FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, i)) ) {
        case 0b0000: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_INACTIVE"); break;
          }
        case 0b0100: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.print(" code: RX_EMPTY");
            (FLEXCANb_MBn_CS(_bus, i) & FLEXCAN_MB_CS_IDE) ? Serial.println("\t(Extended Frame)") : Serial.println("\t(Standard Frame)");
            break;
          }
        case 0b0010: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_FULL"); break;
          }
        case 0b0110: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_OVERRUN"); break;
          }
        case 0b1010: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_RANSWER"); break;
          }
        case 0b0001: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_BUSY"); break;
          }
        case 0b1000: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: TX_INACTIVE"); break;
          }
        case 0b1001: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: TX_ABORT"); break;
          }
        case 0b1100: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.print(" code: TX_DATA (Transmitting)");
            uint32_t extid = (FLEXCANb_MBn_CS(_bus, i) & FLEXCAN_MB_CS_IDE);
            (extid) ? Serial.print("(Extended Frame)") : Serial.print("(Standard Frame)");
            uint32_t dataIn = FLEXCANb_MBn_WORD0(_bus, i);
            uint32_t id = (FLEXCANb_MBn_ID(_bus, i) & FLEXCAN_MB_ID_EXT_MASK);
            if (!extid) id >>= FLEXCAN_MB_ID_STD_BIT_NO;
            Serial.print("(ID: 0x"); Serial.print(id, HEX); Serial.print(")");
            Serial.print("(Payload: "); Serial.print((uint8_t)(dataIn >> 24), HEX);
            Serial.print(" "); Serial.print((uint8_t)(dataIn >> 16), HEX);
            Serial.print(" "); Serial.print((uint8_t)(dataIn >> 8), HEX);
            Serial.print(" "); Serial.print((uint8_t)dataIn, HEX);
            dataIn = FLEXCANb_MBn_WORD1(_bus, i);
            Serial.print(" "); Serial.print((uint8_t)(dataIn >> 24), HEX);
            Serial.print(" "); Serial.print((uint8_t)(dataIn >> 16), HEX);
            Serial.print(" "); Serial.print((uint8_t)(dataIn >> 8), HEX);
            Serial.print(" "); Serial.print((uint8_t)dataIn, HEX);
            Serial.println(")");
            break;
          }
        case 0b1110: {
            Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: TX_TANSWER"); break;
          }
      }
    } // for loop
    return;
  } // fifo detected ends here

  Serial.print("FIFO Disabled\n\tMailboxes:\n");
  for ( uint8_t i = 0; i < FLEXCANb_MAXMB_SIZE(_bus); i++ ) {
    switch ( FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, i)) ) {
      case 0b0000: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_INACTIVE"); break;
        }
      case 0b0100: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.print(" code: RX_EMPTY");
          (FLEXCANb_MBn_CS(_bus, i) & FLEXCAN_MB_CS_IDE) ? Serial.println("\t(Extended Frame)") : Serial.println("\t(Standard Frame)");
          break;
        }
      case 0b0010: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_FULL"); break;
        }
      case 0b0110: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_OVERRUN"); break;
        }
      case 0b1010: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_RANSWER"); break;
        }
      case 0b0001: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: RX_BUSY"); break;
        }
      case 0b1000: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: TX_INACTIVE"); break;
        }
      case 0b1001: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: TX_ABORT"); break;
        }
      case 0b1100: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.print(" code: TX_DATA (Transmitting)");
          uint32_t extid = (FLEXCANb_MBn_CS(_bus, i) & FLEXCAN_MB_CS_IDE);
          (extid) ? Serial.print("(Extended Frame)") : Serial.print("(Standard Frame)");
          uint32_t dataIn = FLEXCANb_MBn_WORD0(_bus, i);
          uint32_t id = (FLEXCANb_MBn_ID(_bus, i) & FLEXCAN_MB_ID_EXT_MASK);
          if (!extid) id >>= FLEXCAN_MB_ID_STD_BIT_NO;
          Serial.print("(ID: 0x"); Serial.print(id, HEX); Serial.print(")");
          Serial.print("(Payload: "); Serial.print((uint8_t)(dataIn >> 24), HEX);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 16), HEX);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 8), HEX);
          Serial.print(" "); Serial.print((uint8_t)dataIn, HEX);
          dataIn = FLEXCANb_MBn_WORD1(_bus, i);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 24), HEX);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 16), HEX);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 8), HEX);
          Serial.print(" "); Serial.print((uint8_t)dataIn, HEX);
          Serial.println(")");
          break;
        }
      case 0b1110: {
          Serial.print("\t\tMB"); Serial.print(i); Serial.println(" code: TX_TANSWER"); break;
        }
    }
  } // for loop
}

FCTP_FUNC uint64_t FCTP_OPT::readIFLAG() {
#if defined(__IMXRT1062__)
  return (((uint64_t)FLEXCANb_IFLAG2(_bus) << 32) | FLEXCANb_IFLAG1(_bus));
#endif
  return FLEXCANb_IFLAG1(_bus);
}

FCTP_FUNC void FCTP_OPT::writeIFLAG(uint64_t value) {
#if defined(__IMXRT1062__)
  FLEXCANb_IFLAG2(_bus) = value >> 32;
#endif
  FLEXCANb_IFLAG1(_bus) = value;
}

FCTP_FUNC void FCTP_OPT::writeIFLAGBit(uint8_t mb_num) { // bugfix by bsundahl1
  if ( mb_num < 32 ) FLEXCANb_IFLAG1(_bus) = (1UL << mb_num);
  else FLEXCANb_IFLAG2(_bus) = (1UL << (mb_num - 32));
}

FCTP_FUNC void FCTP_OPT::writeIMASK(uint64_t value) {
#if defined(__IMXRT1062__)
  FLEXCANb_IMASK2(_bus) = value >> 32;
#endif
  FLEXCANb_IMASK1(_bus) = value;
}

FCTP_FUNC uint64_t FCTP_OPT::readIMASK() {
#if defined(__IMXRT1062__)
  return (((uint64_t)FLEXCANb_IMASK2(_bus) << 32) | FLEXCANb_IMASK1(_bus));
#endif
  return FLEXCANb_IMASK1(_bus);
}

FCTP_FUNC void FCTP_OPT::writeIMASKBit(uint8_t mb_num, bool set) {
  if ( mb_num < 32 ) (( set ) ? FLEXCANb_IMASK1(_bus) |= (1UL << mb_num) : FLEXCANb_IMASK1(_bus) &= ~(1UL << mb_num));
  else (( set ) ? FLEXCANb_IMASK2(_bus) |= (1UL << (mb_num - 32)) : FLEXCANb_IMASK2(_bus) &= ~(1UL << (mb_num - 32)));
}

FCTP_FUNC void FCTP_OPT::writeTxMailbox(uint8_t mb_num, const CAN_message_t &msg) {
  writeIFLAGBit(mb_num);
  uint32_t code = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80 + (mb_num * 0x10)));
  mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
  mbxAddr[1] = (( msg.flags.extended ) ? ( msg.id & FLEXCAN_MB_ID_EXT_MASK ) : FLEXCAN_MB_ID_IDSTD(msg.id));
  if ( msg.flags.remote ) code |= (1UL << 20);
  if ( msg.flags.extended ) code |= (3UL << 21);
  for ( uint8_t i = 0; i < (8 >> 2); i++ ) mbxAddr[2 + i] = (msg.buf[0 + i * 4] << 24) | (msg.buf[1 + i * 4] << 16) | (msg.buf[2 + i * 4] << 8) | msg.buf[3 + i * 4];
  code |= msg.len << 16;
  mbxAddr[0] = code | FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_ONCE);
}

FCTP_FUNC uint8_t FCTP_OPT::mailboxOffset() {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN ) ) return 0; /* return offset 0 since FIFO is disabled */
  uint32_t remaining_mailboxes = FLEXCANb_MAXMB_SIZE(_bus) - 6 /* MAXMB - FIFO */ - ((((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 2);
  if ( FLEXCANb_MAXMB_SIZE(_bus) < (6 + ((((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 2))) remaining_mailboxes = 0;
  return (FLEXCANb_MAXMB_SIZE(_bus) - remaining_mailboxes); /* otherwise return offset MB position after FIFO area */
}

FCTP_FUNC void FCTP_OPT::setMaxMB(uint8_t last) {
  last = constrain(last,1,64);
  last--;
  FLEXCAN_EnterFreezeMode();
  bool fifo_was_cleared = FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN;
  disableFIFO();
  writeIFLAG(readIFLAG()); // (all bits reset when written back) (needed for MAXMB changes)
  FLEXCANb_MCR(_bus) &= ~0x7F; // clear current value
  FLEXCANb_MCR(_bus) |= last; // set mailbox max
  if ( fifo_was_cleared ) enableFIFO();
  FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::softReset() {
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_SOFT_RST;
  while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_SOFT_RST);
}

FCTP_FUNC void FCTP_OPT::FLEXCAN_ExitFreezeMode() {
  FLEXCANb_MCR(_bus) &= ~FLEXCAN_MCR_HALT;
  while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
}

FCTP_FUNC void FCTP_OPT::FLEXCAN_EnterFreezeMode() {
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_FRZ | FLEXCAN_MCR_HALT;
  while (!(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK));
}

FCTP_FUNC void FCTP_OPT::setBaudRate(uint32_t baud, FLEXCAN_RXTX listen_only) {
  currentBitrate = baud;

#if defined(__IMXRT1062__)
  uint32_t clockFreq = getClock() * 1000000;
#else
  uint32_t clockFreq = 16000000;
#endif

  uint32_t divisor = 0, bestDivisor = 0, result = clockFreq / baud / (divisor + 1);
  int error = baud - (clockFreq / (result * (divisor + 1))), bestError = error;

  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();

  while (result > 5) {
    divisor++;
    result = clockFreq / baud / (divisor + 1);
    if (result <= 25) {
      error = baud - (clockFreq / (result * (divisor + 1)));
      if (error < 0) error *= -1;
      if (error < bestError) {
        bestError = error;
        bestDivisor = divisor;
      }
      if ((error == bestError) && (result > 11) && (result < 19)) {
        bestError = error;
        bestDivisor = divisor;
      }
    }
  }

  divisor = bestDivisor;
  result = clockFreq / baud / (divisor + 1);

  if ((result < 5) || (result > 25) || (bestError > 300)) {
    if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
    return;
  }

  result -= 5; // the bitTimingTable is offset by 5 since there was no reason to store bit timings for invalid numbers
  uint8_t bitTimingTable[21][3] = {
    {0, 0, 1}, //5
    {1, 0, 1}, //6
    {1, 1, 1}, //7
    {2, 1, 1}, //8
    {2, 2, 1}, //9
    {2, 3, 1}, //10
    {2, 3, 2}, //11
    {2, 4, 2}, //12
    {2, 5, 2}, //13
    {2, 5, 3}, //14
    {2, 6, 3}, //15
    {2, 7, 3}, //16
    {2, 7, 4}, //17
    {3, 7, 4}, //18
    {3, 7, 5}, //19
    {4, 7, 5}, //20
    {4, 7, 6}, //21
    {5, 7, 6}, //22
    {6, 7, 6}, //23
    {6, 7, 7}, //24
    {7, 7, 7}, //25
  }, propSeg = bitTimingTable[result][0], pSeg1 = bitTimingTable[result][1], pSeg2 = bitTimingTable[result][2];
  FLEXCANb_CTRL1(_bus) = (FLEXCAN_CTRL_PROPSEG(propSeg) | FLEXCAN_CTRL_RJW(1) | FLEXCAN_CTRL_PSEG1(pSeg1) |
                    FLEXCAN_CTRL_PSEG2(pSeg2) | FLEXCAN_CTRL_ERR_MSK | FLEXCAN_CTRL_PRESDIV(divisor));
  ( listen_only != LISTEN_ONLY ) ? FLEXCANb_CTRL1(_bus) &= ~FLEXCAN_CTRL_LOM : FLEXCANb_CTRL1(_bus) |= FLEXCAN_CTRL_LOM; /* listen-only mode */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::setMRP(bool mrp) { /* mailbox priority (1) or FIFO priority (0) */
  FLEXCAN_EnterFreezeMode();
  if ( mrp ) FLEXCANb_CTRL2(_bus) |= FLEXCAN_CTRL2_MRP;
  else FLEXCANb_CTRL2(_bus) &= ~FLEXCAN_CTRL2_MRP;
  FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::setRRS(bool rrs) { /* store remote frames */
  FLEXCAN_EnterFreezeMode();
  if ( rrs ) FLEXCANb_CTRL2(_bus) |= FLEXCAN_CTRL2_RRS;
  else FLEXCANb_CTRL2(_bus) &= ~FLEXCAN_CTRL2_RRS;
  FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::setTX(FLEXCAN_PINS pin) {
#if defined(__IMXRT1062__)
  if ( _bus == CAN3 ) {
    if ( pin == DEF ) {
      IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_36 = 0x19; // pin31 T3B2
      IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_36 = 0x10B0; // pin31 T3B2
    }
  }
  if ( _bus == CAN2 ) {
    if ( pin == DEF ) {
      IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02 = 0x10; // pin 1 T4B1+B2
      IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_02 = 0x10B0; // pin 1 T4B1+B2
    }
  }
  if ( _bus == CAN1 ) {
    if ( pin == DEF ) {
      IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_08 = 0x12; // pin 22 T4B1+B2
      IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_08 = 0x10B0; // pin 22 T4B1+B2
    }
    if ( pin == ALT ) {
      IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_02 = 0x12; // pin 11 T4B1+B2
      IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_02= 0x10B0; // pin 11 T4B1+B2
    }
  }
#endif

#if defined(__MK20DX256__)
  CORE_PIN3_CONFIG = PORT_PCR_MUX(2);
#endif
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
  if ( _bus == CAN0 ) {
    static bool init_pins = 1;
    if ( init_pins ) {
      pin = ( !CORE_PIN3_CONFIG ) ? DEF : ALT; 
      init_pins = 0;
    }
    if ( pin == ALT ) {
      if ( CORE_PIN3_CONFIG == PORT_PCR_MUX(2) ) CORE_PIN3_CONFIG = 0;
      CORE_PIN29_CONFIG = PORT_PCR_MUX(2);
    }
    else if ( pin == DEF ) {
      if ( CORE_PIN29_CONFIG == PORT_PCR_MUX(2) ) CORE_PIN29_CONFIG = 0;
      CORE_PIN3_CONFIG = PORT_PCR_MUX(2);
    }
  } /* Alternative CAN1 pins are not broken out on Teensy 3.6 */
#endif
#if defined(__MK66FX1M0__)
  if ( _bus == CAN1 ) {
    CORE_PIN33_CONFIG = PORT_PCR_MUX(2);
  }
#endif

}

FCTP_FUNC void FCTP_OPT::setRX(FLEXCAN_PINS pin) {
#if defined(__IMXRT1062__)
  /* DAISY REGISTER CAN3
    00 GPIO_EMC_37_ALT9 Selecting Pad: GPIO_EMC_37 for Mode: ALT9
    01 GPIO_AD_B0_15_ALT8 Selecting Pad: GPIO_AD_B0_15 for Mode: ALT8
    10 GPIO_AD_B0_11_ALT8 Selecting Pad: GPIO_AD_B0_11 for Mode: ALT8
  */
  /* DAISY REGISTER CAN2
    00 GPIO_EMC_10_ALT3 Selecting Pad: GPIO_EMC_10 for Mode: ALT3
    01 GPIO_AD_B0_03_ALT0 Selecting Pad: GPIO_AD_B0_03 for Mode: ALT0
    10 GPIO_AD_B0_15_ALT6 Selecting Pad: GPIO_AD_B0_15 for Mode: ALT6
    11 GPIO_B1_09_ALT6 Selecting Pad: GPIO_B1_09 for Mode: ALT6
  */
  /* DAISY REGISTER CAN1
    00 GPIO_SD_B1_03_ALT4 Selecting Pad: GPIO_SD_B1_03 for Mode: ALT4
    01 GPIO_EMC_18_ALT3 Selecting Pad: GPIO_EMC_18 for Mode: ALT3
    10 GPIO_AD_B1_09_ALT2 Selecting Pad: GPIO_AD_B1_09 for Mode: ALT2
    11 GPIO_B0_03_ALT2 Selecting Pad: GPIO_B0_03 for Mode: ALT2
  */
  if ( _bus == CAN3 ) {
    if ( pin == DEF ) {
      IOMUXC_CANFD_IPP_IND_CANRX_SELECT_INPUT = 0x00;
      IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_37 = 0x19; // pin30 T3B2
      IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_37 = 0x10B0; // pin30 T3B2
    }
  }
  if ( _bus == CAN2 ) {
    if ( pin == DEF ) {
      IOMUXC_FLEXCAN2_RX_SELECT_INPUT = 0x01;
      IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03 = 0x10; // pin 0 T4B1+B2
      IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_03 = 0x10B0; // pin 0 T4B1+B2
    }
  }
  if ( _bus == CAN1 ) {
    if ( pin == DEF ) {
      IOMUXC_FLEXCAN1_RX_SELECT_INPUT = 0x02;
      IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_09 = 0x12; // pin 23 T4B1+B2
      IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_09 = 0x10B0; // pin 23 T4B1+B2
    }
    if ( pin == ALT ) {
      IOMUXC_FLEXCAN1_RX_SELECT_INPUT = 0x03;
      IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_03 = 0x12; // pin 13 T4B1+B2
      IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03 = 0x10B0; // pin 13 T4B1+B2
    }
  }
#endif

#if defined(__MK20DX256__)
  CORE_PIN4_CONFIG = PORT_PCR_MUX(2);
#endif
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
  if ( _bus == CAN0 ) {
    static bool init_pins = 1;
    if ( init_pins ) {
      pin = ( !CORE_PIN4_CONFIG ) ? DEF : ALT; 
      init_pins = 0;
    }
    if ( pin == ALT ) {
      if ( CORE_PIN4_CONFIG == PORT_PCR_MUX(2) ) CORE_PIN4_CONFIG = 0;
      CORE_PIN30_CONFIG = PORT_PCR_MUX(2);
    }
    else if ( pin == DEF ) {
      if ( CORE_PIN30_CONFIG == PORT_PCR_MUX(2) ) CORE_PIN30_CONFIG = 0;
      CORE_PIN4_CONFIG = PORT_PCR_MUX(2);
    }
  } /* Alternative CAN1 pins are not broken out on Teensy 3.6 */
#endif
#if defined(__MK66FX1M0__)
  if ( _bus == CAN1 ) {
    CORE_PIN34_CONFIG = PORT_PCR_MUX(2);
  }
#endif

}

FCTP_FUNC bool FCTP_OPT::setMBUserFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t mask) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_USERMASK, mb_num, 1, id1, 0, 0, 0, mask);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBUserFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t mask) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t new_mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD((((id1 | id2) ^ (id1 & id2)) ^ 0x7FF) & mask) : ((((id1 | id2) ^ (id1 & id2)) ^ 0x1FFFFFFF) & mask);
  setMBFilterProcessing(mb_num,id1,new_mask);
  filter_store(FLEXCAN_USERMASK, mb_num, 2, id1, id2, 0, 0, mask);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBUserFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t mask) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t new_mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD((((id1 | id2 | id3) ^ (id1 & id2 & id3)) ^ 0x7FF) & mask) : ((((id1 | id2 | id3) ^ (id1 & id2 & id3)) ^ 0x1FFFFFFF) & mask);
  setMBFilterProcessing(mb_num,id1,new_mask);
  filter_store(FLEXCAN_USERMASK, mb_num, 3, id1, id2, id3, 0, mask);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBUserFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t mask) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t new_mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD((((id1 | id2 | id3 | id4) ^ (id1 & id2 & id3 & id4)) ^ 0x7FF) & mask) : ((((id1 | id2 | id3 | id4) ^ (id1 & id2 & id3 & id4)) ^ 0x1FFFFFFF) & mask);
  setMBFilterProcessing(mb_num,id1,new_mask);
  filter_store(FLEXCAN_USERMASK, mb_num, 4, id1, id2, id3, id4, mask);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOUserFilter(uint8_t filter, uint32_t id1, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* WE ONLY USE TABLE A FOR CUSTOM FILTERS, AS TABLE B+ (PARTIAL IDS) LEAST SIGNIFICANT BITS ARE IGNORED */
  /* ##################################### TABLE A ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) {
//    uint32_t new_mask = mask << (( ide == EXT ) ? 1 : 19) | 0xC0000001;
    uint32_t new_mask = ( ide != EXT ) ? ((((id1) ^ (id1)) ^ 0x7FF) << 19 ) | 0xC0000001 : (((((id1) ^ (id1)) ^ 0x1FFFFFFF) & mask) << 1 ) | 0xC0000001;
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((remote == RTR ? 1 : 0) << 31) |
        ((ide == EXT ? ((id1 & FLEXCAN_MB_ID_EXT_MASK) << 1) : (FLEXCAN_MB_ID_IDSTD(id1) << 1)));
    if ( filter < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, filter) = new_mask;
    FLEXCANb_RXFGMASK(_bus) = 0x3FFFFFFF; /* enforce it for blocks 32->127, single IDs */

    fifo_filter_table[filter][0] = ( ((ide == EXT) ? 1UL : 0UL) << 16); /* extended flag check */
    fifo_filter_store(FLEXCAN_USERMASK, filter, 1, id1, 0, 0, 0, mask);
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOUserFilter(uint8_t filter, uint32_t id1, uint32_t id2, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* WE ONLY USE TABLE A FOR CUSTOM FILTERS, AS TABLE B+ (PARTIAL IDS) LEAST SIGNIFICANT BITS ARE IGNORED */
  /* ##################################### TABLE A ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) {
    uint32_t new_mask = ( ide != EXT ) ? ((((id1 | id2) ^ (id1 & id2)) ^ 0x7FF) << 19 ) | 0xC0000001 : (((((id1 | id2) ^ (id1 & id2)) ^ 0x1FFFFFFF) & mask) << 1 ) | 0xC0000001;
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((remote == RTR ? 1 : 0) << 31) |
        ((ide == EXT ? ((id1 & FLEXCAN_MB_ID_EXT_MASK) << 1) : (FLEXCAN_MB_ID_IDSTD(id1) << 1)));
    if ( filter < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, filter) = new_mask;
    FLEXCANb_RXFGMASK(_bus) = 0x3FFFFFFF; /* enforce it for blocks 32->127, single IDs */

    fifo_filter_table[filter][0] = ( ((ide == EXT) ? 1UL : 0UL) << 16); /* extended flag check */
    fifo_filter_store(FLEXCAN_USERMASK, filter, 2, id1, id2, 0, 0, mask);
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOUserFilter(uint8_t filter, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* WE ONLY USE TABLE A FOR CUSTOM FILTERS, AS TABLE B+ (PARTIAL IDS) LEAST SIGNIFICANT BITS ARE IGNORED */
  /* ##################################### TABLE A ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) {
    uint32_t new_mask = ( ide != EXT ) ? ((((id1 | id2 | id3) ^ (id1 & id2 & id3)) ^ 0x7FF) << 19 ) | 0xC0000001 : (((((id1 | id2 | id3) ^ (id1 & id2 & id3)) ^ 0x1FFFFFFF) & mask) << 1 ) | 0xC0000001;
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((remote == RTR ? 1 : 0) << 31) |
        ((ide == EXT ? ((id1 & FLEXCAN_MB_ID_EXT_MASK) << 1) : (FLEXCAN_MB_ID_IDSTD(id1) << 1)));
    if ( filter < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, filter) = new_mask;
    FLEXCANb_RXFGMASK(_bus) = 0x3FFFFFFF; /* enforce it for blocks 32->127, single IDs */

    fifo_filter_table[filter][0] = ( ((ide == EXT) ? 1UL : 0UL) << 16); /* extended flag check */
    fifo_filter_store(FLEXCAN_USERMASK, filter, 3, id1, id2, id3, 0, mask);
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOUserFilter(uint8_t filter, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* WE ONLY USE TABLE A FOR CUSTOM FILTERS, AS TABLE B+ (PARTIAL IDS) LEAST SIGNIFICANT BITS ARE IGNORED */
  /* ##################################### TABLE A ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) {
    uint32_t new_mask = ( ide != EXT ) ? ((((id1 | id2 | id3 | id4) ^ (id1 & id2 & id3 & id4)) ^ 0x7FF) << 19 ) | 0xC0000001 : (((((id1 | id2 | id3 | id4) ^ (id1 & id2 & id3 & id4)) ^ 0x1FFFFFFF) & mask) << 1 ) | 0xC0000001;
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((remote == RTR ? 1 : 0) << 31) |
        ((ide == EXT ? ((id1 & FLEXCAN_MB_ID_EXT_MASK) << 1) : (FLEXCAN_MB_ID_IDSTD(id1) << 1)));
    if ( filter < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, filter) = new_mask;
    FLEXCANb_RXFGMASK(_bus) = 0x3FFFFFFF; /* enforce it for blocks 32->127, single IDs */

    fifo_filter_table[filter][0] = ( ((ide == EXT) ? 1UL : 0UL) << 16); /* extended flag check */
    fifo_filter_store(FLEXCAN_USERMASK, filter, 4, id1, id2, id3, id4, mask);
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC void FCTP_OPT::setMBFilterProcessing(FLEXCAN_MAILBOX mb_num, uint32_t filter_id, uint32_t calculated_mask) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_RXIMR(_bus, mb_num) = calculated_mask | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0);
  FLEXCANb_MBn_ID(_bus, mb_num) = ((!(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE)) ? FLEXCAN_MB_ID_IDSTD(filter_id) : (filter_id & 0x1FFFFFFF));
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::setMBFilter(FLEXCAN_FLTEN input) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  for (uint8_t i = mailboxOffset(); i < FLEXCANb_MAXMB_SIZE(_bus); i++) {
    if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, i)) >> 3) ) continue; /* skip TX mailboxes */
    if ( input == ACCEPT_ALL ) FLEXCANb_RXIMR(_bus, i) = 0UL | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0); // (RXIMR)
    if ( input == REJECT_ALL ) FLEXCANb_RXIMR(_bus, i) = ~0UL; // (RXIMR)
    FLEXCANb_MBn_ID(_bus, i) = ~0UL;
    mb_filter_table[i][0] = ( ((FLEXCANb_MBn_CS(_bus, i) & 0x600000) ? 1UL : 0UL) << 27); /* extended flag check */
  }
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, FLEXCAN_FLTEN input) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return; /* exit on TX mailbox */ 
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  if ( input == ACCEPT_ALL ) FLEXCANb_RXIMR(_bus, mb_num) = 0UL | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0); // (RXIMR)
  if ( input == REJECT_ALL ) FLEXCANb_RXIMR(_bus, mb_num) = ~0UL; // (RXIMR)
  FLEXCANb_MBn_ID(_bus, mb_num) = 0UL;
  mb_filter_table[mb_num][0] = ( ((FLEXCANb_MBn_CS(_bus, mb_num) & 0x600000) ? 1UL : 0UL) << 27); /* extended flag check */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC bool FCTP_OPT::setMBManualFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t mask) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */
  if (!(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE)) {
    mask = mask << 18;  /* shift mask by 18 for 11 bit filters */
  }
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 1, id1, 0, 0, 0, 0);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1) ^ (id1)) ^ 0x7FF) : ((((id1) ^ (id1)) ^ 0x1FFFFFFF) & 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 1, id1, 0, 0, 0, 0);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1 | id2) ^ (id1 & id2)) ^ 0x7FF) : ((((id1 | id2) ^ (id1 & id2)) ^ 0x1FFFFFFF) & 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 2, id1, id2, 0, 0, 0);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1 | id2 | id3) ^ (id1 & id2 & id3)) ^ 0x7FF) : ((((id1 | id2 | id3) ^ (id1 & id2 & id3)) ^ 0x1FFFFFFF) & 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 3, id1, id2, id3, 0, 0);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1 | id2 | id3 | id4) ^ (id1 & id2 & id3 & id4)) ^ 0x7FF) : ((((id1 | id2 | id3 | id4) ^ (id1 & id2 & id3 & id4)) ^ 0x1FFFFFFF) & 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 4, id1, id2, id3, id4, 0);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1 | id2 | id3 | id4 | id5) ^ (id1 & id2 & id3 & id4 & id5)) ^ 0x7FF) : ((((id1 | id2 | id3 | id4 | id5) ^ (id1 & id2 & id3 & id4 & id5)) ^ 0x1FFFFFFF) & 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 5, id1, id2, id3, id4, id5);
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setMBFilterRange(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2) {
  if ( mb_num < mailboxOffset() || mb_num >= FLEXCANb_MAXMB_SIZE(_bus) ) return 0; /* mailbox not available */
  if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, mb_num)) >> 3) ) return 0; /* exit on TX mailbox */ 
  if ( id1 > id2 || ((id2 > id1) && (id2-id1>1000)) || !id1 || !id2 ) return 0; /* don't play around... */
  uint32_t stage1 = id1, stage2 = id1;
  for ( uint32_t i = id1 + 1; i <= id2; i++ ) {
    stage1 |= i; stage2 &= i;
  }
  uint32_t mask = ( !(FLEXCANb_MBn_CS(_bus, mb_num) & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD( (stage1 ^ stage2) ^ 0x1FFFFFFF ) : (( (stage1 ^ stage2) ^ 0x1FFFFFFF ) & 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_RANGE, mb_num, 2, id1, id2, 0, 0, 0);
  return 1;
}

FCTP_FUNC int FCTP_OPT::readFIFO(CAN_message_t &msg) {
  //delayMicroseconds(150);
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN) ) return 0; /* FIFO is disabled */
  if ( !(FLEXCANb_MCR(_bus) & (1UL << 15)) ) { /* if DMA is not enabled, check interrupt flag, else continue. */
    if ( FLEXCANb_IMASK1(_bus) & FLEXCAN_IMASK1_BUF5M ) return 0; /* FIFO interrupt enabled, polling blocked */
  }
  if ( FLEXCANb_IFLAG1(_bus) & FLEXCAN_IFLAG1_BUF5I ) { /* message available */
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80));
    uint32_t code = mbxAddr[0];
    msg.len = (code & 0xF0000) >> 16;
    msg.flags.remote = (bool)(code & (1UL << 20));
    msg.flags.extended = (bool)(code & (1UL << 21));
    msg.timestamp = code & 0xFFFF;
    msg.id = (mbxAddr[1] & 0x1FFFFFFF) >> ((msg.flags.extended) ? 0 : 18);
    uint32_t data0 = mbxAddr[2]; 
    for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[3 - d] = (uint8_t)(data0 >> (8 * d));
    uint32_t data1 = mbxAddr[3];
    for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[7 - d] = (uint8_t)(data1 >> (8 * d));
    msg.bus = busNumber;
    msg.idhit = code >> 23;
    msg.mb = FIFO; /* store the mailbox the message came from (for callback reference) */
    if ( !(FLEXCANb_MCR(_bus) & (1UL << 15)) ) writeIFLAGBit(5); /* clear FIFO bit only, NOT FOR DMA USE! */
    frame_distribution(msg);
    if ( fifo_filter_match(msg.id) ) return 1;
  }
  return 0; /* message not available */
}

FCTP_FUNC int FCTP_OPT::getFirstTxBox() {
  for (uint8_t i = mailboxOffset(); i < FLEXCANb_MAXMB_SIZE(_bus); i++) {
    if ( (FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, i)) >> 3) ) return i; // if TX
  }
  return -1;
}

FCTP_FUNC int FCTP_OPT::read(CAN_message_t &msg) {
  bool _random = random(0, 2);
  if ( ( !_random ) && ( FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN ) &&
       !( FLEXCANb_IMASK1(_bus) & FLEXCAN_IMASK1_BUF5M ) &&
       ( FLEXCANb_IFLAG1(_bus) & FLEXCAN_IFLAG1_BUF5I ) ) return readFIFO(msg);
  return readMB(msg);
}

FCTP_FUNC int FCTP_OPT::readMB(CAN_message_t &msg) {
  uint64_t iflag = 0;
  for ( uint8_t cycle_limit = 3, mailboxes = mailboxOffset(); mailbox_reader_increment <= FLEXCANb_MAXMB_SIZE(_bus); ++mailbox_reader_increment ) {
    iflag = readIFLAG();
    if ( iflag && (mailbox_reader_increment >= (64 - __builtin_clzll(iflag))) ) { /* break from MSB's if unset, add 1 to prevent undefined behaviour in clz for 0 check */
      mailbox_reader_increment = mailboxOffset();
      if ( !--cycle_limit ) return 0;
    }
    if ( FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN ) {  /* FIFO is enabled, get only remaining RX (if any) */
        if ( mailbox_reader_increment < mailboxes ) mailbox_reader_increment = mailboxes - 1; /* go back to position end of fifo+filter region */
    }
    if ( mailbox_reader_increment >= FLEXCANb_MAXMB_SIZE(_bus) ) {
      mailbox_reader_increment = mailboxOffset();
      if ( !--cycle_limit ) return 0;
    }
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80 + (mailbox_reader_increment * 0x10)));
    if ((readIMASK() & (1ULL << mailbox_reader_increment))) continue; /* don't read interrupt enabled mailboxes */
    uint32_t code = mbxAddr[0];
    if ( (FLEXCAN_get_code(code) >> 3) ) continue; /* skip TX mailboxes */
    //if (!(code & 0x600000) && !(iflag & (1ULL << mailbox_reader_increment))) continue; /* don't read unflagged mailboxes, errata: extended mailboxes iflags do not work in poll mode, must check CS field */
    if ( ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_FULL ) ||
         ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) ) {
      msg.flags.remote = (bool)(code & (1UL << 20));
      msg.flags.extended = (bool)(code & (1UL << 21));
      msg.id = (mbxAddr[1] & 0x1FFFFFFF) >> ((msg.flags.extended) ? 0 : 18);
      if ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) msg.flags.overrun = 1;
      msg.len = (code & 0xF0000) >> 16;
      msg.mb = mailbox_reader_increment++;
      msg.timestamp = code & 0xFFFF;
      msg.bus = busNumber;
      for ( uint8_t i = 0; i < (8 >> 2); i++ ) for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[(4 * i) + 3 - d] = (uint8_t)(mbxAddr[2 + i] >> (8 * d));
      mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | ((msg.flags.extended) ? (FLEXCAN_MB_CS_SRR | FLEXCAN_MB_CS_IDE) : 0);
      (void)FLEXCANb_TIMER(_bus);
      writeIFLAGBit(msg.mb);
      frame_distribution(msg);
      if ( filter_match((FLEXCAN_MAILBOX)msg.mb, msg.id) ) return 1;
    }
  } 
  return 0; /* no messages available */
}

FCTP_FUNC bool FCTP_OPT::struct2queueTx(const CAN_message_t &msg) {
  if (FLEXCANb_ESR1(_bus) & 0x20) return -2;
  if ( txBuffer.size() == txBuffer.capacity() ) return 0; /* no queues available */
  uint8_t buf[sizeof(CAN_message_t)];
  memmove(buf, &msg, sizeof(msg));
  txBuffer.push_back(buf, sizeof(CAN_message_t));
  return -1; /* transmit entry failed, no mailboxes available, queued */
}

FCTP_FUNC int FCTP_OPT::write(FLEXCAN_MAILBOX mb_num, const CAN_message_t &msg) {
  if ( mb_num < mailboxOffset() ) return 0; /* FIFO doesn't transmit */
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80 + (mb_num * 0x10)));
  if ( !((FLEXCAN_get_code(mbxAddr[0])) >> 3) ) return 0; /* not a transmit mailbox */
  if ( msg.seq ) {
    int first_tx_mb = getFirstTxBox();
    if ( FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, first_tx_mb)) == FLEXCAN_MB_CODE_TX_INACTIVE ) {
      writeTxMailbox(first_tx_mb, msg);
      return 1; /* transmit entry accepted */
    }
    else {
      CAN_message_t msg_copy = msg;
      msg_copy.mb = first_tx_mb;
      return struct2queueTx(msg_copy); /* queue if no mailboxes found */
    }
  }
  if ( FLEXCAN_get_code(mbxAddr[0]) == FLEXCAN_MB_CODE_TX_INACTIVE ) {
    writeTxMailbox(mb_num, msg);
    return 1;
  }
  CAN_message_t msg_copy = msg;
  msg_copy.mb = mb_num;
  return struct2queueTx(msg_copy); /* queue if no mailboxes found */
}

FCTP_FUNC int FCTP_OPT::write(const CAN_message_t &msg) {
  if ( msg.seq ) {
    int first_tx_mb = getFirstTxBox();
    if ( FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, first_tx_mb)) == FLEXCAN_MB_CODE_TX_INACTIVE ) {
      writeTxMailbox(first_tx_mb, msg);
      return 1; /* transmit entry accepted */
    }
    else {
      CAN_message_t msg_copy = msg;
      msg_copy.mb = first_tx_mb;
      return struct2queueTx(msg_copy); /* queue if no mailboxes found */
    }
  }
  for (uint8_t i = mailboxOffset(); i < FLEXCANb_MAXMB_SIZE(_bus); i++) {
    if ( FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, i)) == FLEXCAN_MB_CODE_TX_INACTIVE ) {
      writeTxMailbox(i, msg);
      return 1; /* transmit entry accepted */
    }
  }
  CAN_message_t msg_copy = msg;
  msg_copy.mb = -1;
  return struct2queueTx(msg_copy); /* queue if no mailboxes found */
}

FCTP_FUNC void FCTP_OPT::onReceive(const FLEXCAN_MAILBOX &mb_num, _MB_ptr handler) {
  if ( FIFO == mb_num ) {
    _mbHandlers[0] = handler;
    return;
  }
  _mbHandlers[mb_num] = handler;
}

FCTP_FUNC void FCTP_OPT::onReceive(_MB_ptr handler) {
  _mainHandler = handler;
}

FCTP_FUNC void FCTP_OPT::onTransmit(const FLEXCAN_MAILBOX &mb_num, _MB_ptr handler) {
  if ( FIFO == mb_num ) {
    _mbTxHandlers[0] = handler;
    return;
  }
  _mbTxHandlers[mb_num] = handler;
}

FCTP_FUNC void FCTP_OPT::onTransmit(_MB_ptr handler) {
  _mainTxHandler = handler;
}

FCTP_FUNC uint64_t FCTP_OPT::events() {
  if ( !isEventsUsed ) isEventsUsed = 1;
  if ( rxBuffer.size() ) {
    CAN_message_t frame;
    uint8_t buf[sizeof(CAN_message_t)];
    rxBuffer.pop_front(buf, sizeof(CAN_message_t));
    memmove(&frame, buf, sizeof(frame));
    mbCallbacks((FLEXCAN_MAILBOX)frame.mb, frame);
  }
  NVIC_DISABLE_IRQ(nvicIrq);
  if ( txBuffer.size() ) {
    CAN_message_t frame;
    uint8_t buf[sizeof(CAN_message_t)];
    txBuffer.peek_front(buf, sizeof(CAN_message_t));
    memmove(&frame, buf, sizeof(frame));
    if ( frame.mb == -1 ) {
      for (uint8_t i = mailboxOffset(); i < FLEXCANb_MAXMB_SIZE(_bus); i++) {
        if ( FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, i)) == FLEXCAN_MB_CODE_TX_INACTIVE ) {
          //Serial.print("DBG NORM: "); Serial.println(frame.mb);
          writeTxMailbox(i, frame);
          txBuffer.pop_front();
        }
      }
    }
    else if ( FLEXCAN_get_code(FLEXCANb_MBn_CS(_bus, frame.mb)) == FLEXCAN_MB_CODE_TX_INACTIVE ) {
      //Serial.print("DBG SEQ: "); Serial.println(frame.mb);
      writeTxMailbox(frame.mb, frame);
      txBuffer.pop_front();
    }
  }
  NVIC_ENABLE_IRQ(nvicIrq);
  return (uint64_t)(rxBuffer.size() << 12) | txBuffer.size();
}

#if defined(__IMXRT1062__)
static void flexcan_isr_can1() {
  if ( _CAN1 ) _CAN1->flexcan_interrupt();
}

static void flexcan_isr_can2() {
  if ( _CAN2 ) _CAN2->flexcan_interrupt();
}

static void flexcan_isr_can3() {
  if ( _CAN3 ) _CAN3->flexcan_interrupt();
}
#endif
#if defined(__MK20DX256__) || defined(__MK64FX512__)
static void flexcan_isr_can0() {
  if ( _CAN0 ) _CAN0->flexcan_interrupt();
}
#endif
#if defined(__MK66FX1M0__)
static void flexcan_isr_can0() {
  if ( _CAN0 ) _CAN0->flexcan_interrupt();
}
static void flexcan_isr_can1() {
  if ( _CAN1 ) _CAN1->flexcan_interrupt();
}
#endif


FCTP_FUNC void FCTP_OPT::mbCallbacks(const FLEXCAN_MAILBOX &mb_num, const CAN_message_t &msg) {
  if ( mb_num == FIFO ) {
    if ( _mbHandlers[0] ) _mbHandlers[0](msg);
    if ( _mainHandler ) _mainHandler(msg);
    return;
  }
  if ( _mbHandlers[mb_num] ) _mbHandlers[mb_num](msg);
  if ( _mainHandler ) _mainHandler(msg);
}

FCTP_FUNC void FCTP_OPT::struct2queueRx(const CAN_message_t &msg) {
  CANListener *thisListener;
  CAN_message_t cl = msg;
  for (uint8_t listenerPos = 0; listenerPos < SIZE_LISTENERS; listenerPos++) {
    thisListener = listener[listenerPos];
    if (thisListener != nullptr) {
      if (thisListener->callbacksActive & (1UL << cl.mb)) thisListener->frameHandler (cl, cl.mb, cl.bus);
      if (thisListener->generalCallbackActive) thisListener->frameHandler (cl, -1, cl.bus);
    }
  }
  if ( !isEventsUsed ) {
    mbCallbacks((FLEXCAN_MAILBOX)msg.mb, msg);	
    return;	
  }
  uint8_t buf[sizeof(CAN_message_t)];
  memmove(buf, &msg, sizeof(msg));
  rxBuffer.push_back(buf, sizeof(CAN_message_t));
}

FCTP_FUNC void FCTP_OPT::flexcan_interrupt() {
  CAN_message_t msg; // setup a temporary storage buffer
  uint64_t imask = readIMASK(), iflag = readIFLAG();

  if ( !(FLEXCANb_MCR(_bus) & (1UL << 15)) ) { /* if DMA is disabled, ONLY THEN you can handle FIFO in ISR */
    if ( (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN) && (imask & FLEXCAN_IMASK1_BUF5M) && (iflag & FLEXCAN_IFLAG1_BUF5I) ) { /* FIFO is enabled, capture frames if triggered */
      volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80 + (0 * 0x10)));
      uint32_t code = mbxAddr[0];
      msg.len = (code & 0xF0000) >> 16;
      msg.flags.remote = (bool)(code & (1UL << 20));
      msg.flags.extended = (bool)(code & (1UL << 21));
      msg.timestamp = code & 0xFFFF;
      msg.id = (mbxAddr[1] & 0x1FFFFFFF) >> ((msg.flags.extended) ? 0 : 18);
      msg.idhit = code >> 23;
      for ( uint8_t i = 0; i < (8 >> 2); i++ ) for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[(4 * i) + 3 - d] = (uint8_t)(mbxAddr[2 + i] >> (8 * d));
      msg.bus = busNumber;
      msg.mb = FIFO; /* store the mailbox the message came from (for callback reference) */
      (void)FLEXCANb_TIMER(_bus);
      writeIFLAGBit(5); /* clear FIFO bit only! */
      if ( iflag & FLEXCAN_IFLAG1_BUF6I ) writeIFLAGBit(6); /* clear FIFO bit only! */
      if ( iflag & FLEXCAN_IFLAG1_BUF7I ) writeIFLAGBit(7); /* clear FIFO bit only! */
      frame_distribution(msg);
      ext_output1(msg);
      ext_output2(msg);
      ext_output3(msg);
      if (fifo_filter_match(msg.id)) struct2queueRx(msg);
    }
  }

  uint8_t exit_point = 64 - __builtin_clzll(iflag | 1); /* break from MSB's if unset, add 1 to prevent undefined behaviour in clz for 0 check */
  for ( uint8_t mb_num = mailboxOffset(); mb_num < FLEXCANb_MAXMB_SIZE(_bus); mb_num++ ) {
    if ( mb_num >= exit_point ) break; /* early exit from higher unflagged mailboxes */
    if (!(imask & (1ULL << mb_num))) continue; /* don't read non-interrupt mailboxes */
    if (!(iflag & (1ULL << mb_num))) continue; /* don't read unflagged mailboxes */
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80 + (mb_num * 0x10)));
    uint32_t code = mbxAddr[0];
    if ( ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_FULL ) ||
         ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) ) {
      msg.flags.extended = (bool)(code & (1UL << 21));
      msg.id = (mbxAddr[1] & 0x1FFFFFFF) >> ((msg.flags.extended) ? 0 : 18);
      if ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) msg.flags.overrun = 1;
      msg.len = (code & 0xF0000) >> 16;
      msg.mb = mb_num;
      msg.timestamp = code & 0xFFFF;
      msg.bus = busNumber;
      for ( uint8_t i = 0; i < (8 >> 2); i++ ) for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[(4 * i) + 3 - d] = (uint8_t)(mbxAddr[2 + i] >> (8 * d));
      mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | ((msg.flags.extended) ? (FLEXCAN_MB_CS_SRR | FLEXCAN_MB_CS_IDE) : 0);
      (void)FLEXCANb_TIMER(_bus);
      writeIFLAGBit(mb_num);
      if ( filter_match((FLEXCAN_MAILBOX)mb_num, msg.id) ) struct2queueRx(msg); /* store frame in queue */
      frame_distribution(msg);
      ext_output1(msg);
      ext_output2(msg);
      ext_output3(msg);
    }

    else if ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_EMPTY ) {
      /* there are no flags for EMPTY reception boxes, however, when sending remote
         frames, the mailboxes switch to RX_EMPTY and trigger the flag */
      if (!(iflag & (1ULL << mb_num))) continue; /* only process the flagged RX_EMPTY mailboxes */

      msg.flags.extended = (bool)(code & (1UL << 21));
      msg.id = (mbxAddr[1] & 0x1FFFFFFF) >> ((msg.flags.extended) ? 0 : 18);
      if ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) msg.flags.overrun = 1;
      msg.len = (code & 0xF0000) >> 16;
      msg.mb = mb_num;
      msg.timestamp = code & 0xFFFF;
      msg.bus = busNumber;
      for ( uint8_t i = 0; i < (8 >> 2); i++ ) for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[(4 * i) + 3 - d] = (uint8_t)(mbxAddr[2 + i] >> (8 * d));
      if ( mb_num == FIFO ) {
        if ( _mbTxHandlers[0] ) _mbTxHandlers[0](msg);
        if ( _mainTxHandler ) _mainTxHandler(msg);
      }
      else {
        if ( _mbTxHandlers[mb_num] ) _mbTxHandlers[mb_num](msg);
        if ( _mainTxHandler ) _mainTxHandler(msg);
      }

      if ( txBuffer.size() ) {
        CAN_message_t frame;
        uint8_t buf[sizeof(CAN_message_t)];
        txBuffer.peek_front(buf, sizeof(CAN_message_t));
        memmove(&frame, buf, sizeof(frame));
        if ( frame.mb == -1 ) {
          writeTxMailbox(mb_num, frame);
          txBuffer.pop_front();
        }
        else if ( frame.mb == mb_num ) {
          writeTxMailbox(frame.mb, frame);
          txBuffer.pop_front();
        }
      }
      else {
        writeIFLAGBit(mb_num); /* just clear IFLAG if no TX queues exist */
        mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE); /* set it back to a TX mailbox */
      }
    }

    else if ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_TX_INACTIVE ) {
      msg.flags.extended = (bool)(code & (1UL << 21));
      msg.id = (mbxAddr[1] & 0x1FFFFFFF) >> ((msg.flags.extended) ? 0 : 18);
      if ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) msg.flags.overrun = 1;
      msg.len = (code & 0xF0000) >> 16;
      msg.mb = mb_num;
      msg.timestamp = code & 0xFFFF;
      msg.bus = busNumber;
      for ( uint8_t i = 0; i < (8 >> 2); i++ ) for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[(4 * i) + 3 - d] = (uint8_t)(mbxAddr[2 + i] >> (8 * d));
      if ( mb_num == FIFO ) {
        if ( _mbTxHandlers[0] ) _mbTxHandlers[0](msg);
        if ( _mainTxHandler ) _mainTxHandler(msg);
      }
      else {
        if ( _mbTxHandlers[mb_num] ) _mbTxHandlers[mb_num](msg);
        if ( _mainTxHandler ) _mainTxHandler(msg);
      }

      if ( txBuffer.size() ) {
        CAN_message_t frame;
        uint8_t buf[sizeof(CAN_message_t)];
        txBuffer.peek_front(buf, sizeof(CAN_message_t));
        memmove(&frame, buf, sizeof(frame));
        if ( frame.mb == -1 ) {
          writeTxMailbox(mb_num, frame);
          txBuffer.pop_front();
        }
        else if ( frame.mb == mb_num ) {
          writeTxMailbox(frame.mb, frame);
          txBuffer.pop_front();
        }
      }
      else {
        writeIFLAGBit(mb_num); /* just clear IFLAG if no TX queues exist */
      }
    }
  }

  uint32_t esr1 = FLEXCANb_ESR1(_bus);
  static uint32_t last_esr1 = 0;
  if ( (last_esr1 & 0x7FFBF) != (esr1 & 0x7FFBF) ) {
    if ( busESR1.size() < busESR1.capacity() ) {
      busESR1.write(esr1);
      busECR.write(FLEXCANb_ECR(_bus));
      last_esr1 = esr1;
    }
  }
  FLEXCANb_ESR1(_bus) |= esr1;

  asm volatile ("dsb");	
}

FCTP_FUNC bool FCTP_OPT::error(CAN_error_t &error, bool printDetails) {
  if ( !busESR1.size() ) return 0;
  NVIC_DISABLE_IRQ(nvicIrq);
  error.ESR1 = busESR1.read();
  error.ECR = busECR.read();

  if ( (error.ESR1 & 0x400C8) == 0x40080 ) strncpy((char*)error.state, "Idle", (sizeof(error.state) - 1));
  else if ( (error.ESR1 & 0x400C8) == 0x0 ) strncpy((char*)error.state, "Not synchronized to CAN bus", (sizeof(error.state) - 1));
  else if ( (error.ESR1 & 0x400C8) == 0x40040 ) strncpy((char*)error.state, "Transmitting", (sizeof(error.state) - 1));
  else if ( (error.ESR1 & 0x400C8) == 0x40008 ) strncpy((char*)error.state, "Receiving", (sizeof(error.state) - 1));

  error.BIT1_ERR = (error.ESR1 & (1UL << 15)) ? 1 : 0;
  error.BIT0_ERR = (error.ESR1 & (1UL << 14)) ? 1 : 0;
  error.ACK_ERR = (error.ESR1 & (1UL << 13)) ? 1 : 0;
  error.CRC_ERR = (error.ESR1 & (1UL << 12)) ? 1 : 0;
  error.FRM_ERR = (error.ESR1 & (1UL << 11)) ? 1 : 0;
  error.STF_ERR = (error.ESR1 & (1UL << 10)) ? 1 : 0;
  error.TX_WRN = (error.ESR1 & (1UL << 9)) ? 1 : 0;
  error.RX_WRN = (error.ESR1 & (1UL << 8)) ? 1 : 0;

  if ( (error.ESR1 & 0x30) == 0x0 ) strncpy((char*)error.FLT_CONF, "Error Active", (sizeof(error.FLT_CONF) - 1));
  else if ( (error.ESR1 & 0x30) == 0x1 ) strncpy((char*)error.FLT_CONF, "Error Passive", (sizeof(error.FLT_CONF) - 1));
  else strncpy((char*)error.FLT_CONF, "Bus off", (sizeof(error.FLT_CONF) - 1));

  error.RX_ERR_COUNTER = (uint8_t)(error.ECR >> 8);
  error.TX_ERR_COUNTER = (uint8_t)error.ECR;

  if ( printDetails ) printErrors(error);
  NVIC_ENABLE_IRQ(nvicIrq);
  return 1;
}

FCTP_FUNC void FCTP_OPT::printErrors(const CAN_error_t &error) {
  Serial.print("FlexCAN State: "); Serial.print((char*)error.state);
  if ( error.BIT1_ERR ) Serial.print(", BIT1_ERR");
  if ( error.BIT0_ERR ) Serial.print(", BIT0_ERR");
  if ( error.ACK_ERR ) Serial.print(", ACK_ERR");
  if ( error.CRC_ERR ) Serial.print(", CRC_ERR");
  if ( error.FRM_ERR ) Serial.print(", FRM_ERR");
  if ( error.STF_ERR ) Serial.print(", STF_ERR");
  if ( error.RX_WRN ) Serial.printf(", RX_WRN: %d", error.RX_ERR_COUNTER);
  if ( error.TX_WRN ) Serial.printf(", TX_WRN: %d", error.TX_ERR_COUNTER);
  Serial.printf(", FLT_CONF: %s\n", (char*)error.FLT_CONF);
}

FCTP_FUNC void FCTP_OPT::enableDMA(bool state) { /* only CAN3 supports this on 1062, untested */
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  ( !state ) ? FLEXCANb_MCR(_bus) &= ~0x8000 : FLEXCANb_MCR(_bus) |= 0x8000;
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC uint8_t FCTP_OPT::setRFFN(FLEXCAN_RFFN_TABLE rffn) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  FLEXCAN_set_rffn(FLEXCANb_CTRL2(_bus), rffn);
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  uint32_t remaining_mailboxes = FLEXCANb_MAXMB_SIZE(_bus) - 6 /* MAXMB - FIFO */ - ((((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 2);
  if ( FLEXCANb_MAXMB_SIZE(_bus) < (6 + ((((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 2))) remaining_mailboxes = 0;
  return constrain((uint8_t)(FLEXCANb_MAXMB_SIZE(_bus) - remaining_mailboxes), 0, 32);
}

FCTP_FUNC void FCTP_OPT::setFIFOFilter(const FLEXCAN_FLTEN &input) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return; /* FIFO not enabled. */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  for (uint8_t i = 0; i < max_fifo_filters; i++) { /* block all ID's so filtering could be applied. */
    if ( input == REJECT_ALL ) {
      if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) { /* If Table A is chosen for FIFO */
        FLEXCANb_IDFLT_TAB(_bus, i) = 0xFFFFFFFF; /* reset id */
        /* individual masks (RXIMR) will just cover Rx FIFO filters in 0-31 range, and filters 32-127
           will use RXFGMASK. */ 
        if ( i < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, i) = 0x3FFFFFFF; // (RXIMR) /* block all id's (0-31) */
        FLEXCANb_RXFGMASK(_bus) = 0x3FFFFFFF;
      }
      else if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 1 ) { /* If Table B is chosen for FIFO */
        FLEXCANb_IDFLT_TAB(_bus, i) = 0xFFFFFFFF; /* reset id */
        if ( i < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, i) = 0x7FFF7FFF; // (RXIMR) /* block all id's (0-31) */
        FLEXCANb_RXFGMASK(_bus) = 0x7FFF7FFF;
      }
      else if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 2 ) { /* If Table C is chosen for FIFO */
       /* TO BE DONE */ //FLEXCANb_IDFLT_TAB(_bus, i) = 0x6E6E6E6E; /* reset id */
        //FLEXCANb_RXIMR(_bus, i) = 0xFFFFFFFF; // (RXIMR) /* block all id's */
      }
    }
    else if ( input == ACCEPT_ALL ) {
      FLEXCANb_IDFLT_TAB(_bus, i) = 0; /* reset id */
      if ( i < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, i) = 0; // (RXIMR) /* allow all id's */
      FLEXCANb_RXFGMASK(_bus) = 0; /* for masks above IDF 0->31, global is used for rest) */
    }
  }
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC bool FCTP_OPT::setFIFOManualFilter(uint8_t filter, uint32_t id1, uint32_t mask, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* ##################################### TABLE A ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) {
    if (ide != EXT) {
      mask = mask << 19 | 0xC0000001;
    } else {
      mask = mask << 1 | 0xC0000001;
    }
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((remote == RTR ? 1 : 0) << 31) |
        ((ide == EXT ? ((id1 & FLEXCAN_MB_ID_EXT_MASK) << 1) : (FLEXCAN_MB_ID_IDSTD(id1) << 1)));
    if ( filter < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, filter) = mask;// | ((filter < (max_fifo_filters / 2)) ? 0 : (1UL << 30)); // (RXIMR)
    FLEXCANb_RXFGMASK(_bus) = mask;//0x3FFFFFFF; /* enforce it for blocks 32->127, single IDs */
    fifo_filter_table[filter][0] = ( ((ide == EXT) ? 1UL : 0UL) << 16); /* extended flag check */
    fifo_filter_store(FLEXCAN_MULTI, filter, 1, id1, 0, 0, 0, 0);
  }
  /* #################################################################################### */
  /* ##################################### TABLE B ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 1 ) {
    return false;
  }

  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOFilter(uint8_t filter, uint32_t id1, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* ##################################### TABLE A ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) {
    uint32_t mask = ( ide != EXT ) ? ((((id1) ^ (id1)) ^ 0x7FF) << 19 ) | 0xC0000001 : ((((id1) ^ (id1)) ^ 0x1FFFFFFF) << 1 ) | 0xC0000001;
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((remote == RTR ? 1 : 0) << 31) |
        ((ide == EXT ? ((id1 & FLEXCAN_MB_ID_EXT_MASK) << 1) : (FLEXCAN_MB_ID_IDSTD(id1) << 1)));
    if ( filter < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, filter) = mask;// | ((filter < (max_fifo_filters / 2)) ? 0 : (1UL << 30)); // (RXIMR)
    FLEXCANb_RXFGMASK(_bus) = 0x3FFFFFFF; /* enforce it for blocks 32->127, single IDs */

    fifo_filter_table[filter][0] = ( ((ide == EXT) ? 1UL : 0UL) << 16); /* extended flag check */
    fifo_filter_store(FLEXCAN_MULTI, filter, 1, id1, 0, 0, 0, 0);
  }
  /* #################################################################################### */
  /* ##################################### TABLE B ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 1 ) {
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((ide == EXT ? 1 : 0) << 14) | /* STD IDs / EXT IDs */
        ((remote == RTR ? 1 : 0) << 31) | ((remote == RTR ? 1 : 0) << 15) | /* remote frames */
        (ide == EXT ? ((id1 >> (29 - 14)) << 16) : ((id1 & 0x7FF) << 19)) | /* first ID is EXT or STD? */
        (ide == EXT ? ((id1 >> (29 - 14)) <<  0) : ((id1 & 0x7FF) <<  3)) ; /* second ID is EXT or STD? */
    uint32_t mask = ( ide != EXT ) ? ((((id1) ^ (id1)) ^ 0x7FF) << 19 ) | ((remote == RTR)?(1UL<<31):0UL) : ((((id1) ^ (id1)) ^ 0x1FFFFFFF) << 16 ) | ((remote == RTR)?(1UL<<31):0UL);
    mask |= (( ide != EXT ) ? ((((id1) ^ (id1)) ^ 0x7FF) << 3 )  | ((remote == RTR)?(1UL<<15):0UL) : ((((id1) ^ (id1)) ^ 0x1FFFFFFF) << 0 )  | ((remote == RTR)?(1UL<<15):0UL) ) & 0xFFFF;
    mask |= (1UL<<30) | (1UL<<14);
    if ( filter < constrain(mailboxOffset(), 0, 32) ) FLEXCANb_RXIMR(_bus, filter) = mask; // (RXIMR)
    FLEXCANb_RXFGMASK(_bus) = 0x7FFF7FFF; /* enforce it for blocks 32->127, single STD IDs / EXT partial matches */
  }

  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC void FCTP_OPT::fifo_filter_store(FLEXCAN_FILTER_TABLE type, uint8_t filter, uint32_t id_count, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5) {
  fifo_filter_table[filter][0] = (fifo_filter_table[filter][0] & 0xF0000) | filter; // first 7 bits reserved for fifo filter
  fifo_filter_table[filter][0] |= (id_count << 7); // we store the quantity of ids after the fifo filter count 
  /* bit 16-19: extended ids */
  /* bit 28: filter enabled */
  fifo_filter_table[filter][0] |= (type << 29); // we reserve 3 upper bits for type
  fifo_filter_table[filter][1] = id1; // id1
  fifo_filter_table[filter][2] = id2; // id2
  fifo_filter_table[filter][3] = id3; // id3
  fifo_filter_table[filter][4] = id4; // id4
  fifo_filter_table[filter][5] = id5; // id5
}

FCTP_FUNC void FCTP_OPT::enhanceFilter(FLEXCAN_MAILBOX mb_num) {
  if ( mb_num == FIFO ) fifo_filter_table[0][0] |= (1UL << 28); /* enable fifo enhancement */
  else mb_filter_table[mb_num][0] |= (1UL << 28); /* enable mb enhancement */
}

FCTP_FUNC volatile bool FCTP_OPT::fifo_filter_match(uint32_t id) {
  if ( !(fifo_filter_table[0][0] & 0x10000000) ) return 1;
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  for (uint8_t mb_num = 0; mb_num < max_fifo_filters; mb_num++) { /* check fifo filters */
    if ( (fifo_filter_table[mb_num][0] >> 29) == FLEXCAN_MULTI ) {
      for ( uint8_t i = 0; i < ((fifo_filter_table[mb_num][0] & 0x380) >> 7); i++) if ( id == fifo_filter_table[mb_num][i+1] ) return 1;
    }
    else if ( (fifo_filter_table[mb_num][0] >> 29) == FLEXCAN_RANGE ) {
      if ( id >= fifo_filter_table[mb_num][1] && id <= fifo_filter_table[mb_num][2] ) return 1;
    }
    else if ( (fifo_filter_table[mb_num][0] >> 29) == FLEXCAN_USERMASK ) {
      for ( uint8_t i = 1; i < ((fifo_filter_table[mb_num][0] & 0x380) >> 7) + 1; i++) {
        if ( (id & fifo_filter_table[mb_num][5]) == (fifo_filter_table[mb_num][i] & fifo_filter_table[mb_num][5]) ) return 1;
      }
    }
  }
  return 0;
}

FCTP_FUNC volatile bool FCTP_OPT::filter_match(FLEXCAN_MAILBOX mb_num, uint32_t id) {
  if ( !(mb_filter_table[mb_num][0] & 0x10000000) ) return 1;
  if ( (mb_filter_table[mb_num][0] >> 29) == FLEXCAN_MULTI ) {
    for ( uint8_t i = 0; i < ((mb_filter_table[mb_num][0] & 0x380) >> 7); i++) if ( id == mb_filter_table[mb_num][i+1] ) return 1;
  }
  else if ( (mb_filter_table[mb_num][0] >> 29) == FLEXCAN_RANGE ) {
    if ( id >= mb_filter_table[mb_num][1] && id <= mb_filter_table[mb_num][2] ) return 1;
  }
  else if ( (mb_filter_table[mb_num][0] >> 29) == FLEXCAN_USERMASK ) {
    for ( uint8_t i = 1; i < ((mb_filter_table[mb_num][0] & 0x380) >> 7) + 1; i++) {
      if ( (id & mb_filter_table[mb_num][5]) == (mb_filter_table[mb_num][i] & mb_filter_table[mb_num][5]) ) return 1;
    }
  }
  return 0;
}

FCTP_FUNC bool FCTP_OPT::setFIFOFilter(uint8_t filter, uint32_t id1, uint32_t id2, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  if ( filter > 31 ) return 0; /* multi-id & ranges are not allowed */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* ##################################### TABLE A ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) {
    uint32_t mask = ( ide != EXT ) ? ((((id1 | id2) ^ (id1 & id2)) ^ 0x7FF) << 19 ) | 0xC0000001 : ((((id1 | id2) ^ (id1 & id2)) ^ 0x1FFFFFFF) << 1 ) | 0xC0000001;
    FLEXCANb_RXIMR(_bus, filter) = mask; // (RXIMR)
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((remote == RTR ? 1 : 0) << 31) |
        ((ide == EXT ? ((id1 & FLEXCAN_MB_ID_EXT_MASK) << 1) : (FLEXCAN_MB_ID_IDSTD(id1) << 1)));

    fifo_filter_table[filter][0] = ( ((ide == EXT) ? 1UL : 0UL) << 16); /* extended flag check */
    fifo_filter_store(FLEXCAN_MULTI, filter, 2, id1, id2, 0, 0, 0);
  }
  /* #################################################################################### */
  /* ##################################### TABLE B ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 1 ) {
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((ide == EXT ? 1 : 0) << 14) | /* STD IDs / EXT IDs */
        ((remote == RTR ? 1 : 0) << 31) | ((remote == RTR ? 1 : 0) << 15) | /* remote frames */
        (ide == EXT ? ((id1 >> (29 - 14)) << 16) : ((id1 & 0x7FF) << 19)) | /* first ID is EXT or STD? */
        (ide == EXT ? ((id2 >> (29 - 14)) <<  0) : ((id2 & 0x7FF) <<  3)) ; /* second ID is EXT or STD? */
    uint32_t mask = ( ide != EXT ) ? ((((id1) ^ (id1)) ^ 0x7FF) << 19 ) | ((remote == RTR)?(1UL<<31):0UL) : ((((id1) ^ (id1)) ^ 0x1FFFFFFF) << 16 ) | ((remote == RTR)?(1UL<<31):0UL);
    mask |= (( ide != EXT ) ? ((((id2) ^ (id2)) ^ 0x7FF) << 3 )  | ((remote == RTR)?(1UL<<15):0UL) : ((((id2) ^ (id2)) ^ 0x1FFFFFFF) << 0 )  | ((remote == RTR)?(1UL<<15):0UL) ) & 0xFFFF;
    mask |= (1UL<<30) | (1UL<<14);
    FLEXCANb_RXIMR(_bus, filter) = mask; // (RXIMR)
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOFilterRange(uint8_t filter, uint32_t id1, uint32_t id2, const FLEXCAN_IDE &ide, const FLEXCAN_IDE &remote) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  if ( filter > 31 ) return 0; /* multi-id & ranges are not allowed */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  if ( id1 > id2 || ((id2 > id1) && (id2 - id1 > 1000)) || !id1 || !id2 ) return 0; /* don't play around... */
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* ##################################### TABLE A ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) {
    uint32_t stage1 = id1, stage2 = id1;
    for ( uint32_t i = id1 + 1; i <= id2; i++ ) {
      stage1 |= i; stage2 &= i;
    }
    uint32_t mask = ( ide != EXT ) ? (((stage1 ^ stage2) ^ 0x7FF) << 19) | 0xC0000001 : (((stage1 ^ stage2) ^ 0x1FFFFFFF) << 1) | 0xC0000001;
    FLEXCANb_RXIMR(_bus, filter) = mask; // (RXIMR)
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide == EXT ? 1 : 0) << 30) | ((remote == RTR ? 1 : 0) << 31) |
        ((ide == EXT ? ((id1 & FLEXCAN_MB_ID_EXT_MASK) << 1) : (FLEXCAN_MB_ID_IDSTD(id1) << 1)));

    fifo_filter_table[filter][0] = ( ((ide == EXT) ? 1UL : 0UL) << 16); /* extended flag check */
    fifo_filter_store(FLEXCAN_RANGE, filter, 2, id1, id2, 0, 0, 0);
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOFilter(uint8_t filter, uint32_t id1, const FLEXCAN_IDE &ide1, const FLEXCAN_IDE &remote1, uint32_t id2, const FLEXCAN_IDE &ide2, const FLEXCAN_IDE &remote2) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  if ( filter > 31 ) return 0; /* multi-id & ranges are not allowed */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* ##################################### TABLE B ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 1 ) {
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide1 == EXT ? 1 : 0) << 30) | ((ide2 == EXT ? 1 : 0) << 14) | /* STD IDs / EXT IDs */
        ((remote1 == RTR ? 1 : 0) << 31) | ((remote2 == RTR ? 1 : 0) << 15) | /* remote frames */
        (ide1 == EXT ? ((id1 >> (29 - 14)) << 16) : ((id1 & 0x7FF) << 19)) | /* first ID is EXT or STD? */
        (ide2 == EXT ? ((id2 >> (29 - 14)) <<  0) : ((id2 & 0x7FF) <<  3)) ; /* second ID is EXT or STD? */
    uint32_t mask = ( ide1 != EXT ) ? ((((id1) ^ (id1)) ^ 0x7FF) << 19 ) | ((remote1 == RTR)?(1UL<<31):0UL) : ((((id1) ^ (id1)) ^ 0x1FFFFFFF) << 16 ) | ((remote1 == RTR)?(1UL<<31):0UL);
    mask |= (( ide2 != EXT ) ? ((((id2) ^ (id2)) ^ 0x7FF) << 3 )  | ((remote2 == RTR)?(1UL<<15):0UL) : ((((id2) ^ (id2)) ^ 0x1FFFFFFF) << 0 )  | ((remote2 == RTR)?(1UL<<15):0UL) ) & 0xFFFF;
    mask |= (1UL<<30) | (1UL<<14);
    FLEXCANb_RXIMR(_bus, filter) = mask; // (RXIMR)
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOFilter(uint8_t filter, uint32_t id1, uint32_t id2, const FLEXCAN_IDE &ide1, const FLEXCAN_IDE &remote1, uint32_t id3, uint32_t id4, const FLEXCAN_IDE &ide2, const FLEXCAN_IDE &remote2) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  if ( filter > 31 ) return 0; /* multi-id & ranges are not allowed */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* ##################################### TABLE B ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 1 ) {
    uint32_t mask = ( ide1 != EXT ) ? ((((id1 | id2) ^ (id1 & id2)) ^ 0x7FF) << 19 ) | ((remote1 == RTR)?(1UL<<31):0UL) : ((((id1 | id2) ^ (id1 & id2)) ^ 0x1FFFFFFF) << 16 ) | ((remote1 == RTR)?(1UL<<31):0UL);
    mask |= (( ide2 != EXT ) ? ((((id3 | id4) ^ (id3 & id4)) ^ 0x7FF) << 3 )  | ((remote2 == RTR)?(1UL<<15):0UL) : ((((id3 | id4) ^ (id3 & id4)) ^ 0x1FFFFFFF) << 0 )  | ((remote2 == RTR)?(1UL<<15):0UL) ) & 0xFFFF;
    mask |= (1UL<<30) | (1UL<<14);
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide1 == EXT ? 1 : 0) << 30) | ((ide2 == EXT ? 1 : 0) << 14) | /* STD IDs / EXT IDs */
        ((remote1 == RTR ? 1 : 0) << 31) | ((remote2 == RTR ? 1 : 0) << 15) | /* remote frames */
        (ide1 == EXT ? ((id1 >> (29 - 14)) << 16) : ((id1 & 0x7FF) << 19)) | /* first ID is EXT or STD? */
        (ide2 == EXT ? ((id3 >> (29 - 14)) << 0 ) : ((id3 & 0x7FF) << 3 ))  ; /* second ID is EXT or STD? */
    FLEXCANb_RXIMR(_bus, filter) = mask; // (RXIMR)
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC bool FCTP_OPT::setFIFOFilterRange(uint8_t filter, uint32_t id1, uint32_t id2, const FLEXCAN_IDE &ide1, const FLEXCAN_IDE &remote1, uint32_t id3, uint32_t id4, const FLEXCAN_IDE &ide2, const FLEXCAN_IDE &remote2) {
  if ( !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN )) return 0; /* FIFO not enabled. */
  if ( filter > 31 ) return 0; /* multi-id & ranges are not allowed */
  uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
  if ( filter >= max_fifo_filters ) return 0;
  if ( id1 > id2 || ((id2 > id1) && (id2 - id1 > 1000)) || !id1 || !id2 ) return 0; /* don't play around... */
  if ( id3 > id4 || ((id4 > id3) && (id4 - id3 > 1000)) || !id3 || !id4 ) return 0; /* don't play around... */
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  /* ##################################### TABLE B ###################################### */
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 1 ) {
    uint32_t stage1 = id1, stage2 = id1;
    for ( uint32_t i = id1 + 1; i <= id2; i++ ) {
      stage1 |= i; stage2 &= i;
    }
    uint32_t mask = ( ide1 != EXT ) ? ((((stage1 | stage2) ^ (stage1 & stage2)) ^ 0x7FF) << 19 ) | ((remote1 == RTR)?(1UL<<31):0UL) : ((((stage1 | stage2) ^ (stage1 & stage2)) ^ 0x1FFFFFFF) << 16 ) | ((remote1 == RTR)?(1UL<<31):0UL);
    stage1 = stage2 = id3;
    for ( uint32_t i = id3 + 1; i <= id4; i++ ) {
      stage1 |= i; stage2 &= i;
    }
    mask |= (( ide2 != EXT ) ? ((((stage1 | stage2) ^ (stage1 & stage2)) ^ 0x7FF) << 3 )  | ((remote2 == RTR)?(1UL<<15):0UL) : ((((stage1 | stage2) ^ (stage1 & stage2)) ^ 0x1FFFFFFF) << 0 ) | ((remote2 == RTR)?(1UL<<15):0UL) ) & 0xFFFF;
    mask |= (1UL<<30) | (1UL<<14);
    FLEXCANb_IDFLT_TAB(_bus, filter) = ((ide1 == EXT ? 1 : 0) << 30) | ((ide2 == EXT ? 1 : 0) << 14) | /* STD IDs / EXT IDs */
        ((remote1 == RTR ? 1 : 0) << 31) | ((remote2 == RTR ? 1 : 0) << 15) | /* remote frames */
        (ide1 == EXT ? ((id1 >> (29 - 14)) << 16) : ((id1 & 0x7FF) << 19)) | /* first ID is EXT or STD? */
        (ide2 == EXT ? ((id3 >> (29 - 14)) << 0 ) : ((id3 & 0x7FF) << 3 ))  ; /* second ID is EXT or STD? */
    FLEXCANb_RXIMR(_bus, filter) = mask; // (RXIMR)
  }
  /* #################################################################################### */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
  return 1;
}

FCTP_FUNC void FCTP_OPT::setFIFOFilterTable(FLEXCAN_FIFOTABLE letter) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_MCR(_bus) = (FLEXCANb_MCR(_bus) & 0xFFFFFCFF) | FLEXCAN_MCR_IDAM(letter);
  if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 0 ) FLEXCANb_RXFGMASK(_bus) = 0x3FFFFFFF;
  else if ( ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_IDAM_MASK) >> FLEXCAN_MCR_IDAM_BIT_NO) == 1 ) FLEXCANb_RXFGMASK(_bus) = 0x7FFF7FFF;
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTP_FUNC void FCTP_OPT::filter_store(FLEXCAN_FILTER_TABLE type, FLEXCAN_MAILBOX mb_num, uint32_t id_count, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5) {
  mb_filter_table[mb_num][0] = mb_num; // first 7 bits reserved for MB
  mb_filter_table[mb_num][0] |= (id_count << 7); // we store the quantity of ids after the mailboxes 
  /* bit 28: filter enabled */
  mb_filter_table[mb_num][0] |= (type << 29); // we reserve 3 upper bits for type
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(_bus + 0x80 + (mb_num * 0x10)));
  mb_filter_table[mb_num][0] |= ( ((mbxAddr[0] & 0x600000) ? 1UL : 0UL) << 27); /* extended flag check */
  mb_filter_table[mb_num][1] = id1; // id1
  mb_filter_table[mb_num][2] = id2; // id2
  mb_filter_table[mb_num][3] = id3; // id3
  mb_filter_table[mb_num][4] = id4; // id4
  mb_filter_table[mb_num][5] = id5; // id5
}

FCTP_FUNC volatile void FCTP_OPT::frame_distribution(CAN_message_t &msg) {
  if ( !distribution ) return; /* distribution not enabled */
  CAN_message_t frame = msg;

  if ( FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FEN ) {
    uint8_t max_fifo_filters = (((FLEXCANb_CTRL2(_bus) >> FLEXCAN_CTRL2_RFFN_BIT_NO) & 0xF) + 1) * 8; // 8->128
    for (uint8_t i = 0; i < max_fifo_filters; i++) { /* check fifo filters */
      if ( msg.mb == FIFO ) break; // don't distribute to fifo if fifo was the source
      if ( !(fifo_filter_table[i][0] & 0xE0000000) ) continue; // skip unset filters

      if ( (fifo_filter_table[i][0] >> 29) == FLEXCAN_MULTI ) {
        if ( (bool)(fifo_filter_table[i][0] & (1UL << 16)) != msg.flags.extended ) continue; /* extended flag check */
        for ( uint8_t p = 0; p < ((fifo_filter_table[i][0] & 0x380) >> 7); p++) {
          if ( frame.id == fifo_filter_table[i][p+1] ) {
            frame.mb = FIFO;
            struct2queueRx(frame);
          }
        }
      }
      else if ( (fifo_filter_table[i][0] >> 29) == FLEXCAN_RANGE ) {
        if ( (bool)(fifo_filter_table[i][0] & (1UL << 16)) != msg.flags.extended ) continue; /* extended flag check */
        if ( frame.id >= fifo_filter_table[i][1] && frame.id <= fifo_filter_table[i][2] ) {
          frame.mb = FIFO;
          struct2queueRx(frame);
        }
      }
      else if ( (fifo_filter_table[i][0] >> 29) == FLEXCAN_USERMASK ) {
        for ( uint8_t p = 1; p < ((fifo_filter_table[i][0] & 0x380) >> 7) + 1; p++) {
          if ( (frame.id & fifo_filter_table[i][5]) == (fifo_filter_table[i][p] & fifo_filter_table[i][5]) ) {
            frame.mb = FIFO;
            frame.idhit = i;
            struct2queueRx(frame);
          }
        }
      }
    } /* end of fifo scan */
  } /* end of fifo checking */

  frame.idhit = 0;

  for ( uint8_t i = mailboxOffset(); i < FLEXCANb_MAXMB_SIZE(_bus); i++ ) {
    if ( msg.mb == i ) continue; // don't distribute to same mailbox
    if ( !(mb_filter_table[i][0] & 0xE0000000) ) continue; // skip unset filters
    if ( (bool)(mb_filter_table[i][0] & (1UL << 27)) != msg.flags.extended ) continue; /* extended flag check */
    if ( (mb_filter_table[i][0] >> 29) == FLEXCAN_MULTI ) {
      for ( uint8_t p = 0; p < ((mb_filter_table[i][0] & 0x380) >> 7); p++) {
        if ( frame.id == mb_filter_table[i][p+1] ) {
          frame.mb = i;
          struct2queueRx(frame);
        }
      }
    }
    else if ( (mb_filter_table[i][0] >> 29) == FLEXCAN_RANGE ) {
      if ( frame.id >= mb_filter_table[i][1] && frame.id <= mb_filter_table[i][2] ) {
        frame.mb = i;
        struct2queueRx(frame);
      }
    }
    else if ( (mb_filter_table[i][0] >> 29) == FLEXCAN_USERMASK ) {
      if ( filter_match((FLEXCAN_MAILBOX)i, frame.id) ) {
        frame.mb = i;
        struct2queueRx(frame);
      }
    }
  } /* end of mb scan */
}

FCTP_FUNC void FCTP_OPT::enableLoopBack(bool yes) {	
  FLEXCAN_EnterFreezeMode();	
  if ( yes ) {	
    FLEXCANb_MCR(_bus) &= ~(1UL << 17);	
    FLEXCANb_CTRL1(_bus) |= (1UL << 12);	
  }	
  else {	
    FLEXCANb_MCR(_bus) |= (1UL << 17);	
    FLEXCANb_CTRL1(_bus) &= ~(1UL << 12);	
  }	
  FLEXCAN_ExitFreezeMode();	
}

FCTP_FUNC bool FCTP_OPT::attachObj (CANListener *listener) {
  for (uint8_t i = 0; i < SIZE_LISTENERS; i++) {
    if (this->listener[i] == nullptr) {
      this->listener[i] = listener;
      listener->callbacksActive = 0;
      return true;
    }
  }
  return false;
}

FCTP_FUNC bool FCTP_OPT::detachObj (CANListener *listener) {
  for (uint8_t i = 0; i < SIZE_LISTENERS; i++) {
    if (this->listener[i] == listener) {
      this->listener[i] = nullptr;
      return true;
    }
  }
  return false;
}

extern void __attribute__((weak)) ext_output1(const CAN_message_t &msg);
extern void __attribute__((weak)) ext_output2(const CAN_message_t &msg);
extern void __attribute__((weak)) ext_output3(const CAN_message_t &msg);
