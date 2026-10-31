// ------------------------------------------------------------------------
// ltaScene.h
// lithtech (c) 2000
// 
// scene graph associated with lta files
// each node in the graph corresponds to a node in the lta file format.
// ------------------------------------------------------------------------

#ifndef LTA_SCENEGRAPH_DEFINED
#define LTA_SCENEGRAPH_DEFINED

#ifndef LT_TRUE 
#define LT_TRUE 1
#endif
#ifndef LT_FALSE
#define LT_FALSE 0
#endif  

#ifdef _WINDOWS
// windows ...
#include <windows.h>
// handle debug errors from windoze
#pragma warning(disable:4786 4018)
#endif


#include "ltbasetypes.h"
#include "ltaParseNode.h"

// std c++
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
//using namespace std;

// ------------------------------------------------------------------------
// ILTAObject 
// base object for lta import stuff
// every lta node has name.
// ------------------------------------------------------------------------
class ILTAObject {
	std::string m_Name ;
public :
	std::string			& GetName ();
	const std::string	& GetName() const ;
	void				SetName( std::string & name ) ;
	void				SetName( const std::string & name ) ;
};


// ------------------------------------------------------------------------
// CLTAMesh 
// mesh reprsentation. All members public, treat as struct, no funcs/methods etc.
//
// ------------------------------------------------------------------------
class CLTAMesh : public ILTAObject 
{
public :
	std::vector<LTVector> vertex ;
	std::vector<LTVector> normals ;
	std::vector<LTVector> tcoords ;
	std::vector<LTVector> colors ;

	// list of triangles
	std::vector< int >    tri_FaceSet ;
	std::vector< int >    tri_UVSet ;
	std::vector< int >	 tri_ColorSet ;
	std::vector< int >	 tri_NormSet ;
	std::vector< int >	 tri_FaceStripSet ;

	// list of polygons 
};


// ------------------------------------------------------------------------
// Container for geometry
// ------------------------------------------------------------------------
class CLTAGeometry : public ILTAObject {
public :
	enum { MESH, BSURF };

	CLTAMesh m_Mesh ;
};



// ------------------------------------------------------------------------
// Base class for all materials
// this hierarchy should be flat. meaning that all decendents from this
// class should be final. 
// ------------------------------------------------------------------------
class CLTAMaterial : public ILTAObject {

protected :
	int m_Type ;
public :
	enum { PC_MAT, PS2_MAT, RENDERSTYLE  }; 

	CLTAMaterial( int type ) :m_Type(type) {}

	
	int			GetType() const ;
};

// ------------------------------------------------------------------------
// Material definition as it is in the  abc (13) format
// ------------------------------------------------------------------------
class CLTA_ABC_PC_MAT : public CLTAMaterial {

public :
	// defaults
	// talon: 2001's, 0x68 bytes (the constructor 0x4130a7, the 0x68 new
	// in ParseShapeNode): one texture index and the environment texture's
	// (-1, none), no texture array.
	CLTA_ABC_PC_MAT();

	// params for the specular map
	float m_specular_power ;	// 0x14
	float m_specular_scale ;	// 0x18
	
	int   m_texture_index ;		// 0x1c (texture-index)
	int   m_envtex_index ;		// 0x20 (envtex-index)

	// colors 
	float m_diffuse[4];			// 0x24
	float m_ambient[4] ;		// 0x34
	float m_specular[4] ;		// 0x44
	float m_emissive[4] ;		// 0x54
	float m_shininess ;			// 0x64

};


// ------------------------------------------------------------------------
// CLTAShape
// container for a mesh and its appearance
// LTA Node :
// (shape name
//			(geometry ( ... ))
//          (appearance (...)))
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// the appearance of a shape: (appearance (material ...))
// talon: 2001's Shape holds one at 0xc0, 0x14 bytes: a name nothing in the
// image sets or reads and the material, which its constructor sets to NULL
// (inline in Shape's, 0x4131c6..0x4131da) and nothing deletes. The class
// name is ours.
// ------------------------------------------------------------------------
class CLTAAppearance : public ILTAObject {
public :
	CLTAAppearance() :m_pMaterial(NULL) {}

	CLTAMaterial	*m_pMaterial ;
};


// talon: 2001's is 0xe4 bytes (the new in ShapeFromPN, 0x41289e; the
// constructor 0x41317f, the destructor 0x40c13d): the appearance at 0xc0,
// the parent name at 0xd4.
class Shape  : public ILTAObject {
	
