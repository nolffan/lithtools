#ifndef __ILTMODEL_H__
#define __ILTMODEL_H__

#ifndef __LTBASEDEFS_H__
#include "ltbasedefs.h"
#endif

#ifndef __LTRENDERSTYLE_H__
#include "ltrenderstyle.h"
#endif

#ifndef __LTMODULE_H__
#include "ltmodule.h"
#endif


#ifndef __COLLISION_OBJECT_H__
#include "physics/collision_object.h"
#endif 


typedef uint32 HMODELDB ; // handle to model db.

/*!
The \b ILTModel interface contains functions for manipulating models and animations.

There are two implemenations of this interface; one for server, one for client.

Define a holder to get this interface like this:
\code
define_holder_to_instance(ILTModel, your_var, Client);
define_holder_to_instance(ILTModel, your_var, Server);
\endcode
*/

class ILTModel : public IBase {
public:
    interface_version(ILTModel, 0);
		

/*!
\param  filename, file to load
\param  out param : hModelDB, model database handle.

  Loads a model with out associating it with a game object.
  returns a handle to the model database.
  
  Used for: Models and Animation.
*/
	virtual LTRESULT CacheModelDB( const char *Filename, HMODELDB & hModelDB ) =0;


/*!
\param hModelDB, handle to model database.
\param 

  Turns this model into a normal model : If there are no refs associated with it,
  it gets released.
  Resets hModelDB to zero.
  return LT_OK on success.
*/
	virtual LTRESULT UncacheModelDB( HMODELDB &hModelDB ) =0;

/*!
\param hModelDB, handle to model database

  Test if hModelDB points to a model database.
*/

	virtual LTRESULT IsModelDBLoaded( HMODELDB hModelDB) =0;

/*!
\param hObj, handle to engine object representing a model instance
\param hModelDB handle to a model database.

  results : 
  LT_ERROR could be returned for several reasons, check the console for an error message.
  LT_OK    all is well, you may go back to sleep.
  LT_NOTFOUND when the call is inappropriate for the context.

  Notes: 
  1. This call only works on the server side.
  2. This call operates on a modelinstance's underlying model database. It is possible to add/remove a child model 
  to any number of modelinstances, as long as the underlying db is the same, no harm is done. 
*/
	
	virtual LTRESULT AddChildModelDB( HOBJECT hObj, HMODELDB hModelDB ) {return LT_NOTFOUND;}

