#include "nrf24.h"
#include "string.h"
#include "functions.h"

// The address used to test presence of the transceiver,
// note: should not exceed 5 bytes
#define nRF24_TEST_ADDR            "nRF24"

// Addresses of the address registers
static const uint8_t nRF24_ADDR_REGS[7] = {
nRF24_REG_RX_ADDR_P0,
nRF24_REG_RX_ADDR_P1,
nRF24_REG_RX_ADDR_P2,
nRF24_REG_RX_ADDR_P3,
nRF24_REG_RX_ADDR_P4,
nRF24_REG_RX_ADDR_P5,
nRF24_REG_TX_ADDR };

static nrf24_frame_t nrf24_frame;

//
// Reads a value of register
// input:
//   reg - number of register to read
// return: value of register
static uint8_t nRF24_ReadReg(uint8_t reg) {
	uint8_t value;

	nRF24_CSN_L;
	nRF24_LL_RW(reg & nRF24_MASK_REG_MAP);
	value = nRF24_LL_RW(nRF24_CMD_NOP);
	nRF24_CSN_H;

	return value;
}

// Writes a new value to the register
// input:
//   reg - number of register to write
//   value - value to write
static void nRF24_WriteReg(uint8_t reg, uint8_t value) {
	nRF24_CSN_L;
	if (reg < nRF24_CMD_W_REGISTER) {
		// This is a register access
		nRF24_LL_RW(nRF24_CMD_W_REGISTER | (reg & nRF24_MASK_REG_MAP ));
		nRF24_LL_RW(value);
	} else {
		// This is a single byte command or future command/register
		nRF24_LL_RW(reg);
		if ((reg != nRF24_CMD_FLUSH_TX ) && (reg != nRF24_CMD_FLUSH_RX )
				&& (reg != nRF24_CMD_REUSE_TX_PL ) && (reg != nRF24_CMD_NOP )) {
			// Send register value
			nRF24_LL_RW(value);
		}
	}
	nRF24_CSN_H;
}

// Reads to the buffer from a multi-byte register
// input:
//   reg - number of register to read
//   pBuf - pointer to the buffer for register data
//   count - number of bytes to read
static void nRF24_ReadMBReg(uint8_t reg, uint8_t *pBuf, uint8_t count) {
	nRF24_CSN_L;
	nRF24_LL_RW(reg);
	while (count--) {
		*pBuf++ = nRF24_LL_RW(nRF24_CMD_NOP);
	}
	nRF24_CSN_H;
}

// Writes a buffer to a multi-byte register
// input:
//   reg - number of register to write
//   pBuf - pointer to the buffer with data to write
//   count - number of bytes to write
static void nRF24_WriteMBReg(uint8_t reg, uint8_t *pBuf, uint8_t count) {
	nRF24_CSN_L;
	nRF24_LL_RW(reg);
	while (count--) {
		nRF24_LL_RW(*pBuf++);
	}
	nRF24_CSN_H;
}

// Configures the transceiver to its initial state
void nRF24_Init(void) {
	// Write to the registers their initial values
	nRF24_WriteReg(nRF24_REG_CONFIG, 0x00);
	nRF24_WriteReg(nRF24_REG_EN_AA, 0x3F);
	nRF24_WriteReg(nRF24_REG_EN_RXADDR, 0x03);
	nRF24_WriteReg(nRF24_REG_SETUP_AW, 0x03);
	nRF24_WriteReg(nRF24_REG_SETUP_RETR, 0x03);
	nRF24_WriteReg(nRF24_REG_RF_CH, 0x02);
	nRF24_WriteReg(nRF24_REG_RF_SETUP, 0x0E);
	nRF24_WriteReg(nRF24_REG_STATUS, 0x00);
	nRF24_WriteReg(nRF24_REG_RX_PW_P0, 0x00);
	nRF24_WriteReg(nRF24_REG_RX_PW_P1, 0x00);
	nRF24_WriteReg(nRF24_REG_RX_PW_P2, 0x00);
	nRF24_WriteReg(nRF24_REG_RX_PW_P3, 0x00);
	nRF24_WriteReg(nRF24_REG_RX_PW_P4, 0x00);
	nRF24_WriteReg(nRF24_REG_RX_PW_P5, 0x00);
	nRF24_WriteReg(nRF24_REG_DYNPD, 0x00);
	nRF24_WriteReg(nRF24_REG_FEATURE, 0x00);

	// Configure the default RX/TX address values
	uint8_t addr[5];
	uint8_t idx;
	for (idx = 0U; idx < sizeof(addr); idx++) {
		addr[idx] = 0xE7;
	}
	nRF24_SetAddr(nRF24_PIPETX, addr);
	nRF24_SetAddr(nRF24_PIPE0, addr);
	for (idx = 0U; idx < sizeof(addr); idx++) {
		addr[idx] = 0xC2;
	}
	nRF24_SetAddr(nRF24_PIPE1, addr);
	for (idx = 2U; idx < 6U; idx++) {
		addr[0] = idx + 0xC1;
		nRF24_SetAddr(idx, addr);
	}

	// Clear the FIFO's
	nRF24_FlushRX();
	nRF24_FlushTX();

	// Clear any pending interrupt flags
	nRF24_ClearIRQFlags();

	// Deassert CSN pin (chip release)
	nRF24_CSN_H;
}

