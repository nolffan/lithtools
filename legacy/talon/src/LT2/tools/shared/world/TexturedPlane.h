#ifndef __TEXTUREDPLANE_H__
#define __TEXTUREDPLANE_H__

#include "editregion.h"

class CLTANode;
class CLTAFile;


class CTexturedPlane
{
	public:

		CTexturedPlane();
		CTexturedPlane(const CTexturedPlane& rhs);
		// talon: not virtual. The 2001 CEditPoly has m_pUser1 at 0x7c, so each
		// of its two planes is 40 bytes: the name and O, P, Q, no vtable.
		~CTexturedPlane();

		void				CopyTextureAttributes(const CTexturedPlane *pOther, CStringHolder *pStringHolder );
					
		void				GetTextureSpace(CVector &outO, CVector &outP, CVector &outQ)
		{
			outO = O;
			outP = P;
			outQ = Q;
		}

		void				SetTextureSpace(const LTVector& vNormal, const LTVector& newO, const LTVector& newP, const LTVector& newQ);

	// Accessors.
	public:

		const LTVector&		GetO() const				{ return O; }
		const LTVector&		GetP() const 				{ return P; }
		const LTVector&		GetQ() const				{ return Q; }

		void				SetO(const LTVector& tempO)	{ O = tempO; }
		void				SetP(const LTVector& tempP)	{ P = tempP; }
		void				SetQ(const LTVector& tempQ)	{ Q = tempQ; }

	//for streaming too and from an LTA
	public:

		void				SaveTextureInfoLTA( CLTAFile* pFile, uint32 level );
		void				SavePhysicsMaterialLTA( CLTAFile* pFile, uint32 level );
		void				SaveFlagsLTA(CLTAFile* pFile, uint32 level);
		void				SaveShadeLTA(CLTAFile* pFile, uint32 level);

		void				LoadTextureInfoLTA( CLTANode* pNode, CStringHolder* pStringHolder);

		void				LoadTextureInfoTBW( CAbstractIO& InFile, CStringHolder* pStringHolder);
		void				SaveTextureInfoTBW( CAbstractIO& OutFile );

#ifdef DIRECTEDITOR_BUILD
			
	// DEdit specific data.
	public:

		//used to get the dimensions of the texture
		uint32					GetBaseTextureWidth();
		uint32					GetBaseTextureHeight();
		void					UpdateTextureID();

	// talon: stand-in: the plane is laid over a 2001 CEditPoly's fields
	// (CEditPoly::GetTexture): O, P and Q at the poly's 0x34, the name at
	// 0x74 and the texture's file at 0x7c. Nothing of Build 69's detail
	// texture is in the image: always NULL.
	protected:

		LTVector				O, P, Q;

	private:

		uint8					m_PolyFields[0x1c];		// the poly's 0x58..0x74

	public:

		char					*m_pTextureName;		// the poly's 0x74
		uint32					m_SurfaceFlags;			// the poly's 0x78

		// NOTE: This ALWAYS points at a texture.  If m_pTextureName points at a sprite,
		// then this points at the first frame of the sprite's animation.
		struct DFileIdent_t		*m_pTextureFile;		// the poly's 0x7c
		static struct DFileIdent_t	*m_pDetailTextureFile;

#else

		char					*m_pTextureName;

	protected:

		LTVector				O, P, Q;

#endif

};

#endif
