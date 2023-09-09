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
#include <FlexCAN_T4.hpp>
#include "imxrt_flexcan.hpp"
#include "Arduino.h"

static void flexcan_isr_can3fd();

FCTPFD_FUNC FCTPFD_OPT::FlexCAN_T4FD() {
  if ( _bus == CAN3 ) _CAN3 = this;
}

FCTPFD_FUNC void FCTPFD_OPT::setClock(FLEXCAN_CLOCK clock) {
  if ( clock == CLK_OFF ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(3) | CCM_CSCMR2_CAN_CLK_PODF(0);
  if ( clock == CLK_8MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(9);
  if ( clock == CLK_16MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(4);
  if ( clock == CLK_24MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(1) | CCM_CSCMR2_CAN_CLK_PODF(0);
  if ( clock == CLK_20MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(3);
  if ( clock == CLK_30MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(0) | CCM_CSCMR2_CAN_CLK_PODF(1);
  if ( clock == CLK_40MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(1);
  if ( clock == CLK_60MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(0) | CCM_CSCMR2_CAN_CLK_PODF(0);
  if ( clock == CLK_80MHz ) CCM_CSCMR2 = (CCM_CSCMR2 & 0xFFFFFC03) | CCM_CSCMR2_CAN_CLK_SEL(2) | CCM_CSCMR2_CAN_CLK_PODF(0);
}

FCTPFD_FUNC uint32_t FCTPFD_OPT::getClock() {
  const uint8_t clocksrc[4] = {60, 24, 80, 0};
  return clocksrc[(CCM_CSCMR2 & 0x300) >> 8];
}

FCTPFD_FUNC void FCTPFD_OPT::begin() {
  if ( !getClock() ) setClock(CLK_24MHz); /* no clock enabled, enable osc clock */
  CCM_CCGR0 |= CCM_CCGR0_LPUART3(CCM_CCGR_ON); /* hardware bug, FD is unable to operate without an LPUART clock online */
  if ( _bus == CAN3 ) {
    nvicIrq = IRQ_CAN3;
    _VectorsRam[16 + nvicIrq] = flexcan_isr_can3fd;
    CCM_CCGR7 |= 0x3C0;
    busNumber = 3;
  }

  setTX(); setRX();

  FLEXCANb_MCR(_bus) &= ~FLEXCAN_MCR_MDIS; /* enable module */
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_CTRL1(_bus) = FLEXCAN_CTRL_LOM /*| (1UL << 5)*/; /* listen only mode, TSYN */
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_FRZ; /* enable freeze bit */
  while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_LPM_ACK);
  softReset(); /* reset bus */
  while (!(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK));
  FLEXCANb_MCR(_bus) &= ~0x8000; // disable DMA
  FLEXCANb_MCR(_bus) |= (1UL << 16) | (1UL << 11) | (1UL << 17) | 0x3F; // IRMQ, FDEN, SRXDIS, 64MBs
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_AEN; // TX ABORT FEATURE
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_LPRIO_EN; // TX PRIORITY FEATURE
  FLEXCANb_CTRL2(_bus) |= FLEXCAN_CTRL2_RRS | // store remote frames
                                  FLEXCAN_CTRL2_EACEN | /* handles the way filtering works. Library adjusts to whether you use this or not */ 
                                  FLEXCAN_CTRL2_MRP | // mailbox > FIFO priority.
                                  FLEXCAN_CTRL2_ISOCANFDEN;
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_WRN_EN;
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_WAK_MSK;
  FLEXCANb_FDCTRL(_bus) = 0x80008300; // MBDSR0-MBDSR1 = 0; 32MBs each as default, TDCEN, FDRATE(BRS)
  disableFIFO();
  FLEXCAN_ExitFreezeMode();
  NVIC_ENABLE_IRQ(nvicIrq);
}

FCTPFD_FUNC uint8_t FCTPFD_OPT::setRegions(uint8_t size) {
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_FDCTRL(_bus) &= ~0x1B0000; /* clear both memory region bits */
  if ( constrain(size, 8, 64) == 8 ) {
    size = 64;
  }
  else if ( constrain(size, 16, 64) == 16 ) {
    FLEXCANb_FDCTRL(_bus) |= 0x90000;
    size = 42;
  }
  else if ( constrain(size, 32, 64) == 32 ) {
    FLEXCANb_FDCTRL(_bus) |= 0x120000;
    size = 24;
  }
  else {
    FLEXCANb_FDCTRL(_bus) |= 0x1B0000;
    size = 14;
  }
  disableFIFO();
  FLEXCAN_ExitFreezeMode();
  return size;
}

FCTPFD_FUNC uint8_t FCTPFD_OPT::setRegions(uint8_t mbdsr0, uint8_t mbdsr1) {
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_FDCTRL(_bus) &= ~0x1B0000; /* clear both memory region bits */
  if ( constrain(mbdsr0, 8, 64) == 8 ) {
    mbdsr0 = 32;
  }
  else if ( constrain(mbdsr0, 16, 64) == 16 ) {
    FLEXCANb_FDCTRL(_bus) |= 0x10000;
    mbdsr0 = 21;
  }
  else if ( constrain(mbdsr0, 32, 64) == 32 ) {
    FLEXCANb_FDCTRL(_bus) |= 0x20000;
    mbdsr0 = 12;
  }
  else {
    FLEXCANb_FDCTRL(_bus) |= 0x30000;
    mbdsr0 = 7;
  }
  if ( constrain(mbdsr1, 8, 64) == 8 ) {
    mbdsr1 = 32;
  }
  else if ( constrain(mbdsr1, 16, 64) == 16 ) {
    FLEXCANb_FDCTRL(_bus) |= 0x80000;
    mbdsr1 = 21;
  }
  else if ( constrain(mbdsr1, 32, 64) == 32 ) {
    FLEXCANb_FDCTRL(_bus) |= 0x100000;
    mbdsr1 = 12;
  }
  else {
    FLEXCANb_FDCTRL(_bus) |= 0x180000;
    mbdsr1 = 7;
  }
  disableFIFO();
  FLEXCAN_ExitFreezeMode();
  return mbdsr0 + mbdsr1;
}

FCTPFD_FUNC uint32_t FCTPFD_OPT::mailbox_offset(uint8_t mailbox, uint8_t &maxsize) {
  const uint8_t data_size[4] = { 8, 16, 32, 64 };
  const uint8_t mbx_total[4] = { 32, 21, 12, 7 };
  const uint8_t mbx_shift[4] = { 0x10, 0x18, 0x28, 0x48 };
  uint8_t region0 = (FLEXCANb_FDCTRL(_bus) & (3UL << 16)) >> 16;
  uint8_t region1 = (FLEXCANb_FDCTRL(_bus) & (3UL << 19)) >> 19;

  if ( mailbox < mbx_total[region0] ) {
    maxsize = data_size[region0];
    return _bus + 0x80 + (mbx_shift[region0] * mailbox);
  }
  else if ( mailbox < (mbx_total[region0] + mbx_total[region1]) ) {
    maxsize = data_size[region1];
    return _bus + 0x280 + (mbx_shift[region1] * (mailbox - mbx_total[region0]));
  }
  return _bus + 0x80;
}

FCTPFD_FUNC int FCTPFD_OPT::getFirstTxBox() {
  for (uint8_t i = 0, mbsize = 0; i < max_mailboxes(); i++) {
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(i, mbsize)));
    if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) return i; // if TX
  }
  return -1;
}

FCTPFD_FUNC uint8_t FCTPFD_OPT::getFirstTxBoxSize() {
  uint8_t mbsize = 0;
  mailbox_offset(getFirstTxBox(), mbsize);
  return mbsize;
}

FCTPFD_FUNC void FCTPFD_OPT::setBaudRate(FLEXCAN_FDRATES input, FLEXCAN_RXTX listen_only) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_FDCTRL(_bus) = (FLEXCANb_FDCTRL(_bus) & 0xFFFF60FF); /* clear TDC values */
  FLEXCANb_CBT(_bus) &= ~(1UL << 31); /* clear BTE bit to edit CTRL1 register */
  ( listen_only != LISTEN_ONLY ) ? FLEXCANb_CTRL1(_bus) &= ~FLEXCAN_CTRL_LOM : FLEXCANb_CTRL1(_bus) |= FLEXCAN_CTRL_LOM;
  if ( input == CAN_1M_2M ) { /* based on 24MHz and 70% sample point */
    setClock(CLK_24MHz);
    FLEXCANb_FDCTRL(_bus) |= (0x801B8300 & 0x9F00);
    FLEXCANb_FDCBT(_bus) = 0x31423;
    FLEXCANb_CBT(_bus) = 0x800624A6;
  }
  if ( input == CAN_1M_4M ) { /* based on 24MHz and 70% sample point */
    setClock(CLK_24MHz);
    FLEXCANb_FDCTRL(_bus) |= (0x80008300 & 0x9F00);
    FLEXCANb_FDCBT(_bus) = 0x10421;
    FLEXCANb_CBT(_bus) = 0x800624A6;
  }
  if ( input == CAN_1M_6M ) { /* based on 30MHz and 70% sample point */
    setClock(CLK_30MHz);
    FLEXCANb_FDCTRL(_bus) |= (0x80008300 & 0x9F00);
    FLEXCANb_FDCBT(_bus) = 0x401;
    FLEXCANb_CBT(_bus) = 0x80082CE8;
  }
  if ( input == CAN_1M_8M ) { /* based on 40MHz and 70% sample point */
    setClock(CLK_40MHz);
    FLEXCANb_FDCTRL(_bus) |= (0x80008300 & 0x9F00);
    FLEXCANb_FDCBT(_bus) = 0x401;
    FLEXCANb_CBT(_bus) = 0x800B3D4B;
  }
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTPFD_FUNC uint8_t FCTPFD_OPT::max_mailboxes() {
  uint8_t mb_count = 0;
  uint8_t block0 = (FLEXCANb_FDCTRL(_bus) & (3UL << 16)) >> 16;
  uint8_t block1 = (FLEXCANb_FDCTRL(_bus) & (3UL << 19)) >> 19;

  const uint8_t sizes[4] = {32, 21, 12, 7};
  mb_count = sizes[block0] + sizes[block1];

  if ( mb_count > FLEXCANb_MAXMB_SIZE(_bus) ) return FLEXCANb_MAXMB_SIZE(_bus);
  return mb_count;
}

FCTPFD_FUNC int FCTPFD_OPT::write(FLEXCAN_MAILBOX mb_num, const CANFD_message_t &msg) {
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));

  if ( !((FLEXCAN_get_code(mbxAddr[0])) >> 3) ) return 0; /* not a transmit mailbox */
  if ( msg.seq && FLEXCAN_get_code(mbxAddr[0]) != FLEXCAN_MB_CODE_TX_INACTIVE ) return 0; /* non blocking resend sequential frames */
  uint32_t timeout = millis();
  while ( FLEXCAN_get_code(mbxAddr[0]) != FLEXCAN_MB_CODE_TX_INACTIVE ) {
    if ( millis() - timeout > 100 ) return 0;
  }
  writeTxMailbox(mb_num, msg);
  return 1; // transmit entry accepted //
}

FCTPFD_FUNC void FCTPFD_OPT::writeTxMailbox(uint8_t mb_num, const CANFD_message_t &frame) {
  CANFD_message_t msg = frame;
  writeIFLAGBit(mb_num);
  uint8_t mbsize = 0;
  uint32_t code = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
  mbxAddr[1] = (( msg.flags.extended ) ? ( msg.id & FLEXCAN_MB_ID_EXT_MASK ) : FLEXCAN_MB_ID_IDSTD(msg.id));
  if ( msg.flags.extended ) code |= (3UL << 21);
  for ( uint8_t i = 0; i < (mbsize >> 2); i++ ) mbxAddr[2 + i] = (msg.buf[0 + i * 4] << 24) | (msg.buf[1 + i * 4] << 16) | (msg.buf[2 + i * 4] << 8) | msg.buf[3 + i * 4];
  if ( msg.len > mbsize ) msg.len = mbsize;
  code |= len_to_dlc(msg.len) << 16;
  if ( msg.brs ) code |= (1UL << 30); // BRS
  if ( msg.edl ) code |= (1UL << 31); // EDL
  mbxAddr[0] = code | FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_ONCE);
}

FCTPFD_FUNC uint8_t FCTPFD_OPT::len_to_dlc(uint8_t val) {
  if ( val <= 8 );
  else if ( val <= 12 ) val = 9;
  else if ( val <= 16 ) val = 10;
  else if ( val <= 20 ) val = 11;
  else if ( val <= 24 ) val = 12;
  else if ( val <= 32 ) val = 13;
  else if ( val <= 48 ) val = 14;
  else if ( val <= 64 ) val = 15;
  return val;
}

FCTPFD_FUNC bool FCTPFD_OPT::setMB(const FLEXCAN_MAILBOX &mb_num, const FLEXCAN_RXTX &mb_rx_tx, const FLEXCAN_IDE &ide) {
  if ( mb_num >= max_mailboxes() ) return 0;
  writeIMASKBit(mb_num, 0); /* immediately disable mailbox interrupt */
  FLEXCAN_EnterFreezeMode();
  FLEXCANb_RXIMR(_bus, mb_num) = 0UL | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0); // CLEAR MAILBOX MASK
  FLEXCAN_ExitFreezeMode();
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num , mbsize)));
  FLEXCAN_get_code(mbxAddr[0]); // Reading Control Status atomically locks mailbox (if it is RX mode).
  for ( uint8_t i = 0; i < (mbsize >> 2); i++ ) mbxAddr[2 + i] = 0x0; /* clear mailbox data */
  if ( ide == INACTIVE ) mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_INACTIVE);
  else if ( mb_rx_tx == RX ) mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | ((ide != EXT) ? 0 : FLEXCAN_MB_CS_SRR | FLEXCAN_MB_CS_IDE);
  else if ( mb_rx_tx == TX ) mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
  mbxAddr[1] = 0; // clear ID
  FLEXCANb_TIMER(_bus); /* reading timer unlocks individual mailbox */
  writeIFLAGBit(mb_num); /* clear mailbox flag */
  mb_filter_table[mb_num][0] = ( ((mbxAddr[0] & 0x600000) ? 1UL : 0UL) << 27); /* extended flag check */
  return 1;
}

