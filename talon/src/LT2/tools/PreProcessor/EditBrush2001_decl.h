// talon: the 2001 classes EditBrush2001.cpp needs where the shared headers
// (Build 69's) differ, under names of their own (a 2001 suffix), to be
// folded into dtxmgr.h, EditVert.h, BasePoly.h, EditPoly.h and EditBrush.h.

#ifndef __EDITBRUSH2001_DECL_H__
#define __EDITBRUSH2001_DECL_H__

	#include "dtxmgr.h"


	// -------------------------------------------------------------------- //
	// dtxmgr.h, 2001.
	// -------------------------------------------------------------------- //

	// 0x18 bytes: TextureData's eight mips run 0xcc..0x18c (sizeof
	// TextureData is 0x18c, `push $0x18c` at 0x40d154) and its destructor
	// steps through them by 0x18 (0x40d086). After Build 69's four fields,
	// a second plane's pointer and pitch, which dtx_Alloc sets when its
	// seventh argument is (0x40d27a..0x40d286).
	class TextureMipData2001
	{
	public:

						TextureMipData2001();		// 0x40cff0

	public:

		uint32	m_Width, m_Height;

		uint8	*m_Data;
		int32	m_Pitch; // Pitch in bytes.

		uint8	*m_pPlane2;						// 0x10
		uint32	m_Plane2Pitch;					// 0x14
	};


	// 0x18c bytes, one virtual (its vtable 0x4747c4: SetupPFormat,
	// 0x40d0c0). As Build 69's but for the dword at 0xc4, which dtx_Alloc
	// clears (0x40d219) and nothing here reads; the pixel buffer is at 0xc8
	// (0x40d185) and the mips at 0xcc.
	class TextureData2001
	{
	public:

						TextureData2001();			// 0x40d010
						~TextureData2001();			// 0x40d050

		virtual void	SetupPFormat(PFormat *pFormat);

	public:

		BaseResHeader		m_ResHeader;		// 0x04
		DtxHeader			m_Header;			// 0x08
		LTLink				m_Link;				// 0xac
		uint32				m_AllocSize;		// 0xb8

		DtxSection			*m_pSections;		// 0xbc

		SharedTexture		*m_pSharedTexture;	// 0xc0
		uint32				m_Unknown_c4;		// 0xc4

		uint8				*m_pDataBuffer;		// 0xc8

		TextureMipData2001	m_Mips[MAX_DTX_MIPMAPS];	// 0xcc
	};


	TextureData2001*	dtx_Alloc2001(BPPIdent bpp, uint32 baseWidth, uint32 baseHeight, uint32 nMipmaps,
							uint32 *pAllocSize, uint32 *pTextureDataSize, LTBOOL bPlane2);			// 0x40d100
	LTRESULT			dtx_Create2001(ILTStream *pStream, TextureData2001 **ppOut, LTBOOL bLoadSections,
							LTBOOL bSkipImageData=FALSE);											// 0x40d2d0
	void				dtx_Destroy2001(TextureData2001 *pData);									// 0x40d650


	// -------------------------------------------------------------------- //
	// EditVert.h, EditPoly.h and EditBrush.h, 2001.
	// -------------------------------------------------------------------- //

	#include "worldnode.h"
	#include "boundingbox.h"
	#include "basepoly.h"

	class CEditBrush2001;
	class CEditRegion;


	// CEditVert as in Build 69 (40 bytes), but for its assignment, which
	// takes a whole CEditVert by value and copies the position only: every
	// CEditVert assigned in CMoArray<CEditVert>'s code is first copied whole
	// into a 40-byte temporary, then x, y and z stored from it (0x40e1e4,
	// 0x40e271, 0x40e5d5, 0x40ded8). Its default constructor is empty
	// (0x40df70, called where the inline budget is spent).
	class CEditVert2001 : public LTVector
	{
	public:
						CEditVert2001() {}
						CEditVert2001( const CVector &vec ) : LTVector(vec) {}

			void		operator = ( CEditVert2001 other )	{ x=other.x; y=other.y; z=other.z; }

	public:

			// Transformed and projected positions..
			LTVector	m_Transformed;
			LTVector	m_Projected;
			uint32		m_Unknown_24;
	};

	typedef CMoArray<CEditVert2001>		CEditVertArray2001;


	// The 2001 CEditPoly, 0x88 bytes (`push $0x88` at 0x40db72, 0x40dda2):
	// what this module calls of it, in the EditPoly module (0x40f7c0..).
	// Its CBasePoly (0x2c bytes) has one virtual (the vtable 0x474880 has
	// one slot) and its destructor is not virtual (Term calls it directly,
	// 0x40d97b). From the constructors (0x40f7c0, 0x40f8f0): the
	// texture's O, P and Q at 0x34, 0x40, 0x4c, its name at 0x74 ("Default"),
	// 0x70 set to 1 (written after Q by 0x40fcf0: sticktopoly?), the shade
	// at 0x58, a CMoArray at 0x5c whose elements are CBasePolys
	// (SaveTBW saves each with CBasePoly::SaveBasePolyTBW, 0x40dd27), the
	// surface flags at 0x84 (RaySelecter.cpp).
	class CEditPoly2001
	{
	public:

								CEditPoly2001();						// 0x40f7c0
								CEditPoly2001( CEditBrush2001 *pBrush );	// 0x40f8f0
								~CEditPoly2001();						// 0x40f9e0

		// The one virtual (0x40fe10, which copies the other poly's plane
		// and texture): by its place and what it does, Build 69's
		// CopyAttributes.
		virtual void			CopyAttributes( CEditPoly2001 *pOther, CStringHolder *pStringHolder=NULL );

		// 0x40fb30: the arguments as CEditBrush's LoadTBW passes them; the
		// third is not read there.
		BOOL					LoadEditPolyTBW( CAbstractIO& InFile, CStringHolder *pStringHolder, uint32 dwUnknown );
		void					SaveEditPolyTBW( CAbstractIO& OutFile );	// 0x40fc70
		// 0x40fcf0: O, P, Q, the dword at 0x70 and the texture name, which
		// SaveEditPolyTBW writes after the CBasePoly (0x40fc81).
		void					sub_40fcf0( CAbstractIO& OutFile );
		BOOL					CopyEditPoly( CEditPoly2001 *pPoly, BOOL bCopyIndices=TRUE, CStringHolder *pStringHolder=NULL );	// 0x410000

	public:

		CEditPlane				m_Plane;			// 0x04
		CMoWordArray			m_Indices;			// 0x14
		CEditBrush2001			*m_pBrush;			// 0x28

		uint8					m_Unknown_2c[0x5c - 0x2c];
		CMoArray<CBasePoly*>	m_Unknown_5c;		// 0x5c
		uint8					m_Unknown_70[0x88 - 0x70];
	};


	// CEditBrush as EditBrush.h has it (0xb0 bytes, the 2001 layout), with
	// the 2001 CEditPoly and CEditVert. Its vtable (0x4747c8): the scalar
	// deleting destructor (0x40d870), Term (0x40d960), AllocateSameKind
	// (0x40d810), DoCopy (0x40da00), GetUpperLeftCornerPos (0x40df80).
	class CEditBrush2001 : public CWorldNode
	{
		public:

									CEditBrush2001();
			virtual					~CEditBrush2001();

			virtual void			Term();
			virtual CWorldNode*		AllocateSameKind()	{return new CEditBrush2001;}
			virtual void			DoCopy(CWorldNode *pOther);

		// Loading/saving.
		public:

			// The third argument is handed to each poly's loader (0x40dbb2).
			BOOL					LoadTBW( CAbstractIO& InFile, CStringHolder *pStringHolder, uint32 dwUnknown );
			// With bUnknown, each poly is saved as its texture (0x40fcf0)
			// and the CBasePolys of its array at 0x5c (0x40dcfb).
			void					SaveTBW( CAbstractIO& OutFile, BOOL bUnknown );

		// Geometry stuff.
		public:

			void					CopyEditBrush( CEditBrush2001 *pOther, CStringHolder *pStringHolder=NULL );

			void					UpdateBSphere();

			virtual CVector			GetUpperLeftCornerPos();

		// Brush data.
		public:

			CMoArray<CEditPoly2001*>	m_Polies;
			CEditVertArray2001		m_Points;

			CBoundingBox			m_BoundingBox;

			uint32					m_RegionBrushIndex;

			uint8					r, g, b;

			CEditRegion*			m_pRegion;
			uint32					m_Unknown_ac;
	};

#endif  // __EDITBRUSH2001_DECL_H__
