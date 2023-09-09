#include <FlexCAN_T4.hpp>
#include "imxrt_flexcan.hpp"
#include "Arduino.h"

FCTPFD_FUNC bool FCTPFD_OPT::setBaudRateAdvanced(CANFD_timings_t config, uint8_t nominal_choice, uint8_t flexdata_choice, FLEXCAN_RXTX listen_only) {
  return setBaudRate(config, nominal_choice, flexdata_choice, listen_only, 1);
}

FCTPFD_FUNC bool FCTPFD_OPT::setBaudRate(CANFD_timings_t config, FLEXCAN_RXTX listen_only) {
  return setBaudRate(config, 1, 1, listen_only, 0);
}

FCTPFD_FUNC bool FCTPFD_OPT::setBaudRate(CANFD_timings_t config, uint8_t nominal_choice, uint8_t flexdata_choice, FLEXCAN_RXTX listen_only, bool advanced) {
  uint32_t result = 0, result_old = 0;
  double results[10][12] = { { 0 } , { 0 } };

  double baudrate = config.baudrate, propdelay = config.propdelay, bus_length = config.bus_length, req_smp = config.sample;
  double cpi_clock = config.clock;

  baudrate /= 1000;
  double ratio = cpi_clock * 1000 / baudrate;
  double nbt_min = 8, nbt_max = 129, propseg_max = 64, propseg = 0, pseg_1, pseg_2;

  propdelay = (propdelay * 2) + (bus_length * 10);
  uint32_t ipt = 2;
  double temp = 0, pseg1 = 32, pseg2 = 32, rjw = 16, prescaler_min = 1, prescaler_max = 1024;

  if ( advanced ) Serial.println("\n\n      #########################################################################################################################################");
  if ( advanced ) Serial.println("      #\t\t\t\t\t\t\t\t*** CAN NOMINAL CONFIGURATION ***\t\t\t\t\t      #");
  if ( advanced ) Serial.println("      #########################################################################################################################################\n");
  for ( uint32_t prescaler = prescaler_min; prescaler < prescaler_max; prescaler++ ) {
    temp = ratio / prescaler;
    for ( double nbt = nbt_min; nbt < nbt_max; nbt++ ) {
      if ( temp == nbt ) {
        propseg = ceil(propdelay / 1000 * cpi_clock / prescaler);
        if ( propseg <= propseg_max ) {
          for ( double pseg = 1; pseg <= pseg1; pseg++ ) {
            if ( req_smp != 0 ) {
              pseg_1 = round((nbt * req_smp / 100) - 1 - propseg);
              pseg_2 = nbt - 1 - propseg - pseg_1;
              if ( pseg_2 >= ipt && pseg_2 <= pseg2 && pseg_1 > 0 && pseg_1 <= pseg1 ) {
                result = result_old + 1;
                rjw = ( pseg_2 <= rjw ) ? pseg_2 : rjw;
                double error = (double) rjw / 20 / nbt * 100;
                double error1 = (double)( ( pseg_1 <= pseg_2 ) ? pseg_1 : pseg_2 ) / 2 / (13 * nbt - pseg_2) * 100;
                double sclk = (cpi_clock / prescaler);
                double tq = (1000 / (cpi_clock / prescaler));
                double smp = 100 - pseg_2 / nbt * 100;
                results[result - 1][0] = prescaler;
                results[result - 1][1] = sclk;
                results[result - 1][2] = tq;
                results[result - 1][3] = nbt;
                results[result - 1][4] = propseg;
                results[result - 1][5] = pseg_1;
                results[result - 1][6] = pseg_2;
                results[result - 1][7] = rjw;
                results[result - 1][8] = (error <= error1) ? error : error1;
                results[result - 1][9] = smp;
                if ( advanced ) Serial.print("      Prescaler   Sclk(MHz)   Tq(ns)   NBT(#Tq)   Propseg(#Tq)   Pseg1(#Tq)   Pseg2(#Tq)   RJW(#Tq)   ?f(%)   Sample Point(%)     CBT Register\n");
                if ( advanced ) Serial.printf("  %2u)", result);
                if ( advanced ) Serial.printf("      %4u", prescaler);
                if ( advanced ) Serial.printf("     %6.2f", sclk);
                if ( advanced ) Serial.printf("   %7.2f", tq);
                if ( advanced ) Serial.printf("     %2u", (uint32_t)nbt);
                if ( advanced ) Serial.printf("          %2u", (uint32_t)propseg);
                if ( advanced ) Serial.printf("             %2u", (uint32_t)pseg_1);
                if ( advanced ) Serial.printf("            %2u", (uint32_t)pseg_2);
                if ( advanced ) Serial.printf("          %2u", (uint32_t)rjw);
                if ( advanced ) Serial.printf("    %6.2f", ((error <= error1) ? error : error1));
                if ( advanced ) Serial.printf("       %6.2f", smp);
                uint32_t cbt = ((uint32_t)(results[result - 1][4] - 1) << 10); // EPROPSEG
                cbt |= ((uint32_t)(results[result - 1][6] - 1) << 0); // EPSEG2
                cbt |= ((uint32_t)(results[result - 1][5] - 1) << 5); // EPSEG1
                cbt |= ((uint32_t)(results[result - 1][7] - 1) << 16); // ERJW
                cbt |= ((uint32_t)(results[result - 1][0] - 1) << 21); // EPRESDIV
                cbt |= (1UL << 31); // BTF /* use CBT register instead of CTRL1 for FD */
                if ( advanced ) Serial.printf("            0x%8.08X",cbt);

                if ( advanced ) Serial.printf("\n\t%8s%s%.2f%s%u%s%.2f%s", "", "*** Arbitration phase bit time = ", (results[result - 1][2] * results[result - 1][3] / 1000), "us (", (int)results[result - 1][3], "Tq, Tq=", results[result - 1][2], ")\n");
                if ( advanced ) Serial.printf("\t%8s%s%.2f%s", "", "a) Sync: ", results[result - 1][2], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "b) Propseg: ", results[result - 1][2] * results[result - 1][4], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "c) Pseg1: ", results[result - 1][2] * results[result - 1][5], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "d) Pseg2: ", results[result - 1][2] * results[result - 1][6], "ns\n");

                uint32_t scale = (uint32_t)ceil(results[result - 1][2]);
                char _sync[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _sync[i] = 'a';
                _sync[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][4]);
                char _prop[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _prop[i] = 'b';
                _prop[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][5]);
                char _pseg1[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg1[i] = 'c';
                _pseg1[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][6]);
                char _pseg2[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg2[i] = 'd';
                _pseg2[(scale*124/1000)] = '\0';

                if ( advanced ) Serial.printf("%9s%s%-s%s%-s%s%-s%s%-s", "[", _sync, "|", _prop, "|", _pseg1, "|", _pseg2, "]\n");
                if ( advanced ) Serial.printf("%8s%-s%-s%-s%-s%-s%-s", "", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________|");
                if ( advanced ) Serial.printf("\n%8s%u%*s%u%*s%u%*s%u%*s%u%*s%u%*s%u%s", "", 0, 23, "", 200, 22, "", 400, 22, "", 600, 22, "", 800, 22, "", 1000, 22, "", 1200, "(ns)\n\n\n");
              }
              if ( result == result_old ) {
                pseg = nbt - 1 - propseg;
                if ( pseg == 3 && ipt == 2 ) {
                  result = result + 1;
                  double error = (double) rjw / 20 / nbt * 100;
                  double error1 = ( ( pseg_1 <= pseg_2 ) ? (double)pseg_1 : (double)pseg_2 ) / 2 / (13 * nbt - pseg_2) * 100;
                  double sclk = (cpi_clock / prescaler);
                  double tq = (1000 / (cpi_clock / prescaler));
                  double smp = 100 - 2 / nbt * 100;
                  results[result - 1][0] = prescaler;
                  results[result - 1][1] = sclk;
                  results[result - 1][2] = tq;
                  results[result - 1][3] = nbt;
                  results[result - 1][4] = propseg;
                  results[result - 1][5] = 1;
                  results[result - 1][6] = 2;
                  results[result - 1][7] = 1;
                  results[result - 1][8] = (error <= error1) ? error : error1;
                  results[result - 1][9] = smp;
                  if ( advanced ) Serial.print("      Prescaler   Sclk(MHz)   Tq(ns)   NBT(#Tq)   Propseg(#Tq)   Pseg1(#Tq)   Pseg2(#Tq)   RJW(#Tq)   ?f(%)   Sample Point(%)     CBT Register\n");
                  if ( advanced ) Serial.printf("  %2u)", result);
                  if ( advanced ) Serial.printf("      %4u", prescaler);
                  if ( advanced ) Serial.printf("     %6.2f", sclk);
                  if ( advanced ) Serial.printf("   %7.2f", tq);
                  if ( advanced ) Serial.printf("     %2u", (uint32_t)nbt);
                  if ( advanced ) Serial.printf("          %2u", (uint32_t)propseg);
                  if ( advanced ) Serial.printf("             %2u", (uint32_t)1);
                  if ( advanced ) Serial.printf("            %2u", (uint32_t)2);
                  if ( advanced ) Serial.printf("          %2u", (uint32_t)1);
                  if ( advanced ) Serial.printf("    %6.2f", ((error <= error1) ? error : error1));
                  if ( advanced ) Serial.printf("       %6.2f", smp);
                  uint32_t cbt = ((uint32_t)(results[result - 1][4] - 1) << 10); // EPROPSEG
                  cbt |= ((uint32_t)(results[result - 1][6] - 1) << 0); // EPSEG2
                  cbt |= ((uint32_t)(results[result - 1][5] - 1) << 5); // EPSEG1
                  cbt |= ((uint32_t)(results[result - 1][7] - 1) << 16); // ERJW
                  cbt |= ((uint32_t)(results[result - 1][0] - 1) << 21); // EPRESDIV
                  cbt |= (1UL << 31); // BTF /* use CBT register instead of CTRL1 for FD */
                  if ( advanced ) Serial.printf("            0x%8.08X",cbt);

                  if ( advanced ) Serial.printf("\n\t%8s%s%.2f%s%u%s%.2f%s", "", "*** Arbitration phase bit time = ", (results[result - 1][2] * results[result - 1][3] / 1000), "us (", (int)results[result - 1][3], "Tq, Tq=", results[result - 1][2], ")\n");
                  if ( advanced ) Serial.printf("\t%8s%s%.2f%s", "", "a) Sync: ", results[result - 1][2], "ns\t");
                  if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "b) Propseg: ", results[result - 1][2] * results[result - 1][4], "ns\t");
                  if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "c) Pseg1: ", results[result - 1][2] * results[result - 1][5], "ns\t");
                  if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "d) Pseg2: ", results[result - 1][2] * results[result - 1][6], "ns\n");

                  uint32_t scale = (uint32_t)ceil(results[result - 1][2]);
                  char _sync[(scale*124/1000) + 1] = "";
                  for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _sync[i] = 'a';
                  _sync[(scale*124/1000)] = '\0';

                  scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][4]);
                  char _prop[(scale*124/1000) + 1] = "";
                  for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _prop[i] = 'b';
                  _prop[(scale*124/1000)] = '\0';

                  scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][5]);
                  char _pseg1[(scale*124/1000) + 1] = "";
                  for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg1[i] = 'c';
                  _pseg1[(scale*124/1000)] = '\0';

                  scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][6]);
                  char _pseg2[(scale*124/1000) + 1] = "";
                  for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg2[i] = 'd';
                  _pseg2[(scale*124/1000)] = '\0';

                  if ( advanced ) Serial.printf("%9s%s%-s%s%-s%s%-s%s%-s", "[", _sync, "|", _prop, "|", _pseg1, "|", _pseg2, "]\n");
                  if ( advanced ) Serial.printf("%8s%-s%-s%-s%-s%-s%-s", "", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________|");
                  if ( advanced ) Serial.printf("\n%8s%u%*s%u%*s%u%*s%u%*s%u%*s%u%*s%u%s", "", 0, 23, "", 200, 22, "", 400, 22, "", 600, 22, "", 800, 22, "", 1000, 22, "", 1200, "(ns)\n\n\n");
                }
                if ( pseg > (ipt + 1) && pseg <= (pseg1 * 2) ) {
                  if ( (uint32_t)pseg % 2 != 0 ) propseg++;
                  pseg = (nbt - 1 - propseg) / 2;
                  result = result + 1;
                  rjw = ( pseg <= rjw ) ? pseg : rjw;
                  double pseg_val = nbt - 1 - propseg - pseg;
                  double error = (double)rjw / 20 / nbt * 100;
                  double error1 = (((pseg <= pseg_val) ? (double)pseg : (double)pseg_val ) / 2 / (13 * nbt - pseg_val) * 100);
                  double sclk = (cpi_clock / prescaler);
                  double tq = (1000 / (cpi_clock / prescaler));
                  double smp = 100 - pseg / nbt * 100;
                  results[result - 1][0] = prescaler;
                  results[result - 1][1] = sclk;
                  results[result - 1][2] = tq;
                  results[result - 1][3] = nbt;
                  results[result - 1][4] = propseg;
                  results[result - 1][5] = pseg;
                  results[result - 1][6] = pseg_val;
                  results[result - 1][7] = rjw;
                  results[result - 1][8] = (error <= error1) ? error : error1;
                  results[result - 1][9] = smp;
                  if ( advanced ) Serial.print("      Prescaler   Sclk(MHz)   Tq(ns)   NBT(#Tq)   Propseg(#Tq)   Pseg1(#Tq)   Pseg2(#Tq)   RJW(#Tq)   ?f(%)   Sample Point(%)     CBT Register\n");
                  if ( advanced ) Serial.printf("  %2u)", result);
                  if ( advanced ) Serial.printf("      %4u", prescaler);
                  if ( advanced ) Serial.printf("     %6.2f", sclk);
                  if ( advanced ) Serial.printf("   %7.2f", tq);
                  if ( advanced ) Serial.printf("     %2u", (uint32_t)nbt);
                  if ( advanced ) Serial.printf("          %2u", (uint32_t)propseg);
                  if ( advanced ) Serial.printf("             %2u", (uint32_t)pseg);
                  if ( advanced ) Serial.printf("            %2u", (uint32_t)pseg_val);
                  if ( advanced ) Serial.printf("          %2u", (uint32_t)rjw);
                  if ( advanced ) Serial.printf("    %6.2f", ((error <= error1) ? error : error1));
                  if ( advanced ) Serial.printf("       %6.2f", smp);
                  uint32_t cbt = ((uint32_t)(results[result - 1][4] - 1) << 10); // EPROPSEG
                  cbt |= ((uint32_t)(results[result - 1][6] - 1) << 0); // EPSEG2
                  cbt |= ((uint32_t)(results[result - 1][5] - 1) << 5); // EPSEG1
                  cbt |= ((uint32_t)(results[result - 1][7] - 1) << 16); // ERJW
                  cbt |= ((uint32_t)(results[result - 1][0] - 1) << 21); // EPRESDIV
                  cbt |= (1UL << 31); // BTF /* use CBT register instead of CTRL1 for FD */
                  if ( advanced ) Serial.printf("            0x%8.08X",cbt);

                  if ( advanced ) Serial.printf("\n\t%8s%s%.2f%s%u%s%.2f%s", "", "*** Arbitration phase bit time = ", (results[result - 1][2] * results[result - 1][3] / 1000), "us (", (int)results[result - 1][3], "Tq, Tq=", results[result - 1][2], ")\n");
                  if ( advanced ) Serial.printf("\t%8s%s%.2f%s", "", "a) Sync: ", results[result - 1][2], "ns\t");
                  if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "b) Propseg: ", results[result - 1][2] * results[result - 1][4], "ns\t");
                  if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "c) Pseg1: ", results[result - 1][2] * results[result - 1][5], "ns\t");
                  if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "d) Pseg2: ", results[result - 1][2] * results[result - 1][6], "ns\n");

                  uint32_t scale = (uint32_t)ceil(results[result - 1][2]);
                  char _sync[(scale*124/1000) + 1] = "";
                  for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _sync[i] = 'a';
                  _sync[(scale*124/1000)] = '\0';

                  scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][4]);
                  char _prop[(scale*124/1000) + 1] = "";
                  for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _prop[i] = 'b';
                  _prop[(scale*124/1000)] = '\0';

                  scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][5]);
                  char _pseg1[(scale*124/1000) + 1] = "";
                  for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg1[i] = 'c';
                  _pseg1[(scale*124/1000)] = '\0';

                  scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][6]);
                  char _pseg2[(scale*124/1000) + 1] = "";
                  for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg2[i] = 'd';
                  _pseg2[(scale*124/1000)] = '\0';

                  if ( advanced ) Serial.printf("%9s%s%-s%s%-s%s%-s%s%-s", "[", _sync, "|", _prop, "|", _pseg1, "|", _pseg2, "]\n");
                  if ( advanced ) Serial.printf("%8s%-s%-s%-s%-s%-s%-s", "", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________|");
                  if ( advanced ) Serial.printf("\n%8s%u%*s%u%*s%u%*s%u%*s%u%*s%u%*s%u%s", "", 0, 23, "", 200, 22, "", 400, 22, "", 600, 22, "", 800, 22, "", 1000, 22, "", 1200, "(ns)\n\n\n");
                }
              }
              result_old = result;
              break;
            }
          }
        }
      }
    }
  }
  if ( advanced ) Serial.println("\n\n\n");
  if ( nominal_choice && flexdata_choice ) {
    if ( nominal_choice > result ) {
      if ( advanced ) Serial.println("\n\t*** ERROR: Nominal rate selection out of bounds!\n");
      return 0;
    }
    uint32_t cbt = ((uint32_t)(results[nominal_choice - 1][4] - 1) << 10); // EPROPSEG
    cbt |= ((uint32_t)(results[nominal_choice - 1][6] - 1) << 0); // EPSEG2
    cbt |= ((uint32_t)(results[nominal_choice - 1][5] - 1) << 5); // EPSEG1
    cbt |= ((uint32_t)(results[nominal_choice - 1][7] - 1) << 16); // ERJW
    cbt |= ((uint32_t)(results[nominal_choice - 1][0] - 1) << 21); // EPRESDIV
    cbt |= (1UL << 31); // BTF
    uint32_t fdcbt_setting = setBaudRateFD(config, flexdata_choice, advanced);
    if ( fdcbt_setting ) { // SET BOTH CBT AND FDCBT HERE
      FLEXCAN_EnterFreezeMode();
      setClock(config.clock);
      if ( listen_only == LISTEN_ONLY ) {
        FLEXCANb_CBT(_bus) &= ~(1UL << 31); /* clear BTE bit to edit CTRL1 register */
        FLEXCANb_CTRL1(_bus) |= FLEXCAN_CTRL_LOM;
      }
      else {
        FLEXCANb_CBT(_bus) &= ~(1UL << 31); /* clear BTE bit to edit CTRL1 register */
        FLEXCANb_CTRL1(_bus) &= ~FLEXCAN_CTRL_LOM;
      }
      FLEXCANb_CBT(_bus) = cbt;
      FLEXCANb_FDCBT(_bus) = fdcbt_setting;
      if ( advanced ) Serial.print("CTRL1: 0x");
      if ( advanced ) Serial.println(FLEXCANb_CTRL1(_bus),HEX);
      if ( advanced ) Serial.print("CBT: 0x");
      if ( advanced ) Serial.println(FLEXCANb_CBT(_bus),HEX);
      if ( advanced ) Serial.print("FDCBT: 0x");
      if ( advanced ) Serial.println(FLEXCANb_FDCBT(_bus) ,HEX);
      if ( advanced ) Serial.print("FDCTRL: 0x");
      if ( advanced ) Serial.println(FLEXCANb_FDCTRL(_bus) ,HEX);
      FLEXCAN_ExitFreezeMode();
      return 1;
    }
    else return 0;
  }
  else setBaudRateFD(config, 0, advanced); /* continue list printout for FD */
  while ( Serial.read() != 'c' );
  return 1;
}