FCTPFD_FUNC uint8_t FCTPFD_OPT::dlc_to_len(uint8_t val) {
  const uint8_t dlcTolen[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
  return dlcTolen[val];
}

FCTPFD_FUNC void FCTPFD_OPT::FLEXCAN_ExitFreezeMode() {
  FLEXCANb_MCR(_bus) &= ~FLEXCAN_MCR_HALT;
  while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
}

FCTPFD_FUNC void FCTPFD_OPT::FLEXCAN_EnterFreezeMode() {
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_FRZ | FLEXCAN_MCR_HALT;
  while (!(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK));
}

FCTPFD_FUNC void FCTPFD_OPT::softReset() {
  FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_SOFT_RST;
  while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_SOFT_RST);
}

FCTPFD_FUNC void FCTPFD_OPT::setTX(FLEXCAN_PINS pin) {
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
  }
}

FCTPFD_FUNC void FCTPFD_OPT::setRX(FLEXCAN_PINS pin) {
  /* DAISY REGISTER CAN3
    00 GPIO_EMC_37_ALT9 â€” Selecting Pad: GPIO_EMC_37 for Mode: ALT9
    01 GPIO_AD_B0_15_ALT8 â€” Selecting Pad: GPIO_AD_B0_15 for Mode: ALT8
    10 GPIO_AD_B0_11_ALT8 â€” Selecting Pad: GPIO_AD_B0_11 for Mode: ALT8
  */
  /* DAISY REGISTER CAN2
    00 GPIO_EMC_10_ALT3 â€” Selecting Pad: GPIO_EMC_10 for Mode: ALT3
    01 GPIO_AD_B0_03_ALT0 â€” Selecting Pad: GPIO_AD_B0_03 for Mode: ALT0
    10 GPIO_AD_B0_15_ALT6 â€” Selecting Pad: GPIO_AD_B0_15 for Mode: ALT6
    11 GPIO_B1_09_ALT6 â€” Selecting Pad: GPIO_B1_09 for Mode: ALT6
  */
  /* DAISY REGISTER CAN1
    00 GPIO_SD_B1_03_ALT4 â€” Selecting Pad: GPIO_SD_B1_03 for Mode: ALT4
    01 GPIO_EMC_18_ALT3 â€” Selecting Pad: GPIO_EMC_18 for Mode: ALT3
    10 GPIO_AD_B1_09_ALT2 â€” Selecting Pad: GPIO_AD_B1_09 for Mode: ALT2
    11 GPIO_B0_03_ALT2 â€” Selecting Pad: GPIO_B0_03 for Mode: ALT2
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
  }
}

FCTPFD_FUNC void FCTPFD_OPT::enableDMA(bool state) { /* only CAN3 supports this on 1062, untested */
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  ( !state ) ? FLEXCANb_MCR(_bus) &= ~0x8000 : FLEXCANb_MCR(_bus) |= 0x8000;
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTPFD_FUNC void FCTPFD_OPT::writeIFLAG(uint64_t value) {
  FLEXCANb_IFLAG2(_bus) = value >> 32;
  FLEXCANb_IFLAG1(_bus) = value;
}

FCTPFD_FUNC void FCTPFD_OPT::writeIFLAGBit(uint8_t mb_num) {
  if ( mb_num < 32 ) FLEXCANb_IFLAG1(_bus) |= (1UL << mb_num);
  else FLEXCANb_IFLAG2(_bus) |= (1UL << (mb_num - 32));
}

FCTPFD_FUNC void FCTPFD_OPT::writeIMASK(uint64_t value) {
  FLEXCANb_IMASK2(_bus) = value >> 32;
  FLEXCANb_IMASK1(_bus) = value;
}

FCTPFD_FUNC void FCTPFD_OPT::writeIMASKBit(uint8_t mb_num, bool set) {
  if ( mb_num < 32 ) (( set ) ? FLEXCANb_IMASK1(_bus) |= (1UL << mb_num) : FLEXCANb_IMASK1(_bus) &= ~(1UL << mb_num));
  else (( set ) ? FLEXCANb_IMASK2(_bus) |= (1UL << (mb_num - 32)) : FLEXCANb_IMASK2(_bus) &= ~(1UL << (mb_num - 32)));
}

static void flexcan_isr_can3fd() {
  if ( _CAN3 ) _CAN3->flexcan_interrupt();
}

FCTPFD_FUNC void FCTPFD_OPT::enableMBInterrupts(bool status) {
  FLEXCAN_EnterFreezeMode();
  for ( uint8_t mb_num = 0, mbsize = 0; mb_num < max_mailboxes(); mb_num++ ) {
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
    if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) continue; // skip TX mailboxes
    enableMBInterrupt((FLEXCAN_MAILBOX)mb_num, status);
  }
  FLEXCAN_ExitFreezeMode();
}