// Checks the presence of the nRF24L01
// return:
//   zero - transceiver is absent
//   non-zero: transceiver is present
uint8_t nRF24_Check(void) {
	uint8_t rxbuf[sizeof(nRF24_TEST_ADDR) - 1U];
	uint8_t *ptr = (uint8_t*) nRF24_TEST_ADDR;
	uint8_t idx;

	// Write the test address to the TX_ADDR register
	nRF24_WriteMBReg(
	nRF24_CMD_W_REGISTER | nRF24_REG_TX_ADDR, ptr,
			sizeof(nRF24_TEST_ADDR) - 1U);

	// Read it back to the buffer
	nRF24_ReadMBReg(
	nRF24_CMD_R_REGISTER | nRF24_REG_TX_ADDR, rxbuf,
			sizeof(nRF24_TEST_ADDR) - 1U);

	// Compare transmitted and received data...
	for (idx = 0U; idx < sizeof(nRF24_TEST_ADDR) - 1U; idx++) {
		if (rxbuf[idx] != *ptr++) {
			// The transceiver is absent
			return 0U;
		}
	}

	// The transceiver is present
	return !0U;
}

// Set transceiver power mode
// input:
//   mode - new state of power mode, one of nRF24_PWR_xx values
void nRF24_SetPowerMode(nrf24_pwr_mode_t mode) {
	uint8_t reg;

	reg = nRF24_ReadReg(nRF24_REG_CONFIG);
	if (mode == nRF24_PWR_UP) {
		// Set the PWR_UP bit of CONFIG register to wake the transceiver
		// It goes into Standby-I mode with consumption about 26uA
		reg |= nRF24_CONFIG_PWR_UP;
	} else {
		// Clear the PWR_UP bit of CONFIG register to put the transceiver
		// It goes into Power Down mode with consumption about 900nA
		reg &= ~nRF24_CONFIG_PWR_UP;
	}
	nRF24_WriteReg(nRF24_REG_CONFIG, reg);
}

void nrf24_powerdown() {
	nRF24_SetPowerMode(nRF24_PWR_DOWN);
}

void nrf24_powerup() {
	nRF24_SetPowerMode(nRF24_PWR_UP);
}

// Set transceiver operational mode
// input:
//   mode - operational mode, one of nRF24_MODE_xx values
void nRF24_SetOperationalMode(nrf24_mode_t mode) {
	uint8_t reg;

	// Configure PRIM_RX bit of the CONFIG register
	reg = nRF24_ReadReg(nRF24_REG_CONFIG);
	reg &= ~nRF24_CONFIG_PRIM_RX;
	reg |= (mode & nRF24_CONFIG_PRIM_RX );
	nRF24_WriteReg(nRF24_REG_CONFIG, reg);
}

void nrf24_set_mode_rx() {
	nRF24_SetOperationalMode(nRF24_MODE_RX);
}

void nrf24_set_mode_tx() {
	nRF24_SetOperationalMode(nRF24_MODE_TX);
}

