// --------------------------------------------------------------------
// LTA Packer for ABC style models.
// Convert a read in LTA file to the memory format of lt-models.
// ltaModel.cpp
// lithtech.(c) 2000
//
// talon: brought back to 2001's (ModelEdit.exe, Talon, 2001-12-19, the
// module at 0x40bc0a..0x41215b): read through the parse-node reader
// (ltaStream, ltalex, ltaParseNode) rather than Build 69's ltamgr, into the
// 2001 model (one texture per piece, no LOD materials, render styles, OBBs,
// vertex animation or tools info). The functions are in the image's order.
//

// get rid of pesky stl warnings
#pragma warning (disable:4786)

#include "precompile.h"

// LT
#include "model.h"

// lta
#include "ltaModel.h"
#include "ltaStream.h"
#include "model_cleanup.h"

// packer
#include "newgenlod.h"

// c++
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <list>
#include <string>
#pragma warning (disable:4786)

using namespace std;


// ------------------------------------------------------------------------
// the version of the lt model file for this process.
// ------------------------------------------------------------------------
const uint32 LTA_DEFAULT_ANIM_INTERP_TIME = 200 ; // default interp time.

static bool   s_bGenerateNormalsForModelPlease = false ;
static string s_CurBasePath ; // the current process path.


// smmempnproc.cpp
void SmInitMetaModelFromPN( ILTAStream *strm , MetaModel &metaModel, 
						    CParseNode *pnroot, 
							CLTATranslStatus & status  );

static void ApplyModifiers( CParseNode *pnroot, Model *pModel, bool bChildRefs );
static void ApplyChildModelModifiers( CParseNode *pnroot, Model *pModel );
static void cmd_ApplyDeformers( CParseNode *pnroot, Model *pModel, CLTATranslStatus &status );
static void cmd_BuildLOD( CParseNode *add_lod_cmd, Model *pModel, CLTATranslStatus & status );
static void cmd_ApplyAddSockets( CParseNode *pnroot, Model *pModel, CLTATranslStatus & status );
static void ParseSocketPN( CParseNode *socketPN, Model *pModel, ModelSocket *pSocket );
static void cmd_ApplyAddChildModels( CParseNode *pnroot, Model *pModel, CLTATranslStatus & status );
static void cmd_ApplyAddChildModelsAsReferences( CParseNode *pnroot, Model *pModel, CLTATranslStatus & status );
static void cmd_SetWeightSets( CParseNode * pnroot, Model *pModel, CLTATranslStatus &status );
static void cmd_SetAnimBindings( CParseNode *pnroot, Model *pModel, CLTATranslStatus &status );
static void cmd_SetNodeFlags( CParseNode *pnroot, Model *pModel, CLTATranslStatus &status );
static void cmd_GetCommandString( CParseNode *pnroot, Model *pModel, CLTATranslStatus &status );
static void cmd_SetGlobalRadius( CParseNode *pnroot, Model* pModel, CLTATranslStatus &status );
static void ParseTreeToModel( MetaModel & metaModel, CParseNode *pnroot, Model *pModel, CLTATranslStatus &status );
static void ShapeToPiece( MetaModel &metaModel, Shape *shape, ModelPiece *pPiece, CLTATranslStatus & status );
static void NodeToModelNode( Node* root , ModelNode *mroot , CLTATranslStatus & stat );
static void AnimSetToModelAnim( MetaModel & metaModel, int cur_set,  ModelAnim *pModelAnim );
static void ModelNodeToAnimNode( AnimNode * pParentAnimNode, ModelNode * pModelNode, AnimNode * pAnimNode, CAnimSet * pAnimSet, CLTATranslStatus &status );


// ------------------------------------------------------------------------
// Model = ltaModelLoad( instream , allocation )
// creates a new model from an lta stream (0x40bcba).
// ------------------------------------------------------------------------
Model*  ltaModelLoad( istream & is, LAllocCount & allocCount , bool bChildRefs )
{
	s_bGenerateNormalsForModelPlease = false ;

	CParseNode *pnroot    = new CParseNode;
	MetaModel  *metaModel = new MetaModel;
	CLTATranslStatus status ;	
	Model *pRetModel = NULL ;

	ILTAStream *strm = LTAStreamFrom_istream( is );
	if( strm == NULL )
	{
		cerr << " error opening file " << endl;
	}
	else
	{
		CLTATranslStatus::AddToLog("loading lta file\r\n");

		Model *pModel = new Model( &allocCount ) ;
	
		if( pModel != NULL ) 
		{
			// Create The meta model from the parse tree
			SmInitMetaModelFromPN( strm, *metaModel, pnroot, status );

			// converter the meta model into the lt internal format.
			ParseTreeToModel( *metaModel, pnroot, pModel , status );

			// get the on load commands from parse tree.
			CParseNode *on_load_cmds = FindList( pnroot, "on-load-cmds");

			// apply modifiers.
			ApplyModifiers( on_load_cmds, pModel, bChildRefs );

			ChildInfo *pChildInfo = pModel->GetSelfChildModel();

			pModel->SetupChildModelRelation( pChildInfo, pModel, FALSE );

			pModel->ParseCommandString();
			pModel->SetNodeParentOffsets();

			if( status != status.OK  )
			{
				// talon: retail bug: the model, the parse tree, the meta
				// model and the stream all leak.
				status.OnError(" error Reading in LTA file " );
			}
			else
			{
				// talon: 2001 builds the normals whatever the file has.
				CLTATranslStatus::AddToLog("generating normals.\r\n");
				gn_BuildModelVertexNormals( pModel );

				CLTATranslStatus::AddToLog( "--- done --- \r\n" );

				delete metaModel;
				delete strm;
				delete pnroot;

				pRetModel = pModel;
			}
		}
	}

	return pRetModel;
}

