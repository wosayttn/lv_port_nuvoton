/**************************************************************************//**
 * @file     lpgpio_reg.h
 * @version  V1.00
 * @brief    LPGPIO register definition header file
 *
 * @copyright SPDX-License-Identifier: Apache-2.0
 * @copyright Copyright (C) 2023 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/

#ifndef __LPGPIO_REG_H__
#define __LPGPIO_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/**
    @addtogroup REGISTER Control Register
    @{
*/

/**
    @addtogroup LPGPIO General Purpose Input/Output Controller (LPGPIO)
    Memory Mapped Structure for LPGPIO Controller
    @{
*/

typedef struct
{
    /**
     * @var GPIO_T::MODE
     * Offset: 0x00/0x40/0x80/0xC0/0x100/0x140/0x180/0x1C0  Port A-H I/O Mode Control
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[2n+1:2n]|MODEn    |Port A-H I/O Pin[n] Mode Control
     * |        |          |Determine each I/O mode of Px.n pins.
     * |        |          |00 = Px.n is in Input mode.
     * |        |          |01 = Px.n is in Push-pull Output mode.
     * |        |          |10 = Px.n is in Open-drain Output mode.
     * |        |          |11 = Px.n is in Quasi-bidirectional mode.
     * |        |          |Note1: The initial value of this field is defined by CIOINI (CONFIG0 [10]).
     * |        |          |If CIOINI is set to 0, the default value is 0xFFFF_FFFF and all pins will be quasi-bidirectional mode after chip powered on.
     * |        |          |If CIOINI is set to 1, the default value is 0x0000_0000 and all pins will be input mode after chip powered on.
     * |        |          |Note2:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::DINOFF
     * Offset: 0x04/0x44/0x84/0xC4/0x104/0x144/0x184/0x1C4  Port A-H Digital Input Path Disable Control
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n+16]  |DINOFFn   |Port A-H Pin[n] Digital Input Path Disable Control
     * |        |          |Each of these bits is used to control if the digital input path of corresponding Px.n pin is disabled.
     * |        |          |If input is analog signal, users can disable Px.n digital input path to avoid input current leakage.
     * |        |          |0 = Px.n digital input path Enabled.
     * |        |          |1 = Px.n digital input path Disabled (digital input tied to low).
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::DOUT
     * Offset: 0x08/0x48/0x88/0xC8/0x108/0x148/0x188/0x1C8  Port A-H Data Output Value
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n]     |DOUTn     |Port A-H Pin[n] Output Value
     * |        |          |Each of these bits controls the status of a Px.n pin when the Px.n is configured as Push-pull output, Open-drain output or Quasi-bidirectional mode.
     * |        |          |0 = Px.n will drive Low if the Px.n pin is configured as Push-pull output, Open-drain output or Quasi-bidirectional mode.
     * |        |          |1 = Px.n will drive High if the Px.n pin is configured as Push-pull output or Quasi-bidirectional mode.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::DATMSK
     * Offset: 0x0C/0x4C/0x8C/0xCC/0x10C/0x14C/0x18C/0x1CC  Port A-H Data Output Write Mask
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n]     |DATMSKn    |Port A-H Pin[n] Data Output Write Mask
     * |        |          |These bits are used to protect the corresponding DOUT (Px_DOUT[n]) bit.
     * |        |          |When the DATMSK (Px_DATMSK[n]) bit is set to 1, the corresponding DOUT (Px_DOUT[n]) bit is protected.
     * |        |          |If the write signal is masked, writing data to the protect bit is ignored.
     * |        |          |0 = Corresponding DOUT (Px_DOUT[n]) bit can be updated.
     * |        |          |1 = Corresponding DOUT (Px_DOUT[n]) bit protected.
     * |        |          |Note1: This function only protects the corresponding DOUT (Px_DOUT[n]) bit, and will not protect the corresponding PDIO (Pxn_PDIO[n]) bit.
     * |        |          |Note2:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::PIN
     * Offset: 0x10/0x50/0x90/0xD0/0x110/0x150/0x190/0x1D0  Port A-H Pin Value
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n]     |PINn      |Port A-H Pin[n] Pin Value
     * |        |          |Each bit of the register reflects the actual status of the respective Px.n pin.
     * |        |          |If the bit is 1, it indicates the corresponding pin status is high; else the pin status is low.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::DBEN
     * Offset: 0x14/0x54/0x94/0xD4/0x114/0x154/0x194/0x1D4  Port A-H De-Bounce Enable Control Register
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n]     |DBENn     |Port A-H Pin[n] Input Signal De-Bounce Enable Bit
     * |        |          |The DBEN[n] bit is used to enable the de-bounce function for each corresponding bit.
     * |        |          |If the input signal pulse width cannot be sampled by continuous two de-bounce sample cycle, the input signal transition is seen as the signal bounce and will not trigger the interrupt.
     * |        |          |The de-bounce clock source is controlled by DBCLKSRC (GPIO_DBCTL [4]), one de-bounce sample cycle period is controlled by DBCLKSEL (GPIO_DBCTL [3:0]).
     * |        |          |0 = Px.n de-bounce function Disabled.
     * |        |          |1 = Px.n de-bounce function Enabled.
     * |        |          |The de-bounce function is valid only for edge triggered interrupt.
     * |        |          |If the interrupt mode is level triggered, the de-bounce enable bit is ignored.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::INTTYPE
     * Offset: 0x18/0x58/0x98/0xD8/0x118/0x158/0x198/0x1D8  Port A-H Interrupt Trigger Type Control
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n]     |TYPEn     |Port A-H Pin[n] Edge or Level Detection Interrupt Trigger Type Control
     * |        |          |TYPE (Px_INTTYPE[n]) bit is used to control the triggered interrupt is by level trigger or by edge trigger.
     * |        |          |If the interrupt is by edge trigger, the trigger source can be controlled by de-bounce.
     * |        |          |If the interrupt is by level trigger, the input source is sampled by one HCLK clock and generates the interrupt.
     * |        |          |0 = Edge trigger interrupt.
     * |        |          |1 = Level trigger interrupt.
     * |        |          |If the pin is set as the level trigger interrupt, only one level can be set on the registers RHIEN (Px_INTEN[n+16])/FLIEN (Px_INTEN[n]).
     * |        |          |If both levels to trigger interrupt are set, the setting is ignored and no interrupt will occur.
     * |        |          |The de-bounce function is valid only for edge triggered interrupt.
     * |        |          |If the interrupt mode is level triggered, the de-bounce enable bit is ignored.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::INTEN
     * Offset: 0x1C/0x5C/0x9C/0xDC/0x11C/0x15C/0x19C/0x1DC  Port A-H Interrupt Enable Control Register
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n]     |FLIENn    |Port A-H Pin[n] Falling Edge or Low Level Interrupt Trigger Type Enable Bit
     * |        |          |The FLIEN (Px_INTEN[n]) bit is used to enable the interrupt for each of the corresponding input Px.n pin.
     * |        |          |Set bit to 1 also enable the pin wake-up function.
     * |        |          |When setting the FLIEN (Px_INTEN[n]) bit to 1 :
     * |        |          |If the interrupt is level trigger (TYPE (Px_INTTYPE[n]) bit is set to 1), the input Px.n pin will generate the interrupt while this pin state is at low level.
     * |        |          |If the interrupt is edge trigger(TYPE (Px_INTTYPE[n]) bit is set to 0), the input Px.n pin will generate the interrupt while this pin state changed from high to low.
     * |        |          |0 = Px.n level low or high to low interrupt Disabled.
     * |        |          |1 = Px.n level low or high to low interrupt Enabled.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * |[n+16]  |RHIENn    |Port A-H Pin[n] Rising Edge or High Level Interrupt Trigger Type Enable Bit
     * |        |          |The RHIEN (Px_INTEN[n+16]) bit is used to enable the interrupt for each of the corresponding input Px.n pin
     * |        |          |Set bit to 1 also enable the pin wake-up function.
     * |        |          |When setting the RHIEN (Px_INTEN[n+16]) bit to 1 :
     * |        |          |If the interrupt is level trigger (TYPE (Px_INTTYPE[n]) bit is set to 1), the input Px.n pin will generate the interrupt while this pin state is at high level.
     * |        |          |If the interrupt is edge trigger (TYPE (Px_INTTYPE[n]) bit is set to 0), the input Px.n pin will generate the interrupt while this pin state changed from low to high.
     * |        |          |0 = Px.n level high or low to high interrupt Disabled.
     * |        |          |1 = Px.n level high or low to high interrupt Enabled.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::INTSRC
     * Offset: 0x20/0x60/0xA0/0xE0/0x120/0x160/0x1A0/0x1E0  Port A-H Interrupt Source Flag
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n]     |INTSRCn   |Port A-H Pin[n] Interrupt Source Flag
     * |        |          |Write Operation :
     * |        |          |0 = No action.
     * |        |          |1 = Clear the corresponding pending interrupt.
     * |        |          |Read Operation :
     * |        |          |0 = No interrupt at Px.n.
     * |        |          |1 = Px.n generates an interrupt.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::SMTEN
     * Offset: 0x24/0x64/0xA4/0xE4/0x124/0x164/0x1A4/0x1E4  Port A-H Input Schmitt Trigger Enable Register
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[n]     |SMTENn    |Port A-H Pin[n] Input Schmitt Trigger Enable Bit
     * |        |          |0 = Px.n input Schmitt trigger function Disabled.
     * |        |          |1 = Px.n input Schmitt trigger function Enabled.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::SLEWCTL
     * Offset: 0x28/0x68/0xA8/0xE8/0x128/0x168/0x1A8/0x1E8  Port A-H High Slew Rate Control Register
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[2n+1:2n]|HSRENn    |Port A-H Pin[n] High Slew Rate Control
     * |        |          |00 = Px.n output with normal slew rate mode (maximum 40 MHz at 2.7V).
     * |        |          |01 = Px.n output with high slew rate mode (maximum 80 MHz at 2.7V).
     * |        |          |10 = Px.n output with fast slew rate mode (maximum 100 MHz at 2.7V.
     * |        |          |11 = Reserved.
     * |        |          |Note:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     * @var GPIO_T::PUSEL
     * Offset: 0x30/0x70/0xB0/0xF0/0x130/0x170/0x1B0/0x1F0  Port A-H Pull-up and Pull-down Selection Register
     * ---------------------------------------------------------------------------------------------------
     * |Bits    |Field     |Descriptions
     * | :----: | :----:   | :---- |
     * |[2n+1:2n]|PUSELn    |Port A-H Pin[n] Pull-up and Pull-down Enable Register
     * |        |          |Determine each I/O Pull-up/pull-down of Px.n pins.
     * |        |          |00 = Px.n pull-up and pull-up disable.
     * |        |          |01 = Px.n pull-up enable.
     * |        |          |10 = Px.n pull-down enable.
     * |        |          |11 = Reserved.
     * |        |          |Note1:
     * |        |          |Basically, the pull-up control and pull-down control has following behavior limitation
     * |        |          |The independent pull-up control register only valid when MODEn set as tri-state and open-drain mode
     * |        |          |The independent pull-down control register only valid when MODEn set as tri-state mode
     * |        |          |When both pull-up pull-down is set as 1 at tri-state mode, keep I/O in tri-state mode
     * |        |          |Note2:
     * |        |          |Max. n=15 for port A/B/E/G.
     * |        |          |Max. n=14 for port C/D.
     * |        |          |Max. n=11 for port F/H.
     */
    __IO uint32_t MODE;             /*!< [0x000] LPIOn Mode Control                                     */
    __IO uint32_t DOUT;             /*!< [0x004] LPIOn Data Output Value                                */
    __I  uint32_t PIN;              /*!< [0x008] LPIOn Pin Value                                        */
    __O  uint32_t DSRST;            /*!< [0x00C] LPIOn Data Output Set and Reset Control                */
    __O  uint32_t DRST;             /*!< [0x010] LPIOn Data Output Reset Control                        */
} LPGPIO_T;


