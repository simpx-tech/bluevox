#pragma once
inline int32 PositiveMod(const int32 A, const int32 Modulus)
{
	int32 R = A % Modulus;
	if (R < 0) R += Modulus;
	return R;
}