	virtual LTRESULT RemoveChildModelDB( HOBJECT hObj, HMODELDB hModelDB ) {return LT_NOTFOUND;}

/*!
\param h
 */

/*! 
\param hObj Model Reference.
\param pSocketName name of the socket to be fetched.
\param hSocket return parameter of socket handle.
\return \b LT_NOTINITIALIZED if the Model Reference is invalid.
		\b LT_NOTFOUND if socket it not found in the model eference.

Method used to get sockets handles from models by name. 

Used for: Models and Animation.
*/
    virtual LTRESULT GetSocket(HOBJECT hObj,  char *pSocketName, HMODELSOCKET &hSocket);

/*! 
\param hObj     Model instance handle.
\param hSocket  Socket instance handle.
\param transform  Transform
\param bWorldSpace What space the transform is in in relation to parent, true is in world space, false the transform
is local to model space. 
\return \b LT_OK if operation succeeded.
		\b LT_ERROR one of these caused the error :
			1. The transform hierarchy has become invalid, by having a transformation that
			   is not invertable.
			2. the socket is invalid.
		\b LT_NOTINITIALIZED if there is no model associated with hObj.

Fill \b pos and \b rot with the current transform for the socket. If
\b bWorldSpace is \b TRUE, then the transform is returned in world
space. Otherwise, the transform is local to the model (based on its
position and rotation). 

  \note This is potentially an expensive process: the model's animation has to be (partially) 
  revaluated for every call to get GetSocketTransform. If this function is going to be called 
  a lot, consider caching the animation data by enabling the transformation cache. 
 
Used for: Models and Animation.  
*/
	virtual LTRESULT GetSocketTransform(HOBJECT hObj, 
                                        HMODELSOCKET hSocket,
                                        LTransform &transform,
                                        bool bWorldSpace);

/*!
\param hObj		  Model instance reference.
\param NumPieces  return value, representing the number of pieces the model has.
\return \b LT_INVALIDPARAM invalid model instance
		\b LT_OK			operation is done.

	Get the number of pieces a model has.
*/
	virtual LTRESULT GetNumPieces( HOBJECT hObj, uint32 & NumPieces );

/*! 
\param hObj       Model instance reference.
\param pPieceName name of model piece.
\param hPiece     return param handle.
 
\return \b LT_OK is success .
		\b LT_NOTINITIALIZED if there is no model associated with hObj.
		\b LT_NOTFOUND  if the requested piece does not exist.
	

Used for: Models and Animation.
*/
	virtual LTRESULT GetPiece(HOBJECT hObj, 
                              char *pPieceName,
                              HMODELPIECE &hPiece);

/*! 
\param hObj   Model instance handle.
\param hPiece Piece instance handle.
\param bHidden return parameter on hidden status.
 
\return \b LT_OK is success .
		\b LT_INVALIDPARAMS if hObj is not associated with an OT_MODEL
		


Discover whether a piece on the model is hidden.  Only supports the
first \em 32 pieces in the model.  SetPieceHideStatus() returns
\b LT_NOCHANGE if the piece is already hidden (not an error).

\see SetPieceHideStatus()

Used for: Models and Animation.  
*/
	virtual LTRESULT GetPieceHideStatus(HOBJECT hObj, 
                                        HMODELPIECE hPiece,
                                        bool &bHidden);

/*! 
\param hObj  Model instance handle.
\param hPiece  Model piece instance handle.
\param bHidden true/false param to switch the hidden status of piece.
\return \b LT_OK is success .
		\b LT_INVALIDPARAMS if hObj is not associated with an OT_MODEL
		\b LT_NOCHANGE piece already hidden.

Hide or unhide a piece on the model (they are all unhidden by
default). Only supports the first \em 32 pieces in the model.
SetPieceHideStatus() returns \b LT_NOCHANGE if the piece is already
hidden (not an error).

\see GetPieceHideStatus()

Used for: Models and Animation.  
*/
	virtual LTRESULT SetPieceHideStatus(HOBJECT hObj, 
                                        HMODELPIECE hPiece,
                                        bool bHidden);

/*! 
\param hObj    Model instance name.
\param pNodeName name of node in the model's model transformation hierarchy.
\param hNode     return param holding a handle to node.
\return \b LT_OK on success 
		\b LT_INVALIDPARAMS if hObj is not an OT_MODEL
		\b LT_NOTINITIALIZED if hObj is not currently associated with a model.
		\b LT_NOTFOUND       if the requested node is not found.

	
Get a handle on named node.

\see GetNodeTransform
Used for: Models and Animation.
*/
	virtual LTRESULT GetNode(HOBJECT hObj, 
                             char *pNodeName,
                             HMODELNODE &hNode);

	
/*!
\param hObject Model to query.
\param hNode Node to query.
\param pName (return) Buffer to hold name.
\param maxLen Length of buffer.

\return \b LT_INVALIDPARAMS if a parameter is null, else \b LT_OK

Get the name of a model transformation node.

Used for: Models and Animation.
*/
	virtual LTRESULT		GetNodeName(HOBJECT hObj, HMODELNODE hNode,char *name, uint32 maxlen);


/*! 
\param hObj Model instance handle.
\param hNode    Model node handle.
\param transform transform 
\param bWorldSpace 
\return \b LT_OK on success 
		\b LT_INVALIDPARAMS if hObj is not an OT_MODEL
		\b LT_NOTINITIALIZED if hObj is not currently associated with a model.
		\b LT_ERROR if 
			a) The transform hierarchy could not be evaluated for the model
			b) The resultant transformation is invalid.

Get the transformation for a node on a model.  If \b bWorldSpace is \b
TRUE, then the transform is returned in world space. Otherwise, the
transform is local to the model.

\see GetNode

Used for: Models and Animation.  
*/
	virtual LTRESULT GetNodeTransform(HOBJECT hObj,
                                      HMODELNODE hNode,
                                      LTransform &transform,
                                      bool bWorldSpace);


	
/*!
\param hObject Model Instance.
\param hNode Current node. Use \b INVALID_MODEL_NODE to get first node.
\param pNext (return) Next node in list. \b LT_FINISHED means the end of the list.

\return \b LT_FINISHED when there are no more; otherwise, returns \b LT_OK.

Iterate through the nodes of the model. The order of the nodes is as if the 
hierarchy was traversed using pre-order traversal.

\par    Example:

\code
  HMODELNODE hCurNode = INVALID_MODEL_NODE;
  while (interface->GetNextModelNode(hModel, hCurNode, &hCurNode) == LT_OK)
      { ... }
\endcode

Used for: Models and Animation.
*/
	/**/
    virtual LTRESULT GetNextNode(HOBJECT hObject, 
								 HMODELNODE hNode,
								 HMODELNODE &pNext);

/*!
\param hObj ModelInstance handle
\param hNode a handle to the root node.
\return \bLT_INVALIDPARAMS if object is either invalid, or not an OT_MODEL

  GetRootNode returns a handle to the root of the transformation hierarchy for the 
  model. 

  Used for: Models and Animation.
*/

	/**/
	virtual LTRESULT				GetRootNode(HOBJECT hObj, HMODELNODE &hNode);

/*!
\param hObj ModelInstance handle
\param hNode a handle to the node 
\param NumChildren return value of query.
\return \bLT_INVALIDPARAMS if object is either invalid, or not an OT_MODEL

  Returns the number of children this node has. If it is a leaf in the transform hierarchy
  the value will be zero.

Used for: Models and Animation.
*/

