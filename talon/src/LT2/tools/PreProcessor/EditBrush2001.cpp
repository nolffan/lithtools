// talon: 0x0040cff0..0x0040eb40, two modules of the 2001 shared tool code,
// to be folded into the shared files (which are Build 69's) later. The file
// name is ours and provisional.
//
//   0x40cff0..0x40d6f0   dtxmgr (shared/engine/dtxmgr.cpp): the 2001 DTX
//                        loader. Its TextureMipData is 0x18 bytes and its
//                        TextureData has a dword at 0xc4 before the pixel
//                        buffer; dtx_Alloc takes a seventh argument (a second
//                        plane of one byte a pixel for each mip). The rest
//                        of the module (SetupPFormat 0x40d0c0, dtx_ReadOrSkip
//                        0x40d5f0, dtx_Destroy 0x40d650, dtx_FindSection
//                        0x40d670, dtx_SetupDTXFormat2 0x40d6d0) is Build
//                        69's as it is. nolfsrc's engine has the same module
//                        (src/engine/lithtech/clientdsi.cpp, 0x42e7d0..),
//                        which the layouts below agree with.
//   0x40d6f0..0x40eae0   EditBrush (shared/world/EditBrush.cpp): the 2001
//                        CEditBrush, whose CEditPoly and CEditVert are not
//                        Build 69's (EditBrush2001_decl.h), and the
//                        CMoArray<CEditVert> it instantiates (its vtable
//                        0x4747dc). Among the template code, 0x40e7c0 is a
//                        CMoArray<T*> destructor (vtable 0x47483c, T not
//                        known) inside the function table's 0x40e790 row.
//   0x40eae0..0x40eb40   EditObjects' first two static initializers (at
//                        the end of the file).
//
// What is here stands beside the shared classes under names of its own
// (a 2001 suffix), so that it can be compiled and scored before the shared
// headers are brought back to 2001.

#include "bdefs.h"
#include "processing.h"
#include "EditBrush2001_decl.h"


// ======================================================================== //
// dtxmgr (shared/engine/dtxmgr.cpp), 2001.
// ======================================================================== //

// 0x40cff0. The stores are in the source's order (0x00, 0x04, 0x0c, 0x08,
// 0x10, 0x14), as nolfsrc found for the engine's copy.
TextureMipData2001::TextureMipData2001()
{
	m_Width = 0;
	m_Height = 0;
	m_Pitch = 0;
	m_Data = LTNULL;
	m_pPlane2 = LTNULL;
	m_Plane2Pitch = 0;
}


// 0x40d010: the eight mips constructed, the vtable (0x4747c4) stored and
// the buffer cleared.
TextureData2001::TextureData2001()
{
	m_pDataBuffer = LTNULL;
}


// 0x40d050.
TextureData2001::~TextureData2001()
{
	uint32 i;
	DtxSection *pCur, *pNext;

	if(m_Header.m_IFlags & DTX_MIPSALLOCED)
	{
		for(i=0; i < m_Header.m_nMipmaps; i++)
		{
			if(m_Mips[i].m_Data)
				dfree(m_Mips[i].m_Data);
		}
	}

	if(m_pDataBuffer)
		delete m_pDataBuffer;

	// Free the sections;
	pCur = m_pSections;
	while(pCur)
	{
		pNext = pCur->m_pNext;
		dfree(pCur);
		pCur = pNext;
	}
}


// 0x40d0c0 (the vtable's one slot); Build 69's.
void TextureData2001::SetupPFormat(PFormat *pFormat)
{
	dtx_SetupDTXFormat2(m_Header.GetBPPIdent(), pFormat);
}


// 0x40d5f0; Build 69's (static there too).
static void dtx_ReadOrSkip(
	LTBOOL bSkip,
	ILTStream *pStream,
	void *pData,
	uint32 dataLen)
{
	if(bSkip)
	{
		memset(pData, 0, dataLen);
		pStream->SeekTo(pStream->GetPos() + dataLen);
	}
	else
	{
		pStream->Read(pData, dataLen);
	}
}