	CLTAGeometry	m_Geometry;			// 0x10
	CLTAAppearance	m_Appearance;		// 0xc0
	std::string		m_ParentName;		// 0xd4
	
public:
	Shape() ;

	// accessors
	void			SetParentName( const char *parent_name ) ;
	void			SetMeshName  ( const char *mesh_name   ) ;
	
	const std::string& GetParentName() ;
	const std::string& GetMeshName()   ;
	
	CLTAMesh&		GetMesh()			;
	CLTAMaterial*	GetAppearance()		;
	void			SetAppearance(CLTAMaterial *val);
};


// ------------------------------------------------------------------------
// CLTANode
// A node is a hierarchy element. usually a transform
// ------------------------------------------------------------------------
class Node : public ILTAObject {

public :

	// transform for the node 
	LTMatrix mat ;	// s/b called transform 

	// Reference back to shape that points to this node. 
	Shape    *shape ; // shape that references this node.

	// Shapes associated with this transform.
	std::vector<Shape*> shapeList ;
	
	// flat list index
	int      index ;
	
	Node    *parent ;
	std::vector<Node*> children;	
	
public:
	Node() ;
	Node( std::string &name );
	~Node();
	

	// finds the node with the name sname ...
	Node *FindName( const std::string & sname );
	void AddShape( Shape *shape )          ;


	void AddChild( Node *new_node ) ;

};

// ------------------------------------------------------------------------
// CLTAKeyFrames
// time-value pairs, the value is an index
// (keyframe name (times ( ... ) ) )
// ------------------------------------------------------------------------
class CKeyFrame : public ILTAObject {

	struct key_value {
		key_value( int t, int idx ):time(t),index(idx) {}
		key_value( int t, std::string &val ):time(t),value(val) {}
		key_value():time(-1),index(-1) {}

		int time ;
		int index ; //  value ;
		std::string value ; // conform to lt where we have named times.
	};

	int m_LastTime ;

	std::vector<key_value> m_times ;
	
	void Append( const key_value & kv ) ;

	// ------------------------------------------------------------------------
	// 
	// ------------------------------------------------------------------------
	public :
	
		CKeyFrame() :m_LastTime(0) {}

		// add 
		void Add( int time, int value  ) ;

		void Add( int time , std::string & value ) ;

		// add the value at INDEX, NOT TIME
		void AddValue( int index, std::string & val  );


	// assignment  
	void operator=( const CKeyFrame & kf ) ;
	

	// accessors 
	int Size()           const;

	int  GetTime( int i ) const ;
	void SetTime( int index, int new_time ) ;

	const std::string& GetVal( int i ) const ;

	
	// ------------------------------------------------------------------------
	// ret getIndex( time, start-time, end-time, percent-in-between)
	//
	// get the keyframe index, given time (brute force seach)
	// fills ret-params with the the closest frames and the exact parametric point in between
	// returns 1 on success, 0 on not finding any appropriate values
	// ------------------------------------------------------------------------
	int GetIndex( int time, int &start, int &end, float &percent_in_between) const ;
};

// ------------------------------------------------------------------------
// CLTAAnim 
// super class of all animations.
// class holds the global key-times 
// 
//(anim [name]
//		(keyframe (keyframe ... ))
//		(frames ("type" ... )))
//
// ------------------------------------------------------------------------
class CAnim : public ILTAObject {

	int			m_Type ;
	int			m_bKF_Shared ; 	// LT_TRUE if shared, LT_FALSE if not
	CKeyFrame * m_pKeyFrame ;

protected :
	CAnim( int type ):m_Type(type),m_pKeyFrame(NULL),m_bKF_Shared(LT_FALSE) {}
	
public :

	// possible types of anims 
	enum { UNDEFINED, VERTEX, POSQUAT, COLOR };

	CAnim():m_Type(UNDEFINED),m_pKeyFrame(NULL),m_bKF_Shared(LT_FALSE) {}

	virtual ~CAnim() {
		if( m_pKeyFrame != NULL && !m_bKF_Shared )
			delete m_pKeyFrame ;
	}


	int			GetType() ; // get animation type

	// Make Own Copy of KF
	// set from a ref
	void		SetKeyFrameCopy( const CKeyFrame & kf );
	
	
	// Assign a shared version of the keyframe object
	void		ShareKeyFrame( CKeyFrame *kf );
	

	// getKeyFrame 
	CKeyFrame *	GetKeyFrame() ;