// ------------------------------------------------------------------------
// apply modifiers (0x40c3b4)
// apply stuff from the on_load_cmds node
// ------------------------------------------------------------------------
static
void ApplyModifiers( CParseNode *pnroot, Model *pModel, bool bChildRefs )
{
	CLTATranslStatus status ;

	cmd_ApplyDeformers( pnroot, pModel, status) ;
	CLTATranslStatus::AddToLog( "Applying Deformers\r\n");
	if( status != status.OK )
	{
		status.OnError("ApplyDeformers Failed");
		return ;
	}

	status.reset();
	CLTATranslStatus::AddToLog( "Building LOD\r\n");

	// if there is the old style build lod command, process it here.
	cmd_BuildLOD( pnroot, pModel, status );

	if( status!= status.OK ) 
	{
		status.OnError("BuildLOD Failed");
		//return ;
	}
	
	status.reset();
	CLTATranslStatus::AddToLog( "Apply Sockets\r\n");
	cmd_ApplyAddSockets( pnroot, pModel, status );

	if( status != status.OK )
	{
		status.OnError("ApplyModifiers Failed");
		//return ;
	}

	status.reset();
	CLTATranslStatus::AddToLog( "ChildModels:\r\n------------------------\r\n");

	// if we are loading in the child models as references do so, otherwise
	// really load them up.
	if( bChildRefs )
		cmd_ApplyAddChildModelsAsReferences( pnroot, pModel, status );
	else
		cmd_ApplyAddChildModels( pnroot, pModel, status );
	
	if( status != status.OK )
	{
		status.OnError("Failed To Load ChildModels");
		// This is really ok, so we failed to load childmodels...
		// they probably weren't there.
		//return ;
	}

	CLTATranslStatus::AddToLog( "\r\n------------------------\r\n");

	status.reset();
	CLTATranslStatus::AddToLog( "Setting AnimBindings\r\n");
	cmd_SetAnimBindings( pnroot, pModel, status  );

	if( status != status.OK )
	{
		status.OnError( "ApplyModifiers Failed");
		//return ;
	}

	status.reset();
	CLTATranslStatus::AddToLog( "Setting Weights Sets\r\n");
	cmd_SetWeightSets( pnroot, pModel, status );
	 
	if( status != status.OK )
	{
		status.OnError( "cmd_SetWeightSets Failed");
		//return ;
	}

	// command string
	status.reset();
	CLTATranslStatus::AddToLog( "Setting Command String.\r\n" );

	cmd_GetCommandString( pnroot, pModel, status );

	if( status != status.OK )
	{
		status.OnError( "cmd_GetCommandString Failed");
		return ;
	}

	status.reset();
	CLTATranslStatus::AddToLog( "Setting Model Node Flags.\r\n" );

	cmd_SetNodeFlags( pnroot, pModel, status );

	if( status != status.OK )
	{
		status.OnError( "cmd_SetNodeFlags Failed");
		return ;
	}

	cmd_SetGlobalRadius( pnroot, pModel, status );

	if( status != status.OK )
	{
		status.OnError("cmd SetGlobalRadius Failed");
		return ;
	}
}

// ------------------------------------------------------------------------
// container for keeping track of skeletal deformation info comming in from lta.
// ------------------------------------------------------------------------
struct Deformer {
	
	// weight item = index into influence and weight of vertex 
	typedef std::pair< int, float > weight_item_t ;

	typedef std::vector< weight_item_t > weightset_t ;
	
	std::vector< std::string > influences ;
	// a list of lists of weight pairs
	std::vector< weightset_t > weightsets ;

	std::string target_name  ;

};

static void CreateDeformersFromPN( std::vector< Deformer* > &deformers, 
						    std::vector< CParseNode *> &add_deformers_cmds,							
							int & numWeights );
static void LocalizeVertex ( Model *pModel, int cur_piece_index, int node_index, 
					 int Vertex,  float Weight, float *result_vec );

// ------------------------------------------------------------------------
// cmd_ApplyDeformers( parse-node-root, target-model ) (0x40c860)
// apply the lta deformers onto the current lt-model
// ------------------------------------------------------------------------
static
void cmd_ApplyDeformers( CParseNode *pnroot, 
						 Model *pModel, 
						 CLTATranslStatus &status  )
{	
	std::vector< CParseNode * > add_deformer_cmds ;
	std::map< string, uint32, less<string> > NodeNameToIndexMap ;

	FindAll( add_deformer_cmds , pnroot, "add-deformer");

	if( add_deformer_cmds.size() )
	{
		std::vector< Deformer *> deformers ;
		int numWeights=0;
		int weight_vert_cnt = 0;
		
		CreateDeformersFromPN( deformers, add_deformer_cmds , numWeights);
			
		// generate name->index map from nodes 
		for( uint32 nNode = 0 ; nNode < pModel->m_FlatNodeList.GetSize() ; nNode++ )
		{
			char *pctmp = pModel->GetNode( nNode )->GetName() ;
			NodeNameToIndexMap[ pctmp ] = nNode ;
		}
		
		// allocate on numWeights for the model
		if(!pModel->m_VertexWeights.SetSize2(numWeights, pModel->GetAlloc()))
		{
			// error out of mem, crash/burn
			status = status.OUT_OF_MEM ;
			status.m_msg = "out of mem allocating weights ... " ;
			return ;
		}

		// iterate through deformers setting the pieces up
		for( uint32 DefCnt = 0; DefCnt < deformers.size() ; DefCnt ++ )
		{
			uint32 cur_piece_index ;
			// find the piece 
			ModelVert *cur_vert ;
			ModelPiece *cur_piece = pModel->FindPiece( deformers[DefCnt]->target_name.c_str(), &cur_piece_index );

			if(cur_piece == NULL )
				continue;

			Deformer * cur_def = deformers[DefCnt];

			for( uint32 VertCnt = 0 ; VertCnt < cur_piece->m_Verts.GetSize() ; VertCnt ++ ) 
			{
				cur_vert = &cur_piece->m_Verts[ VertCnt ];

				cur_vert->m_nWeights = cur_def->weightsets[ VertCnt ].size();

				if(cur_vert->m_nWeights > 0)
				{
					cur_vert->m_Weights = &pModel->m_VertexWeights[ weight_vert_cnt ];
					weight_vert_cnt += cur_vert->m_nWeights;

					for( int j=0; j < cur_vert->m_nWeights; j++)
					{	
						float weight ;
						// find the node index from the map 
						uint32 wt_idx = cur_def->weightsets[ VertCnt ][ j ].first ;
						uint32 node_index = NodeNameToIndexMap[ cur_def->influences[ wt_idx ].c_str() ];

						cur_vert->m_Weights[j].m_iNode = node_index;

						weight = cur_def->weightsets[ VertCnt ][ j ].second ;
						// localize the vertex, lt expects this.
						LocalizeVertex( pModel, cur_piece_index, cur_vert->m_Weights[j].m_iNode,
										VertCnt, weight,   cur_vert->m_Weights[j].m_Vec  );
					}
				}
			}
		}

		// Free Deformers here.
		{ 
			int num_defs = deformers.size() ;
			for( int i = 0 ;i < num_defs ; i++ )
			{
				delete deformers[i] ;
			}
			// let scope fall out deal with the rest 
		}
	}
}