FCTPFD_FUNC void FCTPFD_OPT::enableMBInterrupt(const FLEXCAN_MAILBOX &mb_num, bool status) {
  FLEXCAN_EnterFreezeMode();
  if ( status ) writeIMASKBit(mb_num); /* enable mailbox interrupt */
  else writeIMASKBit(mb_num, 0); /* disable mailbox interrupt */
  FLEXCAN_ExitFreezeMode();
}

FCTPFD_FUNC void FCTPFD_OPT::onReceive(const FLEXCAN_MAILBOX &mb_num, _MBFD_ptr handler) {
  if ( FIFO == mb_num ) {
    _mbHandlers[0] = handler;
    return;
  }
  _mbHandlers[mb_num] = handler;
}

FCTPFD_FUNC void FCTPFD_OPT::onReceive(_MBFD_ptr handler) {
  _mainHandler = handler;
}

FCTPFD_FUNC void FCTPFD_OPT::mbCallbacks(const FLEXCAN_MAILBOX &mb_num, const CANFD_message_t &msg) {
  if ( mb_num == FIFO ) {
    if ( _mbHandlers[0] ) _mbHandlers[0](msg);
    return;
  }
  if ( _mbHandlers[mb_num] ) _mbHandlers[mb_num](msg);
  if ( _mainHandler ) _mainHandler(msg);
}

