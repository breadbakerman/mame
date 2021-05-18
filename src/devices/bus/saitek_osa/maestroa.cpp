// license:BSD-3-Clause
// copyright-holders:hap
// thanks-to:Berger
/***************************************************************************

Saitek OSA Module: Kasparov Maestro A (SciSys, 1986)

The chess engine revision is in-between Kaplan's Stratos and Turbostar.

Hardware notes:
- PCB label: M6L-PE-012 REV.2
- R65C02P4 @ 4MHz / 5.67MHz / 6MHz
- 32KB ROM (D27C256)
- 8KB RAM (HM6264LP-15)
- 3 more sockets, one for KSO expansion ROM, 2 unused

The PCB is not compatible for upgrading to newer Maestro versions.
CPU is a 4MHz part. The higher speed Maestro versions overclock it.

TODO:
- does not work if cpu speed is 4MHz

***************************************************************************/

#include "emu.h"
#include "maestroa.h"

#include "bus/generic/slot.h"
#include "bus/generic/carts.h"
#include "cpu/m6502/r65c02.h"

#include "softlist.h"


DEFINE_DEVICE_TYPE(OSA_MAESTROA, saitekosa_maestroa_device, "osa_maestroa", "Maestro A")


//-------------------------------------------------
//  initialization
//-------------------------------------------------

saitekosa_maestroa_device::saitekosa_maestroa_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock) :
	device_t(mconfig, OSA_MAESTROA, tag, owner, clock),
	device_saitekosa_expansion_interface(mconfig, *this),
	m_maincpu(*this, "maincpu")
{ }

void saitekosa_maestroa_device::device_start()
{
	save_item(NAME(m_latch_enable));
	save_item(NAME(m_latch));
}

void saitekosa_maestroa_device::device_reset()
{
	p2600_w(0);
}


//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

ROM_START( maestroa )
	ROM_REGION(0x10000, "maincpu", 0)

	ROM_SYSTEM_BIOS(0, "a1", "Maestro A (set 1)")
	ROM_SYSTEM_BIOS(1, "a2", "Maestro A (set 2)")

	ROMX_LOAD("m6a_a29b.u6", 0x8000, 0x8000, CRC(6ee0197a) SHA1(61f201ca64576aca582bc9f2a427638bd79e1fee), ROM_BIOS(0))
	ROMX_LOAD("m6a_v14b.u6", 0x8000, 0x8000, CRC(d566a476) SHA1(ef81b9a0dcfbd8427025cfe9bf738d42a7a1139a), ROM_BIOS(1))
ROM_END

const tiny_rom_entry *saitekosa_maestroa_device::device_rom_region() const
{
	return ROM_NAME(maestroa);
}


//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

void saitekosa_maestroa_device::device_add_mconfig(machine_config &config)
{
	// basic machine hardware
	R65C02(config, m_maincpu, 6_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &saitekosa_maestroa_device::main_map);

	// extension rom
	GENERIC_SOCKET(config, "extrom", generic_plain_slot, "saitek_kso");
	SOFTWARE_LIST(config, "cart_list").set_original("saitek_kso");
}


//-------------------------------------------------
//  internal i/o
//-------------------------------------------------

u8 saitekosa_maestroa_device::p2200_r()
{
	if (!machine().side_effects_disabled())
	{
		// strobe RTS-P
		m_expansion->rts_w(1);
		m_expansion->rts_w(0);
	}

	return 0xff;
}

void saitekosa_maestroa_device::p2400_w(u8 data)
{
	// clock latch
	m_latch = data;
}

u8 saitekosa_maestroa_device::p2400_r()
{
	return m_expansion->data_state();
}

void saitekosa_maestroa_device::p2600_w(u8 data)
{
	// d3: enable latch output
	m_latch_enable = bool(data & 8);

	// d2: STB-P
	m_expansion->stb_w(BIT(data, 2));
}

u8 saitekosa_maestroa_device::p2600_r()
{
	// d7: ACK-P
	return m_expansion->ack_state() ? 0x80 : 0;
}

void saitekosa_maestroa_device::main_map(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x1fff).ram();
	map(0x2200, 0x2200).mirror(0x01ff).r(FUNC(saitekosa_maestroa_device::p2200_r));
	map(0x2400, 0x2400).mirror(0x01ff).rw(FUNC(saitekosa_maestroa_device::p2400_r), FUNC(saitekosa_maestroa_device::p2400_w));
	map(0x2600, 0x2600).mirror(0x01ff).rw(FUNC(saitekosa_maestroa_device::p2600_r), FUNC(saitekosa_maestroa_device::p2600_w));
	map(0x4000, 0x5fff).r("extrom", FUNC(generic_slot_device::read_rom));
	map(0x8000, 0xffff).rom();
}


//-------------------------------------------------
//  host i/o
//-------------------------------------------------

u8 saitekosa_maestroa_device::data_r()
{
	return m_latch_enable ? m_latch : 0xff;
}

void saitekosa_maestroa_device::nmi_w(int state)
{
	m_maincpu->set_input_line(INPUT_LINE_NMI, !state ? ASSERT_LINE : CLEAR_LINE);
}