	virtual LTRESULT 				GetNumChildren(HOBJECT hObj, HMODELNODE hNode, uint32 &NumChildren );

/*!
\param hObj ModelInstance handle
\param hNode a handle to the parent node 
\param index 
\param NumChildren return value of query.
\return \bLT_INVALIDPARAMS if object is either invalid, or not an OT_MODEL

  GetChild returns a handle to the nth child of parent indicated.

 Used for: Models and Animation.
*/

	virtual LTRESULT 				GetChild(HOBJECT hObj, 
											 HMODELNODE parent, 
											 uint32 index,
											 HMODELNODE &child);

/*!
\param hObj ModelInstance handle
\param hNode a handle to the node 
\param parent a handle to the parent of the node.
\return \bLT_INVALIDPARAMS if object is either invalid, or not an OT_MODEL

	Given a node, return a handle to its parent. The value INVALID_MODEL_NODE will
	be returned if the node is the root node.

 Used for: Models and Animation.
*/

	virtual LTRESULT				GetParent(HOBJECT hObj, HMODELNODE node, HMODELNODE &parent);

/*!
\param hObj ModelInstance handle
\param NumNodes return value.
\return \bLT_INVALIDPARAMS if object is either invalid, or not an OT_MODEL

	Returns the number of transformation nodes the model has.

 Used for: Models and Animation.
*/

	virtual LTRESULT				GetNumNodes( HOBJECT hObj, uint32 &num_nodes);

/*!
\param  hObj        Model to process.
\param  hNode		Node to process.
\param  fn          Function to call.
\param  pUserData   User data.
\return 
	\b LT_INVALIDPARAMS     if hObj is either NULL, or not of type \b OT_MODEL
	\b LT_OK                otherwise

  Set callback that gets the world position of the animation node just after it has
  been evaluated.

\note Your function should return as quickly as possible -- it will be
called often. The NodeControlFn will be called for each node on the
model, allowing you to translate and rotate it. 

\par To disable the NodeControlFn, use the RemoveNodeControlFn functions

Used for: Models and Animation.  
*/
	virtual LTRESULT	AddNodeControlFn( HOBJECT hObj, HMODELNODE hNode, NodeControlFn pFn, void *pUserData);

/*!
\param  hObj        Model to process.
\param  fn          Function to call.
\param  pUserData   User data.
\return 
	\b LT_INVALIDPARAMS     if hObj is either NULL, or not of type \b OT_MODEL
	\b LT_OK                otherwise

  Set a callback node processing for every node in a model. See above AddNodeControlFn
  function for details.

Used for: Models and Animation.  
*/
	virtual LTRESULT	AddNodeControlFn( HOBJECT hObj, NodeControlFn pFn, void *pUserData);

/*!
\param  hObj        Model to have function removed from.
\param  hNode		Node to have function removed from.
\param  fn          Function to remove.
\param  pUserData   User data of function to remove (or NULL for any).
\return 
	\b LT_INVALIDPARAMS     if hObj is either NULL, or not of type \b OT_MODEL
	\b LT_OK                otherwise

  Given a node it will look through its registered callbacks, and upon finding any 
  that matches the specified criteria, it will remove it from the list of callback
  functions.

\note If the pUserData value is NULL, the user data of the callbacks will not be
  checked for matching criteria, but if it is non-null it will be checked.

Used for: Models and Animation.  
*/
	virtual LTRESULT	RemoveNodeControlFn( HOBJECT hObj, HMODELNODE hNode, NodeControlFn pFn, void* pUserData);

/*!
\param  hObj        Model to have function removed from.
\param  fn          Function to remove.
\param  pUserData   User data of function to remove (or NULL for any).
\return 
	\b LT_INVALIDPARAMS     if hObj is either NULL, or not of type \b OT_MODEL
	\b LT_OK                otherwise

  Given a model it will look through its registered callbacks, and upon finding any 
  that matches the specified criteria, it will remove it from the list of callback
  functions.

\note If the pUserData value is NULL, the user data of the callbacks will not be
  checked for matching criteria, but if it is non-null it will be checked.

Used for: Models and Animation.  
*/
	virtual LTRESULT	RemoveNodeControlFn( HOBJECT hObj, NodeControlFn pFn, void* pUserData);


/*!
\param hObject The model to query.
\param pStr (return) The buffer to hold the command string.
\param maxLen The length of the buffer.

\return \b LT_INVALIDPARAMS if a paramter is null, \b LT_ERROR if there is no
CommandString, else \b LT_OK Get the command string of a model.

Used for: Models and Animation.
*/
	/**/
    virtual LTRESULT GetCommandString(HOBJECT hObj, char *pStr,
											uint32 maxLen);


/*! 
\param hObj Model instance handle.
\param fn   Instance Hook call back reference.
\return  \b LT_OK success 
		 \b LT_INVALIDPARAMS on invalid hObj, otherwise 

Get the ModelInstance hook callback for a model instance.  \b NULL means disabled.
Used to set an extra clip plane to Model instance. ModelInstanceHookData.m_Flags is set to MIH_CLIPPLANE and
ModelInstanceHookData.m_ClipPlane feild.

\see SetInstanceHookFn

Used for: Models and Animation.
*/
 