FCTPFD_FUNC void FCTPFD_OPT::flexcan_interrupt() {
  CANFD_message_t msg; // setup a temporary storage buffer
  uint64_t imask = readIMASK(), iflag = readIFLAG();

  for ( uint8_t mb_num = 0, mbsize = 0; mb_num < max_mailboxes(); mb_num++ ) {
    if (!(imask & (1ULL << mb_num))) continue; /* don't read non-interrupt mailboxes */
    if (!(iflag & (1ULL << mb_num))) continue; /* don't read unflagged mailboxes */
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
    uint32_t code = mbxAddr[0];
    if ( ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_FULL ) ||
         ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) ) {
      msg.flags.extended = (bool)(code & (1UL << 21));
      msg.edl = (bool)(code & (1UL << 31));
      msg.brs = (bool)(code & (1UL << 30));
      msg.esi = (bool)(code & (1UL << 29));
      msg.id = (mbxAddr[1] & 0x1FFFFFFF) >> ((msg.flags.extended) ? 0 : 18);
      if ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) msg.flags.overrun = 1;
      msg.len = dlc_to_len((code & 0xF0000) >> 16);
      msg.mb = mb_num;
      msg.timestamp = code & 0xFFFF;
      msg.bus = busNumber;
      for ( uint8_t i = 0; i < (mbsize >> 2); i++ ) for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[(4 * i) + 3 - d] = (uint8_t)(mbxAddr[2 + i] >> (8 * d));
      mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | ((msg.flags.extended) ? (FLEXCAN_MB_CS_SRR | FLEXCAN_MB_CS_IDE) : 0);
      (void)FLEXCANb_TIMER(_bus);
      writeIFLAGBit(mb_num);
      if ( filter_match((FLEXCAN_MAILBOX)mb_num, msg.id) ) struct2queueRx(msg); /* store frame in queue */
      frame_distribution(msg);
      ext_outputFD1(msg);
      ext_outputFD2(msg);
      ext_outputFD3(msg);
    }
  }
  FLEXCANb_ESR1(_bus) = FLEXCANb_ESR1(_bus);
}

