// license:BSD-3-Clause
// copyright-holders: Roberto Fresca

#ifndef MAME_INCLUDES_CALOMEGA_H
#define MAME_INCLUDES_CALOMEGA_H

#pragma once

#include "cpu/m6502/m6502.h"
#include "cpu/m6502/r65c02.h"
#include "machine/nvram.h"
#include "machine/timer.h"
#include "machine/6821pia.h"
#include "machine/6850acia.h"
#include "machine/clock.h"
#include "machine/ticket.h"
#include "sound/ay8910.h"
#include "video/mc6845.h"
#include "screen.h"
#include "speaker.h"
#include "emupal.h"
#include "tilemap.h"

class calomega_state : public driver_device
{
public:
	calomega_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_pia(*this, "pia%u", 0U),
		m_maincpu(*this, "maincpu"),
		m_acia6850_0(*this, "acia6850_0"),
		m_aciabaud(*this, "aciabaud"),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette"),
		m_videoram(*this, "videoram"),
		m_colorram(*this, "colorram"),
		m_hopper(*this, "hopper"),
		m_in0(*this, "IN0"),
		m_in0_0(*this, "IN0-0"),
		m_in0_1(*this, "IN0-1"),
		m_in0_2(*this, "IN0-2"),
		m_in0_3(*this, "IN0-3"),
		m_frq(*this, "FRQ"),
		m_sw2(*this, "SW2"),
		m_lamps(*this, "lamp%u", 1U)
	{
	}

	void init_sys903();
	void init_comg080();
	void init_s903mod();
	void init_sys905();
	void init_comg5108();
	void init_cas21iwc();
	void init_pokeriwc();
	void init_any();

	void sys905(machine_config &config);
	void s903mod(machine_config &config);
	void sys906(machine_config &config);
	void sys903(machine_config &config);

protected:
	virtual void machine_start() override { m_lamps.resolve(); }
	virtual void video_start() override;

private:
	void calomega_videoram_w(offs_t offset, uint8_t data);
	void calomega_colorram_w(offs_t offset, uint8_t data);
	uint8_t s903_mux_port_r();
	void s903_mux_w(uint8_t data);
	uint8_t s905_mux_port_r();
	void s905_mux_w(uint8_t data);
	uint8_t pia0_bin_r();
	void pia0_aout_w(uint8_t data);
	void pia0_bout_w(uint8_t data);
	uint8_t pia1_ain_r();
	uint8_t pia1_bin_r();
	uint8_t dummy_pia_r();
	void pia1_aout_w(uint8_t data);
	void pia1_bout_w(uint8_t data);
	void lamps_903a_w(uint8_t data);
	void lamps_903b_w(uint8_t data);
	void lamps_905_w(uint8_t data);
	void dummy_pia_w(uint8_t data);

	DECLARE_READ_LINE_MEMBER(timer_r);
	DECLARE_READ_LINE_MEMBER(vblank_r);
	DECLARE_READ_LINE_MEMBER(dummy_pia_line_r);

	DECLARE_WRITE_LINE_MEMBER(pia1_cb2_w);
	DECLARE_WRITE_LINE_MEMBER(vblank0_w);
	DECLARE_WRITE_LINE_MEMBER(vblank1_w);
	DECLARE_WRITE_LINE_MEMBER(vblank2_w);
	DECLARE_WRITE_LINE_MEMBER(dummy_pia_line_w);
	DECLARE_WRITE_LINE_MEMBER(write_acia_tx);
	DECLARE_WRITE_LINE_MEMBER(write_acia_clock);
	DECLARE_WRITE_LINE_MEMBER(update_aciabaud_scale);

	TIMER_DEVICE_CALLBACK_MEMBER(timer_0);
	TIMER_DEVICE_CALLBACK_MEMBER(timer_1);
	TIMER_DEVICE_CALLBACK_MEMBER(timer_2);

	TILE_GET_INFO_MEMBER(get_bg_tile_info);

	uint32_t screen_update_calomega(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void calomega_palette(palette_device &palette) const;

	void s903mod_map(address_map &map);
	void sys903_map(address_map &map);
	void sys905_map(address_map &map);
	void sys906_map(address_map &map);

	optional_device_array<pia6821_device, 2> m_pia;

	required_device<m6502_device> m_maincpu;
	optional_device<acia6850_device> m_acia6850_0;
	optional_device<clock_device> m_aciabaud;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;


	required_shared_ptr<uint8_t> m_videoram;
	required_shared_ptr<uint8_t> m_colorram;

	required_device<ticket_dispenser_device> m_hopper;

	optional_ioport m_in0;
	optional_ioport m_in0_0;
	optional_ioport m_in0_1;
	optional_ioport m_in0_2;
	optional_ioport m_in0_3;
	optional_ioport m_frq;
	optional_ioport m_sw2;
	output_finder<9> m_lamps;

	uint8_t m_vblank, m_timer, m_tx_line;
	int m_s903_mux_data;
	int m_s905_mux_data;
	int m_pia_data;
	bool m_lockout = false;
	bool m_diverter;
	tilemap_t *m_bg_tilemap;
};

#endif // MAME_INCLUDES_CALOMEGA_H
