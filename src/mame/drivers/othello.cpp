// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina
/*

Othello (version 3.0) - Success 1984
-------------------------------------

driver by Tomasz Slanina

CPU Board:
 D780C          - main CPU (Z80)
 HD46505SP      - CRTC
 D780-C         - Sound CPU (Z80)
 AY-3-8910 x2   - Sound
 D7751C         - ADPCM "Speech processor"
 D8243          - I/O Expander for D7751C (8048 based)

Video Board:
 almost empty - 3/4 soldering pins not populated



Todo:

- hook up upd7751c sample player (it works correctly but there's main cpu side write(latch/command) missing)
- correct colors (based on the color DAC (24 resistors) on pcb
- cocktail mode
- map a bunch of unknown read/writes (related to above i think)

Notes:

DSw 1:2
Limit for help/undo (matta):
- when it's off, you can use each of them twice
 every time you win and advance to the next game
- when it's on, you can only use them twice throughout the game

*/

#include "emu.h"

#include "cpu/mcs48/mcs48.h"
#include "cpu/z80/z80.h"
#include "machine/gen_latch.h"
#include "machine/i8243.h"
#include "sound/ay8910.h"
#include "sound/dac.h"
#include "sound/volt_reg.h"
#include "video/mc6845.h"

#include "screen.h"
#include "speaker.h"


#define TILE_WIDTH 6


class othello_state : public driver_device
{
public:
	othello_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_videoram(*this, "videoram"),
		m_maincpu(*this, "maincpu"),
		m_ay1(*this, "ay1"),
		m_ay2(*this, "ay2"),
		m_palette(*this, "palette"),
		m_soundlatch(*this, "soundlatch")
	{
	}

	/* memory pointers */
	required_shared_ptr<uint8_t> m_videoram;

	/* video-related */
	int    m_tile_bank;

	/* misc */
	int   m_ay_select;
	int   m_ack_data;
	uint8_t m_n7751_command;
//  uint32_t m_n7751_rom_address;
	int m_sound_addr;
	int m_n7751_busy;

	/* devices */
	required_device<cpu_device> m_maincpu;
	required_device<ay8910_device> m_ay1;
	required_device<ay8910_device> m_ay2;
	mc6845_device *m_mc6845;
	device_t *m_n7751;
	required_device<palette_device> m_palette;
	required_device<generic_latch_8_device> m_soundlatch;

	DECLARE_READ8_MEMBER(unk_87_r);
	DECLARE_WRITE8_MEMBER(unk_8a_w);
	DECLARE_WRITE8_MEMBER(unk_8c_w);
	DECLARE_READ8_MEMBER(unk_8c_r);
	DECLARE_READ8_MEMBER(sound_ack_r);
	DECLARE_WRITE8_MEMBER(unk_8f_w);
	DECLARE_WRITE8_MEMBER(tilebank_w);
	DECLARE_READ8_MEMBER(latch_r);
	DECLARE_WRITE8_MEMBER(ay_select_w);
	DECLARE_WRITE8_MEMBER(ack_w);
	DECLARE_WRITE8_MEMBER(ay_address_w);
	DECLARE_WRITE8_MEMBER(ay_data_w);
	DECLARE_READ8_MEMBER(n7751_rom_r);
	DECLARE_READ8_MEMBER(n7751_command_r);
	DECLARE_WRITE8_MEMBER(n7751_p2_w);
	DECLARE_WRITE8_MEMBER(n7751_rom_control_w);
	virtual void machine_start() override;
	virtual void machine_reset() override;
	DECLARE_PALETTE_INIT(othello);
	MC6845_UPDATE_ROW(crtc_update_row);
	void othello(machine_config &config);
	void audio_map(address_map &map);
	void audio_portmap(address_map &map);
	void main_map(address_map &map);
	void main_portmap(address_map &map);
};


MC6845_UPDATE_ROW( othello_state::crtc_update_row )
{
	const rgb_t *palette = m_palette->palette()->entry_list_raw();
	int cx, x;
	uint32_t data_address;
	uint32_t tmp;

	const uint8_t *gfx = memregion("gfx")->base();

	for(cx = 0; cx < x_count; ++cx)
	{
		data_address = ((m_videoram[ma + cx] + m_tile_bank) << 4) | ra;
		tmp = gfx[data_address] | (gfx[data_address + 0x2000] << 8) | (gfx[data_address + 0x4000] << 16);

		for(x = 0; x < TILE_WIDTH; ++x)
		{
			bitmap.pix32(y, (cx * TILE_WIDTH + x) ^ 1) = palette[tmp & 0x0f];
			tmp >>= 4;
		}
	}
}