		virtual LTRESULT GetInstanceHookFn(HOBJECT hObj, 
                                           ModelInstanceHookFn &fn);



/*! 
\param hObj Model Instance Handle
\param fn   call back function.
\return  \b LT_OK on success.
		 \b LT_INVALIDPARAMS on invalid hObj.

Set the hook function for a model.  Set it to \b NULL to disable.
Used to set an extra clip plane to Model instance. ModelInstanceHookData.m_Flags is set to MIH_CLIPPLANE and
ModelInstanceHookData.m_ClipPlane field.

\note Setting this on the server doesn't do anything.


Used for: Models and Animation.  
*/
	virtual LTRESULT SetInstanceHookFn(HOBJECT hObj, 
                                       ModelInstanceHookFn fn);


/*!
\param pObj Model Instance handle.
\param fUpdateDelta time in miliseconds.
\return \b LT_INVALIDPARAMS on invalid hObj, otherwise \b LT_OK.

Update the main anim tracker by \b fUpdateDelta seconds (This is useful
for updating the animation on a model outside of a client update.)

Used for: Models and Animation.  
*/
	virtual LTRESULT UpdateMainTracker(HOBJECT hObj, float fUpdateDelta);


/*! 
\param hAnim the animation of choice.
\param length   the length in miliseconds of the animatoin.
\return \b LT_OK operation succeeded.
		\b LT_INVALIDPARAMS if pModel is null.
		\b LT_NOTINITIALIZED if the pTracker is not currently associated with animation data.
		\b LT_INVALIDVERSION if using SMP networking.

Get the length of the specified animation (in milliseconds).

Used for: Models and Animation.
*/
	virtual LTRESULT GetAnimLength(HOBJECT hModel, HMODELANIM hAnim, 
		 uint32 &length);



/*! 
\param hObj     Model instance.
\param pSetName name of animation weight set.
\param hSet     return reference parameter to weight set.
\return 	    \b LT_NOTINITIALIZED if hObj is invalid, \b LT_OK, success, \b LT_NOTFOUND, weight set not found.

Find the animation blending weight set with the given name.

Used for: Models and Animation.
*/
	virtual LTRESULT FindWeightSet(
                            HOBJECT hObj, 
			                char *pSetName,
                            HMODELWEIGHTSET &hSet);



/*! 
\param hModel     Model instance handle.
\param TrackerID return parameter, identifier for the main tracker.
\return \b LT_INVALIDPARAMS on invalid hModel, \b LT_INVALIDVERSION if \em not using SMP networking; otherwise \b LT_OK.

Get ref for the main animation tracker of a model.

\note The main tracker always uses identifier \b MAIN_TRACKER, so this function is not really needed and will be deprecated.

Used for: Models and Animation.
*/
	virtual LTRESULT GetMainTracker(HOBJECT hModel, 
                                    ANIMTRACKERID &TrackerID);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier.
\param flags     state flags.
\return \b LT_INVALIDPARAMS on invalid hModel, \b LT_INVALIDVERSION if \em not using SMP networking, otherwise \b LT_OK.

Fills in flags with a combination of the \b MS_ flags in basedefs_de.h.

Used for: Models and Animation.  
*/
	virtual LTRESULT GetPlaybackState(HOBJECT hModel, ANIMTRACKERID TrackerID, 
                                      uint32 &flags);

/*! 
\param hModel     Model instance handle.
\param hModel handle to a model.
\param TrackerID  Animation tracker identifier to be associated with new tracker.
\return \b LT_INVALIDPARAMS on invalid hModel,
		\b LT_OVERFLOW if there are already \b MAX_TRACKERS_PER_MODEL attached,
		\b LT_INVALIDVERSION if \em not using SMP networking,
		\b LT_ALREADYEXISTS if the TrackerID is already in use for the model instance,
		otherwise \b LT_OK.

Add trackers on the model.

Used for: Models and Animation.
*/
	virtual LTRESULT AddTracker(HOBJECT hModel, ANIMTRACKERID TrackerID);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier associated with tracker to remove.
\return \b LT_INVALIDPARAMS on invalid hModel or if you try to remove the main tracker, \b LT_INVALIDVERSION if \em not using SMP networking, \b LT_NOTFOUND if TrackerID not in use for the model instance; otherwise \b LT_OK.

Remove trackers on the model.

Used for: Models and Animation.
*/
	virtual LTRESULT RemoveTracker(HOBJECT hModel, ANIMTRACKERID TrackerID);

/*!
\param hModel		Model instance handle.
\param pAnimName	Name of the animation 
\param anim_index	return value: index of animation in model database.
\return \b LT_INVALIDPARAMS on invalid hModel, or if there is no model database associated with hmodel.
		\b LT_OK
	
Used for: Models and Animation.
*/
	virtual LTRESULT GetAnimIndex( HOBJECT hModel, char *pAnimName, uint32 & anim_index );
/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier.
\param hAnim    animation handle.
\return \b LT_INVALIDPARAMS on invalid TrackerID, \b LT_INVALIDVERSION if \em not using SMP networking, otherwise \b LT_OK.

Get the current animation for the tracker.

Used for: Models and Animation.
*/
	virtual LTRESULT GetCurAnim(HOBJECT hModel, ANIMTRACKERID TrackerID, 
                                HMODELANIM &hAnim);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier.
\param hAnim    Animation Handle
\return \b LT_INVALIDPARAMS on invalid TrackerID, \b LT_INVALIDVERSION if \em not using SMP networking, otherwise \b LT_OK.

Set the current animation for the tracker.

Used for: Models and Animation.
*/
	virtual LTRESULT SetCurAnim(HOBJECT hModel, ANIMTRACKERID TrackerID, 
                                HMODELANIM hAnim);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier.
\return \b LT_INVALIDVERSION if \em not using SMP networking, otherwise LT_OK

Resets all animation to first frame.

Used for: Models and Animation.
*/
	virtual LTRESULT ResetAnim(HOBJECT hModel, ANIMTRACKERID TrackerID);

/*!
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier.
\return \b LT_NO if not looping, or an error, \b LT_INVALIDVERSION if \em not using SMP networking; otherwise, returns \b LT_YES.

Used for: Models and Animation.
*/
	virtual LTRESULT GetLooping(HOBJECT hModel, ANIMTRACKERID TrackerID);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier.
\param bLooping looping flag, \b LT_YES loops, \b LT_NO, stops looping.
\return         \b LT_INVALIDVERSION if \em not using SMP networking, otherwise \b LT_OK 

Set whether the current animation on this tracker is looping.

Used for: Models and Animation.
*/
	virtual LTRESULT SetLooping(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bLooping);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier.
\return \b LT_NO if not Playing, or an error, \b LT_INVALIDVERSION if \em not using SMP networking; otherwise, returns \b LT_YES.

Used for: Models and Animation.
*/
	virtual LTRESULT GetPlaying(HOBJECT hModel, ANIMTRACKERID TrackerID);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier of choice.
\param bPlaying boolean flag, true sets animtracker to play, false to not play.
\return         \b LT_INVALIDPARAMS if TrackerID is invalid, \b LT_INVALIDVERSION if \em not using SMP networking, otherwise, returns \b LT_OK.
                
Set whether this tracker is playing its animation.

Used for: Models and Animation.
*/
	virtual LTRESULT SetPlaying(HOBJECT hModel, ANIMTRACKERID TrackerID, bool bPlaying);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier of choice.
\param length   the length in miliseconds of the animation.
\return \b LT_OK operation succeeded.
		\b LT_INVALIDPARAMS if TrackerID is null.
		\b LT_NOTINITIALIZED if the TrackerID is not currently associated with animation data.
		\b LT_INVALIDVERSION if \em not using SMP networking.

Get the length of the current animation (in milliseconds).

Used for: Models and Animation.
*/
	virtual LTRESULT GetCurAnimLength(HOBJECT hModel, ANIMTRACKERID TrackerID, 
		uint32 &length);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier of choice.
\param length   the length in miliseconds of the animatoin.
\return \b LT_OK operation succeeded.
		\b LT_INVALIDPARAMS if TrackerID is null.
		\b LT_NOTINITIALIZED if the TrackerID is not currently associated with animation data.
		\b LT_INVALIDVERSION if \em not using SMP networking.

Get the length of the current animation (in milliseconds).

Used for: Models and Animation.
*/
	virtual LTRESULT GetCurAnimTime(HOBJECT hModel, ANIMTRACKERID TrackerID, 
		uint32 &curTime);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier of choice.
\param curTime time in miliseconds
\return \b LT_OK or if TrackerID is invalid
        \b LT_INVALID_PARAMS  if TrackerID is null
		\b LT_NOTINITIALIZED if the TrackerID is not currently associated with animation data
		\b LT_INVALIDVERSION if \em not using SMP networking.

Moves the animation's current time to curTime. 

Used for: Models and Animation.
*/
	virtual LTRESULT SetCurAnimTime(
                           HOBJECT hModel, ANIMTRACKERID TrackerID, 
			               uint32 curTime);


/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier of choice.
\param hNode      Node to use for movement encoding transform hints
\return \b LT_OK or if TrackerID is invalid
        \b LT_INVALID_PARAMS  if TrackerID is null
		\b LT_NOTINITIALIZED if the TrackerID is not currently associated with animation data
		\b LT_INVALIDVERSION if \em not using SMP networking.

Sets node to use for movement encoding hints. 

Used for: Models and Animation.
*/
	virtual LTRESULT SetHintNode(
                           HOBJECT hModel, ANIMTRACKERID TrackerID, 
			               HMODELNODE hNode);	

	
/*!
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier
\param rate      a float value indicating the rate modifier on the animation.
\return  \b LT_OK
         \b LT_INVALID_PARAMS. if TrackerID is invalid
		 \b LT_INVALIDVERSION if \em not using SMP networking

Sets the current rate of change between frames. Zero rate would stop the animation. Minus one, runs the animation backwards. This is effects only the rate during updates. 

Functional area: Model Animations.
*/
     virtual LTRESULT SetAnimRate( HOBJECT hModel, ANIMTRACKERID TrackerID, float fRate );

/*!
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier
\param rate      a float return value indicating the rate modifier on the animation.
\return  \b LT_INVALID_PARAMS if TrackerID is invalid, \b LT_INVALIDVERSION if \em not using SMP networking; otherwise, returns LT_OK.

Gets the current rate of change between frames. 

Used for: Models and Animation.
*/
     virtual LTRESULT GetAnimRate( HOBJECT hModel, ANIMTRACKERID TrackerID, float &fRate );


/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier
\param hSet    Model animation blending weight set handle.
\return \b LT_OK operation succeeded.
		\b LT_INVALIDPARAMS TrackerID is invalid
		\b LT_INVALIDVERSION if \em not using SMP networking

Get the weight set for this tracker.

Used for: Models and Animation.
*/
	virtual LTRESULT GetWeightSet(
                           HOBJECT hModel, ANIMTRACKERID TrackerID,
                           HMODELWEIGHTSET &hSet);

/*! 
\param hModel     Model instance handle.
\param TrackerID  Animation tracker identifier
\param hSet     Model animation blending weight set handle.
\return \b LT_OK operation succeeded.
		\b LT_INVALIDPARAMS TrackerID is invalid
		\b LT_INVALIDVERSION if \em not using SMP networking

Set the animation blending weight set for this tracker.

Used for: Models and Animation.
*/
	virtual LTRESULT SetWeightSet(HOBJECT hModel, ANIMTRACKERID TrackerID, 
                                  HMODELWEIGHTSET hSet);



/*!
\param hModel Model instance handle
\param float return value: distance of model from camera (based on the model instance's postion).
\return \b LT_OK operation succeeded
		\b LT_INVALIDPARAMS hModel is not valid.
        
       
Get the models's distance from the camera.
  Used for: Models and Animation 
 */
	virtual LTRESULT GetCurrentDistFromCamera( HOBJECT hModel, float &dist );

/*!
\param hModel, model instance handle.
\param hPiece, piece handle
\param NumLODs, return value, number of lods in the piece indicated.
\return \b LT_OK operation succeeded. 
		\b LT_ERROR if hpiece is invalid.
		\b LT_INVALIDPARAMS hModel or hPiece is not valid.
       \b LT_FAIL if operation failed for some other reason.
       
  Returns the number of level of detail geometries the model piece has.

   Used for: Models and Animation 
*/
	virtual LTRESULT GetNumLODs( HOBJECT hModel, HMODELPIECE hPiece, uint32 &num_lods );

	
/*!
\param hModel, model instance handle.
\param hPiece, piece handle
\param distance.
\param lod_number, return value.
\return \b LT_OK operation succeeded. 
		\b LT_ERROR if hpiece is invalid.
		\b LT_INVALIDPARAMS hModel or hPiece is not valid.
       \b LT_FAIL if operation failed for some other reason.
  Returns the level of detail index at the distance requested for the piece.

   Used for: Models and Animation 
*/
	virtual LTRESULT GetLODValFromDist( HOBJECT hModel, HMODELPIECE hPiece, float dist, uint32 &lod_index );


/*!
\param hModel  model instance handle
\param min lod value :
return \b LT_OK operation succeeded. 
	   \b LT_INVALIDPARAMS hModel or hPiece is not valid.
       
       
        This call will change the min lod value for all pieces in a model.
        Caveat: if the min_lod you are setting is more than or equal to the
        number of lods in a piece the min lod will be the number of lods.

   Used for: Models and Animation 
*/
    virtual LTRESULT SetMinLODValue( HOBJECT hModel, uint32 min_lod );
    
/*!
\param hModel  model instance handle
\param min lod value :
return \b LT_OK operation succeeded. 
	   \b LT_INVALIDPARAMS hModel or hPiece is not valid.
       \b LT_FAIL if operation failed for some other reason.
       
        This call will change the max lod value for all pieces in a model.
        Caveat : if a piece's num lods is less then max-lod then the value
        set for that piece will be num lods - 1 instead of max lod.

   Used for: Models and Animation 
*/
        virtual LTRESULT SetMaxLODValue( HOBJECT hModel, uint32 max_lod );
    
/*!
\param hModel, model instance handle.
\param hPiece, piece handle
\param MinValue, the minimum lod index.
return \b LT_OK operation succeeded. 
	   \b LT_INVALIDPARAMS hModel or hPiece is not valid.
       \b LT_FAIL if operation failed for some other reason.
       
  Sets the minimum level of detail geometry to used. By default this is zero. 

   Used for: Models and Animation 
*/
	virtual LTRESULT SetMinLODValue( HOBJECT hModel, HMODELPIECE hPiece, uint32 min_lod );

/*!
\param hModel, model instance handle.
\param hPiece, piece handle
\param MinValue, the minimum lod index.
return \b LT_OK operation succeeded. 
       \b LT_INVALIDPARAMS hModel or hPiece is not valid.
       \b LT_FAIL if operation failed for some other reason.
  Gets the minimum level of detail geometry to used. 

   Used for: Models and Animation 
*/
	virtual LTRESULT GetMinLODValue( HOBJECT hModel, HMODELPIECE hPiece, uint32 &min_lod );


