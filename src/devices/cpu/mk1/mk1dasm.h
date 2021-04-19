// license:BSD-3-Clause
// copyright-holders:AJR

#ifndef MAME_CPU_MK1_MK1DASM_H
#define MAME_CPU_MK1_MK1DASM_H

#pragma once

class mk1_disassembler : public util::disasm_interface
{
public:
	mk1_disassembler();

	virtual u32 opcode_alignment() const override;
	virtual offs_t disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params) override;
};

#endif // MAME_CPU_MK1_MK1DASM_H