FCTPFD_FUNC uint32_t FCTPFD_OPT::setBaudRateFD(CANFD_timings_t config, uint32_t flexdata_choice, bool advanced) {
  uint32_t result = 0, result_old = 0;
  double results[10][12] = { { 0 } , { 0 } };

  double baudrate = config.baudrateFD, propdelay = config.propdelay, req_smp = config.sample;
  double cpi_clock = config.clock;

  baudrate /= 1000;
  double ratio = cpi_clock * 1000 / baudrate;
  double nbt_min = 5, nbt_max = 48, propseg_max = 32, propseg = 0, pseg_1, pseg_2;
  uint32_t tdcen = 0, ipt = 2;
  double temp = 0, ppsegmin, ppsegmax, pseg1 = 8, pseg2 = 8, rjw = 8, prescaler_min = 1, prescaler_max = 1024;
  if ( advanced ) Serial.println("      ####################################################################################################################################################");
  if ( advanced ) Serial.println("      #\t\t\t\t\t\t\t\t*** CAN FLEXDATA CONFIGURATION ***\t\t\t\t\t\t         #");
  if ( advanced ) Serial.println("      ####################################################################################################################################################\n");
  for ( uint32_t prescaler = prescaler_min; prescaler < prescaler_max; prescaler++ ) {
    temp = ratio / prescaler;
    for ( double nbt = nbt_min; nbt < nbt_max; nbt++ ) {
      if ( temp == nbt ) {
        propseg = ceil(propdelay / 1000 * cpi_clock / prescaler);
        tdcen = 0;
        if ( propseg >= (nbt - 2)) { // finished with debug
          ppsegmax = nbt - 3;
          ppsegmin = nbt - 1 - pseg1 - pseg2;
          if ( ppsegmin < 1 ) ppsegmin = 1;
          propseg = round((ppsegmax - ppsegmin) / 2);
          tdcen = 1;
        }
        if ( propseg <= propseg_max ) {
          for ( double pseg = 1; pseg <= pseg1; pseg++ ) {
            pseg_1 = round((nbt * req_smp / 100) - 1 - propseg);
            pseg_2 = nbt - 1 - propseg - pseg_1;
            if ( pseg_2 >= ipt && pseg_2 <= pseg2 && pseg_1 > 0 && pseg_1 <= pseg1 ) {
              result = result_old + 1;
              rjw = ( pseg_2 <= rjw ) ? pseg_2 : rjw;
              double sclk = (cpi_clock / prescaler);
              double tq = (1000 / (cpi_clock / prescaler));
              double smp = 100 - pseg_2 / nbt * 100;
              double tdcoff_val = (!tdcen) ? 0 : cpi_clock / (2 * baudrate / 1000);
              results[result - 1][0] = prescaler;
              results[result - 1][1] = sclk;
              results[result - 1][2] = tq;
              results[result - 1][3] = nbt;
              results[result - 1][4] = propseg;
              results[result - 1][5] = pseg_1;
              results[result - 1][6] = pseg_2;
              results[result - 1][7] = rjw;
              results[result - 1][8] = tdcen;
              results[result - 1][9] = tdcoff_val;
              results[result - 1][10] = smp;
              if ( advanced ) Serial.print("      Prescaler   Sclk(MHz)   Tq(ns)   NBT(#Tq)   Propseg(#Tq)   Pseg1(#Tq)   Pseg2(#Tq)   RJW(#Tq)   TDCEN   TDCOFF   Sample Point(%)     FDCBT Register\n");
              if ( advanced ) Serial.printf("  %2u)", result);
              if ( advanced ) Serial.printf("      %4u", prescaler);
              if ( advanced ) Serial.printf("     %6.2f", sclk);
              if ( advanced ) Serial.printf("   %7.2f", tq);
              if ( advanced ) Serial.printf("     %2u", (uint32_t)nbt);
              if ( advanced ) Serial.printf("          %2u", (uint32_t)propseg);
              if ( advanced ) Serial.printf("             %2u", (uint32_t)pseg_1);
              if ( advanced ) Serial.printf("            %2u", (uint32_t)pseg_2);
              if ( advanced ) Serial.printf("          %2u", (uint32_t)rjw);
              if ( advanced ) Serial.printf("         %2u", (uint32_t)tdcen);
              if ( advanced ) Serial.printf("     %4.2f", tdcoff_val);
              if ( advanced ) Serial.printf("       %6.2f", smp);
              uint32_t fdcbt = 0;
              fdcbt |= ((uint32_t)(results[result - 1][6] - 1) << 0); // FPSEG2
              fdcbt |= ((uint32_t)(results[result - 1][5] - 1) << 5); // FPSEG1
              fdcbt |= ((uint32_t)(results[result - 1][4] - 0) << 10); // FPROPSEG
              fdcbt |= ((uint32_t)(results[result - 1][7] - 1) << 16); // FRJW
              fdcbt |= ((uint32_t)(results[result - 1][0] - 1) << 20); // FPRESDIV
              if ( advanced ) Serial.printf("            0x%8.08X",fdcbt);

              if ( advanced ) Serial.printf("\n\t%8s%s%.2f%s%u%s%.2f%s", "", "*** Data phase bit time = ", (results[result - 1][2] * results[result - 1][3] / 1000), "us (", (int)results[result - 1][3], "Tq, Tq=", results[result - 1][2], ")\n");
              if ( advanced ) Serial.printf("\t%8s%s%.2f%s", "", "a) Sync: ", results[result - 1][2], "ns\t");
              if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "b) Propseg: ", results[result - 1][2] * results[result - 1][4], "ns\t");
              if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "c) Pseg1: ", results[result - 1][2] * results[result - 1][5], "ns\t");
              if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "d) Pseg2: ", results[result - 1][2] * results[result - 1][6], "ns\n");

              uint32_t scale = (uint32_t)ceil(results[result - 1][2]);
              char _sync[(scale*124/1000) + 1] = "";
              for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _sync[i] = 'a';
              _sync[(scale*124/1000)] = '\0';

              scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][4]);
              char _prop[(scale*124/1000) + 1] = "";
              for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _prop[i] = 'b';
              _prop[(scale*124/1000)] = '\0';

              scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][5]);
              char _pseg1[(scale*124/1000) + 1] = "";
              for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg1[i] = 'c';
              _pseg1[(scale*124/1000)] = '\0';

              scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][6]);
              char _pseg2[(scale*124/1000) + 1] = "";
              for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg2[i] = 'd';
              _pseg2[(scale*124/1000)] = '\0';

              if ( advanced ) Serial.printf("%9s%s%-s%s%-s%s%-s%s%-s", "[", _sync, "|", _prop, "|", _pseg1, "|", _pseg2, "]\n");
              if ( advanced ) Serial.printf("%8s%-s%-s%-s%-s%-s%-s", "", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________|");
              if ( advanced ) Serial.printf("\n%8s%u%*s%u%*s%u%*s%u%*s%u%*s%u%*s%u%s", "", 0, 23, "", 200, 22, "", 400, 22, "", 600, 22, "", 800, 22, "", 1000, 22, "", 1200, "(ns)\n\n\n");
            }
            if ( result == result_old ) {
              pseg = nbt - 1 - propseg;
              if ( pseg == 3 && ipt == 2 ) {
                result = result + 1;
                double sclk = (cpi_clock / prescaler);
                double tq = (1000 / (cpi_clock / prescaler));
                double smp = 100 - 2 / nbt * 100;
                double tdcoff_val = (!tdcen) ? 0 : cpi_clock / (2 * baudrate / 1000);
                results[result - 1][0] = prescaler;
                results[result - 1][1] = sclk;
                results[result - 1][2] = tq;
                results[result - 1][3] = nbt;
                results[result - 1][4] = propseg;
                results[result - 1][5] = 1;
                results[result - 1][6] = 2;
                results[result - 1][7] = 1;
                results[result - 1][8] = tdcen;
                results[result - 1][9] = tdcoff_val;
                results[result - 1][10] = smp;
                if ( advanced ) Serial.print("      Prescaler   Sclk(MHz)   Tq(ns)   NBT(#Tq)   Propseg(#Tq)   Pseg1(#Tq)   Pseg2(#Tq)   RJW(#Tq)   TDCEN   TDCOFF   Sample Point(%)     FDCBT Register\n");
                if ( advanced ) Serial.printf("  %2u)", result);
                if ( advanced ) Serial.printf("      %4u", prescaler);
                if ( advanced ) Serial.printf("     %6.2f", sclk);
                if ( advanced ) Serial.printf("   %7.2f", tq);
                if ( advanced ) Serial.printf("     %2u", (uint32_t)nbt);
                if ( advanced ) Serial.printf("          %2u", (uint32_t)propseg);
                if ( advanced ) Serial.printf("             %2u", (uint32_t)1);
                if ( advanced ) Serial.printf("            %2u", (uint32_t)2);
                if ( advanced ) Serial.printf("          %2u", (uint32_t)1);
                if ( advanced ) Serial.printf("         %2u", (uint32_t)tdcen);
                if ( advanced ) Serial.printf("     %4.2f", tdcoff_val);
                if ( advanced ) Serial.printf("       %6.2f", smp);
                uint32_t fdcbt = 0;
                fdcbt |= ((uint32_t)(results[result - 1][6] - 1) << 0); // FPSEG2
                fdcbt |= ((uint32_t)(results[result - 1][5] - 1) << 5); // FPSEG1
                fdcbt |= ((uint32_t)(results[result - 1][4] - 0) << 10); // FPROPSEG
                fdcbt |= ((uint32_t)(results[result - 1][7] - 1) << 16); // FRJW
                fdcbt |= ((uint32_t)(results[result - 1][0] - 1) << 20); // FPRESDIV
                if ( advanced ) Serial.printf("            0x%8.08X",fdcbt);
                if ( advanced ) Serial.printf("\n\t%8s%s%.2f%s%u%s%.2f%s", "", "*** Data phase bit time = ", (results[result - 1][2] * results[result - 1][3] / 1000), "us (", (int)results[result - 1][3], "Tq, Tq=", results[result - 1][2], ")\n");
                if ( advanced ) Serial.printf("\t%8s%s%.2f%s", "", "a) Sync: ", results[result - 1][2], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "b) Propseg: ", results[result - 1][2] * results[result - 1][4], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "c) Pseg1: ", results[result - 1][2] * results[result - 1][5], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "d) Pseg2: ", results[result - 1][2] * results[result - 1][6], "ns\n");

                uint32_t scale = (uint32_t)ceil(results[result - 1][2]);
                char _sync[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _sync[i] = 'a';
                _sync[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][4]);
                char _prop[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _prop[i] = 'b';
                _prop[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][5]);
                char _pseg1[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg1[i] = 'c';
                _pseg1[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][6]);
                char _pseg2[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg2[i] = 'd';
                _pseg2[(scale*124/1000)] = '\0';

                if ( advanced ) Serial.printf("%9s%s%-s%s%-s%s%-s%s%-s", "[", _sync, "|", _prop, "|", _pseg1, "|", _pseg2, "]\n");
                if ( advanced ) Serial.printf("%8s%-s%-s%-s%-s%-s%-s", "", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________|");
                if ( advanced ) Serial.printf("\n%8s%u%*s%u%*s%u%*s%u%*s%u%*s%u%*s%u%s", "", 0, 23, "", 200, 22, "", 400, 22, "", 600, 22, "", 800, 22, "", 1000, 22, "", 1200, "(ns)\n\n\n");
              }
              if ( pseg > (ipt + 1) && pseg <= (pseg1 * 2) ) {
                if ( (uint32_t)pseg % 2 != 0 ) propseg++;
                pseg = (nbt - 1 - propseg) / 2;
                result++;
                rjw = ( pseg <= rjw ) ? pseg : rjw;
                double sclk = (cpi_clock / prescaler);
                double tq = (1000 / (cpi_clock / prescaler));
                double smp = 100 - pseg / nbt * 100;
                double tdcoff_val = (!tdcen) ? 0 : cpi_clock / (2 * baudrate / 1000);
                results[result - 1][0] = prescaler;
                results[result - 1][1] = sclk;
                results[result - 1][2] = tq;
                results[result - 1][3] = nbt;
                results[result - 1][4] = propseg;
                results[result - 1][5] = pseg;
                results[result - 1][6] = pseg;
                results[result - 1][7] = rjw;
                results[result - 1][8] = tdcen;
                results[result - 1][9] = tdcoff_val;
                results[result - 1][10] = smp;
                if ( advanced ) Serial.print("      Prescaler   Sclk(MHz)   Tq(ns)   NBT(#Tq)   Propseg(#Tq)   Pseg1(#Tq)   Pseg2(#Tq)   RJW(#Tq)   TDCEN   TDCOFF   Sample Point(%)     FDCBT Register\n");
                if ( advanced ) Serial.printf("  %2u)", result);
                if ( advanced ) Serial.printf("      %4u", prescaler);
                if ( advanced ) Serial.printf("     %6.2f", sclk);
                if ( advanced ) Serial.printf("   %7.2f", tq);
                if ( advanced ) Serial.printf("     %2u", (uint32_t)nbt);
                if ( advanced ) Serial.printf("          %2u", (uint32_t)propseg);
                if ( advanced ) Serial.printf("             %2u", (uint32_t)pseg);
                if ( advanced ) Serial.printf("            %2u", (uint32_t)pseg);
                if ( advanced ) Serial.printf("          %2u", (uint32_t)rjw);
                if ( advanced ) Serial.printf("         %2u", (uint32_t)tdcen);
                if ( advanced ) Serial.printf("     %4.2f", tdcoff_val);
                if ( advanced ) Serial.printf("       %6.2f", smp);
                uint32_t fdcbt = 0;
                fdcbt |= ((uint32_t)(results[result - 1][6] - 1) << 0); // FPSEG2
                fdcbt |= ((uint32_t)(results[result - 1][5] - 1) << 5); // FPSEG1
                fdcbt |= ((uint32_t)(results[result - 1][4] - 0) << 10); // FPROPSEG
                fdcbt |= ((uint32_t)(results[result - 1][7] - 1) << 16); // FRJW
                fdcbt |= ((uint32_t)(results[result - 1][0] - 1) << 20); // FPRESDIV
                if ( advanced ) Serial.printf("            0x%8.08X",fdcbt);
                if ( advanced ) Serial.printf("\n\t%8s%s%.2f%s%u%s%.2f%s", "", "*** Data phase bit time = ", (results[result - 1][2] * results[result - 1][3] / 1000), "us (", (int)results[result - 1][3], "Tq, Tq=", results[result - 1][2], ")\n");
                if ( advanced ) Serial.printf("\t%8s%s%.2f%s", "", "a) Sync: ", results[result - 1][2], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "b) Propseg: ", results[result - 1][2] * results[result - 1][4], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "c) Pseg1: ", results[result - 1][2] * results[result - 1][5], "ns\t");
                if ( advanced ) Serial.printf("%8s%s%.2f%s", "", "d) Pseg2: ", results[result - 1][2] * results[result - 1][6], "ns\n");

                uint32_t scale = (uint32_t)ceil(results[result - 1][2]);
                char _sync[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _sync[i] = 'a';
                _sync[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][4]);
                char _prop[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _prop[i] = 'b';
                _prop[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][5]);
                char _pseg1[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg1[i] = 'c';
                _pseg1[(scale*124/1000)] = '\0';

                scale = (uint32_t)ceil(results[result - 1][2] * results[result - 1][6]);
                char _pseg2[(scale*124/1000) + 1] = "";
                for ( uint32_t i = 0; i < (scale*149/1200) + 1; i++ ) _pseg2[i] = 'd';
                _pseg2[(scale*124/1000)] = '\0';

                if ( advanced ) Serial.printf("%9s%s%-s%s%-s%s%-s%s%-s", "[", _sync, "|", _prop, "|", _pseg1, "|", _pseg2, "]\n");
                if ( advanced ) Serial.printf("%8s%-s%-s%-s%-s%-s%-s", "", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________", "|________________________|");
                if ( advanced ) Serial.printf("\n%8s%u%*s%u%*s%u%*s%u%*s%u%*s%u%*s%u%s\n\n\n\n", "", 0, 23, "", 200, 22, "", 400, 22, "", 600, 22, "", 800, 22, "", 1000, 22, "", 1200, "(ns)\n\n\n");
              }
            }
            result_old = result;
            break;
          }
        }
      }
    }
  }
  if ( flexdata_choice ) {
    if ( flexdata_choice > result ) {
      if ( advanced ) Serial.println("\n\t*** ERROR: FD rate selection out of bounds!\n");
      return 0;
    }
    uint32_t fdcbt = 0;
    fdcbt |= ((uint32_t)(results[flexdata_choice - 1][6] - 1) << 0); // FPSEG2
    fdcbt |= ((uint32_t)(results[flexdata_choice - 1][5] - 1) << 5); // FPSEG1
    fdcbt |= ((uint32_t)(results[flexdata_choice - 1][4] - 0) << 10); // FPROPSEG
    fdcbt |= ((uint32_t)(results[flexdata_choice - 1][7] - 1) << 16); // FRJW
    fdcbt |= ((uint32_t)(results[flexdata_choice - 1][0] - 1) << 20); // FPRESDIV

    FLEXCANb_FDCTRL(_bus) = (FLEXCANb_FDCTRL(_bus) & 0xFFFF60FF); /* clear TDC values */
    if ( ((uint32_t)(results[flexdata_choice - 1][8])) ) { /* enable TDC if available */
      FLEXCANb_FDCTRL(_bus) |= (1UL << 15) | (((uint32_t)(results[flexdata_choice - 1][9])) << 8);
    }
    return fdcbt;
  }
  return 0;
}
