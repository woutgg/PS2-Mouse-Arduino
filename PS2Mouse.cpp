#include "HardwareSerial.h"
#include "Arduino.h"
#include "PS2Mouse.h"
#include "Streaming.h"

#define while25usec(cond) { \
	unsigned long stop = microSeconds() + 25; \
	while ((cond) && microSeconds() < stop) {;} \
}

PS2Mouse::PS2Mouse(int clock_pin, int data_pin, int mode)
: _clock_pin(clock_pin), _data_pin(data_pin), _mode(mode), _initialized(false), _reporting_enabled(false)
{ /* empty */ }

bool PS2Mouse::initialize() {
	pull_high(_clock_pin);
	pull_high(_data_pin);
	delay(20);
	Serial << "+";
	write(0xff); // Send Reset to the mouse
	Serial << "+";
	read_byte();  // Read ack byte
	Serial << "+";
	//delay(200); // Not sure why this needs the delay
	int bat_result = read_byte();  // result of BAT (0xAA=succes, 0xFC=error)
	Serial << "+";
	int dev_id = read_byte();  // device ID (0x00 or optionally 0x03 for intellimouse after magic init)
	Serial << "+";
	delay(20); // Not sure why this needs the delay

	if (bat_result != 0xAA) return false;

	if (_mode == PS2_REMOTE) {
		set_remote_mode();
	} else {
		enable_data_reporting(); // Tell the mouse to start sending data again
	}
	delayMicroseconds(100);

	_initialized = 1;
	return true;
}

int PS2Mouse::clock_pin() const {
	return _clock_pin;
}

int PS2Mouse::data_pin() const {
	return _data_pin;
}

void PS2Mouse::set_remote_mode() {
	set_mode(0xf0);
	_mode = PS2_REMOTE;
}

void PS2Mouse::set_stream_mode() {
	set_mode(0xea);
	_mode = PS2_STREAM;
}

// This only effects data reporting in Stream mode.
void PS2Mouse::enable_data_reporting() {
	if (!_reporting_enabled) {
		write(0xf4); // Send enable data reporting
		read_byte(); // Read Ack Byte
		_reporting_enabled = true;
	}
}

// Disabling data reporting in Stream Mode will make it behave like Remote Mode
void PS2Mouse::disable_data_reporting() {
	if (_reporting_enabled) {
		write(0xf5); // Send disable data reporting
		read_byte(); // Read Ack Byte    
		_reporting_enabled = false;
	}
}

void PS2Mouse::set_resolution(int resolution) {
	if (_mode == PS2_STREAM) {
		enable_data_reporting();
	}
	write(0xe8); // Send Set Resolution
	read_byte(); // Read ack Byte
	write(resolution); // Send resolution setting
	read_byte(); // Read ack Byte
	if (_mode == PS2_STREAM) {
		disable_data_reporting();
	}
	delayMicroseconds(100);
}

void PS2Mouse::set_scaling_2_1() {
	set_mode(0xe7); // Set the scaling to 2:1
}

void PS2Mouse::set_scaling_1_1() {
	set_mode(0xe6); // set the scaling to 1:1
}

void PS2Mouse::set_sample_rate(int rate) {
	if (_mode == PS2_STREAM) {
		disable_data_reporting(); // Tell the mouse to stop sending data.
	}
	write(0xf3); // Tell the mouse we are going to set the sample rate.
	read_byte(); // Read Ack Byte
	write(rate); // Send Set Sample Rate
	read_byte(); // Read ack byte
	if (_mode == PS2_STREAM) {
		enable_data_reporting(); // Tell the mouse to start sending data again
	}
	delayMicroseconds(100);
}

int PS2Mouse::read() {
	return read_byte();
}

void PS2Mouse::write(int data) {
	char i;
	char parity = 1;
	pull_high(_data_pin);
	pull_high(_clock_pin);
	delayMicroseconds(300);
	pull_low(_clock_pin);
	delayMicroseconds(300);
	pull_low(_data_pin);
	delayMicroseconds(10);
	pull_high(_clock_pin); // Start Bit
	while (digitalRead(_clock_pin)) {;} // wait for mouse to take control of clock)
	// clock is low, and we are clear to send data 
	for (i=0; i < 8; i++) {
		if (data & 0x01) {
			pull_high(_data_pin);
		} else {
			pull_low(_data_pin);
		}
		// wait for clock cycle 
		while (!digitalRead(_clock_pin)) {;}
		while (digitalRead(_clock_pin)) {;}
		parity = parity ^ (data & 0x01);
		data = data >> 1;
	}  
	// parity 
	if (parity) {
		pull_high(_data_pin);
	} else {
		pull_low(_data_pin);
	}
	while (!digitalRead(_clock_pin)) {;}
	while (digitalRead(_clock_pin)) {;}  
	pull_high(_data_pin);
	delayMicroseconds(50);
	while (digitalRead(_clock_pin)) {;}
	while ((!digitalRead(_clock_pin)) || (!digitalRead(_data_pin))) {;} // wait for mouse to switch modes
	pull_low(_clock_pin); // put a hold on the incoming data.
}

int * PS2Mouse::report(int data[]) {
	if (_mode == PS2_REMOTE) {
		write(0xeb); // Send Read Data
		read_byte(); // Read Ack Byte
	}
	data[0] = read(); // Status bit
	data[1] = read_movement_x(data[0]); // X Movement Packet
	data[2] = read_movement_y(data[0]); // Y Movement Packet
	return data;
}


/*********************
 * PRIVATE FUNCTIONS *
 *********************/

int PS2Mouse::read_movement_x(int status) {
	int x = read_byte();
	if (bitRead(status, 4)) {
		for(int i = 8; i < 16; ++i) {
			x |= (1<<i);
		}
	}
	return x;
}

int PS2Mouse::read_movement_y(int status) {
	int y = read_byte();
	if (bitRead(status, 5)) {
		for(int i = 8; i < 16; ++i) {
			y |= (1<<i);
		}
	}
	return y;
}

void PS2Mouse::set_mode(int data) {
	if (_mode == PS2_STREAM) {
		disable_data_reporting(); // Tell the mouse to stop sending data.
	}
	write(data);  // Send Set Mode
	read_byte();  // Read Ack byte
	if (_mode == PS2_STREAM) {
		enable_data_reporting(); // Tell the mouse to start sending data again
	}
	if (_initialized) {
		delayMicroseconds(100);    
	}
}

int PS2Mouse::read_bit() {
	while (digitalRead(_clock_pin)) {;}
	int bit = digitalRead(_data_pin);  
	while (!digitalRead(_clock_pin)) {;}
	return bit;
}

int PS2Mouse::read_byte() {
	int data = 0;
	pull_high(_clock_pin);
	pull_high(_data_pin);
	delayMicroseconds(50);
	while (digitalRead(_clock_pin)) {;}
	delayMicroseconds(5);  // not sure why.
	while (!digitalRead(_clock_pin)) {;} // eat start bit
	for (int i = 0; i < 8; i++) {
		bitWrite(data, i, read_bit());
	}
	read_bit(); // Partiy Bit 
	read_bit(); // Stop bit should be 1
	pull_low(_clock_pin);
	return data;
}

void PS2Mouse::pull_low(int pin) {
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);  
}

void PS2Mouse::pull_high(int pin) {
	pinMode(pin, INPUT);
	digitalWrite(pin, HIGH);
}