	/*!
\param hModel, model instance handle.
\param hPiece, piece handle
\param MinValue, the maximum lod index.
return \b LT_OK operation succeeded. 
	   \b LT_INVALIDPARAMS hModel or hPiece is not valid.
       \b LT_FAIL if operation failed for some other reason.
       
  Sets the maximum level of detail geometry to used. By default this is the 
  number of lods associated with the peice.

   Used for: Models and Animation 
*/
	virtual LTRESULT SetMaxLODValue( HOBJECT hModel, HMODELPIECE hPiece, uint32 max_lod );

/*!
\param hModel, model instance handle.
\param hPiece, piece handle
\param MinValue, the maximum lod index.
return \b LT_OK operation succeeded. 
	   \b LT_INVALIDPARAMS hModel or hPiece is not valid.
       \b LT_NOTFOUND if the piece index is invalid
       
  Gets the maximum level of detail geometry to used. By default this value should be the 
  same as the one returned by GetNumLODs().

   Used for: Models and Animation 
*/
	virtual LTRESULT GetMaxLODValue( HOBJECT hModel, HMODELPIECE hPiece, uint32 &max_lod );


/*!
\param hModel, model instance handle.
return \b LT_OK operation succeeded. 
	   \b LT_INVALIDPARAMS hModel is not valid.

  Resets the min/max lod values to those specified in the model database for the model
  instance.

   Used for: Models and Animation 
*/
	virtual LTRESULT ResetMinMaxLODValues( HOBJECT hModel );

/*!
\param	hModel, model instance handle.
\return \b LT_OK oepration succeeded
		\b LT_INVALIDPARAMS hmodel is not valid.

  SetCollisionEnable to true to create collision objects from the model database.
	SetCollisionEnable to false to delete the collision objects. Make sure that 
	you are not refering to them in other functions after this call.

  The Collision objects are not distributed.

   Used for: Models and Animation 
  */
	 virtual LTRESULT EnableCollisionObjects( HOBJECT hModel, bool enable );
	//virtual LTRESULT SetCollisionEnable( HOBJECT hModel, bool enable );
/*!
\param	hModel, model instance handle.\br
\return \b LT_INVALIDPARAMS hmodel is not valid.\br
		\b LT_YES if collision objects are enabled for the model instance.\br
		\b LT_NO  if not.

  Gets wether the collision object information is enabled.

   Used for: Models and Animation 
  */
	 virtual LTRESULT IsCollisionObjectsEnabled( HOBJECT hModel );


/*!
\param	hModel, model instance handle.
\param  num_collision_objects the number of collision objects associated with the model.
\return \b LT_OK 		
		\b LT_INVALIDPARAMS hmodel is not valid.

  This function will fill num_collision_objects with the number of collision objects the model has.
  If the value is zero (0), none where allocated, or there are none in the model database.

  Used for: Models and Animation 
 */

