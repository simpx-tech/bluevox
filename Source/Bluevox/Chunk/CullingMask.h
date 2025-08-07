#pragma once

enum class ECullingMask : uint32
{
	None         = 0,
	CornerNW     = 1 << 0,
	CornerNE     = 1 << 1,
	CornerSW     = 1 << 2,
	CornerSE     = 1 << 3,
	BorderN      = 1 << 4,
	BorderE      = 1 << 5,
	BorderS      = 1 << 6,
	BorderW      = 1 << 7,
	DiagonalSWNE = 1 << 8,
	DiagonalNWSE = 1 << 9,
	Layer        = 1 << 10,
};

inline ECullingMask operator|(const ECullingMask A, const ECullingMask B)
{
	return static_cast<ECullingMask>(static_cast<uint32>(A) | static_cast<uint32>(B));
}

inline ECullingMask operator&(const ECullingMask A, const ECullingMask B)
{
	return static_cast<ECullingMask>(static_cast<uint32>(A) & static_cast<uint32>(B));
}

inline ECullingMask& operator|=(ECullingMask& A, const ECullingMask B)
{
	A = A | B;
	return A;
}

inline ECullingMask& operator&=(ECullingMask& A, const ECullingMask B)
{
	A = A & B;
	return A;
}

inline bool HasCulling(const ECullingMask Flags, const ECullingMask Flag)
{
	return (static_cast<uint32>(Flags) & static_cast<uint32>(Flag)) != 0;
}

static auto AllMasks = ECullingMask::CornerNW | ECullingMask::CornerNE |
	ECullingMask::CornerSW |
	ECullingMask::CornerSE | ECullingMask::BorderN | ECullingMask::BorderE |
	ECullingMask::BorderS | ECullingMask::BorderW | ECullingMask::DiagonalSWNE |
	ECullingMask::DiagonalNWSE | ECullingMask::Layer;

static auto AllMaskUint = static_cast<uint32>(AllMasks);