// Configure transceiver CRC scheme
// input:
//   scheme - CRC scheme, one of nRF24_CRC_xx values
// note: transceiver will forcibly turn on the CRC in case if auto acknowledgment
//       enabled for at least one RX pipe
void nRF24_SetCRCScheme(nrf24_crc_scheme_t scheme) {
	uint8_t reg;

	// Configure EN_CRC[3] and CRCO[2] bits of the CONFIG register
	reg = nRF24_ReadReg(nRF24_REG_CONFIG);
	reg &= ~nRF24_MASK_CRC;
	reg |= (scheme & nRF24_MASK_CRC );
	nRF24_WriteReg(nRF24_REG_CONFIG, reg);
}

void nrf24_set_crc_scheme_1byte() {
	nRF24_SetCRCScheme(nRF24_CRC_1byte);
}

void nrf24_set_crc_scheme_2byte() {
	nRF24_SetCRCScheme(nRF24_CRC_2byte);
}

void nrf24_set_crc_scheme_off() {
	nRF24_SetCRCScheme(nRF24_CRC_off);
}

// Set RF frequency channel
// input:
//   channel - radio frequency channel, can be a value from 0 to 127
// note: frequency will be (2400 + channel)MHz
// note: PLOS_CNT[7:4] bits of the OBSERVER_TX register will be reset
void nRF24_SetRFChannel(uint8_t channel) {
	nRF24_WriteReg(nRF24_REG_RF_CH, channel);
}

// Set automatic retransmission parameters
// input:
//   ard - auto retransmit delay, one of nRF24_ARD_xx values
//   arc - count of auto retransmits, value form 0 to 15
// note: in order to disable automatic retransmission,
//       the value of the variable XXX should be zero
void nRF24_SetAutoRetr(nrf24_ard_delays_t ard, uint8_t arc) {
	if (arc > 15) {
		arc = 15;
	}
	nRF24_WriteReg(
	nRF24_REG_SETUP_RETR,
			(uint8_t) ((ard << 4) | (arc & nRF24_MASK_RETR_ARC )));
}

// Set of address widths
// input:
//   addr_width - RX/TX address field width, value from 3 to 5
// note: this setting is common for all pipes
void nRF24_SetAddrWidth(nrf24_addr_width_t addr_width) {
	nRF24_WriteReg(nRF24_REG_SETUP_AW, addr_width - 2U);
}

// Set static RX address for a specified pipe
// input:
//   pipe - pipe to configure address, one of nRF24_PIPEx values
//   addr - pointer to the buffer with address
// note: pipe can be a number from 0 to 5 (RX pipes) and 6 (TX pipe)
// note: the length of the buffer for pipes 0 and 1 must not be less
//       of the actual address width configured in the transceiver
// note: for the pipes[2..5] only the first byte from the addr buffer
//       will be written since the pipes[1..5] share the four
//       most significant address bytes
// note: always ensure that none of the data pipes have the same address
void nRF24_SetAddr(nrf24_pipes_t pipe, const uint8_t *addr) {
	uint8_t addr_width;

	// RX_ADDR_Px register
	switch (pipe) {
	case nRF24_PIPETX:
	case nRF24_PIPE0:
	case nRF24_PIPE1:
		// Get address width
		addr_width = nRF24_GetAddrWidth();
#if (!nRF24_ADDR_REVERSE)
		nRF24_WriteMBReg(
		nRF24_CMD_W_REGISTER | nRF24_ADDR_REGS[pipe], (uint8_t*) addr,
				addr_width);
#else
			// Write address in reverse order
			nRF24_CSN_L;
			nRF24_LL_RW(nRF24_CMD_W_REGISTER | nRF24_ADDR_REGS[pipe]);
			while (addr_width--) { nRF24_LL_RW(*(addr + addr_width)); }
			nRF24_CSN_H;
#endif // nRF24_ADDR_REVERSE
		break;
	case nRF24_PIPE2:
	case nRF24_PIPE3:
	case nRF24_PIPE4:
	case nRF24_PIPE5:
		// Write first byte from the addr buffer,
		// it will be the LSByte of the pipe address
		nRF24_WriteReg(nRF24_ADDR_REGS[pipe], *addr);
		break;
	default:
		// Incorrect pipe number -> do nothing
		break;
	}
}