/**
    @addtogroup GPIO_CONST GPIO Bit Field Definition
    Constant Definitions for GPIO Controller
@{ */

#define LPGPIO_MODE_MODE_Pos            (0)                                         /*!< LPGPIO_T::MODE: MODE Position              */
#define LPGPIO_MODE_MODE_Msk            (0xffUL << LPGPIO_MODE_MODE_Pos)            /*!< LPGPIO_T::MODE: MODE Mask                  */

#define LPGPIO_MODE_MODE0_Pos           (0)                                         /*!< LPGPIO_T::MODE: MODE0 Position             */
#define LPGPIO_MODE_MODE0_Msk           (0x1UL << LPGPIO_MODE_MODE0_Pos)            /*!< LPGPIO_T::MODE: MODE0 Mask                 */

#define LPGPIO_MODE_MODE1_Pos           (1)                                         /*!< LPGPIO_T::MODE: MODE1 Position             */
#define LPGPIO_MODE_MODE1_Msk           (0x1UL << LPGPIO_MODE_MODE1_Pos)            /*!< LPGPIO_T::MODE: MODE1 Mask                 */

#define LPGPIO_MODE_MODE2_Pos           (2)                                         /*!< LPGPIO_T::MODE: MODE2 Position             */
#define LPGPIO_MODE_MODE2_Msk           (0x1UL << LPGPIO_MODE_MODE2_Pos)            /*!< LPGPIO_T::MODE: MODE2 Mask                 */