	virtual LTRESULT GetNumCollisionObjects( HOBJECT hModel , uint32 &num_collision_objects);


/*!
\param hModel model instance handle.
\param objects array of pointers to ILTCollisionObject type. (see notes)
\param node_refs array of node handles 
\param size      size of both objects and node_refs array.
\return LT_INVALIDPARAM invalid model-instance
		LT_ERROR		size greater than number of collision objects in model.
		LT_OK			success.

  Use this function to get a handle on the collision objects of the model. 
  The objects and node_refs array must be a pre-allocated array that the engine
  will fill with data. Do not delete the contents of the objects array, that would 
  put the engine in an indeterminate state.
  When the collision object facility of the model is disabled throught GetCollisionObjectEnable(false)
  the contents of the objects array are invalid.

  Used for: Models and Animation 
  */
	virtual LTRESULT GetCollisionObjects(HOBJECT hModel ,  
										 ILTCollisionObject **objects, 
										 HMODELNODE          *node_refs, 
										 uint32               size );

/*!
\param hModel model instance handle
\param index which collision object, index must be >= 0 and < GetNumCollisionObjects.
\param pcollision_object a reference to a pointer to a collision object. (see note)
\return LT_INVALIDPARAMS if hModel is invalid.
		LT_ERROR         if index is out of range.
		LT_OK			 else.
  
	Gets an individual collision object from the model instance. This function will return X if there 
  no collision objects associated with this model instance. 
  
  Do not change the values pointed to by pcollision_object.
  
Used for: Models and Animation 
*/
	virtual LTRESULT GetCollisionObject( HOBJECT hModel, 
										 uint32 index,
										 const ILTCollisionObject *&pcollision_object);
										 

/*!
\param hModel model instance handle.
\param enable true to allow user to call UpdateCollisionObjects	

  Normally collision objects are updated right after animations are by the engine.
  If the user want to explicity call the collision update, they may 
  do it be enabling it using this function. \see UpdateCollisionObjects

  Used For : Model & Animation 

*/
	virtual LTRESULT EnableExplicitCollisionObjectUpdate(HOBJECT hModel, bool enable );

/*!
\param hModel model instance handle 
\returns LT_INVALIDPARAM bad model instance handle
	     LT_YES          if update collision objects are enabled.
		 LT_NO			 if not.

  Query to see if collision objects are enabled.

  Used For : Model & Animation 
  */
	virtual LTRESULT IsUpdateCollisionObjectsEnabled( HOBJECT hModel );

/*!
\param hModel model instance handle.
\return LT_INVALIDPARAM invalid model-instance 
	    LT_ERROR		not enabled, \see EnableExplicitCollisionObjectUpdate
		LT_OK			ok.

	Update the collision objects according to the current animation.

  Used For : Model & Animation 
*/
	virtual LTRESULT UpdateCollisionObjects( HOBJECT hModel );




/*!
\param	hModel, model instance handle.
\param  enable  true to create transform cache, false to destroy it.
\return \b LT_OK oepration succeeded
		\b LT_INVALIDPARAMS hmodel is not valid.

	This function creates a cache of the current animation transform set. 
	This is used to make transformation queries much more efficient.
	The transform cache is updated before the model is rendered. 
	This data is not distributed, meaning if you set this up on the server, 
	the client does not enjoy the more efficient queries and vis versa.

	If enable is true, the transform cache is created for the context the function was called, 
	if enable is false, the transform cache is delete for the context the function was called. 
	If the transfrom cache is disabled before being enabled, nothing happens.

 Used for: Models and Animation 
*/
		virtual LTRESULT EnableTransformCache( HOBJECT hModel, bool enable );
		

