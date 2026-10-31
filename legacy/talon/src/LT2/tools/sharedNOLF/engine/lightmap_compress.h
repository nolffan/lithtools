// This module defines the lightmap compression and decompression routines.
// The compression is a simple lossless RLE scheme.

// These routines are used by the preprocessor, dedit, and the engine while
// dealing with lightmap data.

// talon: NOLF's (0x422040..0x422270), written from the image: 16-bit
// (5-5-5) pixels, and shadow maps of on/off runs. AvP2's (its 0x414fb0..)
// has 32-bit pixels.

#ifndef __LIGHTMAP_COMPRESS_H__
#define __LIGHTMAP_COMPRESS_H__

	// The largest lightmap, on a side and in all.
	#define LIGHTMAP_MAX_PIXELS			32
	#define LIGHTMAP_MAX_TOTAL_PIXELS	(LIGHTMAP_MAX_PIXELS*LIGHTMAP_MAX_PIXELS)

	// Each pixel is written as it is, or, starting a run of two or more,
	// with its high bit set and followed by the run's length (a byte).
	LTBOOL CompressLMData(uint16 *pData, uint32 width, uint32 height,
		uint16 *pOut, uint32 &outLen);

	// pOut MUST be LIGHTMAP_MAX_TOTAL_PIXELS long.
	LTBOOL DecompressLMData(uint16 *pCompressed, uint32 dataLen, uint16 *pOut);

	// A shadow map is lengths (bytes) of runs of unlit and lit pixels, in
	// turn, starting with unlit.
	LTBOOL CompressShadowMap(uint16 *pData, uint32 width, uint32 height,
		uint8 *pOut, uint32 &outLen);

	// pOut MUST be LIGHTMAP_MAX_TOTAL_PIXELS long; lit pixels are 0xFF.
	LTBOOL DecompressShadowMap(uint8 *pCompressed, uint32 dataLen, uint8 *pOut);

#endif