#define LPGPIO_MODE_MODE3_Pos           (3)                                         /*!< LPGPIO_T::MODE: MODE3 Position             */
#define LPGPIO_MODE_MODE3_Msk           (0x1UL << LPGPIO_MODE_MODE3_Pos)            /*!< LPGPIO_T::MODE: MODE3 Mask                 */

#define LPGPIO_MODE_MODE4_Pos           (4)                                         /*!< LPGPIO_T::MODE: MODE4 Position             */
#define LPGPIO_MODE_MODE4_Msk           (0x1UL << LPGPIO_MODE_MODE4_Pos)            /*!< LPGPIO_T::MODE: MODE4 Mask                 */

#define LPGPIO_MODE_MODE5_Pos           (5)                                         /*!< LPGPIO_T::MODE: MODE5 Position             */
#define LPGPIO_MODE_MODE5_Msk           (0x1UL << LPGPIO_MODE_MODE5_Pos)            /*!< LPGPIO_T::MODE: MODE5 Mask                 */

#define LPGPIO_MODE_MODE6_Pos           (6)                                         /*!< LPGPIO_T::MODE: MODE6 Position             */
#define LPGPIO_MODE_MODE6_Msk           (0x1UL << LPGPIO_MODE_MODE6_Pos)            /*!< LPGPIO_T::MODE: MODE6 Mask                 */