PALETTE_INIT_MEMBER(othello_state, othello)
{
	int i;
	for (i = 0; i < palette.entries(); i++)
	{
		palette.set_pen_color(i, rgb_t(0xff, 0x00, 0xff));
	}

	/* only colors  2,3,7,9,c,d,f are used */
	palette.set_pen_color(0x02, rgb_t(0x00, 0xff, 0x00));
	palette.set_pen_color(0x03, rgb_t(0xff, 0x7f, 0x00));
	palette.set_pen_color(0x07, rgb_t(0x00, 0x00, 0x00));
	palette.set_pen_color(0x09, rgb_t(0xff, 0x00, 0x00));
	palette.set_pen_color(0x0c, rgb_t(0x00, 0x00, 0xff));
	palette.set_pen_color(0x0d, rgb_t(0x7f, 0x7f, 0x00));
	palette.set_pen_color(0x0f, rgb_t(0xff, 0xff, 0xff));
}

void othello_state::main_map(address_map &map)
{
	map(0x0000, 0x1fff).rom();
	map(0x8000, 0x97ff).noprw(); /* not populated */
	map(0x9800, 0x9fff).ram().share("videoram");
	map(0xf000, 0xffff).ram();
}

READ8_MEMBER(othello_state::unk_87_r)
{
	/* n7751_status_r ?  bit 7 = ack/status from device connected  to port 8a? */
	return machine().rand();
}

WRITE8_MEMBER(othello_state::unk_8a_w)
{
	/*


	m_n7751_command = (data & 0x07);
	m_n7751->set_input_line(0, ((data & 0x08) == 0) ? ASSERT_LINE : CLEAR_LINE);
	//m_n7751->set_input_line(0, (data & 0x02) ? CLEAR_LINE : ASSERT_LINE);
	machine().scheduler().boost_interleave(attotime::zero, attotime::from_usec(100));
	*/

	logerror("8a -> %x\n", data);
}

WRITE8_MEMBER(othello_state::unk_8c_w)
{
	logerror("8c -> %x\n", data);
}

READ8_MEMBER(othello_state::unk_8c_r)
{
	return machine().rand();
}

READ8_MEMBER(othello_state::sound_ack_r)
{
	return m_ack_data;
}

WRITE8_MEMBER(othello_state::unk_8f_w)
{
	logerror("8f -> %x\n", data);
}

WRITE8_MEMBER(othello_state::tilebank_w)
{
	m_tile_bank = (data == 0x0f) ? 0x100 : 0x00;
	logerror("tilebank -> %x\n", data);
}

void othello_state::main_portmap(address_map &map)
{
	map.global_mask(0xff);
	map(0x08, 0x08).w("crtc", FUNC(mc6845_device::address_w));
	map(0x09, 0x09).rw("crtc", FUNC(mc6845_device::register_r), FUNC(mc6845_device::register_w));
	map(0x80, 0x80).portr("INP");
	map(0x81, 0x81).portr("SYSTEM");
	map(0x83, 0x83).portr("DSW");
	map(0x86, 0x86).w(FUNC(othello_state::tilebank_w));
	map(0x87, 0x87).r(FUNC(othello_state::unk_87_r));
	map(0x8a, 0x8a).w(FUNC(othello_state::unk_8a_w));
	map(0x8c, 0x8c).rw(FUNC(othello_state::unk_8c_r), FUNC(othello_state::unk_8c_w));
	map(0x8d, 0x8d).r(FUNC(othello_state::sound_ack_r)).w(m_soundlatch, FUNC(generic_latch_8_device::write));
	map(0x8f, 0x8f).w(FUNC(othello_state::unk_8f_w));
}

READ8_MEMBER(othello_state::latch_r)
{
	int retval = m_soundlatch->read(space, 0);
	m_soundlatch->clear_w(space, 0, 0);
	return retval;
}

WRITE8_MEMBER(othello_state::ay_select_w)
{
	m_ay_select = data;
}

WRITE8_MEMBER(othello_state::ack_w)
{
	m_ack_data = data;
}

WRITE8_MEMBER(othello_state::ay_address_w)
{
	if (m_ay_select & 1) m_ay1->address_w(space, 0, data);
	if (m_ay_select & 2) m_ay2->address_w(space, 0, data);
}

WRITE8_MEMBER(othello_state::ay_data_w)
{
	if (m_ay_select & 1) m_ay1->data_w(space, 0, data);
	if (m_ay_select & 2) m_ay2->data_w(space, 0, data);
}

