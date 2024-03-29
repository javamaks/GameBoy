#include "cpu.h"

/*
	����� ���������� Gameboy
	���������������� ��������� Zilog Z80
*/

// ������������� ���������� �������� ������ ����������
void CPU::init(Memory* _memory)
{
	memory = _memory;
	reset();
}

// ��������������� ������� ������� ������, ������������� ����������
void CPU::reset()
{
	/*
		������������������ ��������� Gameboy:

		* ��� ������� Gameboy ��������� ��������� ROM �������� 256 ����,
		������� ��������� ��������� ������ Gameboy, �������� �� ����������.

		* �� ���������� ������ �� ��������� 0x104 - 0x133 � ���������� �������,
		����� ��������� ������� Nintendo.
		* ������� Nintendo �������������� �� �������� ������, � ������ ��� ����������� ����.
		* ���� �����-���� ���� �� ���������, �� ���������� ��������� � ���������������� ��� ��������.

		* ���� ��� ����������������� �������� �������� �������, ���������� ROM �����������, � ��������
		����������� � ������ 0x100 �� ���������� ���������� ���������.
	*/
	reg_A = 0x01;
	reg_B = 0x00;
	reg_C = 0x13;
	reg_D = 0x00;
	reg_E = 0xD8;
	reg_F = 0xB0;
	reg_H = 0x01;
	reg_L = 0x4D;
	reg_SP = 0xFFFE;
	reg_PC = 0x100;
}

void CPU::save_state(ofstream &file)
{
	file.write((char*)&reg_A, sizeof(reg_A));
	file.write((char*)&reg_B, sizeof(reg_B));
	file.write((char*)&reg_C, sizeof(reg_C));
	file.write((char*)&reg_D, sizeof(reg_D));
	file.write((char*)&reg_E, sizeof(reg_E));
	file.write((char*)&reg_F, sizeof(reg_F));
	file.write((char*)&reg_H, sizeof(reg_H));
	file.write((char*)&reg_L, sizeof(reg_L));
	file.write((char*)&reg_SP, sizeof(reg_SP));
	file.write((char*)&reg_PC, sizeof(reg_PC));
}

void CPU::load_state(ifstream &file)
{
	file.read((char*)&reg_A, sizeof(reg_A));
	file.read((char*)&reg_B, sizeof(reg_B));
	file.read((char*)&reg_C, sizeof(reg_C));
	file.read((char*)&reg_D, sizeof(reg_D));
	file.read((char*)&reg_E, sizeof(reg_E));
	file.read((char*)&reg_F, sizeof(reg_F));
	file.read((char*)&reg_H, sizeof(reg_H));
	file.read((char*)&reg_L, sizeof(reg_L));
	file.read((char*)&reg_SP, sizeof(reg_SP));
	file.read((char*)&reg_PC, sizeof(reg_PC));
}

void CPU::op(int pc, int cycle)
{
	reg_PC += pc;

	/*
		����� ���������� �� 4, ������ ��� � �����������
		����� ��������, ������������ � ����������� �� ����������������,
		������� ���� ���������� ��� �������� �����, � �� �������� �����.
		1 �������� ���� ����� 1/4 ��������� �����
	*/

	num_cycles += (cycle * 4);
}

void CPU::set_flag(int flag, bool value)
{
	if (value == true)
		reg_F |= flag;
	else
		reg_F &= ~(flag);
}

// �������� 8 ���

void CPU::LD(Byte& destination, Byte value)
{
	destination = value;
}

void CPU::LD(Byte& destination, Address addr)
{
	destination = memory->read(addr);
}

void CPU::LD(Address addr, Byte value)
{
	memory->write(addr, value);
}

// �������� 16 ���

void CPU::LD(Pair reg_pair, Byte upper, Byte lower)
{
	reg_pair.set(upper, lower);
}

void CPU::LD(Byte_2& reg_pair, Byte upper, Byte lower)
{
	reg_pair = combine(upper, lower);
}