	/*!
\param hModel, model instance handle.
\return \bLT_YES if the transform cache is enabled.
		\bLT_NO  if not.
		\bLT_INVALIDPARAM if hModel is invalid

  Query to see if model instance has its animation transforms cached.

  Used for: Models and Animation
*/
	virtual LTRESULT IsTransformCacheEnabled( HOBJECT hModel );

	/*
\param hModel, model instance handle.
\return \b LT_OK 
		\b LT_INVALIDPARAMS 

  forces an update on the trandform cache with current anims.
  */
	virtual LTRESULT UpdateCachedTransforms( HOBJECT hModel );

/*!
\param hObj Model to query.
\param pFilename (return) Buffer for model filename (can use \b LTNULL).
\param fileBufLen The \b pFilename buffer length.
\param pSkinName (return) Buffer for skin filename (can use LTNULL).
\param skinBufLen The \b pSkinName buffer length.

\return \b LT_INVALIDPARAMS if the object is not a model.
		\b LT_UNSUPPORTED if called from the client.
		\b LT_OK          otherwise.

Get the model filenames.  Initializes \b pFilename or \b pSkinName to zero
length if the model doesn't have a filename or skin.

  \note This function is only valid on the server.

Used for: Models and Animation.
*/
    virtual LTRESULT GetFilenames(HOBJECT hObj, char *pFilename, uint32 fileBufLen, char *pSkinName, uint32 skinBufLen)=0;

/*!

*/