// Configure RF output power in TX mode
// input:
//   tx_pwr - RF output power, one of nRF24_TXPWR_xx values
void nRF24_SetTXPower(nrf24_txpwr_t tx_pwr) {
	uint8_t reg;

	// Configure RF_PWR[2:1] bits of the RF_SETUP register
	reg = nRF24_ReadReg(nRF24_REG_RF_SETUP);
	reg &= ~nRF24_MASK_RF_PWR;
	reg |= tx_pwr;
	nRF24_WriteReg(nRF24_REG_RF_SETUP, reg);
}

// Configure transceiver data rate
// input:
//   data_rate - data rate, one of nRF24_DR_xx values
void nRF24_SetDataRate(nrf24_datarates_t data_rate) {
	uint8_t reg;

	// Configure RF_DR_LOW[5] and RF_DR_HIGH[3] bits of the RF_SETUP register
	reg = nRF24_ReadReg(nRF24_REG_RF_SETUP);
	reg &= ~nRF24_MASK_DATARATE;
	reg |= data_rate;
	nRF24_WriteReg(nRF24_REG_RF_SETUP, reg);
}

// Configure a specified RX pipe
// input:
//   pipe - number of the RX pipe, value from 0 to 5
//   aa_state - state of auto acknowledgment, one of nRF24_AA_xx values
//   payload_len - payload length in bytes
void nRF24_SetRXPipe(nrf24_rx_pipes_t pipe, nrf24_aa_t aa_state,
		uint8_t payload_len) {
	uint8_t reg;
	if (payload_len > 32) {
		payload_len = 32;
	}
	// Enable the specified pipe (EN_RXADDR register)
	reg =
			(nRF24_ReadReg(nRF24_REG_EN_RXADDR) | (1U << pipe))
					& nRF24_MASK_EN_RX;
	nRF24_WriteReg(nRF24_REG_EN_RXADDR, reg);

	// Set RX payload length (RX_PW_Px register)
	nRF24_WriteReg(nRF24_REG_RX_PW_P0 + pipe, payload_len & nRF24_MASK_RX_PW);

	// Set auto acknowledgment for a specified pipe (EN_AA register)
	reg = nRF24_ReadReg(nRF24_REG_EN_AA);
	if (aa_state == nRF24_AA_ON) {
		reg |= (1U << pipe);
	} else {
		reg &= ~(1U << pipe);
	}
	nRF24_WriteReg(nRF24_REG_EN_AA, reg);
}

// Disable specified RX pipe
// input:
//   PIPE - number of RX pipe, value from 0 to 5
void nRF24_ClosePipe(nrf24_rx_pipes_t pipe) {
	uint8_t reg;

	reg = nRF24_ReadReg(nRF24_REG_EN_RXADDR);
	reg &= ~(1U << pipe);
	reg &= nRF24_MASK_EN_RX;
	nRF24_WriteReg(nRF24_REG_EN_RXADDR, reg);
}

// Enable the auto retransmit (a.k.a. enhanced ShockBurst) for the specified RX pipe
// input:
//   pipe - number of the RX pipe, value from 0 to 5
void nRF24_EnableAA(nrf24_rx_pipes_t pipe) {
	uint8_t reg;

	// Set bit in EN_AA register
	reg = nRF24_ReadReg(nRF24_REG_EN_AA);
	reg |= (1U << pipe);
	nRF24_WriteReg(nRF24_REG_EN_AA, reg);
}

void nrf24_enable_ShockBurst(nrf24_rx_pipes_t pipe) {
	nRF24_EnableAA(pipe);
}

// Disable the auto retransmit (a.k.a. enhanced ShockBurst) for one or all RX pipes
// input:
//   pipe - number of the RX pipe, value from 0 to 5, any other value will disable AA for all RX pipes
void nRF24_DisableAA(uint8_t pipe) {
	if (pipe > 5U) {
		// Disable Auto-ACK for ALL pipes
		nRF24_WriteReg(nRF24_REG_EN_AA, 0x00);
	} else {
		// Clear bit in the EN_AA register
		uint8_t reg;
		reg = nRF24_ReadReg(nRF24_REG_EN_AA);
		reg &= ~(1U << pipe);
		nRF24_WriteReg(nRF24_REG_EN_AA, reg);
	}
}

