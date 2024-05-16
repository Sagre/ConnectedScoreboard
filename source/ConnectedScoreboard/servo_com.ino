#include <PololuMaestro.h>
#include <servo_com.h>

/* On boards with a hardware serial port available for use, use
that port to communicate with the Maestro. For other boards,
create a SoftwareSerial object using pin 10 to receive (RX) and
pin 11 to transmit (TX). */


/* Next, create a Maestro object using the serial port.

Uncomment one of MicroMaestro or MiniMaestro below depending
on which one you have. */
HardwareSerial maestroSerial(1);
MicroMaestro maestro(maestroSerial);


void servo_com_init(void)
{
  // Set the serial baud rate.
  maestroSerial.begin(9600, SERIAL_8N1, 47, 48);
  maestroSerial.print(0xAA);
}

void servo_com_set_target(servo_com_channels_e channel_e, uint16_t deg)
{
  uint16_t target_quarter_microseconds = 4000 * (1 + ((float)deg/90));
  maestro.setTarget((uint8_t) channel_e, target_quarter_microseconds);
}