#define LPGPIO_MODE_MODE7_Pos           (7)                                         /*!< LPGPIO_T::MODE: MODE7 Position             */
#define LPGPIO_MODE_MODE7_Msk           (0x1UL << LPGPIO_MODE_MODE7_Pos)            /*!< LPGPIO_T::MODE: MODE7 Mask                 */

#define LPGPIO_MODE_LPPDMA_EN_Pos       (31)                                        /*!< LPGPIO_T::MODE: LPPDMA_EN Position         */
#define LPGPIO_MODE_LPPDMA_EN_Msk       (0x1UL << LPGPIO_MODE_LPPDMA_EN_Pos)        /*!< LPGPIO_T::MODE: LPPDMA_EN Mask             */

#define LPGPIO_DOUT_DOUT_Pos            (0)                                         /*!< LPGPIO_T::DOUT: DOUT Position              */
#define LPGPIO_DOUT_DOUT_Msk            (0xffUL << LPGPIO_DOUT_DOUT_Pos)            /*!< LPGPIO_T::DOUT: DOUT Mask                  */

#define LPGPIO_DOUT_DOUT0_Pos           (0)                                         /*!< LPGPIO_T::DOUT: DOUT0 Position             */
#define LPGPIO_DOUT_DOUT0_Msk           (0x1UL << LPGPIO_DOUT_DOUT0_Pos)            /*!< LPGPIO_T::DOUT: DOUT0 Mask                 */

