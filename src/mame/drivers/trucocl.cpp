// license:BSD-3-Clause
// copyright-holders:Ernesto Corvi
/***************************************************************************

Truco Clemente (c) 1991 Miky SRL

driver by Ernesto Corvi

Notes:
- After one game you can't play anymore.
- Audio is almost there.
- I think this runs on a heavily modified PacMan type of board.

----------------------------------
Additional Notes (Roberto Fresca):
----------------------------------
Mainboard: Pacman bootleg jamma board.
Daughterboard: Custom made, plugged in the 2 roms and Z80 mainboard sockets.

  - 01 x Z80
  - 03 x 27c010
  - 02 x am27s19
  - 03 x GAL 16v8b      (All of them have the same contents... Maybe read protected.)
  - 01 x PAL CE 20v8h   (The fuse map is suspect too)
  - 01 x lm324n

  To not overload the driver, I put the rest of technical info in
  http://robbie.mameworld.info/trucocl.htm

- Added 2 "hidden" color proms (am27s19)
- One GAL is connected to the color proms inputs.
- The name of the company is "Miky SRL" instead of "Caloi Miky SRL".
  Caloi (Carlos Loiseau), is the Clemente's creator.

***************************************************************************/

#include "emu.h"
#include "includes/trucocl.h"

#include "cpu/z80/z80.h"
#include "machine/watchdog.h"
#include "sound/volt_reg.h"
#include "screen.h"
#include "speaker.h"


WRITE8_MEMBER(trucocl_state::irq_enable_w)
{
	m_irq_mask = (data & 1) ^ 1;
}


void trucocl_state::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_DAC_IRQ:
		m_maincpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);
		break;
	default:
		assert_always(false, "Unknown id in trucocl_state::device_timer");
	}
}


WRITE8_MEMBER(trucocl_state::audio_dac_w)
{
	uint8_t *rom = memregion("maincpu")->base();
	int dac_address = ( data & 0xf0 ) << 8;
	int sel = ( ( (~data) >> 1 ) & 2 ) | ( data & 1 );

	if ( m_cur_dac_address != dac_address )
	{
		m_cur_dac_address_index = 0;
		m_cur_dac_address = dac_address;
	}
	else
	{
		m_cur_dac_address_index++;
	}

	if ( sel & 1 )
		dac_address += 0x10000;

	if ( sel & 2 )
		dac_address += 0x10000;

	dac_address += 0x10000;

	m_dac->write(rom[dac_address+m_cur_dac_address_index]);

	m_dac_irq_timer->adjust(attotime::from_hz( 16000 ));
}

void trucocl_state::main_map(address_map &map)
{
	map(0x0000, 0x3fff).rom();
	map(0x4000, 0x43ff).ram().w(FUNC(trucocl_state::trucocl_videoram_w)).share("videoram");
	map(0x4400, 0x47ff).ram().w(FUNC(trucocl_state::trucocl_colorram_w)).share("colorram");
	map(0x4c00, 0x4fff).ram();
	map(0x5000, 0x5000).w(FUNC(trucocl_state::irq_enable_w));
	map(0x5000, 0x503f).portr("IN0");
	map(0x5080, 0x5080).w(FUNC(trucocl_state::audio_dac_w));
	map(0x50c0, 0x50c0).w("watchdog", FUNC(watchdog_timer_device::reset_w));
	map(0x8000, 0xffff).rom();
}

static INPUT_PORTS_START( trucocl )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_IMPULSE(2)
INPUT_PORTS_END


static const gfx_layout tilelayout =
{
	8,8,        /* 8*8 characters */
	0x10000/32, /* 2048 characters */
	4,          /* 4 bits per pixel */
	{ 0, 1,2,3 },
	{ 0, 4, 0x8000*8+0,0x8000*8+4, 8*8+0, 8*8+4, 0x8000*8+8*8+0,0x8000*8+8*8+4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8        /* every char takes 16 consecutive bytes */
};



static GFXDECODE_START( gfx_trucocl )
	GFXDECODE_ENTRY( "gfx1", 0,         tilelayout,      0, 2 )
	GFXDECODE_ENTRY( "gfx1", 0x10000, tilelayout,      0, 2 )
GFXDECODE_END

INTERRUPT_GEN_MEMBER(trucocl_state::trucocl_interrupt)
{
	if(m_irq_mask)
		device.execute().set_input_line(0, HOLD_LINE);

}

MACHINE_CONFIG_START(trucocl_state::trucocl)
	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", Z80, 18432000/6)
	MCFG_DEVICE_PROGRAM_MAP(main_map)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", trucocl_state,  trucocl_interrupt)

	MCFG_WATCHDOG_ADD("watchdog")

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(32*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(trucocl_state, screen_update_trucocl)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_DEVICE_ADD("gfxdecode", GFXDECODE, "palette", gfx_trucocl)
	MCFG_PALETTE_ADD("palette", 32)
	MCFG_PALETTE_INIT_OWNER(trucocl_state, trucocl)

	/* sound hardware */
	SPEAKER(config, "speaker").front_center();

	MCFG_DEVICE_ADD("dac", DAC_8BIT_R2R, 0) MCFG_SOUND_ROUTE(ALL_OUTPUTS, "speaker", 0.5) // unknown DAC
	MCFG_DEVICE_ADD("vref", VOLTAGE_REGULATOR, 0) MCFG_VOLTAGE_REGULATOR_OUTPUT(5.0)
	MCFG_SOUND_ROUTE(0, "dac", 1.0, DAC_VREF_POS_INPUT) MCFG_SOUND_ROUTE(0, "dac", -1.0, DAC_VREF_NEG_INPUT)
MACHINE_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( trucocl )
	ROM_REGION( 0x40000, "maincpu", 0 ) /* ROMs + space for additional RAM + samples */
	ROM_LOAD( "trucocl.01", 0x00000, 0x20000, CRC(c9511c37) SHA1(d6a0fa573c8d2faf1a94a2be26fcaafe631d0699) )
	ROM_LOAD( "trucocl.03", 0x20000, 0x20000, CRC(b37ce38c) SHA1(00bd506e9a03cb8ed65b0b599514db6b9b0ee5f3) ) /* samples */

	ROM_REGION( 0x20000, "gfx1", 0 )
	ROM_LOAD( "trucocl.02", 0x0000, 0x20000, CRC(bda803e5) SHA1(e4fee42f23be4e0dc8926b6294e4b3e4a38ff185) ) /* tiles */

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "27s19.u2",    0x0000, 0x0020, CRC(75aeff6a) SHA1(fecd117ec9bb8ac2834d422eb507ec78410aff0f) )
	ROM_LOAD( "27s19.u1",    0x0020, 0x0020, CRC(f952f823) SHA1(adc6a05827b1bc47d84827808c324d93ee0f32b9) )
ROM_END

/*************************************
 *
 *  Game drivers
 *
 *************************************/

void trucocl_state::init_trucocl()
{
	m_cur_dac_address = -1;
	m_cur_dac_address_index = 0;

	m_dac_irq_timer = timer_alloc(TIMER_DAC_IRQ);
}



/******************************************************************************/
//    YEAR  NAME      PARENT  MACHINE  INPUT    STATE          INIT          MONITOR
GAME( 1991, trucocl,  0,      trucocl, trucocl, trucocl_state, init_trucocl, ROT0, "Miky SRL", "Truco Clemente", MACHINE_IMPERFECT_SOUND | MACHINE_NOT_WORKING )
