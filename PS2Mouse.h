#ifndef PS2Mouse_h
#define PS2Mouse_h

#define PS2_REMOTE 1
#define PS2_STREAM 2

class PS2Mouse {
public:
	PS2Mouse(int, int, int mode = PS2_REMOTE);
	bool initialize();

	int clock_pin() const;
	int data_pin() const;
	
	void set_remote_mode();
	void set_stream_mode();
	void enable_data_reporting();
	void disable_data_reporting();
	void set_resolution(int);
	void set_scaling_2_1();
	void set_scaling_1_1();
	void set_sample_rate(int);

	int read();
	void write(int);
	int* report(int data[]);
	
private:
	const int _clock_pin;
	const int _data_pin;

	int _mode;
	int _initialized;
	int _reporting_enabled;

	int read_movement_x(int);
	int read_movement_y(int);
	void set_mode(int);

	int read_bit();
	int read_byte();
	void pull_high(int);
	void pull_low(int);
};

#endif

