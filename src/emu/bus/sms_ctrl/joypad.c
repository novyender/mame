/**********************************************************************

    Sega Master System "Control Pad"/generic joystick emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "joypad.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type SMS_JOYPAD = &device_creator<sms_joypad_device>;


static INPUT_PORTS_START( sms_joypad )
	PORT_START("JOYPAD")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_DEVICE 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_DEVICE 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_DEVICE 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_DEVICE 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )   // Vcc
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_DEVICE  // TL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )   // TH
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_DEVICE  // TR
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor sms_joypad_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( sms_joypad );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  sms_joypad_device - constructor
//-------------------------------------------------

sms_joypad_device::sms_joypad_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, SMS_JOYPAD, "Control Pad", tag, owner, clock, "sms_joypad", __FILE__),
	device_sms_control_port_interface(mconfig, *this),
	m_joypad(*this, "JOYPAD")
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void sms_joypad_device::device_start()
{
}


//-------------------------------------------------
//  sms_peripheral_r - joypad read
//-------------------------------------------------

UINT8 sms_joypad_device::peripheral_r()
{
	return m_joypad->read();
}