// ------------------------------------------------------------------------
static 
void CreateDeformersFromPN( std::vector< Deformer* > &deformers, 
						    std::vector< CParseNode *> &add_deformers_cmds,							
							int & numWeights )
{

	numWeights = 0;
	CParseNode *cur_pn ;
	int WtCnt   =0;
	CLTATranslStatus errors ;

	for( uint32 i = 0 ;i < add_deformers_cmds.size() ; i++ )
	{
		cur_pn = add_deformers_cmds[i] ; // get the lsit (add-deformer ... 

		
		if( cur_pn->getNumElements() > 1 )  // we have something in there ... 
		{
			// get the skel-deformer node 
			cur_pn = cur_pn->getElement( 1 ) ;
			CParseNode *target, *influences, *weightsets ;


			target =     ShallowFindList( cur_pn, "target");
			influences = ShallowFindList( cur_pn, "influences");
			weightsets = ShallowFindList( cur_pn, "weightsets" ); // (ws ( ( i w i w  ) ... )

			if( target == NULL )
			{
				errors << " found deformer a null target! syntax error in file ??" ;
				errors.OnError(" CrateDeformersFromPN " , 0 );
				continue ;
			}

			// get the current ;
			deformers.push_back( new Deformer() );
			Deformer *curDeformer = deformers.back();

			// set target name
			curDeformer->target_name = target->getElement(1)->getValue();
			
	
			influences = influences->getElement(1) ;// pop

			curDeformer->influences.reserve( influences->getNumElements() ) ;
			for( int iInfls = 0 ; iInfls < (int)influences->getNumElements() ; iInfls ++ )
			{
				curDeformer->influences.push_back( influences->getElement(iInfls)->getValue());		
			}
		
			// pop (weightsets 
			weightsets = weightsets->getElement(1) ;
			// alloc weight sets array
			curDeformer->weightsets.reserve( weightsets->getNumElements() );
			CParseNode *weights ;
			int nVerts = weightsets->getNumElements() ;

			// for every 
			for( int iWS = 0 ; iWS < nVerts ; iWS ++ )
			{
				// weights = ( i w i w i w )
				weights = weightsets->getElement( iWS );
				int ws_size = weights->getNumElements() /2 ; // remember list of  i w sets
				
				WtCnt += ws_size ;
	
				// add a new vector of weight pairs
			
				curDeformer->weightsets.push_back( Deformer::weightset_t(ws_size) );
				
				float wt_sum = 0 ;
				int   weight_list_size = weights->getNumElements() ;

				// iNumweights index into the weights index for the vertex 
				for( int iWtElem = 0, cnt=0 ; iWtElem < weight_list_size ; iWtElem+=2 , cnt++)
				{
					numWeights++ ;
					int   NodeIndex = atoi( weights->getElement( iWtElem ) ->getValue() ) ;
					float NodeWeight= (float)atof( weights->getElement( iWtElem+1) ->getValue() ) ;
					wt_sum += NodeWeight ;
	
					Deformer::weight_item_t WeightItem( NodeIndex, NodeWeight );

					curDeformer->weightsets[iWS][cnt] =  WeightItem ;
				}
			} 
		}
	}
}

// ------------------------------------------------------------------------
// create localized vertex from global (0x40cf2f).
// ------------------------------------------------------------------------
static
void LocalizeVertex ( Model *pModel, int cur_piece_index, int node_index, 
					 int Vertex,  float Weight, float *result_vec )
{
	ModelPiece *pModelPiece = pModel->GetPiece(cur_piece_index);

	LTMatrix mat = pModel->m_FlatNodeList[ node_index ]->GetInvGlobalTransform();

	LTVector vec =  pModelPiece->m_Verts[ Vertex ].m_Vec;
	LTVector new_vec ;

	mat.Apply( vec, new_vec ); 

	result_vec[0] = new_vec.x  * Weight ;
	result_vec[1] = new_vec.y  * Weight ;
	result_vec[2] = new_vec.z  * Weight ;
	result_vec[3] = Weight ;

}

// types used in cmd_BuildLOD 
struct distTriPair        { float triPercent ; float dist; };
struct pieceNamePriorPair { const char* nameRef; int priority ; };

// ------------------------------------------------------------------------
static
void cmd_BuildLOD( CParseNode *add_lod_cmd, Model *pModel, CLTATranslStatus & status  ) 
{
	BuildLODRequest request;
	LODRequestInfo info;
	int  i;
	CParseNode *lod_cmd ;
	std::vector< pieceNamePriorPair > piece_weights;
	std::vector< distTriPair  > dist_tri_prcnt_lst ;
	float NumTris ;

	if (!pModel)
		return;

	// find the lod command in the parse tree
	lod_cmd = FindList( add_lod_cmd ,"set-repl-lod-original");

	// quit if it ain't there.
	if( lod_cmd == NULL )
		return ;

	// get the number of triangles the model has now.
	NumTris = (float)pModel->CalcNumTris();
	
	// get peice-weights from node
	CParseNode *piece_weightsPN = FindList( lod_cmd, "piece-priorities");

	if( piece_weightsPN == NULL )
	{
		// bad error incomplete node 
		return ;
	}

	piece_weightsPN = piece_weightsPN->getElement(1) ;// (cdr (piece-weights ( ... ))
	
	// piece_weights = ( ( name value ) ( name value ) ... )
	for( i = 0 ;i < (int)piece_weightsPN->getNumElements() ; i++ )
	{
		pieceNamePriorPair item ;
		CParseNode *name_val_pair = piece_weightsPN->getElement(i);

		item.nameRef   = name_val_pair->getElement(0)->getValue () ;
		item.priority= atoi( name_val_pair->getElement(1)->getValue() );

		piece_weights.push_back( item );
	}
	
	
	CParseNode *tri_prcnt = FindList( lod_cmd, "tri-%"); // 
	CParseNode *dist_lst  = FindList( lod_cmd, "dists");

	if( tri_prcnt == NULL || dist_lst == NULL )
	{
		status = status.FAIL ;
		status.m_msg = "set-repl-lod-original nodes tri- or dists are missing, incomplete node";
		// bad error incomplete node ...
		return ;
	}
	
	tri_prcnt = tri_prcnt->getElement(1) ; // cdr on (tri-% ( ... ) )
	dist_lst  = dist_lst ->getElement(1) ; // cdr on (dists ( ... ) )

	for( i = 0 ; i < (int)tri_prcnt->getNumElements() ; i++ )
	{
		distTriPair item ;

		item.triPercent = (float)atof( tri_prcnt->getElement(i)->getValue ()  ) ;
		item.dist       = (float)atof( dist_lst ->getElement(i)->getValue ()  ) ;
		dist_tri_prcnt_lst.push_back( item ) ;
	}

	// we have nothing to lod, let's move on.
	if( tri_prcnt->getNumElements() == 0 )
		return ;

	// ------------------------------------------------------------------------
	// build up the request from the pn data 
	// ------------------------------------------------------------------------
	
	request.m_pModel = pModel;


	// Init the piece weights.
	request.m_PieceWeights.SetSizeInit2(pModel->NumPieces(), 1.0f);

	// for now we are assuming that the model pieces are in the same order
	// as the exporters... Normally we should search the names.
	for( i = 0 ;(uint32)i < pModel->NumPieces() ; i++ )
	{
		request.m_PieceWeights[i]= (float)piece_weights[i].priority ;
	}
	

	// create the lod infos so we can build those levels.
	for( i = 0 ;(uint32)i < dist_tri_prcnt_lst.size() ; i++ )
	{
		info.m_Dist = dist_tri_prcnt_lst[i].dist ;
		info.m_nTris= (uint)(dist_tri_prcnt_lst[i].triPercent  * NumTris) ; 
		request.m_LODInfos.Append(info);
	}
		
	if (!BuildLODs(&request))
	{
		status.m_msg = "cmd_BuildLOD Error: BuildLODs failed " ;
		status       = status.FAIL;
		status.OnError("cmd_BuildLOD");
		return ;
	}
	
	// talon: Build 69 copies each piece's textures to its LODs here; 2001
	// has one texture per piece, and only the count of LODs is left
	// (0x40d25d).
	uint32 num_lods = dist_tri_prcnt_lst.size();
}

