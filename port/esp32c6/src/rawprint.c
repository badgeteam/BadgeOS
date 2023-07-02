
// SPDX-License-Identifier: MIT

#include "rawprint.h"

#include "time.h"
#include "port/hardware.h"

const char hextab[] = "0123456789ABCDEF";

// Simple printer.
void rawprint(const char *msg) {
	if (!msg) return;
	char prev = 0;
	while (*msg) {
		if (*msg == '\r') {
			rawputc('\r');
			rawputc('\n');
		} else if (*msg == '\n') {
			if (prev != '\r') {
				rawputc('\r');
				rawputc('\n');
			}
		} else {
			rawputc(*msg);
		}
		prev = *msg;
		msg ++;
	}
}

// Simple printer.
void rawputc(char msg) {
	WRITE_REG(UART0_BASE, msg);
}

// Bin 2 hex printer.
void rawprinthex(uint64_t val, int digits) {
	for (; digits > 0; digits --) {
		rawputc(hextab[(val >> (digits*4-4)) & 15]);
	}
}

// 16-bit double dabble.
void double_dabble_16(uint16_t val, char outbuf[5]) {
	// Shift in first 3 bits.
	uint32_t buf = val >> 13;
	val <<= 3;
	
	// Perform double dabble on the remaining 13 bits.
	for (int i = 0; i < 13; i++) {
		uint32_t mask = ((buf | (buf >> 1)) & (buf >> 2)) | (buf >> 3);
		buf += (mask & 0x11111) * 3;
		buf <<= 1;
		buf |= val >> 15;
		val <<= 1;
	}
	
	// Output the ASCII values.
	for (int i = 0; i < 5; i++) {
		outbuf[4 - i] = '0' + ((buf >> (4*i)) & 15);
	}
}

// 32-bit double dabble.
void double_dabble_32(uint32_t val, char outbuf[10]) {
	// Shift in first 3 bits.
	uint64_t buf = val >> 29;
	val <<= 3;
	
	// Perform double dabble on the remaining 29 bits.
	for (int i = 0; i < 29; i++) {
		uint64_t mask = ((buf | (buf >> 1)) & (buf >> 2)) | (buf >> 3);
		buf += (mask & 0x1111111111) * 3;
		buf <<= 1;
		buf |= val >> 31;
		val <<= 1;
	}
	
	// Output the ASCII values.
	for (int i = 0; i < 10; i++) {
		outbuf[9 - i] = '0' + ((buf >> (4*i)) & 15);
	}
}

// 64-bit double dabble.
void double_dabble_64(uint64_t val, char outbuf[20]) {
	// Shift in first 3 bits.
	uint64_t buf_lo = val >> 61;
	uint16_t buf_hi = 0;
	val <<= 3;
	
	// Perform double dabble on the remaining 61 bits.
	for (int i = 0; i < 61; i++) {
		// Increment the digits.
		uint64_t mask = ((buf_lo | (buf_lo >> 1)) & (buf_lo >> 2)) | (buf_lo >> 3);
		buf_lo += (mask & 0x1111111111111111) * 3;
		mask = ((buf_hi | (buf_hi >> 1)) & (buf_hi >> 2)) | (buf_hi >> 3);
		buf_hi += (mask & 0x1111) * 3;
		
		// Shift the bits.
		buf_hi <<= 1;
		buf_hi |= buf_lo >> 63;
		buf_lo <<= 1;
		buf_lo |= val >> 63;
		val    <<= 1;
	}
	
	// Output the ASCII values.
	for (int i = 0; i < 16; i++) {
		outbuf[19 - i] = '0' + ((buf_lo >> (4*i)) & 15);
	}
	for (int i = 0; i < 4; i++) {
		outbuf[3 - i] = '0' + ((buf_hi >> (4*i)) & 15);
	}
}

// Bin 2 dec printer.
void rawprintudec(uint64_t val, int digits) {
	char buf[20];
	int buf_digits = 0;
	
	// Perform double dabble.
	if (!(val & 0xffffffffffff0000)) {
		// 16-bit.
		double_dabble_16(val, buf);
		buf_digits = 5;
	} else if (!(val & 0xffffffff00000000)) {
		// 32-bit.
		double_dabble_32(val, buf);
		buf_digits = 10;
	} else {
		// 64-bit.
		double_dabble_64(val, buf);
		buf_digits = 20;
	}
	
	// Count number of digits in output bouffler.
	int i;
	for (i = 0; i+1 < buf_digits && buf[i] == '0'; i++);
	int min_digits = buf_digits - i;
	if (digits < min_digits) {
		digits = min_digits;
	}
	
	// Padding with zeroes.
	for (int i = buf_digits; i < digits; i++) {
		rawputc('0');
	}
	
	// Print out remaining digits.
	if (buf_digits > digits) {
		for (int i = 0; i < digits; i++) {
			rawputc(buf[i - digits + buf_digits]);
		}
	} else {
		for (int i = 0; i < buf_digits; i++) {
			rawputc(buf[i]);
		}
	}
}

// Bin 2 dec printer.
void rawprintdec(int64_t val, int digits) {
	if (val < 0) {
		rawputc('-');
		val = -val;
	}
	rawprintudec(val, digits);
}

// Current uptime printer for logging.
void rawprintuptime() {
	uint64_t now    = time_us() / 1000;
	uint16_t millis = now % 1000;
	uint64_t secs   = now / 1000;
	rawputc('[');
	rawprintudec(secs, 5);
	rawputc('.');
	rawprintudec(millis, 3);
	rawputc(']');
}