void CPU::LDHL(Byte value)
{
	// �������� �� -128 �� +127, ����� �� ������� ��� ��������, � ����� ��������?

	Byte_2_Signed signed_val = (Byte_2_Signed) (Byte_Signed) value;
	Byte_2 result = (Byte_2) ((Byte_2_Signed) reg_SP + signed_val);

	set_flag(FLAG_CARRY, (result & 0xFF) < (reg_SP & 0xFF)); // �����������, ���� ������� �� ���� 15
	set_flag(FLAG_HALF_CARRY, (result & 0xF) < (reg_SP & 0xF)); // �����������, ���� ������� �� ���� 11
	set_flag(FLAG_ZERO, false); // �����
	set_flag(FLAG_SUB, false); // �����

	Pair(reg_H, reg_L).set(result);
}

void CPU::LDNN(Byte low, Byte high)
{
	Byte lsb = low_byte(reg_SP);
	Byte msb = high_byte(reg_SP);

	Address addr = Pair(high, low).address();

	LD(addr++, lsb);
	LD(addr, msb);
}

// �������� �� ������

void CPU::PUSH(Byte high, Byte low)
{
	memory->write(--reg_SP, high);
	memory->write(--reg_SP, low);
}

void CPU::POP(Byte& high, Byte& low)
{
	low = memory->read(reg_SP++);
	high = memory->read(reg_SP++);
}

// �������������� �������� ALU

void CPU::ADD(Byte& target, Byte value)
{
	Byte_2 result = (Byte_2) target + (Byte_2) value;

	set_flag(FLAG_HALF_CARRY, ((target & 0xF) + (value & 0xF)) > 0xF);
	set_flag(FLAG_CARRY, (result > 0xFF));
	set_flag(FLAG_ZERO, (result & 0xFF) == 0);
	set_flag(FLAG_SUB, false);

	target = (result & 0xFF);
}

void CPU::ADD(Byte& target, Address addr)
{
	Byte val = memory->read(addr);
	ADD(target, val);
}

void CPU::ADC(Byte& target, Byte value)
{
	Byte_2 carry = (reg_F & FLAG_CARRY) ? 1 : 0;
	Byte_2 result = (Byte_2) target + (Byte_2) value + carry;

	set_flag(FLAG_HALF_CARRY, ((target & 0x0F) + (value & 0xF) + (Byte) carry) > 0x0F);
	set_flag(FLAG_CARRY, (result > 0xFF));
	set_flag(FLAG_ZERO, (result & 0xFF) == 0);
	set_flag(FLAG_SUB, false);

	target = (result & 0xFF);
}

void CPU::ADC(Byte& target, Address addr)
{
	Byte val = memory->read(addr);
	ADC(target, val);
}

void CPU::SUB(Byte& target, Byte value)
{
	Byte_2_Signed result = (Byte_2_Signed)target - (Byte_2_Signed)value;

	Byte_2_Signed s_target = (Byte_2_Signed)target;
	Byte_2_Signed s_value = (Byte_2_Signed)value;

	set_flag(FLAG_HALF_CARRY, (((s_target & 0xF) - (s_value & 0xF)) < 0));
	set_flag(FLAG_CARRY, result < 0);
	set_flag(FLAG_SUB, true);
	set_flag(FLAG_ZERO, (result & 0xFF) == 0);

	target = (Byte)(result & 0xFF);
}

void CPU::SUB(Byte& target, Address addr)
{
	Byte val = memory->read(addr);
	SUB(target, val);
}

void CPU::SBC(Byte& target, Byte value)
{
	Byte_2 carry = (reg_F & FLAG_CARRY) ? 1 : 0;
	Byte_2_Signed result = (Byte_2_Signed)target - (Byte_2_Signed)value - carry;

	Byte_2_Signed s_target = (Byte_2_Signed)target;
	Byte_2_Signed s_value = (Byte_2_Signed)value;

	set_flag(FLAG_HALF_CARRY, (((s_target & 0xF) - (s_value & 0xF) - carry) < 0));
	set_flag(FLAG_CARRY, result < 0);
	set_flag(FLAG_SUB, true);
	set_flag(FLAG_ZERO, (result & 0xFF) == 0);

	target = (Byte) (result & 0xFF);
}