FCTPFD_FUNC void FCTPFD_OPT::struct2queueRx(const CANFD_message_t &msg) {
  if ( !isEventsUsed ) {
    mbCallbacks((FLEXCAN_MAILBOX)msg.mb, msg);	
    return;	
  }
  uint8_t buf[sizeof(CANFD_message_t)];
  memmove(buf, &msg, sizeof(msg));
  rxBuffer.push_back(buf, sizeof(CANFD_message_t));
}

FCTPFD_FUNC void FCTPFD_OPT::struct2queueTx(const CANFD_message_t &msg) {
  uint8_t buf[sizeof(CANFD_message_t)];
  memmove(buf, &msg, sizeof(msg));
  txBuffer.push_back(buf, sizeof(CANFD_message_t));
}

FCTPFD_FUNC int FCTPFD_OPT::write(const CANFD_message_t &msg) {
  if ( msg.seq ) {
    if ( !write((FLEXCAN_MAILBOX)getFirstTxBox(), msg) ) struct2queueTx(msg);
    return 1;
  }
  for (uint8_t i = 0, mbsize = 0; i < max_mailboxes(); i++) {
    if (readIMASK() & (1ULL << i)) continue; /* don't write interrupt enabled mailboxes */
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(i, mbsize)));
    if ( FLEXCAN_get_code(mbxAddr[0]) == FLEXCAN_MB_CODE_TX_INACTIVE ) {
      writeTxMailbox(i, msg);
      return 1; /* transmit entry accepted */
    }
  }
  return 0; /* transmit entry failed, no mailboxes or queues available */
}

FCTPFD_FUNC uint64_t FCTPFD_OPT::events() {
  if ( !isEventsUsed ) isEventsUsed = 1;
  if ( rxBuffer.size() ) {
    CANFD_message_t frame;
    uint8_t buf[sizeof(CANFD_message_t)];
    rxBuffer.pop_front(buf, sizeof(CANFD_message_t));
    memmove(&frame, buf, sizeof(frame));
    if ( _mbHandlers[frame.mb] ) _mbHandlers[frame.mb](frame);
    if ( _mainHandler ) _mainHandler(frame);
  }
  if ( txBuffer.size() ) {
    CANFD_message_t frame;
    uint8_t buf[sizeof(CANFD_message_t)];
    txBuffer.peek_front(buf, sizeof(CANFD_message_t));
    memmove(&frame, buf, sizeof(frame));
    if ( write((FLEXCAN_MAILBOX)getFirstTxBox(), frame) ) txBuffer.pop_front();
  }
  return (uint64_t)(rxBuffer.size() << 12) | txBuffer.size();
}

FCTPFD_FUNC int FCTPFD_OPT::read(CANFD_message_t &msg) {
  return readMB(msg);
}