// ------------------------------------------------------------------------
static 
void cmd_ApplyAddSockets( CParseNode *pnroot, Model *pModel, CLTATranslStatus & status )
{
	CParseNode *cmd ;
	std::vector< CParseNode *> add_sock_cmds ;
	
	cmd = FindList( pnroot, "add-sockets");

	if( cmd == NULL )
		return ; // no add-sockets commands .

	// talon: 2001 searches the whole of pnroot for socket lists, not the
	// add-sockets list (0x40d37b).
	FindAll( add_sock_cmds, pnroot, "socket" );

	int numSocks = add_sock_cmds.size() ;

	// no socks no go
	if( numSocks == 0 )
		return ;

	// allocate model's sockets 
	if(!pModel->m_Sockets.SetSizeInit4(numSocks, LTNULL, pModel->GetAlloc()))
	{
		status = status.OUT_OF_MEM ;
		status.m_msg = "if(!pModel->m_Sockets.SetSizeInit4(numSocks, LTNULL, pModel->GetAlloc())) FAILED";
		return ;
	}
	
	// go through the add-socket commands; build the sockets
	for( uint32 i = 0 ;i < add_sock_cmds.size() ; i++ )
	{
		CParseNode *add_sock = add_sock_cmds[i] ; 
		ModelSocket *pSocket = LNew(pModel->GetAlloc(), ModelSocket);
		
		if(!pSocket)
		{
			status = status.OUT_OF_MEM ;
			status.m_msg = " LNew on ModelSocket FAILED " ;
			return ;
		}

		ParseSocketPN( add_sock, pModel, pSocket );
		pModel->m_Sockets[i] = pSocket;
	}
}

//
//		 ==> ModelSocket
// ------------------------------------------------------------------------
static 
void ParseSocketPN( CParseNode *socketPN, Model *pModel, ModelSocket *pSocket )
{
	CParseNode *parent ;
	const char *name = socketPN->getElement(1)->getValue() ;
	const char *parent_name ;

	if( name != NULL )
	{
		// set the name 
		strcpy( pSocket->m_Name , name );

		// get the parent node 
		parent = ShallowFindList( socketPN, "parent");
		parent_name = parent->getElement(1)->getValue() ; // ( parent "name " )

		pModel->FindNode( parent_name , &pSocket->m_iNode );
	//	pSocket->m_iNode = node_index ;

		// get the pos / quat 
		CParseNode *ParseNode = ShallowFindList( socketPN, "pos");
		LTA::ParseVector( ParseNode->getElement(1), &(pSocket->m_Pos.x) );

		ParseNode = ShallowFindList( socketPN, "quat") ;
		LTA::ParseVector( ParseNode->getElement(1), pSocket->m_Rot.m_Quat );
		

	}else 
	{
		// error socket requires a name 
		CLTATranslStatus error ;
		error << " error socket requires a name " ; 
		error.OnError("parse socket nodes" ,0);
	}
	
}

// ------------------------------------------------------------------------
// child model info structure used in loading child models
// ------------------------------------------------------------------------
struct ChildModel {

	ChildModel():file_name(NULL), scale_skel(0) {}
	~ChildModel() {
		// talon: retail: strdup'd, given back with delete (0x40d9d0)
		if( file_name != NULL ) delete file_name ;
	}

	char * file_name ;
	int save_index ;
	int scale_skel;
};

static void ChildModelFromPN( ChildModel & cm, CParseNode *pn_cm );

// ------------------------------------------------------------------------
// apply the add-child-models cmd (0x40d53e)
// ------------------------------------------------------------------------
static
void cmd_ApplyAddChildModels(	CParseNode *pnroot, Model *pModel, 
								CLTATranslStatus & status )
{
	CParseNode *add_childmodels ;
	int			list_size ;
	int			i ;
	char		errString[1024];

	ChildInfo *pChildInfo = pModel->GetSelfChildModel();

	add_childmodels = FindList( pnroot, "add-childmodels");

	// no childmodels? ok move along.. 
	if( add_childmodels == NULL )
	{
		return ;
	}
		
	add_childmodels = add_childmodels->getElement(1) ; // cdr on (add-childmodels 
	
	list_size = add_childmodels->getNumElements() ;
	
	pModel->InitChildInfo( 0, pChildInfo, pModel, "SELF");

	for(  i = 0 ;i < list_size ; i++ )
	{
		string tmp_filename;
		ChildModel cm;

		ChildModelFromPN( cm, add_childmodels->getElement(i));

		string filename (cm.file_name) ;
		int save_index       = cm.save_index ;
		int scale_skel  = cm.scale_skel ;

		CLTATranslStatus::AddToLog( "\tLoading Child Model :" );
		CLTATranslStatus::AddToLog( filename );
		CLTATranslStatus::AddToLog( "\r\n" );

		filename += ".lta";

		if( s_CurBasePath != "" )
		{
			tmp_filename = s_CurBasePath ;
			tmp_filename += filename ;
		}
		else
			tmp_filename = filename ;

		Model *child_model = ltaLoadChildModel( tmp_filename.c_str(), pModel->GetAlloc() );
		
		if( child_model == NULL )
		{
			CLTATranslStatus::AddToLog( "Error: ");
			CLTATranslStatus::AddToLog( filename.c_str());
			CLTATranslStatus::AddToLog( " ChildModel Load Failed...\r\n" );
		}
		else
		{
			child_model->SetSaveIndex( save_index );
			
			// add the child model to 
			if( !pModel->AddChildModel( child_model, (char*)filename.c_str(), 
										errString, scale_skel ) )
			{
				CLTATranslStatus::AddToLog( "Error : " );
				CLTATranslStatus::AddToLog( errString );
				CLTATranslStatus::AddToLog( "\r\n" );
			}
		}
	} // for create new ChildInfos
}


// ------------------------------------------------------------------------
// 
// ------------------------------------------------------------------------
static
void ChildModelFromPN( ChildModel & cm, CParseNode *pn_cm )
{
	CParseNode *pn_list ;
	pn_list = ShallowFindList( pn_cm, "filename");
			
	if( pn_list == NULL )
	{
		CLTATranslStatus::AddToLog("Warning: child model node missing filename ... skipping\r\n");
		return ;
	}
				
	cm.file_name = strdup( pn_list->getElement(1)->getValue() );

	pn_list = ShallowFindList( pn_cm,"save-index");
	if(pn_list)
		cm.save_index = atoi( pn_list->getElement(1)->getValue());

	pn_list = ShallowFindList( pn_cm,"scale-skel");
	if( pn_list )
		cm.scale_skel = 1;

}



// ------------------------------------------------------------------------
// Apply addchildmodels as references
// add the child models to the model db as references not as real child model data.
// Normally this would break anything trying to get animation information out of the 
// parent model db, but we are only going to load up the model to save it out again, and
// we don't need the child info data.
// ------------------------------------------------------------------------
static
void cmd_ApplyAddChildModelsAsReferences(	
								CParseNode *pnroot, Model *pModel, 
								CLTATranslStatus & status )
{
	CParseNode *add_childmodels ;
	int			list_size ;
	int			i ;
	char		errString[1024];

	ChildInfo *pChildInfo = pModel->GetSelfChildModel();

	add_childmodels = FindList( pnroot, "add-childmodels");

	// no childmodels? ok move along.. 
	if( add_childmodels == NULL )
	{
		return ;
	}
		
	add_childmodels = add_childmodels->getElement(1) ; // cdr on (add-childmodels 
	
	list_size = add_childmodels->getNumElements() ;
	
	pModel->InitChildInfo( 0, pChildInfo, pModel, "SELF");

	for(  i = 0 ;i < list_size ; i++ )
	{
		string tmp_filename ;
		ChildModel cm ;

		ChildModelFromPN( cm, add_childmodels->getElement(i));

		string filename (cm.file_name) ;
		int save_index  = cm.save_index ;
		int scale_skel  = cm.scale_skel ;

		CLTATranslStatus::AddToLog( "\tAdding Child Model Reference:" );
		CLTATranslStatus::AddToLog( filename );
		CLTATranslStatus::AddToLog( "\r\n" );

		// talon: 2001 only logs the reference (0x40dac3): Build 69's
		// AddChildModelRef is not called.
		
	} // for create new ChildInfos
}