void nrf24_disable_ShockBurst(nrf24_rx_pipes_t pipe) {
	nRF24_DisableAA(pipe);
}

// Get address length
// return: the length of the address configured in the transceiver,
//         value from 3 to 5
uint8_t nRF24_GetAddrWidth(void) {
	return nRF24_ReadReg(nRF24_REG_SETUP_AW) + 2U;
}

// Get value of the STATUS register
// return: value of STATUS register
uint8_t nRF24_GetStatus(void) {
	return nRF24_ReadReg(nRF24_REG_STATUS);
}

// Get pending IRQ flags
// return: current status of RX_DR, TX_DS and MAX_RT bits of the STATUS register
uint8_t nRF24_GetIRQFlags(void) {
	return (nRF24_ReadReg(nRF24_REG_STATUS) & nRF24_MASK_STATUS_IRQ );
}

// Get status of the RX FIFO
// return: one of the nRF24_STATUS_RXFIFO_xx values
uint8_t nRF24_GetStatus_RXFIFO(void) {
	return (nRF24_ReadReg(nRF24_REG_FIFO_STATUS) & nRF24_MASK_RXFIFO );
}

// Get status of the TX FIFO
// return: one of the nRF24_STATUS_TXFIFO_xx values
// note: the TX_REUSE bit ignored
uint8_t nRF24_GetStatus_TXFIFO(void) {
	return ((nRF24_ReadReg(nRF24_REG_FIFO_STATUS) & nRF24_MASK_TXFIFO ) >> 4);
}

// Get pipe number for the payload available for reading from RX FIFO
// return: pipe number or 0x07 if the RX FIFO is empty
uint8_t nRF24_GetRXSource(void) {
	return ((nRF24_ReadReg(nRF24_REG_STATUS) & nRF24_MASK_RX_P_NO ) >> 1);
}

// Get auto retransmit statistic
// return: value of OBSERVE_TX register which contains two counters encoded in nibbles:
//   high - lost packets count (max value 15, can be reseted by write to RF_CH register)
//   low  - retransmitted packets count (max value 15, reseted when new transmission starts)
uint8_t nRF24_GetRetransmitCounters(void) {
	return (nRF24_ReadReg(nRF24_REG_OBSERVE_TX));
}

// Reset packet lost counter (PLOS_CNT bits in OBSERVER_TX register)
void nRF24_ResetPLOS(void) {
	uint8_t reg;

	// The PLOS counter is reset after write to RF_CH register
	reg = nRF24_ReadReg(nRF24_REG_RF_CH);
	nRF24_WriteReg(nRF24_REG_RF_CH, reg);
}

// Flush the TX FIFO
void nRF24_FlushTX(void) {
	nRF24_WriteReg(nRF24_CMD_FLUSH_TX, nRF24_CMD_NOP);
}

// Flush the RX FIFO
void nRF24_FlushRX(void) {
	nRF24_WriteReg(nRF24_CMD_FLUSH_RX, nRF24_CMD_NOP);
}

// Clear any pending IRQ flags
void nRF24_ClearIRQFlags(void) {
	uint8_t reg;

	// Clear RX_DR, TX_DS and MAX_RT bits of the STATUS register
	reg = nRF24_ReadReg(nRF24_REG_STATUS);
	reg |= nRF24_MASK_STATUS_IRQ;
	nRF24_WriteReg(nRF24_REG_STATUS, reg);
}

// Write TX payload
// input:
//   pBuf - pointer to the buffer with payload data
//   length - payload length in bytes
void nRF24_WritePayload(uint8_t *pBuf, uint8_t length) {
	nRF24_WriteMBReg(nRF24_CMD_W_TX_PAYLOAD, pBuf, length);
}