FCTPFD_FUNC int FCTPFD_OPT::readMB(CANFD_message_t &msg) {
  uint8_t cycle_limit = 3;
  for ( uint8_t mbsize = 0; mailbox_reader_increment <= max_mailboxes(); ++mailbox_reader_increment ) {
    if ( mailbox_reader_increment >= max_mailboxes() ) {
      mailbox_reader_increment = 0;
      if ( !--cycle_limit ) return 0;
    }
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mailbox_reader_increment , mbsize)));
    if ((readIMASK() & (1ULL << mailbox_reader_increment))) continue; /* don't read interrupt enabled mailboxes */
    uint32_t code = mbxAddr[0];
    if ( (FLEXCAN_get_code(code) >> 3) ) continue; /* skip TX mailboxes */
    // if (!(code & 0x600000) && !(readIFLAG() & (1ULL << mailbox_reader_increment))) continue; /* don't read unflagged mailboxes, errata: extended mailboxes iflags do not work in poll mode, must check CS field */
    if ( ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_FULL ) ||
         ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) ) {
      msg.flags.extended = (bool)(code & (1UL << 21));
      msg.edl = (bool)(code & (1UL << 31));
      msg.brs = (bool)(code & (1UL << 30));
      msg.esi = (bool)(code & (1UL << 29));
      msg.id = (mbxAddr[1] & 0x1FFFFFFF) >> ((msg.flags.extended) ? 0 : 18);
      if ( FLEXCAN_get_code(code) == FLEXCAN_MB_CODE_RX_OVERRUN ) msg.flags.overrun = 1;
      msg.len = dlc_to_len((code & 0xF0000) >> 16);
      msg.mb = mailbox_reader_increment++;
      msg.timestamp = code & 0xFFFF;
      msg.bus = busNumber;
      for ( uint8_t i = 0; i < (mbsize >> 2); i++ ) for ( int8_t d = 0; d < 4 ; d++ ) msg.buf[(4 * i) + 3 - d] = (uint8_t)(mbxAddr[2 + i] >> (8 * d));
      mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | ((msg.flags.extended) ? (FLEXCAN_MB_CS_SRR | FLEXCAN_MB_CS_IDE) : 0);
      (void)FLEXCANb_TIMER(_bus); /* Read free-running timer to unlock Rx Message Buffer. */
      writeIFLAGBit(msg.mb);
      frame_distribution(msg);
      if ( filter_match((FLEXCAN_MAILBOX)msg.mb, msg.id) ) return 1;
    }
  } 
  return 0; /* no messages available */
}

FCTPFD_FUNC void FCTPFD_OPT::enableFIFO(bool status) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  writeIMASK(0ULL); // disable all FIFO/MB Interrupts
  FLEXCANb_RXMGMASK(_bus) = FLEXCANb_RXFGMASK(_bus) = 0;
  writeIFLAG(readIFLAG()); // (all bits reset when written back)
  FLEXCANb_MCR(_bus) &= ~0x20008000; /* we disable DMA, Legacy FIFO (we never use this in FD mode!) */
  for (uint8_t i = 0; i < max_mailboxes(); i++ ) FLEXCANb_RXIMR(_bus, i) = 0 | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0); // CLEAR MAILBOX MASKS (RXIMR)
  for (uint8_t i = 0, mbsize = 0; i < max_mailboxes(); i++) { /* clear valid MB codes from memory regions */
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(i, mbsize)));
    mbxAddr[0] = mbxAddr[1] = 0;
  }

  if ( status ) { /* enable FIFO */
    // TODO: FD FIFO, ### WARNING: NXP claims, after being tested to fail despite documented and advertised, that the 1062 does NOT support FD FIFO mode!!!
  }
  else { // FIFO disabled default setup of mailboxes, 0-7 RX, 8-15 TX
    for (uint8_t i = 0, mbsize = 0; i < max_mailboxes(); i++ ) { // clear all mailboxes
      volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(i, mbsize)));
      if ( i < max_mailboxes() / 2 ) {
        mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY) | ((i < (max_mailboxes() / 4)) ? 0 : FLEXCAN_MB_CS_IDE | FLEXCAN_MB_CS_SRR);
        FLEXCANb_RXIMR(_bus, i) = 0UL | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0); // (RXIMR)
      }
      else {
        mbxAddr[0] = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE);
      }
    }
  }
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTPFD_FUNC void FCTPFD_OPT::setMBFilterProcessing(FLEXCAN_MAILBOX mb_num, uint32_t filter_id, uint32_t calculated_mask) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  FLEXCANb_RXIMR(_bus, mb_num) = calculated_mask | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0);
  mbxAddr[1] = ((!(mbxAddr[0] & FLEXCAN_MB_CS_IDE)) ? FLEXCAN_MB_ID_IDSTD(filter_id) : FLEXCAN_MB_ID_IDEXT(filter_id));
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTPFD_FUNC bool FCTPFD_OPT::setMBFilterRange(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2) {
  if ( mb_num >= max_mailboxes() ) return 0; /* mailbox not available */
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) return 0; /* exit on TX mailbox */ 
  if ( id1 > id2 || ((id2 > id1) && (id2-id1>1000)) || !id1 || !id2 ) return 0; /* don't play around... */
  uint32_t stage1 = id1, stage2 = id1;
  for ( uint32_t i = id1 + 1; i <= id2; i++ ) {
    stage1 |= i; stage2 &= i;
  }
  uint32_t mask = ( !(mbxAddr[0] & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD( (stage1 ^ stage2) ^ 0x1FFFFFFF ) : FLEXCAN_MB_ID_IDEXT( (stage1 ^ stage2) ^ 0x1FFFFFFF );
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_RANGE, mb_num, 2, id1, id2, 0, 0, 0, mask);
  return 1;
}