// 0x40d100. Build 69's with a seventh argument: when it is set, each mip
// gets a second plane of one byte a pixel after its pixels (its pointer and
// pitch at 0x10 and 0x14). dtx_Create passes 0 (0x40d3a3). The stores are
// in nolfsrc's order for the engine's copy (m_AllocSize after m_pSections),
// and the second loop's pitch is an if-chain.
TextureData2001* dtx_Alloc2001(BPPIdent bpp, uint32 baseWidth, uint32 baseHeight, uint32 nMipmaps,
	uint32 *pAllocSize, uint32 *pTextureDataSize, LTBOOL bPlane2)
{
	TextureData2001 *pRet;
	TextureMipData2001 *pMip;
	uint32 i, size, width, height;
	uint32 textureDataSize;
	uint8 *pOutData;

	width = baseWidth;
	height = baseHeight;
	textureDataSize = 0;
	for(i=0; i < nMipmaps; i++)
	{
		size = CalcImageSize(bpp, width, height);
		if(bPlane2)
			size += width * height;
		textureDataSize += size;

		width >>= 1;
		height >>= 1;
	}

	pRet = new TextureData2001;
	if(!pRet)
		return LTNULL;

	pRet->m_pDataBuffer = new uint8[textureDataSize];
	if(!pRet->m_pDataBuffer)
	{
		delete pRet;
		return LTNULL;
	}

	pRet->m_ResHeader.m_Type = LT_RESTYPE_DTX;
	pRet->m_Header.m_ResType = LT_RESTYPE_DTX;
	pRet->m_Header.m_IFlags = DTX_SECTIONSFIXED;
	pRet->m_Header.m_UserFlags = 0;
	pRet->m_Header.m_nMipmaps = (uint16)nMipmaps;
	pRet->m_Header.m_BaseWidth = (uint16)baseWidth;
	pRet->m_Header.m_BaseHeight = (uint16)baseHeight;
	pRet->m_Header.m_Version = CURRENT_DTX_VERSION;
	pRet->m_Header.m_CommandString[0] = 0;
	pRet->m_Link.m_pData = pRet;
	pRet->m_pSections = LTNULL;
	pRet->m_AllocSize = sizeof(TextureData2001) + textureDataSize;
	pRet->m_Header.m_nSections = 0;
	pRet->m_Header.m_ExtraLong[2] = 0;
	pRet->m_Header.m_ExtraLong[1] = 0;
	pRet->m_Header.m_ExtraLong[0] = 0;
	pRet->m_Header.SetBPPIdent(bpp);
	pRet->m_pSharedTexture = LTNULL;
	pRet->m_Unknown_c4 = 0;

	// Setup the mipmap structures.
	width = baseWidth;
	height = baseHeight;
	pOutData = pRet->m_pDataBuffer;
	for(i=0; i < nMipmaps; i++)
	{
		pMip = &pRet->m_Mips[i];

		pMip->m_Width = width;
		pMip->m_Height = height;
		pMip->m_Data = pOutData;

		if(bpp == BPP_32)
			pMip->m_Pitch = (int32)width * sizeof(uint32);
		else if(bpp == BPP_32P)
			pMip->m_Pitch = width;
		else
			pMip->m_Pitch = 0;

		pOutData += CalcImageSize(bpp, width, height);

		if(bPlane2)
		{
			pMip->m_pPlane2 = pOutData;
			pMip->m_Plane2Pitch = width;
			pOutData += width * height;
		}

		width >>= 1;
		height >>= 1;
	}

	if(pAllocSize) *pAllocSize = pRet->m_AllocSize;
	if(pTextureDataSize) *pTextureDataSize = textureDataSize;

	return pRet;
}


