// This module defines the lightmap compression and decompression routines.
// The compression is a simple lossless RLE scheme.

// These routines are used by the preprocessor, dedit, and the engine while
// dealing with lightmap data.

// talon: the 2001 one (0x414fb0..0x415200), written from the image: 32-bit
// pixels, and shadow maps of on/off runs.

#ifndef __LIGHTMAP_COMPRESS_H__
#define __LIGHTMAP_COMPRESS_H__

	// The largest lightmap, on a side and in all.
	#define LIGHTMAP_MAX_PIXELS			32
	#define LIGHTMAP_MAX_TOTAL_PIXELS	(LIGHTMAP_MAX_PIXELS*LIGHTMAP_MAX_PIXELS)

	// Each pixel is written as it is, or, starting a run of two or more,
	// with its high bit set and followed by the run's length (a byte).
	LTBOOL CompressLMData(uint32 *pData, uint32 width, uint32 height,
		uint32 *pOut, uint32 &outLen);

	// pOut MUST be LIGHTMAP_MAX_TOTAL_PIXELS long.
	LTBOOL DecompressLMData(uint32 *pCompressed, uint32 dataLen, uint32 *pOut);

	// A shadow map is lengths (bytes) of runs of unlit and lit pixels, in
	// turn, starting with unlit.
	LTBOOL CompressShadowMap(uint32 *pData, uint32 width, uint32 height,
		uint8 *pOut, uint32 &outLen);

	// pOut MUST be LIGHTMAP_MAX_TOTAL_PIXELS long; lit pixels are 0xFF.
	LTBOOL DecompressShadowMap(uint8 *pCompressed, uint32 dataLen, uint8 *pOut);

#endif