// Read top level payload available in the RX FIFO
// input:
//   pBuf - pointer to the buffer to store a payload data
//   length - pointer to variable to store a payload length
// return: one of nRF24_RX_xx values
//   nRF24_RX_PIPEX - packet has been received from the pipe number X
//   nRF24_RX_EMPTY - the RX FIFO is empty
nRF24_RXResult nRF24_ReadPayload(uint8_t *pBuf, uint8_t *length) {
	uint8_t pipe;

	// Extract a payload pipe number from the STATUS register
	pipe = (nRF24_ReadReg(nRF24_REG_STATUS) & nRF24_MASK_RX_P_NO ) >> 1;

	// RX FIFO empty?
	if (pipe < 6U) {
		// Get payload length
		*length = nRF24_ReadReg(nRF24_REG_RX_PW_P0 + pipe);

		// Read a payload from the RX FIFO
		if (*length) {
			nRF24_ReadMBReg(nRF24_CMD_R_RX_PAYLOAD, pBuf, *length);
		}

		return ((nRF24_RXResult) pipe);
	}

	// The RX FIFO is empty
	*length = 0U;

	return nRF24_RX_EMPTY;
}

// Function to transmit data packet
// input:
//   pBuf - pointer to the buffer with data to transmit
//   length - length of the data buffer in bytes
// return: one of nRF24_TX_xx values
nRF24_TXResult nRF24_TransmitPacket(uint8_t *pBuf, uint8_t length) {
	volatile uint32_t wait = nRF24_WAIT_TIMEOUT;
	uint8_t status;

	flags.tx_busy = 1;

	// Deassert the CE pin (in case if it still high)
	nRF24_CE_L;

	// Transfer a data from the specified buffer to the TX FIFO
	nRF24_WritePayload(pBuf, length);

	// Start a transmission by asserting CE pin (must be held at least 10us)
	nRF24_CE_H;

	// Poll the transceiver status register until one of the following flags will be set:
	//   TX_DS  - means the packet has been transmitted
	//   MAX_RT - means the maximum number of TX retransmits happened
	// note: this solution is far from perfect, better to use IRQ instead of polling the status
	do {
		status = nRF24_GetStatus();
		if (status & (nRF24_FLAG_TX_DS | nRF24_FLAG_MAX_RT )) {
			break;
		}
	} while (wait--);

	// Deassert the CE pin (Standby-II --> Standby-I)
	nRF24_CE_L;

	if (!wait) {
		// Timeout
		return nRF24_TX_TIMEOUT;
	}

	// Clear pending IRQ flags
	nRF24_ClearIRQFlags();

	if (status & nRF24_FLAG_MAX_RT) {
		// Auto retransmit counter exceeds the programmed maximum limit (FIFO is not removed)
		return nRF24_TX_MAXRT;
	}

	if (status & nRF24_FLAG_TX_DS) {
		// Successful transmission
		return nRF24_TX_SUCCESS;
	}

	// Some banana happens, a payload remains in the TX FIFO, flush it
	nRF24_FlushTX();

	return nRF24_TX_ERROR;
}

void nrf_enable_irq(nrf_irq_t irq) {
	uint8_t reg;
	// Configure PRIM_RX bit of the CONFIG register
	reg = nRF24_ReadReg(nRF24_REG_CONFIG);
	reg &= ~irq;
	nRF24_WriteReg(nRF24_REG_CONFIG, reg);
	return;
}

void nrf_disable_irq(nrf_irq_t irq) {
	uint8_t reg;
	// Configure PRIM_RX bit of the CONFIG register
	reg = nRF24_ReadReg(nRF24_REG_CONFIG);
	reg |= irq;
	nRF24_WriteReg(nRF24_REG_CONFIG, reg);
	return;
}

void nrf24_pipe_set_payload_length(int8_t pipe, uint8_t length) {

}

void nrf24_init_rx() {
	// Configure RX PIPE
	uint32_t addr = NRF_ADDR_WALLBOX;
	nRF24_SetAddr(NRF_WALLBOX_PIPE, (const uint8_t*) &addr); // program address for pipe
	nRF24_SetRXPipe(NRF_WALLBOX_PIPE, nRF24_AA_ON, 32); // Auto-ACK: enabled, payload length: 10 bytes

	// Set operational mode (PRX == receiver)
	nRF24_SetOperationalMode(nRF24_MODE_RX);

	nrf_enable_irq(nrf_irq_rx_dr);

	// Put the transceiver to the RX mode
	nRF24_CE_H;

}