	// returns 0 on fail
	int			GetKeyFrameIndexFromTime( int tm, int &start, int &end, float &percent );
	
	// get target name 
	virtual std::string GetTargetName() = 0;
};

// ------------------------------------------------------------------------
// CLTAVtxAnim
// version of the animation node that holds vertex animation.
/* 
(anim [name]
	(keyframe (keyframe ... ))
	(frames ("type" ... )))

this is the anim node, this class will be instanced if the "type" is vertex.
*/
// ------------------------------------------------------------------------

class CVtxAnim : public CAnim  {

	Shape *  m_target ;

	struct DefVertexLst {
		std::vector<LTVector> m_vVertex ;
	};

	std::vector<DefVertexLst> m_vFrames ;

public :

	CVtxAnim(): CAnim(VERTEX),m_target(NULL) {}
	

	// set what we are acting on,
	// name the animation after target
	void SetTarget( Shape * bs)   ;
	Shape *GetTarget();
	
	// add an animation
	void AppendVertexList ( std::vector<LTVector> & v_list );

	// frame acces 
	int NumVertexPerFrame();
	// get value at frame/mesh index
	LTVector Get( int frame, int index ) ;
	// get target name
	std::string GetTargetName() ;
	
};

// ------------------------------------------------------------------------
// CLTAPosQuatAnim
// version of anim that holds position/quaternion pairs.
// ------------------------------------------------------------------------
class CPosQuatAnim : public CAnim {

private :
	// private structure that holds pos/quat pairs
	struct PosQuat {	
		float pos[3];
		float quat[4];
	};

	std::vector<PosQuat> m_vPosQuatDat ;
	Node *m_target ;

	
	void SetPosQuat( PosQuat & pq, float *p, float *q );
	void GetPosQuat( PosQuat &pq, float *p , float *q );


public :
	
	CPosQuatAnim(): CAnim(POSQUAT),m_target(NULL) {}

	// gets frame X
	void Get( int frame, LTVector & pos, float *quat );


	// append a new position/quaternion on to list
	void Append( float *p, float *q );


	// set what we are acting on,
	// name the animation after target
	void SetTarget( Node * bs);
	
	Node *GetTarget();

	std::string GetTargetName() ;
};

// ------------------------------------------------------------------------
// 
// ------------------------------------------------------------------------
class CChannelAnim : public CAnim {
	
	
};

// ------------------------------------------------------------------------
// CLTAAnimSet
// collection of anims :
// (animset name 
//			(keyframe (keyframe ... ))
//			(anims    ( ... )))
// 
// ------------------------------------------------------------------------
class CAnimSet : public ILTAObject {
public :	
	CKeyFrame m_KeyFrame ;
	
	// list of animation frames
	std::vector<CAnim*> m_vAnims ;
	
	~CAnimSet();
	
	// ------------------------------------------------------------------------
	// share the animset's keyframe with the sub anims
	// ------------------------------------------------------------------------
	void KFFix() ;

	// append
	void AddAnim( CAnim * nan ) ;

	
	// find an animation by what its target is 
	CAnim *FindAnimByTargetName( const std::string & name );

	// find an animation by name. Returns null if not found.
	CAnim *FindAnim( const std::string & name );
	int NumAnims() { return m_vAnims.size(); }
	CAnim *GetAnim( uint32 index ) { return m_vAnims[index]; }

	void SetKeyFrame( const CKeyFrame & new_kf );
	CKeyFrame *GetKeyFrame()  ;
	
	// empty returns if there are no anims... 
	int Empty() ;
	// returns the type of all the anims. if the set is heterogenous or empty it returns UNDEFINED
	int GetAnimType() ;
	
};


// ------------------------------------------------------------------------
// CLTAMetaModel
// Intermediary model, stores stuff read from lta.
// ------------------------------------------------------------------------
// talon: 2001's is 0x28 bytes (the new at 0x40bcf7; its constructor
// 0x40bf5c and destructor 0x40bf8a are inline, emitted in ltaModel): the
// anim sets, the last anim, the hierarchy, the shapes. No LOD groups, OBBs
// or tools info.
class MetaModel {

public :
	std::vector<CAnimSet*> m_vAnimSets ;	// 0x00
	int m_LastAnim ;						// 0x10

	Node *rootNode ;		// hierarchy	// 0x14
	std::vector<Shape*> shapes ;			// 0x18

// METHODS 