// 0x40d2d0..0x40d5e2 (786 bytes: the function table's 0x40d417 row is its
// own tail, the target of the allocation test's jne at 0x40d3d6, not a
// function). Build 69's, with the 2001 mip records and dtx_Alloc's seventh
// argument; m_pSharedTexture is cleared before the header is copied.
LTRESULT dtx_Create2001(ILTStream *pStream,
	TextureData2001 **ppOut,
	LTBOOL bLoadSections,
	LTBOOL bSkipImageData)
{
	DtxHeader hdr;
	TextureData2001 *pRet;
	uint32 i, allocSize, textureDataSize;
	SectionHeader sectionHeader;
	DtxSection *pSection;
	uint32 iMipmap;
	TextureMipData2001 *pMip;
	uint32 y, size;



	STREAM_READ(hdr);
	if(hdr.m_ResType != LT_RESTYPE_DTX)
		RETURN_ERROR(1, dtx_Create, LT_INVALIDDATA);

	// Correct version and valid data?
	if(hdr.m_Version != CURRENT_DTX_VERSION)
		RETURN_ERROR(1, dtx_Create, LT_INVALIDVERSION);

	// talon: retail bug (0x40d380): up to 15 mipmaps pass, but a texture has
	// eight mip records (MAX_DTX_MIPMAPS), so a header with 9..15 makes
	// dtx_Alloc and the reads below write past the 0x18c-byte TextureData.
	if(hdr.m_nMipmaps == 0 || hdr.m_nMipmaps > 15)
		RETURN_ERROR(1, dtx_Create, LT_INVALIDDATA);

	// Allocate it.
	pRet = dtx_Alloc2001(hdr.GetBPPIdent(), hdr.m_BaseWidth, hdr.m_BaseHeight, hdr.m_nMipmaps,
		&allocSize, &textureDataSize, LTFALSE);

	if(!pRet)
		RETURN_ERROR(1, dtx_Create, LT_OUTOFMEMORY);

	pRet->m_pSharedTexture = LTNULL;
	memcpy(&pRet->m_Header, &hdr, sizeof(DtxHeader));

	// Read in mipmap data.
	for(iMipmap=0; iMipmap < hdr.m_nMipmaps; iMipmap++)
	{
		pMip = &pRet->m_Mips[iMipmap];

		if(hdr.GetBPPIdent() == BPP_32)
		{
			for(y=0; y < pMip->m_Height; y++)
			{
				// Read the line.
				dtx_ReadOrSkip(
					bSkipImageData,
					pStream,
					&pMip->m_Data[y*pMip->m_Pitch],
					pMip->m_Width * sizeof(uint32));
			}
		}
		else
		{
			size = CalcImageSize(hdr.GetBPPIdent(), pMip->m_Width, pMip->m_Height);
			dtx_ReadOrSkip(
				bSkipImageData,
				pStream,
				pMip->m_Data,
				size);
		}
	}

	// Read in the sections?
	if(hdr.m_IFlags & DTX_SECTIONSFIXED)
	{
		if(bLoadSections)
		{
			for(i=0; i < hdr.m_nSections; i++)
			{
				STREAM_READ(sectionHeader);
				pSection = (DtxSection*)dalloc((unsigned long)((sizeof(DtxSection)-1) + sectionHeader.m_DataLen));
				memcpy(&pSection->m_Header, &sectionHeader, sizeof(pSection->m_Header));
				pStream->Read(pSection->m_Data, sectionHeader.m_DataLen);
				pSection->m_pNext = pRet->m_pSections;
				pRet->m_pSections = pSection;
			}
		}
	}
	else
	{
		// Fix it...
		pRet->m_Header.m_IFlags |= DTX_SECTIONSFIXED;
		pRet->m_Header.m_nSections = 0;
	}

	// Check the error status.
	if(pStream->ErrorStatus() != LT_OK)
	{
		dtx_Destroy2001(pRet);
		RETURN_ERROR(1, dtx_Create, LT_INVALIDDATA);
	}

	*ppOut = pRet;

	return LT_OK;
}


// 0x40d650; Build 69's.
void dtx_Destroy2001(TextureData2001 *pData)
{
	if(pData)
	{
		delete pData;
	}
}