void othello_state::audio_map(address_map &map)
{
	map(0x0000, 0x1fff).rom();
	map(0x8000, 0x83ff).ram();
}

void othello_state::audio_portmap(address_map &map)
{
	map.global_mask(0xff);
	map(0x00, 0x00).r(FUNC(othello_state::latch_r));
	map(0x01, 0x01).w(FUNC(othello_state::ay_data_w));
	map(0x03, 0x03).w(FUNC(othello_state::ay_address_w));
	map(0x04, 0x04).w(FUNC(othello_state::ack_w));
	map(0x08, 0x08).w(FUNC(othello_state::ay_select_w));
}

WRITE8_MEMBER(othello_state::n7751_rom_control_w)
{
	/* P4 - address lines 0-3 */
	/* P5 - address lines 4-7 */
	/* P6 - address lines 8-11 */
	/* P7 - ROM selects */
	switch (offset)
	{
		case 0:
			m_sound_addr = (m_sound_addr & ~0x00f) | ((data & 0x0f) << 0);
			break;

		case 1:
			m_sound_addr = (m_sound_addr & ~0x0f0) | ((data & 0x0f) << 4);
			break;

		case 2:
			m_sound_addr = (m_sound_addr & ~0xf00) | ((data & 0x0f) << 8);
			break;

		case 3:
			m_sound_addr &= 0xfff;
			{
				if (!BIT(data, 0)) m_sound_addr |= 0x0000;
				if (!BIT(data, 1)) m_sound_addr |= 0x1000;
				if (!BIT(data, 2)) m_sound_addr |= 0x2000;
				if (!BIT(data, 3)) m_sound_addr |= 0x3000;
			}
			break;
	}
}

READ8_MEMBER(othello_state::n7751_rom_r)
{
	return memregion("n7751data")->base()[m_sound_addr];
}

READ8_MEMBER(othello_state::n7751_command_r)
{
	return 0x80 | ((m_n7751_command & 0x07) << 4);
}

WRITE8_MEMBER(othello_state::n7751_p2_w)
{
	i8243_device *device = machine().device<i8243_device>("n7751_8243");

	/* write to P2; low 4 bits go to 8243 */
	device->p2_w(space, offset, data & 0x0f);

	/* output of bit $80 indicates we are ready (1) or busy (0) */
	/* no other outputs are used */
	m_n7751_busy = data;
}

static INPUT_PORTS_START( othello )
	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )      PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Coinage ) )      PORT_DIPLOCATION("SW1:2,3")
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x08, 0x00, "Limit for Matta" )   PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )     PORT_DIPLOCATION("SW1:5") /* stored at $fd1e */
	PORT_DIPNAME( 0x60, 0x60, "Timer (seconds)" )   PORT_DIPLOCATION("SW1:6,7")
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPSETTING(    0x40, "8" )
	PORT_DIPSETTING(    0x60, "10" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Difficulty ) )   PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Hard ) )

	PORT_START("INP")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )  PORT_PLAYER(2)

	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )

	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

INPUT_PORTS_END

void othello_state::machine_start()
{
	m_mc6845 = machine().device<mc6845_device>("crtc");
	m_n7751 = machine().device("n7751");

	save_item(NAME(m_tile_bank));
	save_item(NAME(m_ay_select));
	save_item(NAME(m_ack_data));
	save_item(NAME(m_n7751_command));
	save_item(NAME(m_sound_addr));
	save_item(NAME(m_n7751_busy));
}

void othello_state::machine_reset()
{
	m_tile_bank = 0;
	m_ay_select = 0;
	m_ack_data = 0;
	m_n7751_command = 0;
	m_sound_addr = 0;
	m_n7751_busy = 0;
}