// ------------------------------------------------------------------------
// set the weight set from the lta to the model
// read in :
// (anim-weightsets (
//   (anim-weightset (name "name" ) (weights ( a b c ... ))
//   .. 
// ))	
// ------------------------------------------------------------------------
static
void cmd_SetWeightSets( CParseNode * pnroot, Model *pModel, CLTATranslStatus &status )
{

	CParseNode *anim_ws = FindList(pnroot, "anim-weightsets");
	
	// if anim-weightsets does not exists check for add-animweightsets ... 
	// if that's not there either move on.
	if( anim_ws == NULL)
	{
		anim_ws = FindList( pnroot, "add-animweightsets" );
		if( anim_ws == NULL )
			return ;
	}
	

	anim_ws = anim_ws->getElement(1) ; // cdr on list 

	 
//	pModel->m_WeightSets.SetSize2(anim_ws->getNumElements(), pModel->GetAlloc() );

	for( int iWsCnt = 0 ; iWsCnt < (int)anim_ws->getNumElements() ; iWsCnt++ )
	{
		const char *name ;
		CParseNode *wspn ;
		CParseNode *pn ;

		wspn = anim_ws->getElement( iWsCnt );

		pn = FindList(wspn, "name");
		if( pn )
			name = pn->getElement(1)->getValue() ;
		else 
			name = NULL ;

		pn = FindList( wspn, "weights" );

		if( pn ) 
			pn = pn ->getElement(1) ; // cdr the weights lst

		// talon: retail bug: a weight set without a (weights ...) list
		// dereferences NULL below; one without a name is named NULL.
		WeightSet* pWtSet = new WeightSet( pModel ) ;

		pWtSet->SetName( name );
		int nWeights = pn->getNumElements();

		pWtSet->m_Weights.SetSize2(nWeights, pModel->GetAlloc());


		for( int iwsvals = 0 ; iwsvals < (int)pn->getNumElements() ; iwsvals++ )
		{
			float val = (float)atof( pn->getElement(iwsvals)->getValue() );
			pWtSet->m_Weights[ iwsvals ] = val ;
		}
		
		pModel->AddWeightSet( pWtSet );
	}

}



// ------------------------------------------------------------------------
// for every anim-binding in the lta set the model- anim data to it
// (anim-bindings  (
//    (anim-binding (name "name" ) (dims ( x y z ) ) (translation ( x y z )) (interp-time x))
// )
// ------------------------------------------------------------------------
static
void cmd_SetAnimBindings( CParseNode *pnroot, Model *pModel, CLTATranslStatus &status )
{
	CParseNode *anim_bindings = FindList(pnroot, "anim-bindings" );
	
	if( anim_bindings )
	{
		anim_bindings = anim_bindings->getElement(1) ; // cdr the node

		if( anim_bindings )
		{
			for( int iCurBind = 0 ; iCurBind < (int)anim_bindings->getNumElements() ; iCurBind++ )
			{
				const char	*model_anim_name  = NULL ;
				const char  *weight_set_name  = NULL ;

				LTVector dim( 0.0f , 0.0f , 0.0f);
				LTVector trans( 0.0f , 0.0f , 0.0f);
				uint32   interp_time = LTA_DEFAULT_ANIM_INTERP_TIME  ; 

				CParseNode *abPN = anim_bindings->getElement(iCurBind);
				CParseNode *tmp ;
				
				// slurp up data 
				// get (name "name")
				model_anim_name = NULL ;
				tmp = ShallowFindList( abPN, "name"); 
				if( tmp  && tmp->getElement(1)->isString() )
				{
					model_anim_name = tmp->getElement(1)->getValue();
				}
				else {
					continue ; // why bother, bad name 
				}

				// get (dims ( x y z ))
				tmp = ShallowFindList( abPN, "dims" ); 
				if( tmp )
				{
					tmp = tmp->getElement(1);
					LTA::ParseVector( tmp, &dim.x );				
				}

				tmp = ShallowFindList( abPN, "translation" );
				if( tmp )
				{
					tmp = tmp->getElement(1);
					LTA::ParseVector( tmp, &trans.x );				
				}

				tmp = ShallowFindList( abPN, "interp-time" );
				if( tmp )
				{
					interp_time = atoi( tmp->getElement(1)->getValue() );
				}

				// get associated weight set if there's one
				tmp = ShallowFindList( abPN, "weight-set");
				if( tmp )
				{
					weight_set_name = tmp->getElement(1)->getValue() ;
				}


				//- spit 
				uint32 index ;
				AnimInfo *anim_info = pModel->FindAnimInfo( (char*)model_anim_name, pModel, &index );

				if( anim_info == NULL )
				{
					// woops... no model anim
					continue ;
				}

				if( anim_info->m_pAnim )
				{
					anim_info->m_pAnim->m_InterpolationMS = interp_time ;
				}

				anim_info->			m_vDims = dim ;
				anim_info->	m_vTranslation = trans ; 

				if( weight_set_name != NULL ) 
				{
					uint32 index ;
					WeightSet *WtSet = pModel->FindWeightSet( (char*)weight_set_name, &index );
					
					if(WtSet && anim_info->m_pAnim)
					{
						anim_info->m_pAnim->m_ModelEditWeightSet = index ;
					}
				}
				// ok, next one
			}// for each binding

		}// if there's a bindings list
	}// if there's a binding node
}


// ------------------------------------------------------------------------
// : (set-node-flags ( ("node-name" flag) ("node-name" flag ) ))
// ------------------------------------------------------------------------
static
void cmd_SetNodeFlags( CParseNode *pnroot, Model *pModel, CLTATranslStatus &status )
{

	CParseNode *node_flags = FindList(pnroot, "set-node-flags" );
	
	if( node_flags == NULL )
		return ;

	node_flags = node_flags->getElement(1) ;

	if( node_flags->getNumElements() > 0)
	{
		for( int iNode = 0 ;iNode < (int)node_flags->getNumElements() ; iNode++ )
		{
			ModelNode *pNode ;
			CParseNode *n_f_pair = node_flags->getElement( iNode ) ;
			const char *str = n_f_pair->getElement(0)->getValue();
			int value = atoi(n_f_pair->getElement(1)->getValue());

			pNode = pModel->FindNode( str );
			pNode->m_Flags = value ;
		}
	}
}


// ------------------------------------------------------------------------
// parse into model
// : (set-command-string "a string" )
// ------------------------------------------------------------------------
static
void cmd_GetCommandString( CParseNode *pnroot, Model *pModel, CLTATranslStatus &status )
{

	CParseNode *cmd_str = FindList(pnroot, "set-command-string" );
	
	if( cmd_str && cmd_str->getNumElements() > 1)
	{
		pModel->m_CommandString = strdup( cmd_str->getElement(1)->getValue() );
	}
	
}