	// constructors/
	MetaModel() :m_LastAnim(-1),rootNode(NULL) {}
	~MetaModel()
	{
		for( uint32 i = 0 ; i < shapes.size() ; i++ )
		{
			Shape * shape = shapes[i] ;
			delete shape ;
			shapes[i] = NULL;
		}
		shapes.resize(0);

		for ( i = 0 ; i < m_vAnimSets.size() ; i++ )
		{
			CAnimSet *pAnimSet = m_vAnimSets[i];
			delete pAnimSet ;
			m_vAnimSets[i] = NULL ;
		}
		m_vAnimSets.resize(0);

		if( rootNode != NULL )
			delete rootNode ;
	}

	
	// ------------------------------------------------------------------------
	// animation set access
	// ------------------------------------------------------------------------
	
	// Create a new animations set. 
	// metaModel is responsible for creating sets
	CAnimSet * CreateAnimSet();
	
	// create named animation set.
	CAnimSet * CreateAnimSet(std::string & name );
	
	// add an anim set.
	void AppendAnimSet( CAnimSet *newAnimSet );
	
	// returns null if empty.
	CAnimSet * GetLastAnimSet();
	
	int  NumAnimSets();

	// get the anim set // note, no range check ... 
	CAnimSet *GetAnimSet( int i ) ;
	
	// find a shape by name .
	Shape * FindShape( const std::string & name ) ;
	
	// find a node by name 
	Node * FindNode( const std::string & name );
	
	// hook shape to parent node; associates a shape to a node.
	int HookShapeToParent( Shape *shape );

	// removes animsets that have no data.
	void RemoveEmptyAnimSets();
	
	// sum up the number of verts in all the shapes .
	int CalcNumVertex() ;
};


// ------------------------------------------------------------------------
// CLTATranslStatus
// lta-translator-status
// a light weight error class for static funcs in this file

// ------------------------------------------------------------------------
class CLTATranslStatus {
	//
	static std::ostream *s_CurErrorLog ;
	// 
	static void (*s_fnLoadLogCallBack) (const char *);
public:
	
	enum { OK, FAIL, SUCCEED, OUT_OF_MEM, BAD_PARAM};
	int m_status ;
	std::string m_msg ;

	// default constructor
	CLTATranslStatus() :m_status(OK) {}
	// functor constr 
	CLTATranslStatus( char *errormsg, char *ErrorTitle=NULL ) { m_msg = errormsg ; OnError( ErrorTitle ) ; }

	
	// evaluation operator:
	// set the status value
	CLTATranslStatus & operator=( int val  ) { m_status = val ; return *this;}
	// compare status values.
	bool operator!=( int val ) { return val != m_status ; }
	bool operator==(int val  ) { return val == m_status ; }

	// call this to announce an error has occured.
	void OnError( char *ErrorTitle=NULL, int bShowErrWin = 1) ;
	

	// ------------------------------------------------------------------------
	// seterror log ( out-stream )
	// static function that sets the error log stream to which 
	// all messages are going to be written to.
	// ------------------------------------------------------------------------
	static void SetErrorLog( std::ostream * ostr );
	// sets the function that gets called each time AddToLog gets called.
	static void SetLoadLogCB( void (*fnPtr)(const char *) );

	static void AddToLog( const std::string & str );

	// returns the current outstream that all msgs are sent to.
	std::ostream& errLog() ;

	// the << operator. treat instance as a out-stream
	CLTATranslStatus & operator<<( const char *msg );
	
	// resets the status for more use. 
	void reset() ;
};


// ------------------------------------------------------------------------
// LTA->ParseNode Function set
// ------------------------------------------------------------------------
class LTA {
public :

	// Interpret Parse nodes' hierarchy node, put result into meta-model.
	static 
	void HierarchyFromPN(	MetaModel & metaModel,		// data container
							CParseNode *pnroot,			// parse node tree
							CLTATranslStatus & status   // error reporting
							); 

	
	// Evaluate the parse tree for shapes.
	// fill the metamodel container with new data.
	

	

	

	

	// process & add parsenode that reps a shape node into metamodel
	static 
	void ShapeFromPN( MetaModel & metaModel, CParseNode *pnroot );

	// ------------------------------------------------------------------------
	// Get the tools info node from the lta and associate it with the meta model.
	// ------------------------------------------------------------------------
	

	// ------------------------------------------------------------------------
	// Starting point for creating an in-memory representation of a model 
	// lta file.
	// ------------------------------------------------------------------------
	