FCTPFD_FUNC void FCTPFD_OPT::filter_store(FLEXCAN_FILTER_TABLE type, FLEXCAN_MAILBOX mb_num, uint32_t id_count, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5, uint32_t mask) {
  mb_filter_table[mb_num][0] = mb_num; // first 7 bits reserved for MB
  mb_filter_table[mb_num][0] |= (id_count << 7); // we store the quantity of ids after the mailboxes 
  /* bit 28: filter enabled */
  mb_filter_table[mb_num][0] |= (type << 29); // we reserve 3 upper bits for type
  uint8_t mbsize;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  mb_filter_table[mb_num][0] |= ( ((mbxAddr[0] & 0x600000) ? 1UL : 0UL) << 27); /* extended flag check */
  mb_filter_table[mb_num][1] = id1; // id1
  mb_filter_table[mb_num][2] = id2; // id2
  mb_filter_table[mb_num][3] = id3; // id3
  mb_filter_table[mb_num][4] = id4; // id4
  mb_filter_table[mb_num][5] = id5; // id5
  mb_filter_table[mb_num][6] = mask; // mask
}

FCTPFD_FUNC void FCTPFD_OPT::enhanceFilter(FLEXCAN_MAILBOX mb_num) {
  if ( !(mb_filter_table[mb_num][0] & 0xE0000000) ) return;
  mb_filter_table[mb_num][0] |= (1UL << 28); /* enable enhancement */
}

FCTPFD_FUNC bool FCTPFD_OPT::filter_match(FLEXCAN_MAILBOX mb_num, uint32_t id) {
  if ( !(mb_filter_table[mb_num][0] & 0x10000000) ) return 1;
  if ( (mb_filter_table[mb_num][0] >> 29) == FLEXCAN_MULTI ) {
    for ( uint8_t i = 0; i < ((mb_filter_table[mb_num][0] & 0x380) >> 7); i++) if ( id == mb_filter_table[mb_num][i+1] ) return 1;
  }
  else if ( (mb_filter_table[mb_num][0] >> 29) == FLEXCAN_RANGE ) {
    if ( id >= mb_filter_table[mb_num][1] && id <= mb_filter_table[mb_num][2] ) return 1;
  }
  return 0;
}

FCTPFD_FUNC void FCTPFD_OPT::frame_distribution(CANFD_message_t &msg) {
  if ( !distribution ) return; /* distribution not enabled */
  CANFD_message_t frame = msg;
  for ( uint8_t i = 0; i < max_mailboxes(); i++ ) {
    if ( frame.mb == i ) continue; // don't distribute to same mailbox
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
  }
}

FCTPFD_FUNC void FCTPFD_OPT::setMBFilter(FLEXCAN_FLTEN input) {
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  for (uint8_t i = 0, mbsize = 0; i < max_mailboxes(); i++) {
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(i, mbsize)));
    if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) continue; /* skip TX mailboxes */
    if ( input == ACCEPT_ALL ) FLEXCANb_RXIMR(_bus, i) = 0UL | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0); // (RXIMR)
    if ( input == REJECT_ALL ) FLEXCANb_RXIMR(_bus, i) = ~0UL; // (RXIMR)
    mbxAddr[1] = 0UL;
    mb_filter_table[i][0] = ( ((mbxAddr[0] & 0x600000) ? 1UL : 0UL) << 27); /* extended flag check */
  }
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTPFD_FUNC void FCTPFD_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, FLEXCAN_FLTEN input) {
  if ( mb_num >= max_mailboxes() ) return; /* mailbox not available */
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) return; /* exit on TX mailbox */ 
  bool frz_flag_negate = !(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
  FLEXCAN_EnterFreezeMode();
  if ( input == ACCEPT_ALL ) FLEXCANb_RXIMR(_bus, mb_num) = 0UL | ((FLEXCANb_CTRL2(_bus) & FLEXCAN_CTRL2_EACEN) ? (1UL << 30) : 0); // (RXIMR)
  if ( input == REJECT_ALL ) FLEXCANb_RXIMR(_bus, mb_num) = ~0UL; // (RXIMR)
  mbxAddr[1] = 0UL;
  mb_filter_table[mb_num][0] = ( ((mbxAddr[0] & 0x600000) ? 1UL : 0UL) << 27); /* extended flag check */
  if ( frz_flag_negate ) FLEXCAN_ExitFreezeMode();
}

FCTPFD_FUNC bool FCTPFD_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1) {
  if ( mb_num >= max_mailboxes() ) return 0; /* mailbox not available */
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(mbxAddr[0] & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1) ^ (id1)) ^ 0x7FF) : FLEXCAN_MB_ID_IDEXT(((id1) ^ (id1)) ^ 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 1, id1, 0, 0, 0, 0, mask);
  return 1;
}

FCTPFD_FUNC bool FCTPFD_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2) {
  if ( mb_num >= max_mailboxes() ) return 0; /* mailbox not available */
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(mbxAddr[0] & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1 | id2) ^ (id1 & id2)) ^ 0x7FF) : FLEXCAN_MB_ID_IDEXT(((id1 | id2) ^ (id1 & id2)) ^ 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 2, id1, id2, 0, 0, 0, mask);
  return 1;
}

FCTPFD_FUNC bool FCTPFD_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3) {
  if ( mb_num >= max_mailboxes() ) return 0; /* mailbox not available */
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(mbxAddr[0] & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1 | id2 | id3) ^ (id1 & id2 & id3)) ^ 0x7FF) : FLEXCAN_MB_ID_IDEXT(((id1 | id2 | id3) ^ (id1 & id2 & id3)) ^ 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 3, id1, id2, id3, 0, 0, mask);
  return 1;
}