#define LPGPIO_DOUT_DOUT1_Pos           (1)                                         /*!< LPGPIO_T::DOUT: DOUT1 Position             */
#define LPGPIO_DOUT_DOUT1_Msk           (0x1UL << LPGPIO_DOUT_DOUT1_Pos)            /*!< LPGPIO_T::DOUT: DOUT1 Mask                 */

#define LPGPIO_DOUT_DOUT2_Pos           (2)                                         /*!< LPGPIO_T::DOUT: DOUT2 Position             */
#define LPGPIO_DOUT_DOUT2_Msk           (0x1UL << LPGPIO_DOUT_DOUT2_Pos)            /*!< LPGPIO_T::DOUT: DOUT2 Mask                 */

#define LPGPIO_DOUT_DOUT3_Pos           (3)                                         /*!< LPGPIO_T::DOUT: DOUT3 Position             */
#define LPGPIO_DOUT_DOUT3_Msk           (0x1UL << LPGPIO_DOUT_DOUT3_Pos)            /*!< LPGPIO_T::DOUT: DOUT3 Mask                 */

#define LPGPIO_DOUT_DOUT4_Pos           (4)                                         /*!< LPGPIO_T::DOUT: DOUT4 Position             */
#define LPGPIO_DOUT_DOUT4_Msk           (0x1UL << LPGPIO_DOUT_DOUT4_Pos)            /*!< LPGPIO_T::DOUT: DOUT4 Mask                 */

#define LPGPIO_DOUT_DOUT5_Pos           (5)                                         /*!< LPGPIO_T::DOUT: DOUT5 Position             */
#define LPGPIO_DOUT_DOUT5_Msk           (0x1UL << LPGPIO_DOUT_DOUT5_Pos)            /*!< LPGPIO_T::DOUT: DOUT5 Mask                 */

#define LPGPIO_DOUT_DOUT6_Pos           (6)                                         /*!< LPGPIO_T::DOUT: DOUT6 Position             */
#define LPGPIO_DOUT_DOUT6_Msk           (0x1UL << LPGPIO_DOUT_DOUT6_Pos)            /*!< LPGPIO_T::DOUT: DOUT6 Mask                 */

#define LPGPIO_DOUT_DOUT7_Pos           (7)                                         /*!< LPGPIO_T::DOUT: DOUT7 Position             */
#define LPGPIO_DOUT_DOUT7_Msk           (0x1UL << LPGPIO_DOUT_DOUT7_Pos)            /*!< LPGPIO_T::DOUT: DOUT7 Mask                 */

#define LPGPIO_PIN_PIN_Pos              (0)                                         /*!< LPGPIO_T::PIN: PIN Position                */
#define LPGPIO_PIN_PIN_Msk              (0xffUL << LPGPIO_PIN_PIN_Pos)              /*!< LPGPIO_T::PIN: PIN Mask                    */

#define LPGPIO_PIN_PIN0_Pos             (0)                                         /*!< LPGPIO_T::PIN: PIN0 Position               */
#define LPGPIO_PIN_PIN0_Msk             (0x1UL << LPGPIO_PIN_PIN0_Pos)              /*!< LPGPIO_T::PIN: PIN0 Mask                   */