	///////////////////////////////////////////////////////////////////////
	// Utils 

	typedef std::vector< std::pair<int,int> > CAnimPairLst  ;

	typedef std::vector< std::pair<int,int> > CClassifiedAnimLst ;
	


	// given a classified list list and an index into that list
	// return true if the classification for that index indicates that
	// the the animation should be merged.
	static
	int ShouldMerge( CClassifiedAnimLst & CurLst, int index );
	
	
	// ------------------------------------------------------------------------
	// CollectAndClassifyAnims( metaModel, result-list )
	// check the animation sets in the MetaModel for name matches. 
	// a name match is an indication that a vertex animset and a pos/quat animset
	// should be merged. The criteria is an animation named nameVA and name both have
	// the same root name. The VA postfix indicates that it is an vertex animation.
	// ------------------------------------------------------------------------
	static 
	int CollectAndClassifyAnims( MetaModel & metaModel, CClassifiedAnimLst & );

	// ------------------------------------------------------------------------
	// ParseVector( pn-list, destination-vector)
	// The pn-list should be a list that contains an array of floats.
	// The destination vector should have enough space to fill the result.
	// pn-list => ( a b c ) = destination-vector[] = { a, b, c };
	// ------------------------------------------------------------------------
	static
	void ParseVector( CParseNode *vrt, float * vert );

	// ------------------------------------------------------------------------
	// finds all the animsets in the parsenode tree and processes them.
	// ------------------------------------------------------------------------
	

	// AnimSetFromPN ( MetaModel, start-parse-node, return-status )
	// Evaluate parsenode for anim-set information,
	// put result into metaModel .
	static
	void AnimSetFromPN( MetaModel & metaModel,          // data container
						CParseNode *pnroot,				// parse node tree = (animset
						CLTATranslStatus & status		// error reporting
					);


};


/////////////////////////////////////////////////////////////////////////////
// ------------------------------------------------------------------------//
// Inlines																   //
// ------------------------------------------------------------------------//
////////////////////////////////////////////////////////////////////////////

// ------------------------------------------------------------------------
// ILTAObject 
// base object for lta import stuff
// every lta node has name.
// ------------------------------------------------------------------------
inline std::string & ILTAObject::GetName () { return m_Name ; }
inline const  std::string & ILTAObject::GetName() const { return m_Name ; }
inline void     ILTAObject::SetName( std::string & name ) { m_Name = name ; } 
inline void     ILTAObject::SetName( const std::string & name ) { m_Name = name ; }

// ------------------------------------------------------------------------
// Base class for all materials
// this hierarchy should be flat. meaning that all decendents from this
// class should be final. 
// ------------------------------------------------------------------------
	
inline int CLTAMaterial::GetType() const  { return m_Type ; } 

// ------------------------------------------------------------------------
// CLTAShape
// ------------------------------------------------------------------------

inline	void Shape::SetParentName( const char *parent_name ) { m_ParentName = parent_name ; }
inline	void Shape::SetMeshName  ( const char *mesh_name   ) { m_Geometry.m_Mesh.SetName( mesh_name ) ; } 	
inline	const std::string& Shape::GetParentName() { return m_ParentName ; } 
inline	const std::string& Shape::GetMeshName()   { return m_Geometry.m_Mesh.GetName() ; } 
inline	CLTAMesh&		Shape::GetMesh()			{ return m_Geometry.m_Mesh ; }
inline	CLTAMaterial*	Shape::GetAppearance()		{ return m_Appearance.m_pMaterial  ; }

inline void Shape::SetAppearance(CLTAMaterial *val) { m_Appearance.m_pMaterial = val ; }



// ------------------------------------------------------------------------
// CLTANode
// A node is a hierarchy element. usually a transform
// ------------------------------------------------------------------------	
	
inline void Node::AddShape( Shape *shape )           { shapeList.push_back(shape); }
inline void Node::AddChild( Node *new_node ) { children.push_back( new_node ); }



// ------------------------------------------------------------------------
// CLTAKeyFrames
// ------------------------------------------------------------------------
inline	int  CKeyFrame::Size()           const { return m_times.size() ; }
inline	void CKeyFrame::Append( const key_value & kv ) { m_times.push_back( kv ) ; }
inline	void CKeyFrame::Add( int time, int value  ) { m_times.push_back( key_value( time, value ) ) ; } 
inline	void CKeyFrame::Add( int time , std::string & value ) { m_times.push_back( key_value( time, value ) ) ; }
inline	void CKeyFrame::AddValue( int index, std::string & val  )   { m_times[ index ] .value = val ; }
inline	void CKeyFrame::operator=( const CKeyFrame & kf ) 
{
	m_times.clear();
	for( uint32 i = 0 ; i < kf.m_times.size() ; i++ )
		Append( kf.m_times[i] );
}

	// accessors 