MACHINE_CONFIG_START(othello_state::othello)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu",Z80,XTAL(8'000'000)/2)
	MCFG_DEVICE_PROGRAM_MAP(main_map)
	MCFG_DEVICE_IO_MAP(main_portmap)
	MCFG_DEVICE_VBLANK_INT_DRIVER("screen", othello_state,  irq0_line_hold)

	MCFG_DEVICE_ADD("audiocpu",Z80,XTAL(3'579'545))
	MCFG_DEVICE_PROGRAM_MAP(audio_map)
	MCFG_DEVICE_IO_MAP(audio_portmap)

	MCFG_DEVICE_ADD("n7751", N7751, XTAL(6'000'000))
	MCFG_MCS48_PORT_T1_IN_CB(GND) // labelled as "TEST", connected to ground
	MCFG_MCS48_PORT_P2_IN_CB(READ8(*this, othello_state, n7751_command_r))
	MCFG_MCS48_PORT_BUS_IN_CB(READ8(*this, othello_state, n7751_rom_r))
	MCFG_MCS48_PORT_P1_OUT_CB(WRITE8("dac", dac_byte_interface, data_w))
	MCFG_MCS48_PORT_P2_OUT_CB(WRITE8(*this, othello_state, n7751_p2_w))
	MCFG_MCS48_PORT_PROG_OUT_CB(WRITELINE("n7751_8243", i8243_device, prog_w))

	MCFG_I8243_ADD("n7751_8243", NOOP, WRITE8(*this, othello_state,n7751_rom_control_w))


	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(64*6, 64*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 64*6-1, 0*8, 64*8-1)
	MCFG_SCREEN_UPDATE_DEVICE("crtc", h46505_device, screen_update)

	MCFG_PALETTE_ADD("palette", 0x10)
	MCFG_PALETTE_INIT_OWNER(othello_state, othello)

	MCFG_MC6845_ADD("crtc", H46505, "screen", 1000000 /* ? MHz */)   /* H46505 @ CPU clock */
	MCFG_MC6845_SHOW_BORDER_AREA(false)
	MCFG_MC6845_CHAR_WIDTH(TILE_WIDTH)
	MCFG_MC6845_UPDATE_ROW_CB(othello_state, crtc_update_row)

	/* sound hardware */
	SPEAKER(config, "speaker").front_center();

	MCFG_GENERIC_LATCH_8_ADD("soundlatch")

	MCFG_DEVICE_ADD("ay1", AY8910, 2000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "speaker", 0.15)

	MCFG_DEVICE_ADD("ay2", AY8910, 2000000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "speaker", 0.15)

	MCFG_DEVICE_ADD("dac", DAC_8BIT_R2R, 0) MCFG_SOUND_ROUTE(ALL_OUTPUTS, "speaker", 0.3) // unknown DAC
	MCFG_DEVICE_ADD("vref", VOLTAGE_REGULATOR, 0) MCFG_VOLTAGE_REGULATOR_OUTPUT(5.0)
	MCFG_SOUND_ROUTE(0, "dac", 1.0, DAC_VREF_POS_INPUT) MCFG_SOUND_ROUTE(0, "dac", -1.0, DAC_VREF_NEG_INPUT)
MACHINE_CONFIG_END

ROM_START( othello )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "4.ic59",   0x0000, 0x2000, CRC(9f82fe14) SHA1(59600264ccce787383827fc5aa0f2c23728f6946))

	ROM_REGION( 0x10000, "audiocpu", 0 )
	ROM_LOAD( "3.ic32",   0x0000, 0x2000, CRC(2bb4f75d) SHA1(29a659031acf0d50f374f440b8d353bcf98145a0))

	ROM_REGION( 0x1000, "n7751", 0 )      /* 4k for 7751 onboard ROM */
	ROM_LOAD( "7751.bin",     0x0000, 0x0400, CRC(6a9534fc) SHA1(67ad94674db5c2aab75785668f610f6f4eccd158) )

	ROM_REGION( 0x4000, "n7751data", 0 ) /* 7751 sound data */
	ROM_LOAD( "1.ic48", 0x0000, 0x2000, CRC(c3807dea) SHA1(d6339380e1239f3e20bcca2fbc673ad72e9ca608))
	ROM_LOAD( "2.ic49", 0x2000, 0x2000, CRC(a945f3e7) SHA1(ea18efc18fda63ce1747287bbe2a9704b08daff8))

	ROM_REGION( 0x6000, "gfx", 0 )
	ROM_LOAD( "5.ic40",   0x0000, 0x2000, CRC(45fdc1ab) SHA1(f30f6002e3f34a647effac8b0116c8ed064e226a))
	ROM_LOAD( "6.ic41",   0x2000, 0x2000, CRC(467a731f) SHA1(af80e854522e53fb1b9af7945b2c803a654c6f65))
	ROM_LOAD( "7.ic42",   0x4000, 0x2000, CRC(a76705f7) SHA1(b7d2a65d65d065732ddd0b3b738749369b382b48))
ROM_END

GAME( 1984, othello,  0,       othello,  othello, othello_state, empty_init, ROT0, "Success", "Othello (version 3.0)", MACHINE_WRONG_COLORS | MACHINE_IMPERFECT_SOUND | MACHINE_SUPPORTS_SAVE )