#define LPGPIO_PIN_PIN1_Pos             (1)                                         /*!< LPGPIO_T::PIN: PIN1 Position               */
#define LPGPIO_PIN_PIN1_Msk             (0x1UL << LPGPIO_PIN_PIN1_Pos)              /*!< LPGPIO_T::PIN: PIN1 Mask                   */

#define LPGPIO_PIN_PIN2_Pos             (2)                                         /*!< LPGPIO_T::PIN: PIN2 Position               */
#define LPGPIO_PIN_PIN2_Msk             (0x1UL << LPGPIO_PIN_PIN2_Pos)              /*!< LPGPIO_T::PIN: PIN2 Mask                   */

#define LPGPIO_PIN_PIN3_Pos             (3)                                         /*!< LPGPIO_T::PIN: PIN3 Position               */
#define LPGPIO_PIN_PIN3_Msk             (0x1UL << LPGPIO_PIN_PIN3_Pos)              /*!< LPGPIO_T::PIN: PIN3 Mask                   */

#define LPGPIO_PIN_PIN4_Pos             (4)                                         /*!< LPGPIO_T::PIN: PIN4 Position               */
#define LPGPIO_PIN_PIN4_Msk             (0x1UL << LPGPIO_PIN_PIN4_Pos)              /*!< LPGPIO_T::PIN: PIN4 Mask                   */

#define LPGPIO_PIN_PIN5_Pos             (5)                                         /*!< LPGPIO_T::PIN: PIN5 Position               */
#define LPGPIO_PIN_PIN5_Msk             (0x1UL << LPGPIO_PIN_PIN5_Pos)              /*!< LPGPIO_T::PIN: PIN5 Mask                   */

#define LPGPIO_PIN_PIN6_Pos             (6)                                         /*!< LPGPIO_T::PIN: PIN6 Position               */
#define LPGPIO_PIN_PIN6_Msk             (0x1UL << LPGPIO_PIN_PIN6_Pos)              /*!< LPGPIO_T::PIN: PIN6 Mask                   */

#define LPGPIO_PIN_PIN7_Pos             (7)                                         /*!< LPGPIO_T::PIN: PIN7 Position               */
#define LPGPIO_PIN_PIN7_Msk             (0x1UL << LPGPIO_PIN_PIN7_Pos)              /*!< LPGPIO_T::PIN: PIN7 Mask                   */

#define LPGPIO_DSRST_DSET_Pos           (0)                                         /*!< LPGPIO_T::DSRST: DSET Position             */
#define LPGPIO_DSRST_DSET_Msk           (0xffUL << LPGPIO_DSRST_DSET_Pos)           /*!< LPGPIO_T::DSRST: DSET Mask                 */

#define LPGPIO_DSRST_DSET0_Pos          (0)                                         /*!< LPGPIO_T::DSRST: DSET0 Position            */
#define LPGPIO_DSRST_DSET0_Msk          (0x1UL << LPGPIO_DSRST_DSET0_Pos)           /*!< LPGPIO_T::DSRST: DSET0 Mask                */

#define LPGPIO_DSRST_DSET1_Pos          (1)                                         /*!< LPGPIO_T::DSRST: DSET1 Position            */
#define LPGPIO_DSRST_DSET1_Msk          (0x1UL << LPGPIO_DSRST_DSET1_Pos)           /*!< LPGPIO_T::DSRST: DSET1 Mask                */

#define LPGPIO_DSRST_DSET2_Pos          (2)                                         /*!< LPGPIO_T::DSRST: DSET2 Position            */
#define LPGPIO_DSRST_DSET2_Msk          (0x1UL << LPGPIO_DSRST_DSET2_Pos)           /*!< LPGPIO_T::DSRST: DSET2 Mask                */

#define LPGPIO_DSRST_DSET3_Pos          (3)                                         /*!< LPGPIO_T::DSRST: DSET3 Position            */
#define LPGPIO_DSRST_DSET3_Msk          (0x1UL << LPGPIO_DSRST_DSET3_Pos)           /*!< LPGPIO_T::DSRST: DSET3 Mask                */

