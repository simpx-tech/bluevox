#pragma once
inline int FloorDiv(const int A, const int B)
{
	int Div = A / B;
	if (A < 0 && A % B != 0)
	{
		--Div;
	}
	return Div;
}