inline	int  CKeyFrame::GetTime( int i ) const { return m_times[i].time ; } 
inline	void CKeyFrame::SetTime( int index, int new_time ) { m_times[index].time = new_time ;}
inline	const std::string& 
             CKeyFrame::GetVal( int i ) const { return m_times[i].value ; } 

	
// ------------------------------------------------------------------------
// ret CKeyFrame::GetIndex( time, start-time, end-time, percent-in-between)
//
// get the keyframe index, given time.
// fills ret-params with the the closest frames and the exact parametric point in between
// returns 1 on success, 0 on not finding any appropriate values
// ------------------------------------------------------------------------
inline	int CKeyFrame::GetIndex( int time, int &start, int &end, float &percent_in_between) const 
	{
		int timesSize = m_times.size();

		if( timesSize >= 2 )
		for( int i = 1 ;i < timesSize ; i++ )
		{
			if( time <= m_times[i].time )
			{
				start = i -1;
				end   = i   ;

				int startT = m_times[start].time ;
				int endT   = m_times[end].time ;

				percent_in_between = (( time - startT ) / (float)( endT - startT ));
				return 1;
			}
		}
		else {
			if( m_times.size() == 1 )
			{
				if( m_times[0].time == time )
				{
					start = 0;
					end   = 0;
					percent_in_between = 1.0f ;
					return 1;
				}
			}
		}
		return 0;
	}


// ------------------------------------------------------------------------
// CLTAAnim 
// ------------------------------------------------------------------------
inline int CAnim::GetType() { return m_Type ; }

// Make Own Copy of KF set from a ref
inline void CAnim::SetKeyFrameCopy( const CKeyFrame & kf )
{
	if( m_pKeyFrame == NULL )
	{
		m_pKeyFrame = new CKeyFrame ;	
	}

	*m_pKeyFrame = kf ;
	m_bKF_Shared = LT_FALSE ;
}

// Assign a shared version of the keyframe object
inline void CAnim::ShareKeyFrame( CKeyFrame *kf )
{
	// if not null, if already shared then overwrite the pointer, else delete old
	// data and assign the reference
	if( m_pKeyFrame != NULL )
	{
		if( m_bKF_Shared == LT_TRUE )
		{
			m_pKeyFrame = kf ;
		}else // the kf is not shared delete it to share it
		{
			delete m_pKeyFrame ;
		}
	}

	m_pKeyFrame = kf ;
	m_bKF_Shared = LT_TRUE ;	
}

inline CKeyFrame *CAnim::GetKeyFrame() { return m_pKeyFrame ; }

// returns 0 on fail
inline int  CAnim::GetKeyFrameIndexFromTime( int tm, int &start, int &end, float &percent )
{
	if( m_pKeyFrame ){
		return m_pKeyFrame->GetIndex( tm, start, end, percent );
	}else
		return 0;
}


// ------------------------------------------------------------------------
// CLTAVtxAnim
// version of the animation node that holds vertex animation.
/* 
(anim [name]
	(keyframe (keyframe ... ))
	(frames ("type" ... )))

this is the anim node, this class will be instanced if the "type" is vertex.
*/
// ------------------------------------------------------------------------

	// set what we are acting on,
	// name the animation after target
inline	void CVtxAnim::SetTarget( Shape * bs)               { m_target = bs ; if(bs!=NULL) SetName( bs->GetName() ) ;}
	inline Shape *CVtxAnim::GetTarget()                         { return m_target ; }
	
	// add an animation
inline	void CVtxAnim::AppendVertexList ( std::vector<LTVector> & v_list )
	{
		DefVertexLst newList ;
		newList.m_vVertex = v_list ;
		m_vFrames.push_back( newList  );
	}

	
	// frame acces 
inline	int CVtxAnim::NumVertexPerFrame()              { return m_vFrames[0].m_vVertex.size() ; }

inline	LTVector CVtxAnim::Get( int frame, int index ) 
	{
		// if debugging 
		
		//ASSERT( frame < m_vFrames.size() );
		//ASSERT( index < m_vFrames[frame].m_vVertex.size() );

		return m_vFrames[frame].m_vVertex[index];
	}