	virtual LTRESULT GetModelDBFilename( HOBJECT hObj, char *pRetFilename, uint32 fileBufLen ) = 0;

/*!

*/
	virtual LTRESULT GetSkinFilenames( HOBJECT hObj, char **pRetFilenames, uint32 fileBufLen, uint32 numEntries ) = 0 ;

		
};



/*!
The \b ILTModelClient interface contains client only functions for 
manipulating models and animations.

Define a holder to get this interface like this:
\code
define_holder(ILTModelClient, your_var);
\endcode
*/

class ILTModelClient : public ILTModel {
public:
    interface_version_derived(ILTModelClient, ILTModel, 0);

/*!
\param hModel       ModelInstance handle.
\param index		Index into model instance's list of renderstyles.
\param pRenderStyle	Pointer to the render style.	
\return \b LT_OK

Get's the PieceLOD's render style.

\see SetPieceHideStatus()

Used for: Models Rendering.  
*/
	virtual LTRESULT GetRenderStyle(HOBJECT hModel, uint32 hPiece, CRenderStyle** ppRenderStyle) = 0;


/*!
\param hModel       Model instance handle.
\param index		Index into model instance's list of renderstyles.
\param pRenderStyle	Pointer to the render style.	
\return \b LT_OK

Set's the PieceLOD's render style.

\see GetPieceHideStatus()

Used for: Models Rendering.  
*/
	virtual LTRESULT SetRenderStyle(HOBJECT pModel, uint32 hPiece, CRenderStyle* pRenderStyle) = 0;
};
		

#endif //! __ILTMODEL_H__
















