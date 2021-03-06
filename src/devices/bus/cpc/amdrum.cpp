// license:BSD-3-Clause
// copyright-holders:Barry Rodewald
/*
 * amdrum.cpp
 *
 *  Created on: 23/08/2014
 */

#include "emu.h"
#include "amdrum.h"

#include "speaker.h"


//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(CPC_AMDRUM, cpc_amdrum_device, "cpc_amdrum", "Amdrum")


void cpc_amdrum_device::device_add_mconfig(machine_config &config)
{
	SPEAKER(config, "speaker").front_center();
	ZN428E(config, m_dac, 0).add_route(ALL_OUTPUTS, "speaker", 0.5);
	// no pass-through
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

cpc_amdrum_device::cpc_amdrum_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, CPC_AMDRUM, tag, owner, clock),
	device_cpc_expansion_card_interface(mconfig, *this),
	m_slot(nullptr),
	m_dac(*this,"dac")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cpc_amdrum_device::device_start()
{
	m_slot = dynamic_cast<cpc_expansion_slot_device *>(owner());
	address_space &space = m_slot->cpu().space(AS_IO);
	space.install_write_handler(0xff00,0xffff, write8smo_delegate(*this, FUNC(cpc_amdrum_device::dac_w)));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cpc_amdrum_device::device_reset()
{
	// TODO
}

void cpc_amdrum_device::dac_w(uint8_t data)
{
	m_dac->write(data);
}