void CPU::SBC(Byte& target, Address addr)
{
	Byte val = memory->read(addr);
	SBC(target, val);
}

void CPU::AND(Byte& target, Byte value)
{
	target &= value;

	set_flag(FLAG_ZERO, (target == 0));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, true);
	set_flag(FLAG_CARRY, false);
}

void CPU::AND(Byte& target, Address addr)
{
	Byte val = memory->read(addr);
	AND(target, val);
}

void CPU::OR(Byte& target, Byte value)
{
	target |= value;

	set_flag(FLAG_ZERO, (target == 0));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_CARRY, false);
}

void CPU::OR(Byte& target, Address addr)
{
	Byte val = memory->read(addr);
	OR(target, val);
}

void CPU::XOR(Byte& target, Byte value)
{
	target ^= value;

	set_flag(FLAG_ZERO, (target == 0));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_CARRY, false);
}

void CPU::XOR(Byte& target, Address addr)
{
	Byte val = memory->read(addr);
	XOR(target, val);
}
// ��������� A � n. ��� �� ���� ��������� A - n, �� ���������� �������������

void CPU::CP(Byte& target, Byte value)
{
	int result = target - value;

	set_flag(FLAG_ZERO, (result == 0)); // �����������, ���� ��������� ����� ����
	set_flag(FLAG_SUB, true); // �����������
	set_flag(FLAG_HALF_CARRY, (((target & 0xF) - (value & 0xF)) < 0));
	set_flag(FLAG_CARRY, (result < 0)); // �����������, ���� ����
}

void CPU::CP(Byte& target, Address addr)
{
	Byte val = memory->read(addr);
	CP(target, val);
}

void CPU::INC(Byte& target)
{
	Byte result = target + 1;
	set_flag(FLAG_ZERO, (result == 0));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, ((((target & 0xF) + 1) & 0x10) != 0));

	target = result;
}

void CPU::INC(Address addr)
{
	Byte value = memory->read(addr);
	INC(value);
	memory->write(addr, value);
}

void CPU::DEC(Byte& target)
{
	Byte result = target - 1;
	set_flag(FLAG_ZERO, (result == 0));
	set_flag(FLAG_SUB, true);
	set_flag(FLAG_HALF_CARRY, (((target & 0xF) - 1) < 0));

	target = result;
}

void CPU::DEC(Address addr)
{
	Byte value = memory->read(addr);
	DEC(value);
	memory->write(addr, value);
}

// ���������� 16 ���

void CPU::ADD16(Byte_2 target, Byte_2 value)
{
	int result = target + value;
	set_flag(FLAG_SUB, false); // �����
	set_flag(FLAG_HALF_CARRY, ((((target & 0xFFF) + (value & 0xFFF)) & 0x1000) != 0)); // ����������, ���� ������� � ���� 11
	set_flag(FLAG_CARRY, (result > 0xFFFF)); // ����������, ���� ������� � ���� 15
}

void CPU::ADDHL(Pair reg_pair)
{
	Byte_2 target = Pair(reg_H, reg_L).get();
	Byte_2 value = reg_pair.get();
	Byte_2 result = target + value;

	ADD16(target, value); // ���������� ��������������� �����
	
	Pair(reg_H, reg_L).set(result);
}

void CPU::ADDHLSP()
{
	Byte high = high_byte(reg_SP);
	Byte low = low_byte(reg_SP);

	ADDHL(Pair(high, low));
}

void CPU::ADDSP(Byte value)
{
	Byte_2_Signed val_signed = (Byte_2_Signed) (Byte_Signed) value;
	Byte_2 result = (Byte_2) ((Byte_2_Signed) reg_SP + val_signed);

	set_flag(FLAG_CARRY, (result & 0xFF) < (reg_SP & 0xFF));
	set_flag(FLAG_HALF_CARRY, (result & 0xF) < (reg_SP & 0xF));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_ZERO, false);

	reg_SP = result;
}

void CPU::INC(Pair reg_pair)
{
	reg_pair.set(reg_pair.get() + 1);// ������� ���������, ��� ����� �� ����������
}