// ======================================================================== //
// EditBrush (shared/world/EditBrush.cpp), 2001: what Processor links of it,
// in image order (0x40d6f0..0x40e0f0, then its template code). Build 69's
// but for the CEditPoly and CEditVert it has, LoadTBW and SaveTBW.
// ======================================================================== //

// 0x40d6f0; Build 69's (EditBrush.cpp, already matched there).
CEditBrush2001::CEditBrush2001()
{
	m_Type			= Node_Brush;

	m_Unknown_ac	= 1;		// talon: 0x40d7cf
	m_pRegion		= NULL;

	m_Polies.SetCacheSize( 15 );
	m_Points.SetCacheSize( 15 );

	r = g = b = 255;

}


// 0x40d890; Build 69's.
CEditBrush2001::~CEditBrush2001()
{
	Term();
}


// 0x40d960. Build 69's, but the 2001 CEditPoly's destructor is not
// virtual (called directly, 0x40d97b).
void CEditBrush2001::Term()
{
	DeleteAndClearArray( m_Polies );
	m_Points.Term();

	CWorldNode::Term();
}


// 0x40da00; Build 69's.
void CEditBrush2001::DoCopy(CWorldNode *pOther)
{
	CopyEditBrush((CEditBrush2001*)pOther->AsBrush());
	CWorldNode::DoCopy(pOther);
}


// 0x40da20. Build 69's LoadTBW as it was: a third argument handed to each
// poly's loader, WORD counters (the `and $0xffff` at 0x40dae3 and
// 0x40dbcc), each point's coordinates read through m_Points[i] (the array
// reloaded for each, 0x40daba), and no UpdatePlanes or UpdateBoundingInfo
// at the end.
//
// talon: two retail bugs, kept. The WORD counters never reach a count over
// 65535 (the loops do not end). And when a poly fails to load, the
// m_Polies slots after it are left as SetSize allocated them, not
// initialised (BaseNew<CEditPoly*>, 0x40f600), and Term deletes every
// non-NULL slot; the caller (0x410870) has already put the brush in the
// region.
BOOL CEditBrush2001::LoadTBW( CAbstractIO& InFile, CStringHolder *pStringHolder, uint32 dwUnknown )
{
	//clean up the brush
	Term();

	//load in our color
	InFile >> r >> g >> b;

	//how many points we have
	uint32 nNumPoints;
	InFile >> nNumPoints;

	//resize our point list
	m_Points.SetSize(nNumPoints);

	//read them in
	for( WORD nCurrPoint = 0; nCurrPoint < nNumPoints; nCurrPoint++ )
	{
		//position
		InFile >> m_Points[nCurrPoint].x >> m_Points[nCurrPoint].y >> m_Points[nCurrPoint].z;
	}

	//now read in the polygons
	uint32 nNumPolygons;
	InFile >> nNumPolygons;

	m_Polies.SetSize( nNumPolygons );

	for( WORD nCurrPoly = 0; nCurrPoly < nNumPolygons; nCurrPoly++ )
	{
		m_Polies[nCurrPoly] = new CEditPoly2001( this );
		if( !m_Polies[nCurrPoly]->LoadEditPolyTBW(InFile, pStringHolder, dwUnknown ) )
			return FALSE;
	}

	return TRUE;
}