inline	std::string CVtxAnim::GetTargetName() {
		if( m_target )
			return m_target->GetName() ;
		return "";
	}


// ------------------------------------------------------------------------
// CLTAPosQuatAnim
// ------------------------------------------------------------------------
inline	void CPosQuatAnim::SetPosQuat( PosQuat & pq, float *p, float *q )
{
	memcpy( pq.pos, p, sizeof(float)*3);
	memcpy( pq.quat, q, sizeof(float)*4);
}

inline	void CPosQuatAnim:: GetPosQuat( PosQuat &pq, float *p , float *q )
{
	memcpy( p, pq.pos, sizeof(float)*3);
	memcpy( q, pq.quat,sizeof(float)*4);
}


// gets frame X
inline	void CPosQuatAnim:: Get( int frame, LTVector & pos, float *quat )
{
	GetPosQuat( m_vPosQuatDat[frame], &pos.x, quat); 
}

// append a new position/quaternion on to list
inline	void CPosQuatAnim:: Append( float *p, float *q )
{
	PosQuat pq ;
	SetPosQuat( pq, p, q );
	m_vPosQuatDat.push_back( pq );
}

// set what we are acting on,
// name the animation after target
inline	void CPosQuatAnim:: SetTarget( Node * bs)       { m_target = bs ; if(bs!=NULL) SetName( bs->GetName() ) ;}

inline	Node *CPosQuatAnim::GetTarget()                         { return m_target ; }

inline	std::string CPosQuatAnim::GetTargetName() { if( m_target) return m_target->GetName() ; else return std::string("");}


// ------------------------------------------------------------------------
// CLTAAnimSet
// ------------------------------------------------------------------------
	
inline	void CAnimSet::KFFix() 
{
	if( m_vAnims.size() > 0 ) 
	{
		CKeyFrame *pKF ;
		for( uint32 i = 0  ; i < m_vAnims.size() ; i++ )
		{
			pKF = m_vAnims[0]->GetKeyFrame() ;
			if( pKF != NULL )
				break ;
		}
		SetKeyFrame( *pKF );
	}
}

	// append
inline	void CAnimSet::AddAnim( CAnim * new_anim ) {		m_vAnims.push_back( new_anim ) ; }
	
	// find an animation by what its target is 
inline	CAnim *CAnimSet::FindAnimByTargetName( const std::string & name )
{
	for( uint32 i = 0 ;i < m_vAnims.size() ; i++ )
	{
		if( m_vAnims[i]->GetTargetName() == name ) 
		{
			return m_vAnims[i];
		}
	}
	return NULL ;
}

	// findAnim( name )
	// find an animation by name. Returns null if not found.
inline	CAnim *CAnimSet::FindAnim( const std::string & name )
{
	for( uint32 i = 0 ;i < m_vAnims.size() ; i++ )
	{
		if( m_vAnims[i]->GetName() == name ) 
		{
			return m_vAnims[i];
		}
	}
	return NULL ;
}

inline	void CAnimSet::SetKeyFrame( const CKeyFrame & new_kf )	{m_KeyFrame = new_kf ;	}

inline	CKeyFrame *CAnimSet::GetKeyFrame()  { return & m_KeyFrame ; }
	
	// empty returns if there are no anims... 
inline	int CAnimSet::Empty() 
	{
		return m_vAnims.size() != 0 ;
	}

	// returns the type of all the anims. if the set is heterogenous or empty it returns UNDEFINED
inline	int CAnimSet::GetAnimType() {
		int type=CAnim::UNDEFINED ;
		
		if( m_vAnims.size() > 0 )
		{
			type = m_vAnims[0]->GetType();

			for( uint32 i = 1 ;i < m_vAnims.size() ; i++ )
			{
				
				int t = m_vAnims[i]->GetType() ;
				if( t != type )
				{
					return CAnim::UNDEFINED  ;
				}
			}
		}
		return type ;
	}


// ------------------------------------------------------------------------
// lod-sets
// lod group :
// (distance - shape )
// ------------------------------------------------------------------------

	
inline CAnimSet * MetaModel::CreateAnimSet()
{
	
	CAnimSet * as = new CAnimSet ;
	if( as == NULL )
		return NULL ;
	m_LastAnim++;
	m_vAnimSets.push_back( as );
	return as ;
}

// create named animation set.
inline CAnimSet * MetaModel::CreateAnimSet(std::string & name )
{
	CAnimSet *as = CreateAnimSet();
	as->SetName( name );
	return as ;
}