FCTPFD_FUNC bool FCTPFD_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4) {
  if ( mb_num >= max_mailboxes() ) return 0; /* mailbox not available */
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(mbxAddr[0] & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1 | id2 | id3 | id4) ^ (id1 & id2 & id3 & id4)) ^ 0x7FF) : FLEXCAN_MB_ID_IDEXT(((id1 | id2 | id3 | id4) ^ (id1 & id2 & id3 & id4)) ^ 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 4, id1, id2, id3, id4, 0, mask);
  return 1;
}

FCTPFD_FUNC bool FCTPFD_OPT::setMBFilter(FLEXCAN_MAILBOX mb_num, uint32_t id1, uint32_t id2, uint32_t id3, uint32_t id4, uint32_t id5) {
  if ( mb_num >= max_mailboxes() ) return 0; /* mailbox not available */
  uint8_t mbsize = 0;
  volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(mb_num, mbsize)));
  if ( (FLEXCAN_get_code(mbxAddr[0]) >> 3) ) return 0; /* exit on TX mailbox */ 
  uint32_t mask = ( !(mbxAddr[0] & FLEXCAN_MB_CS_IDE) ) ? FLEXCAN_MB_ID_IDSTD(((id1 | id2 | id3 | id4 | id5) ^ (id1 & id2 & id3 & id4 & id5)) ^ 0x7FF) : FLEXCAN_MB_ID_IDEXT(((id1 | id2 | id3 | id4 | id5) ^ (id1 & id2 & id3 & id4 & id5)) ^ 0x1FFFFFFF);
  setMBFilterProcessing(mb_num,id1,mask);
  filter_store(FLEXCAN_MULTI, mb_num, 5, id1, id2, id3, id4, id5, mask);
  return 1;
}

FCTPFD_FUNC void FCTPFD_OPT::mailboxStatus() {
  Serial.print("FIFO Disabled\n\tMailboxes:\n");
  for ( uint8_t i = 0, mbsize = 0; i < max_mailboxes(); i++ ) {
    volatile uint32_t *mbxAddr = &(*(volatile uint32_t*)(mailbox_offset(i, mbsize)));
    switch ( FLEXCAN_get_code(mbxAddr[0]) ) {
      case 0b0000: {
          Serial.printf("\t\tMB%u%s\n", i, " code: RX_INACTIVE");
          break;
        }
      case 0b0100: {
          Serial.printf("\t\tMB%u%s%s\n", i, " code: RX_EMPTY",((mbxAddr[0] & FLEXCAN_MB_CS_IDE)?"\t(Extended Frame)":"\t(Standard Frame)"));
          break;
        }
      case 0b0010: {
          Serial.printf("\t\tMB%u%s\n", i, " code: RX_FULL");
          break;
        }
      case 0b0110: {
          Serial.printf("\t\tMB%u%s\n", i, " code: RX_OVERRUN");
          break;
        }
      case 0b1010: {
          Serial.printf("\t\tMB%u%s\n", i, " code: RX_ANSWER");
          break;
        }
      case 0b0001: {
          Serial.printf("\t\tMB%u%s\n", i, " code: RX_BUSY");
          break;
        }
      case 0b1000: {
          Serial.printf("\t\tMB%u%s\n", i, " code: TX_INACTIVE");
          break;
        }
      case 0b1001: {
          Serial.printf("\t\tMB%u%s\n", i, " code: TX_ABORT");
          break;
        }
      case 0b1100: {
          Serial.printf("\t\tMB%u%s", i, " code: TX_DATA (Transmitting)");
          uint32_t extid = (mbxAddr[0] & FLEXCAN_MB_CS_IDE);
          (extid) ? Serial.print("(Extended Frame)") : Serial.print("(Standard Frame)");
          uint32_t dataIn = mbxAddr[2];
          uint32_t id = (FLEXCANb_MBn_ID(_bus, i) & FLEXCAN_MB_ID_EXT_MASK);
          if (!extid) id >>= FLEXCAN_MB_ID_STD_BIT_NO;
          Serial.print("(ID: 0x"); Serial.print(id, HEX); Serial.print(")");
          Serial.print("(Payload: "); Serial.print((uint8_t)(dataIn >> 24), HEX);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 16), HEX);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 8), HEX);
          Serial.print(" "); Serial.print((uint8_t)dataIn, HEX);
          dataIn = mbxAddr[3];
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 24), HEX);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 16), HEX);
          Serial.print(" "); Serial.print((uint8_t)(dataIn >> 8), HEX);
          Serial.print(" "); Serial.print((uint8_t)dataIn, HEX);
          Serial.println(")");
          break;
        }
      case 0b1110: {
          Serial.printf("\t\tMB%u%s\n", i, " code: TX_ANSWER");
          break;
        }
    }
  } // for loop
}

extern void __attribute__((weak)) ext_outputFD1(const CANFD_message_t &msg);
extern void __attribute__((weak)) ext_outputFD2(const CANFD_message_t &msg);
extern void __attribute__((weak)) ext_outputFD3(const CANFD_message_t &msg);