// ------------------------------------------------------------------------
// (set-global-radius float)
// ------------------------------------------------------------------------
static
void cmd_SetGlobalRadius( CParseNode *pnroot, Model* pModel, CLTATranslStatus &status )
{
	CParseNode *cmd_str = FindList(pnroot, "set-global-radius" );
	
	if( cmd_str && cmd_str->getNumElements() > 1)
	{
		float val = (float)atof(cmd_str->getElement(1)->getValue());

		pModel->m_GlobalRadius = val ;
	}
}

// ------------------------------------------------------------------------
// ParseTreeToModel (0x40e0d9)
// notes :
// 1. convert the parse tree to an lt model 
// The hierarchy node is not a node in itself, its the model. 
// 2. we're doing something funky here, if the hierarchy has more than 
// one child, then the siblings of the first node in the list becomes
// its children. 
// ------------------------------------------------------------------------
static
void ParseTreeToModel(	MetaModel & metaModel, 
						CParseNode *pnroot, 
						Model      *pModel,
						CLTATranslStatus &status )
{
	int i ;
	
	CLTATranslStatus::AddToLog( "Creating Model \r\n" );
	
	// ------------------------------------------------------------------------
	// build the ltmodel node hierarchy
	// ------------------------------------------------------------------------
	// shapes to pieces 
	int nPieces = metaModel.shapes.size();

	//allocate space for pieces
	if(!pModel->m_Pieces.SetSizeInit4(nPieces, LTNULL, pModel->GetAlloc() ) )
	{
		status.m_status = status.FAIL ;
		status.m_msg    = " out of mem ... " ;
		status.OnError("CRITICAL ERROR");
		return ;
	}

	CLTATranslStatus shp2pce_stat ;
	CLTATranslStatus::AddToLog( "Creating Pieces \r\n" );
	// convert each shape into a piece.
	for( i = 0 ; i < nPieces ; i++ )
	{
		ModelPiece *pPiece = LNew_1P(pModel->GetAlloc(), ModelPiece, pModel );
		if( !pPiece )
		{
			status.m_status = status.FAIL ;
			status.m_msg    = " out of mem ... " ;
			status.OnError("CRITICAL ERROR");
			return ;
		}
		pModel->m_Pieces[i] = pPiece ;
		ShapeToPiece(metaModel, metaModel.shapes[i], pPiece ,shp2pce_stat);
		
		if( shp2pce_stat.m_status != CLTATranslStatus::OK )
			shp2pce_stat.OnError("error converting shape to piece " );
	}
	
	// Convert hierarchy nodes from lta in memory format to lt-model 
	CLTATranslStatus nstat ;
	NodeToModelNode( metaModel.rootNode , pModel->GetRootNode(), nstat);

	if( nstat.m_status != nstat.OK )
		nstat.OnError( "->NodeToModelNode error " );

	// required for later processing, sets up flatlists and such.
	if(!pModel->PrecalcNodeLists())
	{	
	}

	// ------------------------------------------------------------------------
	//  Process animation sets into ModelAnims, stick them into new LT model.
	// ------------------------------------------------------------------------
	
	// talon: constructed and destroyed, otherwise unused (-0x80).
	CLTATranslStatus anim_stat ;

	// AnimsToAdd is a collection of model anims to add lt-model container 
	// after we have processed everything. (we may not want all anims loaded etc...)
	vector<ModelAnim*> vAnimsToAdd ;

	// -----------------------------------------------------------------------
	// Get animations from lta, convert to mem format.
	// ------------------------------------------------------------------------
	LTA::CClassifiedAnimLst anPairLst ;
 	
	LTA::CollectAndClassifyAnims(metaModel, anPairLst );
	CLTATranslStatus::AddToLog( "Creating AnimNodes \r\n" );

	// Process through the anim pairs
	for( uint32 iCurAnimPair = 0 ;iCurAnimPair < anPairLst.size() ; iCurAnimPair++ )
	{
		pair<int,int> &AnimPair = anPairLst[iCurAnimPair];

		ModelAnim *pAnim = LNew_1P( pModel->GetAlloc() , ModelAnim, pModel);
		
		// store new model-anims without adding them to lt-model.
		// we want to manipulate them before putting them in lt-model
		vAnimsToAdd.push_back( pAnim );

		pAnim->GetRootNode()->m_pNode = pModel->GetRootNode() ;

		// Convert the lta-animation set to lt-modelanim
		AnimSetToModelAnim( metaModel, AnimPair.first, pAnim );
	}

	// Add the collected 
	// after collecting anims add it to model 
	for( uint32 iAnim = 0 ; iAnim < vAnimsToAdd.size() ; iAnim++ )
	{
		ModelAnim *pAnim = vAnimsToAdd[iAnim] ;
		if(	pModel->AddAnimToSelf(pAnim) == NULL )
		{
			MessageBox(NULL, "BIG FAT ERROR ADDING ANIM", "in ParseTreeToModel",MB_OK);
		}
	}

	// must do this flatten (create index arrays) for anims
	if(!pModel->PrecalcNodeLists())
	{	
	}

	// talon: the loop over the first animation's nodes is still here in
	// 2001 (0x40e45b), with nothing in it: Build 69's vertex-animation
	// hook-up has no 2001 counterpart.
	if( vAnimsToAdd.size() > 0 )
	{
		ModelAnim *pAnimSet = vAnimsToAdd[0] ;

		if( pAnimSet != NULL && pAnimSet->m_AnimNodes != NULL )
		{
			for( uint32 iNode = 0 ; iNode < pModel->NumNodes() ; iNode++ )
			{
			}
		}
	}
}