void CPU::INCSP()
{
	reg_SP++;
}

void CPU::DEC(Pair reg_pair)
{
	reg_pair.set(reg_pair.get() - 1); // ������� ���������, ��� ����� �� ����������
}

void CPU::DECSP()
{
	reg_SP--;
}

// �������� ������ � ��������
// ��������� �� 1 ��� �����
void CPU::RL(Byte& target, bool carry, bool zero_flag)
{
	int bit7 = ((target & 0x80) != 0);
	target = target << 1;

	target |= (carry) ? ((reg_F & FLAG_CARRY) != 0) : bit7;

	set_flag(FLAG_ZERO, ((zero_flag) ? (target == 0) : false));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_CARRY, (bit7 != 0));
}

void CPU::RL(Address addr, bool carry)
{
	Byte value = memory->read(addr);
	RL(value, carry, true);
	memory->write(addr, value);
}

void CPU::RR(Byte& target, bool carry, bool zero_flag)
{
	int bit1 = ((target & 0x1) != 0);
	target = target >> 1;

	target |= (carry) ? (((reg_F & FLAG_CARRY) != 0) << 7) : (bit1 << 7);

	set_flag(FLAG_ZERO, ((zero_flag) ? (target == 0) : false));
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_CARRY, (bit1 != 0));
}

void CPU::RR(Address addr, bool carry)
{
	Byte value = memory->read(addr);
	RR(value, carry, true);
	memory->write(addr, value);
}

// ����� �����
void CPU::SL(Byte& target)
{
	Byte result = target << 1;

	set_flag(FLAG_CARRY, (target & 0x80) != 0);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_ZERO, (result == 0));

	target = result;
}

void CPU::SL(Address addr)
{
	Byte data = memory->read(addr);
	SL(data);
	memory->write(addr, data);
}

// ����� ������
void CPU::SR(Byte& target, bool include_top_bit)
{
	bool top_bit_set = is_bit_set(target, BIT_7);

	Byte result;

	if (include_top_bit)
		result = (top_bit_set) ? ((target >> 1) | 0x80) : (target >> 1);
	else
		result = target >> 1;

	set_flag(FLAG_CARRY, (target & 0x01) != 0);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_ZERO, (result == 0));

	target = result;
}

void CPU::SR(Address addr, bool include_top_bit)
{
	Byte data = memory->read(addr);
	SR(data, include_top_bit);
	memory->write(addr, data);
}

// �� �� �����, ��� ����� ������, �� ��� 7 �� ����������
void CPU::SRA(Byte& target)
{
	// ���������� ���� 7 �� ����������
	int bit7 = ((target & 0x80) != 0);
	RR(target, true);
	target |= (bit7 << 7);
	set_flag(FLAG_ZERO, (target == 0));
}

void CPU::SRA(Address addr)
{
	Byte value = memory->read(addr);
	SRA(value);
	memory->write(addr, value);
}

// �� �� �����, ��� ����� ������, �� ��� 7 ������������
void CPU::SRL(Byte& target)
{
	RR(target, true, true);
}

void CPU::SRL(Address addr)
{
	Byte value = memory->read(addr);
	SRL(value);
	memory->write(addr, value);
}

// �������� ������� ������� � ������� ��������
void CPU::SWAP(Byte& target)
{
	Byte first = target >> 4;
	Byte second = target << 4;

	Byte swapped = first | second;

	target = swapped;

	set_flag(FLAG_CARRY, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_ZERO, (target == 0));
}

void CPU::SWAP(Address addr)
{
	Byte value = memory->read(addr);
	SWAP(value);
	memory->write(addr, value);
}

// �������� � ������

void CPU::BIT(Byte target, int bit)
{
	set_flag(FLAG_ZERO, (((1 << bit) & ~target) != 0));
	set_flag(FLAG_HALF_CARRY, true);
	set_flag(FLAG_SUB, false);
}

void CPU::BIT(Address addr, int bit)
{
	Byte value = memory->read(addr);
	BIT(value, bit);
}

void CPU::SET(Byte& target, int bit)
{
	target = (target | (1 << bit));
}