#define LPGPIO_DSRST_DSET4_Pos          (4)                                         /*!< LPGPIO_T::DSRST: DSET4 Position            */
#define LPGPIO_DSRST_DSET4_Msk          (0x1UL << LPGPIO_DSRST_DSET4_Pos)           /*!< LPGPIO_T::DSRST: DSET4 Mask                */

#define LPGPIO_DSRST_DSET5_Pos          (5)                                         /*!< LPGPIO_T::DSRST: DSET5 Position            */
#define LPGPIO_DSRST_DSET5_Msk          (0x1UL << LPGPIO_DSRST_DSET5_Pos)           /*!< LPGPIO_T::DSRST: DSET5 Mask                */

#define LPGPIO_DSRST_DSET6_Pos          (6)                                         /*!< LPGPIO_T::DSRST: DSET6 Position            */
#define LPGPIO_DSRST_DSET6_Msk          (0x1UL << LPGPIO_DSRST_DSET6_Pos)           /*!< LPGPIO_T::DSRST: DSET6 Mask                */

#define LPGPIO_DSRST_DSET7_Pos          (7)                                         /*!< LPGPIO_T::DSRST: DSET7 Position            */
#define LPGPIO_DSRST_DSET7_Msk          (0x1UL << LPGPIO_DSRST_DSET7_Pos)           /*!< LPGPIO_T::DSRST: DSET7 Mask                */

#define LPGPIO_DSRST_DRESET_Pos         (16)                                        /*!< LPGPIO_T::DSRST: DRESET Position           */
#define LPGPIO_DSRST_DRESET_Msk         (0xffUL << LPGPIO_DSRST_DRESET_Pos)         /*!< LPGPIO_T::DSRST: DRESET Mask               */

#define LPGPIO_DSRST_DRESET0_Pos        (16)                                        /*!< LPGPIO_T::DSRST: DRESET0 Position          */
#define LPGPIO_DSRST_DRESET0_Msk        (0x1UL << LPGPIO_DSRST_DRESET0_Pos)         /*!< LPGPIO_T::DSRST: DRESET0 Mask              */

#define LPGPIO_DSRST_DRESET1_Pos        (17)                                        /*!< LPGPIO_T::DSRST: DRESET1 Position          */
#define LPGPIO_DSRST_DRESET1_Msk        (0x1UL << LPGPIO_DSRST_DRESET1_Pos)         /*!< LPGPIO_T::DSRST: DRESET1 Mask              */

#define LPGPIO_DSRST_DRESET2_Pos        (18)                                        /*!< LPGPIO_T::DSRST: DRESET2 Position          */
#define LPGPIO_DSRST_DRESET2_Msk        (0x1UL << LPGPIO_DSRST_DRESET2_Pos)         /*!< LPGPIO_T::DSRST: DRESET2 Mask              */

#define LPGPIO_DSRST_DRESET3_Pos        (19)                                        /*!< LPGPIO_T::DSRST: DRESET3 Position          */
#define LPGPIO_DSRST_DRESET3_Msk        (0x1UL << LPGPIO_DSRST_DRESET3_Pos)         /*!< LPGPIO_T::DSRST: DRESET3 Mask              */

#define LPGPIO_DSRST_DRESET4_Pos        (20)                                        /*!< LPGPIO_T::DSRST: DRESET4 Position          */
#define LPGPIO_DSRST_DRESET4_Msk        (0x1UL << LPGPIO_DSRST_DRESET4_Pos)         /*!< LPGPIO_T::DSRST: DRESET4 Mask              */

#define LPGPIO_DSRST_DRESET5_Pos        (21)                                        /*!< LPGPIO_T::DSRST: DRESET5 Position          */
#define LPGPIO_DSRST_DRESET5_Msk        (0x1UL << LPGPIO_DSRST_DRESET5_Pos)         /*!< LPGPIO_T::DSRST: DRESET5 Mask              */