void nrf24_init_gen() {
	// This is simple receiver with Enhanced ShockBurst:
	//   - RX address: 'ESB'
	//   - payload: 10 bytes
	//   - RF channel: 40 (2440MHz)
	//   - data rate: 2Mbps
	//   - CRC scheme: 2 byte

	// The transmitter sends a 10-byte packets to the address 'ESB' with Auto-ACK (ShockBurst enabled)
	nRF24_Init();
	nRF24_Check();
	// Set RF channel
	nRF24_SetRFChannel(40);

	// Set data rate
	nRF24_SetDataRate(nRF24_DR_250kbps);

	// Set CRC scheme
	nRF24_SetCRCScheme(nRF24_CRC_2byte);

	// Set address width, its common for all pipes (RX and TX)
	nRF24_SetAddrWidth(4);

	// Set TX power for Auto-ACK (maximum, to ensure that transmitter will hear ACK reply)
	nRF24_SetTXPower(nRF24_TXPWR_0dBm);

	// Wake the transceiver
	nRF24_SetPowerMode(nRF24_PWR_UP);

}


int8_t nrf_write_frame(nrf24_frame_t frame) {
	memcpy(&nrf24_frame, &frame, sizeof(frame));
	return 0;
}

nrf24_frame_t nrf_get_frame() {
	return nrf24_frame;
}

void nrf24_init_tx() {
	// Configure TX PIPE
	const uint32_t addr = NRF_ADDR_DISP;
	nRF24_SetAddr(nRF24_PIPETX, (const uint8_t*) &addr); // program address for pipe#0, must be same as TX (for Auto-ACK)
	// Enable Auto-ACK for pipe#0 (for ACK packets)
	nRF24_EnableAA(NRF_WALLBOX_PIPE);

	// Set TX power (maximum)
	nRF24_SetTXPower(nRF24_TXPWR_0dBm);

	// Configure auto retransmit: 10 retransmissions with pause of 2500s in between
	nRF24_SetAutoRetr(nRF24_ARD_2500us, 15);

	// Set operational mode (PTX == transmitter)
	nRF24_SetOperationalMode(nRF24_MODE_TX);

	nrf_disable_irq(nrf_irq_rx_dr);
	nrf_enable_irq(nrf_irq_tx_ds);

	// Clear any pending IRQ flags
	nRF24_ClearIRQFlags();

}

int8_t nrf_add_qeue(uint8_t cmd, data_union_t *ptr, uint32_t addr) {

	if (cmd > NRF24_CMD_MAX_ENUM) {
		return -1;
	}

	nrf24_frame_queue_t item;

	item.addr = addr;
	item.frame.cmd = cmd;
	item.frame.size = 24;
	if (ptr != NULL) {
		memcpy(&item.frame.data[0], ptr, NRF24_TX_SIZE);
	}
	if (nrf_queue_enqueue(&item) == ENQUEUE_RESULT_FULL) {
		return -1;
	}

	return 0;
}

static uint32_t nrf24_tx_ctr = 0;

int8_t nrf_transmit_next_item() {

	nrf24_frame_queue_t item;
	enum dequeue_result res;
	res = nrf_queue_dequeue(&item);
	nRF24_SetAddr(nRF24_PIPETX, (const uint8_t*) &item.addr);
	if (res == DEQUEUE_RESULT_SUCCESS) {
		if (item.frame.cmd < NRF24_CMD_MAX_ENUM) {
			if (item.frame.size <= 32) {
				item.frame.tx_ctr = nrf24_tx_ctr;
				nRF24_TXResult nrf_res = nRF24_TransmitPacket(
						(uint8_t*) &item.frame, 32);
				uint8_t otx = nRF24_GetRetransmitCounters();
				UNUSED(otx);
				if (nrf_res != nRF24_TX_SUCCESS) {
					nRF24_ResetPLOS();
					return -1;
				}
				nrf24_tx_ctr++;
				return 0;
			}
		}
	}
	return -1;
}