void CPU::SET(Address addr, int bit)
{
	Byte value = memory->read(addr);
	SET(value, bit);
	memory->write(addr, value);
}

void CPU::RES(Byte& target, int bit)
{
	target = (~(1 << bit) & target);
}

void CPU::RES(Address addr, int bit)
{
	Byte value = memory->read(addr);
	RES(value, bit);
	memory->write(addr, value);
}

// �������� ��������
// ����������� ������� �� �����

void CPU::JP(Pair target)
{
	reg_PC = target.address();
	op(0, 1); // �������� 1 ����, ���� ������� �������
}
// �������� ������� �� �����, ���� ���� ZERO �������

void CPU::JPNZ(Pair target)
{
	if ((reg_F & FLAG_ZERO) == 0)
		JP(target);
}
// �������� ������� �� �����, ���� ���� ZERO ����������

void CPU::JPZ(Pair target)
{
	if ((reg_F & FLAG_ZERO) != 0)
		JP(target);
}
// �������� ������� �� �����, ���� ���� CARRY �������

void CPU::JPNC(Pair target)
{
	if ((reg_F & FLAG_CARRY) == 0)
		JP(target);
}
// �������� ������� �� �����, ���� ���� CARRY ����������

void CPU::JPC(Pair target)
{
	if ((reg_F & FLAG_CARRY) != 0)
		JP(target);
}

// ������� �� ����� ������������ �������� ��������� �� -127 �� +129 �����
void CPU::JR(Byte value)
{
	Byte_Signed signed_val = ((Byte_Signed)(value));
	reg_PC += signed_val; // ��� ������� ��� ��������� 2, �� ������� � ������������
	op(0, 1); // �������� 1 ����, ���� ������� �������

}
// �������� ������� �� ����� ������������ �������� ���������, ���� ���� ZERO �������

void CPU::JRNZ(Byte value)
{
	if ((reg_F & FLAG_ZERO) == 0)
		JR(value);
}
// �������� ������� �� ����� ������������ �������� ���������, ���� ���� ZERO ����������

void CPU::JRZ(Byte value)
{
	if ((reg_F & FLAG_ZERO) != 0)
		JR(value);
}
// �������� ������� �� ����� ������������ �������� ���������, ���� ���� CARRY �������

void CPU::JRNC(Byte value)
{
	if ((reg_F & FLAG_CARRY) == 0)
		JR(value);
}
// �������� ������� �� ����� ������������ �������� ���������, ���� ���� CARRY ����������

void CPU::JRC(Byte value)
{
	if ((reg_F & FLAG_CARRY) != 0)
		JR(value);
}
// ������� �� �����, ������������ � �������� HL

void CPU::JPHL()
{
	reg_PC = Pair(reg_H, reg_L).address();
}
// �������������� ����������
// ����� ������������ �� ������

void CPU::CALL(Byte low, Byte high)
{
	memory->write(--reg_SP, high_byte(reg_PC));
	memory->write(--reg_SP, low_byte(reg_PC));

	JP(Pair(high, low));
	op(0, 3);
}
// �������� ����� ������������ �� ������, ���� ���� ZERO �������

void CPU::CALLNZ(Byte low, Byte high)
{
	if ((reg_F & FLAG_ZERO) == 0)
		CALL(low, high);
}
// �������� ����� ������������ �� ������, ���� ���� ZERO ����������

void CPU::CALLZ(Byte low, Byte high)
{
	if ((reg_F & FLAG_ZERO) != 0)
		CALL(low, high);
}
// �������� ����� ������������ �� ������, ���� ���� CARRY �������

void CPU::CALLNC(Byte low, Byte high)
{
	if ((reg_F & FLAG_CARRY) == 0)
		CALL(low, high);
}
// �������� ����� ������������ �� ������, ���� ���� CARRY ����������

void CPU::CALLC(Byte low, Byte high)
{
	if ((reg_F & FLAG_CARRY) != 0)
		CALL(low, high);
}
// ������� �� ������������

void CPU::RET()
{
	Byte low = memory->read(reg_SP++);
	Byte high = memory->read(reg_SP++);

	reg_PC = Pair(high, low).get();
	op(0, 3);
}
// ������� �� ������������ � ����������� ������������ ����������

void CPU::RETI()
{
	interrupt_master_enable = true;
	RET();
}
// �������� ������� �� ������������, ���� ���� ZERO �������

void CPU::RETNZ()
{
	if ((reg_F & FLAG_ZERO) == 0)
	{
		RET();
		op(0, 2);
	}
}
// �������� ������� �� ������������, ���� ���� ZERO ����������

void CPU::RETZ()
{
	if ((reg_F & FLAG_ZERO) != 0)
	{
		RET();
		op(0, 2);
	}
}
// �������� ������� �� ������������, ���� ���� CARRY �������

void CPU::RETNC()
{
	if ((reg_F & FLAG_CARRY) == 0)
	{
		RET();
		op(0, 2);
	}
}
// �������� ������� �� ������������, ���� ���� CARRY ����������

void CPU::RETC()
{
	if ((reg_F & FLAG_CARRY) != 0)
	{
		RET();
		op(0, 2);
	}
}
// ����� ���������� � ����������� ������ � �����

void CPU::RST(Address addr)
{
	memory->write(--reg_SP, high_byte(reg_PC));
	memory->write(--reg_SP, low_byte(reg_PC));

	reg_PC = addr;
}
// ���������� ��������� ������������
// Decimal Adjust Accumulator
// Binary coded decimal representation is used to set the contents of
// Register A to a binary coded decimal number
// confusing af, referenced wadatsumi emulator after my implementation failed tests
// will come back and try to understand this fully
void CPU::DAA()
{
	Byte high = high_nibble(reg_A);
	Byte low = low_nibble(reg_A);

	bool add = ((reg_F & FLAG_SUB) == 0);
	bool carry = ((reg_F & FLAG_CARRY) != 0);
	bool half_carry = ((reg_F & FLAG_HALF_CARRY) != 0);

	Byte_2 result = (Byte_2) reg_A;
	Byte_2 correction = (carry) ? 0x60 : 0x00;

	if (half_carry || (add) && ((result & 0x0F) > 9))
		correction |= 0x06;

	if (carry || (add) && (result > 0x99))
		correction |= 0x60;

	if (add)
		result += correction;
	else
		result -= correction;

	if (((correction << 2) & 0x100) != 0)
		set_flag(FLAG_CARRY, true);

	set_flag(FLAG_HALF_CARRY, false);
	reg_A = (Byte)(result & 0xFF);
	set_flag(FLAG_ZERO, reg_A == 0);
}
// �������� ����� � ������������

void CPU::CPL()
{
	reg_A = ~reg_A;
	set_flag(FLAG_HALF_CARRY, true);
	set_flag(FLAG_SUB, true);
}
// ��������� ����� CARRY � �������

void CPU::SCF()
{
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_CARRY, true);
}
// �������� ����� CARRY

void CPU::CCF()
{
	set_flag(FLAG_SUB, false);
	set_flag(FLAG_HALF_CARRY, false);
	set_flag(FLAG_CARRY, ((reg_F & FLAG_CARRY) ? 1 : 0) ^ 1);
}
// ��� ��������

void CPU::NOP()
{
	// ����� ������� ��������

}
// ��������� ����������

void CPU::HALT()
{
	// ��������� ���������������, ����� � ����� HALT
	// ������������� ���� � ���������� LCD ���������� ������
	// ������ �������� ����������
	// ����� HALT ���������� ����������� ��� �������� ������

	// �������� HALT ������� ����������� ����������
	// �� ������ ����������

	halted = true;
	op(-1, 0); // ���� ����������, ��������� ���������� HALT �� ����������

	// ��������, ���������� ��������� ���������� �����



}
// ��������� ����������

void CPU::STOP()
{
}
// ���������� ������������ ����������

void CPU::DI()
{
	interrupt_master_enable = false;
}
// ���������� ������������ ����������

void CPU::EI()
{
	interrupt_master_enable = true;
}
// �������

void CPU::debug()
{
	parse_opcode(0xE8);
}