// 0x40dc00. Build 69's SaveTBW with a second argument: when it is set,
// each poly is saved as its texture (0x40fcf0) and the CBasePolys of its
// array at 0x5c, the count first.
void CEditBrush2001::SaveTBW( CAbstractIO& OutFile, BOOL bUnknown )
{
	//write out the brush color
	OutFile << r << g << b;

	//write out the list of points used by this brush
	uint32 nNumPoints = m_Points.GetSize();
	OutFile << nNumPoints;

	for(uint32 nCurrPoint = 0; nCurrPoint < m_Points.GetSize(); nCurrPoint++)
	{
		//position
		OutFile << m_Points[nCurrPoint].x << m_Points[nCurrPoint].y << m_Points[nCurrPoint].z;
	}

	//write out the list of polygons used by this brush
	uint32 nNumPolies = m_Polies.GetSize();
	OutFile << nNumPolies;

	//now write out the polygons
	for(uint32 nCurrPoly = 0; nCurrPoly < m_Polies.GetSize(); nCurrPoly++)
	{
		CEditPoly2001 *pPoly = m_Polies[nCurrPoly];

		if(bUnknown)
		{
			pPoly->sub_40fcf0(OutFile);

			uint32 nNumBasePolies = pPoly->m_Unknown_5c.GetSize();
			OutFile << nNumBasePolies;

			for(uint32 nCurrBasePoly = 0; nCurrBasePoly < pPoly->m_Unknown_5c.GetSize(); nCurrBasePoly++)
				pPoly->m_Unknown_5c[nCurrBasePoly]->SaveBasePolyTBW(OutFile);
		}
		else
		{
			pPoly->SaveEditPolyTBW(OutFile);
		}
	}
}


// 0x40dd60..0x40df70 (528 bytes; the function table's 0x40ddc2 row is the
// second byte of the call at 0x40ddc1, not a function). Build 69's.
void CEditBrush2001::CopyEditBrush( CEditBrush2001 *pOther, CStringHolder *pStringHolder )
{
	uint32		i;


	m_Polies.SetSize( pOther->m_Polies.GetSize() );
	for( i=0; i < pOther->m_Polies; i++ )
	{
		m_Polies[i] = new CEditPoly2001;
		m_Polies[i]->CopyEditPoly( pOther->m_Polies[i], TRUE, pStringHolder );
		m_Polies[i]->m_pBrush = this;
	}

	m_Points.CopyArray( pOther->m_Points );
	m_BoundingBox = pOther->m_BoundingBox;

	r = pOther->r;
	g = pOther->g;
	b = pOther->b;
}


// 0x40df80; Build 69's.
CVector CEditBrush2001::GetUpperLeftCornerPos()
{
	CVector vUpperLeft;
	vUpperLeft.x=0.0f;
	vUpperLeft.y=0.0f;
	vUpperLeft.z=0.0f;

	if (m_Points <= 0)
	{
		return vUpperLeft;
	}

	// Set the intial upper left point
	vUpperLeft=m_Points[0];

	// Find the smallest bounding edge that would appear in the upper left corner of the DEdit windows
	uint32 i;
	for( i=1; i < m_Points; i++ )
	{
		if (m_Points[i].x < vUpperLeft.x)
		{
			vUpperLeft.x=m_Points[i].x;
		}
		if (m_Points[i].y > vUpperLeft.y)
		{
			vUpperLeft.y=m_Points[i].y;
		}
		if (m_Points[i].z > vUpperLeft.z)
		{
			vUpperLeft.z=m_Points[i].z;
		}
	}

	return vUpperLeft;
}


// 0x40e030; Build 69's.
void CEditBrush2001::UpdateBSphere()
{
	if(m_Points)
	{
		m_BoundingBox.Init( m_Points[0] );
		for( uint32 i=1; i < m_Points; i++ )
			m_BoundingBox.Extend( m_Points[i] );
	}
}


// ======================================================================== //
// EditObjects (shared/world/EditObjects.cpp): the dynamic initializers of
// its first two globals, which the image puts here, ahead of the "DUMMY"
// string property's (0x40eb40) and EditObjects' own code. Build 69's
// globals; g_DummyPos is 0x484a38 (CWorldNode::GetPos returns it,
// 0x44e8fa), g_DummyRotation 0x4848f0. Under 2001 names here, to go back to
// EditObjects.cpp (EditObjects2001.cpp's module) when this is folded in.
// ======================================================================== //

CVector			g_DummyPos2001(0.0f, 0.0f, 0.0f);		// 0x40eae0 -> 0x40eaf0
LTVector		g_DummyRotation2001(0.0f, 0.0f, 0.0f);	// 0x40eb10 -> 0x40eb20