#define LPGPIO_DSRST_DRESET6_Pos        (22)                                        /*!< LPGPIO_T::DSRST: DRESET6 Position          */
#define LPGPIO_DSRST_DRESET6_Msk        (0x1UL << LPGPIO_DSRST_DRESET6_Pos)         /*!< LPGPIO_T::DSRST: DRESET6 Mask              */

#define LPGPIO_DSRST_DRESET7_Pos        (23)                                        /*!< LPGPIO_T::DSRST: DRESET7 Position          */
#define LPGPIO_DSRST_DRESET7_Msk        (0x1UL << LPGPIO_DSRST_DRESET7_Pos)         /*!< LPGPIO_T::DSRST: DRESET7 Mask              */

#define LPGPIO_DRST_DRESET_Pos          (0)                                         /*!< LPGPIO_T::DRST: DRESET Position            */
#define LPGPIO_DRST_DRESET_Msk          (0xffUL << LPGPIO_DRST_DRESET_Pos)          /*!< LPGPIO_T::DRST: DRESET Mask                */

#define LPGPIO_DRST_DRESET0_Pos         (0)                                         /*!< LPGPIO_T::DRST: DRESET0 Position           */
#define LPGPIO_DRST_DRESET0_Msk         (0x1UL << LPGPIO_DRST_DRESET0_Pos)          /*!< LPGPIO_T::DRST: DRESET0 Mask               */

#define LPGPIO_DRST_DRESET1_Pos         (1)                                         /*!< LPGPIO_T::DRST: DRESET1 Position           */
#define LPGPIO_DRST_DRESET1_Msk         (0x1UL << LPGPIO_DRST_DRESET1_Pos)          /*!< LPGPIO_T::DRST: DRESET1 Mask               */

#define LPGPIO_DRST_DRESET2_Pos         (2)                                         /*!< LPGPIO_T::DRST: DRESET2 Position           */
#define LPGPIO_DRST_DRESET2_Msk         (0x1UL << LPGPIO_DRST_DRESET2_Pos)          /*!< LPGPIO_T::DRST: DRESET2 Mask               */

#define LPGPIO_DRST_DRESET3_Pos         (3)                                         /*!< LPGPIO_T::DRST: DRESET3 Position           */
#define LPGPIO_DRST_DRESET3_Msk         (0x1UL << LPGPIO_DRST_DRESET3_Pos)          /*!< LPGPIO_T::DRST: DRESET3 Mask               */

#define LPGPIO_DRST_DRESET4_Pos         (4)                                         /*!< LPGPIO_T::DRST: DRESET4 Position           */
#define LPGPIO_DRST_DRESET4_Msk         (0x1UL << LPGPIO_DRST_DRESET4_Pos)          /*!< LPGPIO_T::DRST: DRESET4 Mask               */

#define LPGPIO_DRST_DRESET5_Pos         (5)                                         /*!< LPGPIO_T::DRST: DRESET5 Position           */
#define LPGPIO_DRST_DRESET5_Msk         (0x1UL << LPGPIO_DRST_DRESET5_Pos)          /*!< LPGPIO_T::DRST: DRESET5 Mask               */

#define LPGPIO_DRST_DRESET6_Pos         (6)                                         /*!< LPGPIO_T::DRST: DRESET6 Position           */
#define LPGPIO_DRST_DRESET6_Msk         (0x1UL << LPGPIO_DRST_DRESET6_Pos)          /*!< LPGPIO_T::DRST: DRESET6 Mask               */

#define LPGPIO_DRST_DRESET7_Pos         (7)                                         /*!< LPGPIO_T::DRST: DRESET7 Position           */
#define LPGPIO_DRST_DRESET7_Msk         (0x1UL << LPGPIO_DRST_DRESET7_Pos)          /*!< LPGPIO_T::DRST: DRESET7 Mask               */

/** @} LPGPIO_CONST */
/** @} end of LPGPIO register group */
/** @} end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* __LPGPIO_REG_H__ */