static void
ShapeToPiece(	MetaModel &metaModel, 
				Shape *shape, ModelPiece *pPiece, 
				CLTATranslStatus & status )
{

	char *tmp = (char*)shape->GetName().c_str(); // use shape's name as piece name
	pPiece->SetName( tmp );
	
	// get tris / get the face set info
	int i ;
	int nTris = shape->GetMesh().tri_FaceSet.size()/3;
	int bHasTCoords = shape->GetMesh().tcoords.size() > 0  ;

	if(!pPiece->m_Tris.SetSize2(nTris, pPiece->GetAlloc()))
	{
		// error outta room
		status.m_status = status.OUT_OF_MEM ;
		return ;

	}

	// collect the u-v texture coords.

	int tcnt = 0;
	std::vector< int > & uv_fs = shape->GetMesh().tri_UVSet ;
	std::vector< LTVector > & tcoords = shape->GetMesh().tcoords ;
	
	for(  i = 0 ; i < nTris ; i++ )
	{
		for( int j  = 0 ; j < 3 ; j++ )
		{
			pPiece->m_Tris[i].m_Indices[j] = shape->GetMesh().tri_FaceSet[tcnt];

			if( bHasTCoords )
			{
				pPiece->m_Tris[i].m_UVs[j].tu = tcoords[ uv_fs[tcnt] ].x ;
				pPiece->m_Tris[i].m_UVs[j].tv = tcoords[ uv_fs[tcnt] ].y ;
			}
			tcnt++ ;
		}
	}

	// get the vecs
	int nVerts = shape->GetMesh().vertex.size() ;
	int nNorms = shape->GetMesh().normals.size(); ;
	bool DoNormsPerVerts = false ;

	if ( nNorms == nVerts )
		DoNormsPerVerts = true ;
	

	if(!pPiece->m_Verts.SetSize2(nVerts, pPiece->GetAlloc()))
	{
		status.m_status = status.OUT_OF_MEM ;
		return ;
	}
	
	// Get VERTEX 

	ModelVert *pVert ;
	for( i = 0 ; i < nVerts ; i++ )
	{
		pVert = &pPiece->m_Verts[i] ;
		
		pVert->m_nWeights = 0;      // mesh deform
		pVert->m_iReplacement = 0 ; // lod

		pVert->m_Vec = shape->GetMesh().vertex[i] ;
	
		
		if( DoNormsPerVerts )
			pVert->m_Normal = shape->GetMesh().normals[i] ;

		/*
		reading weighted deform data is done later.
		*/
	}

	// if we don't have one normal per vertex fudge it here.
	if( !DoNormsPerVerts && nNorms )
	{
		s_bGenerateNormalsForModelPlease = true ;
			
			// A ET! there are more norms than verts, we need to average them
			// this algoritm ain't cutting it, don't have time. let importer recalc norms.
			// which in the abc case if fine.
		vector< int >  &nrm_fs = shape->GetMesh().tri_NormSet ;

		if( nrm_fs.size() > 0)
		{
			ModelVert *pVert ;
			for( i = 0 ; i < nVerts ; i++ )
			{			
				pVert = &pPiece->m_Verts[i] ;
				pVert->m_Normal += shape->GetMesh().normals[ nrm_fs[i ] ] ;

			}
		}
	}

	if( nNorms == 0 )
		s_bGenerateNormalsForModelPlease = true ;

	status.m_status = status.OK ;
}

static void
NodeToModelNode( Node* root , ModelNode *mroot , CLTATranslStatus & stat )
{
	ASSERT(root);
	ASSERT(mroot);

	mroot->m_NodeIndex = root->index ;
	// if the node is named "null" then mark it as removable
	mroot->m_Flags = root->GetName() == "null" ? MNODE_REMOVABLE : 0 ;
	mroot->SetGlobalTransform ( root->mat );
	mroot->SetName( (char*) root->GetName().c_str() );
	
	// allocate space for children of this node
	if( ! (mroot->m_Children.SetSizeInit4(root->children.size(), 
										  LTNULL, 
										   mroot->GetAlloc()) ))
	{
		stat.m_status = stat.OUT_OF_MEM ;
		return ;
	}

	for( uint32 i = 0 ;i < root->children.size() ; i++ )
	{
		ModelNode *pChild ;
		pChild = LNew_1P( mroot->GetAlloc(), ModelNode, mroot->GetModel()) ;
		if( !pChild )
		{
			stat.m_status = stat.OUT_OF_MEM ;
			return ;
		}
		pChild->m_iParentNode  = mroot->m_NodeIndex ;
		mroot->m_Children[i] = pChild ;
		NodeToModelNode( root->children[i], mroot->m_Children[i], stat );

		if( stat.m_status != stat.OK )
		{
			return ;
		}
	}

}



// ------------------------------------------------------------------------
// AnimSetToModelAnim( AnimSet , ModelAnim )
// convert from CAnimSet to ModelAnim (0x40e933)
// ------------------------------------------------------------------------
static 
void AnimSetToModelAnim( MetaModel & metaModel, int cur_set,  ModelAnim *pModelAnim )
{
	CAnimSet *pAnimSet = metaModel.GetAnimSet(cur_set);

	const CKeyFrame &KeyFrame = *pAnimSet->GetKeyFrame() ;

	 //set manim->m_pName ;
	pModelAnim->SetName( pAnimSet->GetName().c_str() );
	// model weight sets ?
	// m_InterpolationMS  // attack time
	// int nKeyFrames = aset->
	int nKeyFrames =KeyFrame.Size();
	int res = pModelAnim->m_KeyFrames.SetSize2(nKeyFrames, pModelAnim->GetAlloc() ) ;
	if( !res )
	{
		CLTATranslStatus( "could not alloc keys ", "AnimSetToModelAnim" );
		return ;
	}
	
	for( int i = 0 ;i < nKeyFrames ; i++ )
	{
		pModelAnim->m_KeyFrames[i].m_Time = KeyFrame.GetTime( i ) ;

		// if there's a string dup it, other wise ... 
		string str = KeyFrame.GetVal( i );
		if( str.size() > 0 )		
			pModelAnim->m_KeyFrames[i].m_pString = strdup(str.c_str());
		else
			pModelAnim->m_KeyFrames[i].m_pString = "";

	}

	AnimNode *pAnimNodeRoot = pModelAnim->GetRootNode();
	
	CLTATranslStatus ModelNodeToAnimNodeConv_status ;
	// create anin data from model data.
	ModelNodeToAnimNode( pAnimNodeRoot, 
						 pModelAnim->GetRootNode()->m_pNode, 
						 pAnimNodeRoot , 
						 pAnimSet,
						 ModelNodeToAnimNodeConv_status );

	if(ModelNodeToAnimNodeConv_status != ModelNodeToAnimNodeConv_status.OK )
	{
		ModelNodeToAnimNodeConv_status.OnError("AnimSetToModelAnim");
		return ; 
	}

	//// now that we've set up the animnode hierarchy....

}