// add an anim set.
inline void MetaModel::AppendAnimSet( CAnimSet *newAnimSet )
{
	m_vAnimSets.push_back( newAnimSet );
	m_LastAnim++;
}

// returns null if empty.
inline CAnimSet * MetaModel::GetLastAnimSet()
{
	if( m_LastAnim >= 0 )
		return m_vAnimSets[m_LastAnim];
	else return NULL ;
}

inline int  MetaModel::NumAnimSets(){ return m_vAnimSets.size() ; }

// get the anim set // note, no range check ... 
inline CAnimSet *MetaModel::GetAnimSet( int i ) 
{
	return m_vAnimSets[i]; 
}


// ------------------------------------------------------------------------
// find a shape by name .
// ------------------------------------------------------------------------
inline Shape * MetaModel::FindShape( const std::string & name ) 
{
	for( uint32 i = 0 ;i < shapes.size() ; i++ )
	{
		if( shapes[i]->GetName() == name ) 
		{
			return shapes[i] ;
		}
	}
	return NULL ;
}

inline Node * MetaModel::FindNode( const std::string & name )
{
	if( rootNode )
		return rootNode->FindName( name );
	return NULL ; 
}

// hook shape to parent node
// associates a shape to a node.
inline int MetaModel::HookShapeToParent( Shape *shape )
{
	Node *pNode = FindNode( shape->GetParentName() );
	if( pNode != NULL )
	{
		pNode->shape = shape ;
		return 1;
	}
	
	return 0 ;
}

// removes animsets that have no data.
inline void MetaModel::RemoveEmptyAnimSets()
{
	CAnimSet *pCurAnimSet ;
	std::vector<CAnimSet*>::iterator it;
	

	for(it = m_vAnimSets.begin(); it != m_vAnimSets.end() ; it++ )
	{
		pCurAnimSet = *it ;
		if( pCurAnimSet->Empty() ) {
			m_vAnimSets.erase( it );
			// delete pCurAnim
		}
	}
}

// sum up the number of verts in all the shapes .
inline int MetaModel::CalcNumVertex() 
{
	int totalVs = 0;
	for( uint32 i = 0 ;i < shapes.size(); i++ )
		totalVs+= shapes[i]->GetMesh().vertex.size() ;
	return totalVs ;
}

// add a lod set with name to meta model
inline void CLTATranslStatus::OnError( char *ErrorTitle, int bShowErrWin ) 
{	
	std::string msg ;
	if( ErrorTitle )
	{
		msg = ErrorTitle ;
	}else 
		msg = "error " ;
#ifdef _WINDOWS
	if( bShowErrWin )
		MessageBox(NULL, m_msg.c_str(), msg.c_str() , MB_OK );
#endif
	if( s_fnLoadLogCallBack )
	{
		s_fnLoadLogCallBack( msg.c_str() );
		s_fnLoadLogCallBack( m_msg.c_str());
		s_fnLoadLogCallBack( "\r\n");
	}

	errLog() << msg.c_str() << " " << m_msg.c_str() << std::endl;
}

// ------------------------------------------------------------------------
// seterror log ( out-stream )
// static function that sets the error log stream to which 
// all messages are going to be written to.
// ------------------------------------------------------------------------
inline void CLTATranslStatus::SetErrorLog( std::ostream * ostr )
{
	s_CurErrorLog = ostr ;
}

inline void CLTATranslStatus::SetLoadLogCB( void (*fnPtr)(const char *) )
{
	s_fnLoadLogCallBack = fnPtr ;
}

inline void CLTATranslStatus::AddToLog( const std::string & str )
{
	if( s_fnLoadLogCallBack )
		s_fnLoadLogCallBack( str.c_str() );
}

// returns the current buffer that all msgs are sent to.
inline std::ostream& CLTATranslStatus::errLog() { if(s_CurErrorLog) return *s_CurErrorLog ; else return std::cout ; }

// the << operator. treat instance as a out-stream
inline CLTATranslStatus & CLTATranslStatus::operator<<( const char *msg ){
	m_msg += std::string(msg);

	//if(s_fnLoadLogCallBack != NULL ){
		//s_fnLoadLogCallBack( m_msg.c_str() );
	//}

	return *this ;
}


// resets the status for more use. 
inline void CLTATranslStatus::reset() {
	m_status = OK;
	m_msg = "";
}


#endif