static void
ModelNodeToAnimNode(  AnimNode  * pParentAnimNode,
					  ModelNode * pModelNode, 
					  AnimNode  * pAnimNode, 
					  CAnimSet  * pAnimSet,
					  CLTATranslStatus &status )
{
	int numKF = pAnimNode->GetAnim()->NumKeyFrames();	
	LAlloc* alloc  = pAnimNode->GetAlloc() ;

	if(!pAnimNode->m_KeyFrames.SetSize2( numKF, alloc) )
	{
		status = status.OUT_OF_MEM ;
		status.m_msg = "m_KeyFrames.SetSize2 failed ...  ";

		return ;
	}

	// setup hierarchical linkage
	pAnimNode->m_pNode = pModelNode;
	pAnimNode->m_pParentNode = pParentAnimNode;
	
	// set the kf
	LTMatrix id_mat ;
	id_mat.Identity();

	// note: you should really fill this if  apropriate
	if( pAnimSet != NULL )
	{
		CAnim *pltaAnim  = pAnimSet->FindAnimByTargetName( string(pModelNode->GetName()) );
		
		if( pltaAnim == NULL )
		{
			
		}
		// if we have a vertex animation set stuff the keyframe data
		if( pltaAnim != NULL && pltaAnim->GetType() == CAnim::VERTEX )
		{
			CVtxAnim *pVAnim = (CVtxAnim*)pltaAnim ; // upgrade the pointers
			Shape    *pShape = pVAnim->GetTarget() ;
			uint32    iPiece ;

			// find the associated peice in the model and assign it to the cur anim node
			if(pAnimNode->GetModel()->FindPiece( pShape->GetName().c_str(), &iPiece ) == NULL )
			{
				status = status.FAIL ;
				string msg = " could not find piece for anim node. ";
				msg += pShape->GetMeshName().c_str() ;
				
				status.OnError("ModelNodeToAnimNode");
				return ;
			}

			// talon: 2001's model has no vertex animation; of Build 69's
			// key frame setup only this is left (0x40edb8).
			LAlloc *alloc = pAnimNode->GetAlloc();
		}
		else  if ( pltaAnim != NULL && pltaAnim->GetType() == CAnim::POSQUAT )
		{
			LTVector pos(0,10,0);
			float    quat[4];

			CPosQuatAnim *pPQAnim = (CPosQuatAnim*)pltaAnim ;
			// for now fill all the kfs with first frame data.	
			for( int iframe = 0 ; iframe< numKF ; iframe++ ){

				pPQAnim->Get( iframe, pos, quat );
				//pAnimNode->m_KeyFrames[iframe].m_Translation = pos ;
				//pAnimNode->m_KeyFrames[iframe].m_Quaternion.Init(quat[0],quat[1],quat[2],quat[3]);
				
				pPQAnim->Get( iframe, pAnimNode->m_KeyFrames[iframe].m_Translation ,
									  pAnimNode->m_KeyFrames[iframe].m_Quaternion.m_Quat);					
			}
		} 
		else 
		{
			// either there is no animation for this node or one that's not
			// recognized by this lta reader.
			// we'll just generate pos-quat values for this animation from
			// the original node value.

			CLTATranslStatus status ;
			if( pltaAnim == NULL ){
				status.m_msg = "could not find anim for ";
				status.m_msg += string(pModelNode->GetName()) ;
				status.m_msg += " in AnimSet : " ;
				status.m_msg += string(pAnimSet->GetName() );
			}else { 
				status.m_msg = "invalid animation type serious error";
			}
			status.OnError("in ModelNodeToAnimNode", LT_FALSE);

			LTMatrix &mat = pModelNode->GetGlobalTransform();
			LTVector pos;		
			mat.GetTranslation(pos);
			LTRotation quat;
			quat_ConvertFromMatrix(quat.m_Quat , mat.m );
			
			for( int iframe =0; iframe < numKF ; iframe++ )
			{
				pAnimNode->m_KeyFrames[iframe].m_Translation = pos;
				pAnimNode->m_KeyFrames[iframe].m_Quaternion.Init( quat.m_Quat[0], quat.m_Quat[1], quat.m_Quat[2], quat.m_Quat[3] );
			}
		}
	}
		

	// Load child nodes.
	if(!pAnimNode->m_Children.SetSizeInit4(pModelNode->NumChildren(), LTNULL, alloc))
	{
		status = status.OUT_OF_MEM ;
		status.m_msg = "m_Children.SetSizeInit4 failed ...  ";
		status.OnError("ModelNodeToAnimNode");
		return ;
	}

	for( uint32 i=0; i < pModelNode->NumChildren(); i++)
	{
		AnimNode *pChild = LNew_2P(alloc, AnimNode, pAnimNode->GetAnim(), pAnimNode);
		if(!pChild)
		{
			status = status.OUT_OF_MEM ;
			status.m_msg = " LNew_2P failed Out of memory" ;
			status.OnError("ModelNodeToAnimNode");
			return ;
		}
			
		pChild->m_pNode = pModelNode->m_Children[i];
		ModelNodeToAnimNode(  pAnimNode, pModelNode->m_Children[i], pChild, pAnimSet, status );
		
		if( status != status.OK )
		{
			// should free pChild 
			return ;
		}

		pAnimNode->m_Children[i] = pChild;
	}
}

// ------------------------------------------------------------------------
// ltaLoadChildModel (0x40efd7)
// Child models don't need to be as complete as main models.
// so all we're going to load is the base model and some of the anim bindings.
// returns NULL if some sort of error has occured
// ------------------------------------------------------------------------
Model *ltaLoadChildModel( const char* filename, LAlloc * allocCount  )
{
	s_bGenerateNormalsForModelPlease = false ;

	CParseNode *pnroot    = new CParseNode;
	MetaModel  *metaModel = new MetaModel;
	CLTATranslStatus status ;	

	ifstream InFile( filename );
	
	if( !InFile )
	{
		status.m_msg = "could not open file " ;
		status.m_msg += filename ;
		status.OnError("Opening ChildModel",0);
		delete metaModel ;
		delete pnroot ;
		return NULL ;
	}

	ILTAStream *strm = LTAStreamFrom_istream( InFile );
		
	Model *pModel = new Model( allocCount ) ;
	
	if( pModel == NULL )
	{
		status.m_msg ="Error : Out of memory, failed new in ltaLoadChildModel";
		status.OnError("ltaLoadChildModel",0);
		delete metaModel ;
		delete strm ;
		delete pnroot ;
		return NULL ;
	}
	
	SmInitMetaModelFromPN( strm, *metaModel, pnroot, status );

	ParseTreeToModel( *metaModel, pnroot, pModel , status );

	if( status != status.OK  )
	{
		// talon: retail bug: the model leaks.
		status.OnError(" error Reading in LTA file " );
		
		delete metaModel ;
		delete strm ;
		delete pnroot ;
		InFile.close();
		return NULL;
	}

	// on load commands 
	// important ...
	CParseNode *on_load_cmds = FindList( pnroot, "on-load-cmds");
	ApplyChildModelModifiers( on_load_cmds, pModel );

	ChildInfo *pChildInfo = pModel->GetSelfChildModel();

	pModel->SetupChildModelRelation( pChildInfo, pModel, FALSE );

	pModel->ParseCommandString();
	pModel->SetNodeParentOffsets();

	gn_BuildModelVertexNormals( pModel );

	delete metaModel ;
	delete strm ;
	delete pnroot ;

	return pModel;
}




// ------------------------------------------------------------------------
// apply child model modifiers (0x40f274)
// apply stuff from the on_load_cmds node that's needed. Don't 
// bother with things that are not going to be seen/used.
// ------------------------------------------------------------------------
static 
void ApplyChildModelModifiers( CParseNode *pnroot, Model *pModel )
{
	CLTATranslStatus status ;

	cmd_ApplyDeformers( pnroot, pModel, status) ;
	status.AddToLog( "deformers\r\n");
	if( status != status.OK )
	{
		status.OnError("ApplyModifiers Failed");
		return ;
	}
	
	status.reset();
	CLTATranslStatus::AddToLog( "Apply Sockets\r\n");
	cmd_ApplyAddSockets( pnroot, pModel, status );

	if( status != status.OK )
	{
		status.OnError("ApplyModifiers Failed");
		return ;
	}

	status.reset();
	CLTATranslStatus::AddToLog( "AnimBindings\r\n");
	cmd_SetAnimBindings( pnroot, pModel, status  );

	if( status != status.OK )
	{
		status.OnError( "ApplyModifiers Failed");
		return ;
	}

	status.reset();
	CLTATranslStatus::AddToLog( "Set Weights Sets\r\n");
	cmd_SetWeightSets( pnroot, pModel, status );
	 
	if( status != status.OK )
	{
		status.OnError( "ApplyModifiers Failed");
		return ;
	}


	status.reset();
	CLTATranslStatus::AddToLog( "command string\r\n" );

	cmd_GetCommandString( pnroot, pModel, status );

	if( status != status.OK )
	{
		status.OnError( "ApplyModifiers Failed");
		return ;
	}
}

