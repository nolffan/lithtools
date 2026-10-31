#include "LTAFile.h"

LTAFile::LTAFile( bool externalKeys )
{
	this->externalKeys = externalKeys;
	numKeys = 0;

	nodeAlloc = new CLTADefaultAlloc;

	root = new CLTANode;
	header = new CLTANode;
	polyhedronList = new CLTANode;
	nodeHierarchy = new CLTANode;
	globalPropList = new CLTANode;
	navigatorPosList = new CLTANode;
	nodeRoot = new CLTANode;
	polyRoot = new CLTANode;
	propRoot = new CLTANode;
	curPolyhedronRoot = NULL;

	keyRoot = NULL;
	keyList = NULL;

	// first container, terrain object for terrain, container for brushes
	CLTANode* container = new CLTANode;

	// setup the node tree
	nodeList = new LTANodeList;
	nodeList->containerNode = nodeRoot;
	nodeList->next = NULL;
	nodeList->prev = NULL;

	// setup the property tree
	propList = NULL;

	// setup the polyhedron info
	curPolyhedron = 0;
	curNodeID = 3;		// first two are hardcoded below
	nextPropID = 1;		// only applies to brush export
	curPropID = 0;		// only applies to brush export
	curBrushID = 0;		// only applies to brush export

	// setup the base level of the tree
	A( root, N("world") );
	A( root, header );
	A( root, polyhedronList );
	A( root, nodeHierarchy );
	A( root, globalPropList );
	A( root, navigatorPosList );

	// setup the header tree
	A(
	A(
	header,
		N("header")
	),
		A(
		N(),
			A(
			N(),
				N("versioncode 1")
			)
		)
	);

	// setup the navigatorposlist tree
	A(
	A(
	navigatorPosList,
		N("navigatorposlist")
	),
		N()
	);

	// setup the polyhedronlist tree
	A(
	A(
	polyhedronList,
		N("polyhedronlist")
	),
		polyRoot	// new polyhedrons are appended to this node
	);

	// setup the nodehierarchy
	A(
	A(
	nodeHierarchy,
		N("nodehierarchy")
	),
		A(
		A(
		A(
		A(
		A(
		A(
		A(
		N(),
			N("worldnode")
		),
			A(
			N(),
				N("type null")
			)
		),
			A(
			N(),
				N("label \"Container\"")
			)
		),
			A(
			N(),
				N("nodeid 1")
			)
		),
			A(
			A(
			N(),
				N("flags")
			),
				A(
				N(),
					N("worldroot")
				)
			)
		),
			A(
			A(
			N(),
				N("properties")
			),
				A(
				N(),
					N("propid 0")
				)
			)
		),
			A(
			A(
			N(),
				N("childlist")
			),
				container
			)
		)
	);

	// exporting as brushes, create a container
	A(
	container,
		A(
		A(
		A(
		A(
		A(
		A(
		A(
		N(),
			N("worldnode")
		),
			A(
			N(),
				N("type null")
			)
		),
			A(
			N(),
				N("label \"MAX Brushes\"")
			)
		),
			A(
			N(),
				N("nodeid 2")
			)
		),
			A(
			A(
			N(),
				N("flags")
			),
				N()
			)
		),
			A(
			A(
			N(),
				N("properties")
			),
				A(
				N(),
					N("propid 0")
				)
			)
		),
			A(
			A(
			N(),
				N("childlist")
			),
				nodeRoot	// new nodes are appended to this
			)
		)
	);

	// setup the globalproplist
	A(
	A(
	globalPropList,
		N("globalproplist")
	),
		propRoot	// new properties are appended to this
	);

	// add the empty property used by containers
	A(
	propRoot,
		A(
		A(
		N(),
			N("proplist")
		),
			N()
		)
	);

	// setup external key file
	if( externalKeys )
	{
		keyRoot = new CLTANode;
		keyList = new CLTANode;

		A( keyRoot, N("keys") );
		A( keyRoot, keyList );
	}
}

LTAFile::~LTAFile()
{
	// recursively delete the tree
	nodeAlloc->FreeNode( root );

	if( externalKeys )
		nodeAlloc->FreeNode( keyRoot );

	// delete the property list
	PropertyList* curProp = propList;
	while( curProp )
	{
		PropertyList* delProp = curProp;
		curProp = delProp->next;
		delete delProp;
	}

	// delete the node list
	assert( nodeList->next == NULL );
	delete nodeList;

	// delete the memory allocator
	delete nodeAlloc;
}

// add a brush property to the tree if needed and set the curPropID
void LTAFile::AddProperty( const char* name )
{
	// search for an existing property with this name
	bool foundProp = false;
	PropertyList* curProp = propList;
	while( curProp )
	{
		if( curProp->isBrush && (curProp->name == TSTR(name)) )
		{
			assert( curProp->id > 0 );
			foundProp = true;
			break;
		}
		curProp = curProp->next;
	}

	// found an existing property with this name, set the id and return
	if( foundProp )
	{
		curPropID = curProp->id;
		return;
	}

	// add the property to the list
	PropertyList* newProp = new PropertyList;
	newProp->name = name;
	newProp->isBrush = true;
	newProp->id = this->nextPropID++;
	newProp->next = propList;
	propList = newProp;
	curPropID = newProp->id;

	// setup strings for the tree
	TSTR propName;
	propName.printf( "string \"Name\" () (data \"%s\")", name );

	// add the property to the property tree
	A(
	propRoot,
		A(
		A(
		N(),
			N("proplist")
		),
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			 A(
			 A(
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			N(),
				A(
				N(),
					N(propName)
				)
			),
				A(
				N(),
					N("vector \"Pos\" ( distance ) (data (vector (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N("rotation \"Rotation\" () (data (eulerangles (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N("string \"Type\" (staticlist) (data \"Normal\")")
				)
			),
				A(
				N(),
					N("string \"Lighting\" (staticlist) (data \"Gouraud\")")
				)
			),
				A(
				N(),
					N("bool \"NoSnap\" () (data 1)")
				)
			),
				A(
				N(),
					N("bool \"NotAStep\" () (data 0)")
				)
			),
				A(
				N(),
					N("bool \"Detail\" () (data 1)")
				)
			),
				A(
				N(),
					N("longint \"LightControl\" (groupowner group1) (data 0.000)")
				)
			),
				A(
				N(),
					N("color \"AmbientLight\" (group1) (data (vector (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N("longint \"LMGridSize\" (group1) (data 0.000)")
				)
			),
				A(
				N(),
					N("bool \"ClipLight\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("bool \"CastShadowMesh\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("bool \"ReceiveLight\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("bool \"ReceiveShadows\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("bool \"ReceiveSunlight\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("real \"LightPenScale\" (group1) (data 0.000)")
				)
			),
				A(
				N(),
					N("real \"CreaseAngle\" (group1) (data 45.000)")
				)
			)
		)
	);
}

// adds a new worldmodel as a child, and sets the root to that object
void LTAFile::AddWorldModel( const char* name, const Point3& pos )
{
	// location in the tree we are adding the worldmodel to
	CLTANode* curRoot = nodeList->containerNode;

	// create the root below this one
	CLTANode* nextRoot = new CLTANode;

	// update the lta node list
	LTANodeList* prevNode = nodeList;
	nodeList = new LTANodeList;
	prevNode->next = nodeList;
	nodeList->prev = prevNode;
	nodeList->next = NULL;
	nodeList->containerNode = nextRoot;

	// add the property to the list
	PropertyList* newProp = new PropertyList;
	newProp->name = name;
	newProp->isBrush = false;
	newProp->id = nextPropID++;
	newProp->next = propList;
	propList = newProp;
	curPropID = newProp->id;

	// setup strings for the tree
	TSTR propName, propPos;
	propName.printf( "string \"Name\" () (data \"%s\")", name );
	propPos.printf( "vector \"Pos\" ( distance ) (data (vector (%f %f %f)))", pos.x, pos.y, pos.z );

	// add the property to the property tree
	A(
	propRoot,
		A(
		A(
		N(),
			N("proplist")
		),
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			 A(
			 A(
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			 A(
			 A(
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			 A(
			 A(
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			 A(
			 A(
			A(
			A(
			A(
			A(
			N(),
				A(
				N(),
					N(propName)
				)
			),
				A(
				N(),
					N(propPos)
				)
			),
				A(
				N(),
					N("rotation \"Rotation\" () (data (eulerangles (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N("longint \"DamageProperties\" (groupowner group1) (data 0.000)")
				)
			),
				A(
				N(),
					N("longint \"ExplosionProperties\" (groupowner group2) (data 0.000)")
				)
			),
				A(
				N(),
					N("string \"DebrisType\" (staticlist) (data \"Glass large\")")
				)
			),
				A(
				N(),
					N("string \"SurfaceOverride\" (staticlist) (data \"Unknown\")")
				)
			),
				A(
				N(),
					N("bool \"Visible\" () (data 1)")
				)
			),
				A(
				N(),
					N("bool \"Solid\" () (data 1)")
				)
			),
				A(
				N(),
					N("bool \"Rayhit\" () (data 1)")
				)
			),
				A(
				N(),
					N("bool \"Gravity\" () (data 0)")
				)
			),
				A(
				N(),
					N("string \"BlendMode\" (staticlist) (data \"None\")")
				)
			),
				A(
				N(),
					N("bool \"BlockLight\" () (data 1)")
				)
			),
				A(
				N(),
					N("bool \"BoxPhysics\" () (data 1)")
				)
			),
				A(
				N(),
					N("bool \"FogDisable\" () (data 0)")
				)
			),
				A(
				N(),
					N("bool \"IsKeyframed\" () (data 1)")
				)
			),
				A(
				N(),
					N("real \"Alpha\" () (data 1.000)")
				)
			),
				A(
				N(),
					N("string \"Attachments\" ()")
				)
			),
				A(
				N(),
					N("bool \"RemoveAttachments\" () (data 1)")
				)
			),
				A(
				N(),
					N("vector \"AttachDir\" () (data (vector (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N("longint \"Disturbances\" (groupowner group6) (data 0.000)")
				)
			),
				A(
				N(),
					N("real \"HitPoints\" (group1) (data 100.000)")
				)
			),
				A(
				N(),
					N("real \"MaxHitPoints\" (group1) (data 100.000)")
				)
			),
				A(
				N(),
					N("real \"Armor\" (group1) (data 100.000)")
				)
			),
				A(
				N(),
					N("real \"MaxArmor\" (group1) (data 100.000)")
				)
			),
				A(
				N(),
					N("longint \"DamageTriggerCounter\" (group1) (data 0.000)")
				)
			),
				A(
				N(),
					N("string \"DamageCommand\" (group1 notifychange)")
				)
			),
				A(
				N(),
					N("longint \"DamageTriggerNumSends\" (group1) (data 1.000)")
				)
			),
				A(
				N(),
					N("string \"DamagerMessage\" (group1)")
				)
			),
				A(
				N(),
					N("string \"DeathCommand\" (group1 notifychange)")
				)
			),
				A(
				N(),
					N("string \"PlayerDeathCommand\" (group1 notifychange)")
				)
			),
				A(
				N(),
					N("string \"KillerMessage\" (group1)")
				)
			),
				A(
				N(),
					N("bool \"CanHeal\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("bool \"CanRepair\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("bool \"CanDamage\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("real \"Mass\" (group1) (data 2000.000)")
				)
			),
				A(
				N(),
					N("bool \"CreateExplosion\" (group2) (data 0)")
				)
			),
				A(
				N(),
					N("longint \"WeaponId\" (group2) (data 0.000)")
				)
			),
				A(
				N(),
					N("bool \"FireAlongForward\" (group2) (data 0)")
				)
			),
				A(
				N(),
					N("real \"DamageFactor\" (group2) (data 1.000)")
				)
			),
				A(
				N(),
					N("string \"Spawn\" (group2)")
				)
			),
				A(
				N(),
					N("bool \"NeverDestroy\" (group1) (data 1)")
				)
			),
				A(
				N(),
					N("longint \"DestroyAlarmLevel\" (group6) (data 0.000)")
				)
			),
				A(
				N(),
					N("real \"StimulusRadius\" (group6) (data 0.000)")
				)
			)
		)
	);

	// adding a new worldmodel node
	TSTR nodeid, propid;
	nodeid.printf( "nodeid %d", curNodeID++ );
	propid.printf( "propid %d", curPropID );

	// add the worldmodel node to the node tree
	A(
	curRoot,
		A(
		A(
		A(
		A(
		A(
		A(
		N(),
			N("worldNode")
		),
			A(
			N(),
				N("type object")
			)
		),
			A(
			N(),
				N(nodeid)
			)
		),
			A(
			N(),
				N("flags ()")
			)
		),
			A(
			A(
			A(
			N(),
				N("properties")
			),
				A(
				N(),
					N("name \"WorldModel\"")
				)
			),
				A(
				N(),
					N(propid)
				)
			)
		),
			A(
			A(
			N(),
				N("childlist")
			),
				nextRoot
			)
		)
	);
}

// adds a new keyframer as a child, and sets the root to that object
void LTAFile::AddKeyFramer( const char* name, const char* worldModelName, const char* keyBaseName, const char* keyFile, const Point3& pos )
{
	// location in tree we are adding the container to
	CLTANode* curRoot = nodeList->containerNode;

	// create the root below this one
	CLTANode* nextRoot = new CLTANode;

	// update the lta node list
	LTANodeList* prevNode = nodeList;
	nodeList = new LTANodeList;
	prevNode->next = nodeList;
	nodeList->prev = prevNode;
	nodeList->next = NULL;
	nodeList->containerNode = nextRoot;

	// add the property to the list
	PropertyList* newProp = new PropertyList;
	newProp->name = name;
	newProp->isBrush = false;
	newProp->id = nextPropID++;
	newProp->next = propList;
	propList = newProp;
	curPropID = newProp->id;

	// setup strings for the tree
	TSTR propName, propPos, propModelName, propKeyName, propKeyFile;
	propName.printf( "string \"Name\" () (data \"%s\")", name );
	propPos.printf( "vector \"Pos\" ( distance ) (data (vector (%f %f %f)))", pos.x, pos.y, pos.z );
	propModelName.printf( "string \"ObjectName\" () (data \"%s\")", worldModelName );
	propKeyName.printf( "string \"BaseKeyName\" () (data \"%s\")", keyBaseName );

	if( externalKeys )
		propKeyFile.printf( "string \"ExternalKeyFile\" () (data \"%s\")", keyFile );
	else
		propKeyFile.printf( "string \"ExternalKeyFile\" ()" );

	// add the property to the property tree
	A(
	propRoot,
		A(
		A(
		N(),
			N("proplist")
		),
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			 A(
			 A(
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			 A(
			 A(
			A(
			N(),
				A(
				N(),
					N(propName)
				)
			),
				A(
				N(),
					N(propPos)
				)
			),
				A(
				N(),
					N("rotation \"Rotation\" () (data (eulerangles (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N(propModelName)
				)
			),
				A(
				N(),
					N(propKeyName)
				)
			),
				A(
				N(),
					N(propKeyFile)
				)
			),
				A(
				N(),
					N("real \"KeyDataIndex\" (hidden) (data -1.0)")
				)
			),
				A(
				N(),
					N("bool \"PushObjects\" () (data 0)")
				)
			),
				A(
				N(),
					N("bool \"StartActive\" () (data 0)")
				)
			),
				A(
				N(),
					N("bool \"StartPaused\" () (data 0)")
				)
			),
				A(
				N(),
					N("bool \"Looping\" () (data 0)")
				)
			),
				A(
				N(),
					N("bool \"AlignToPath\" () (data 0)")
				)
			),
				A(
				N(),
					N("bool \"IgnoreOffsets\" () (data 1)")
				)
			),
				A(
				N(),
					N("string \"TargetName\" ()")
				)
			),
				A(
				N(),
					N("vector \"TargetOffset\" () (data (vector (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N("string \"ActiveSound\" ()")
				)
			),
				A(
				N(),
					N("real \"SoundRadius\" () (data 1000.000)")
				)
			),
				A(
				N(),
					N("string \"RotationWave\" () (data \"LINEAR\")")
				)
			),
				A(
				N(),
					N("longint \"Waveform\" (groupowner group1) (data 0.000)")
				)
			),
				A(
				N(),
					N("real \"TotalPathTime\" (group1) (data 0.000)")
				)
			),
				A(
				N(),
					N("string \"Wavetype\" (group1 staticlist) (data \"Linear\")")
				)
			)
		)
	);

	// adding a new keyframer node
	TSTR nodeid, propid;
	nodeid.printf( "nodeid %d", curNodeID++ );
	propid.printf( "propid %d", curPropID );

	// add the keyframer node to the tree
	A(
	curRoot,
		A(
		A(
		A(
		A(
		A(
		A(
		N(),
			N("worldnode")
		),
			A(
			N(),
				N("type object")
			)
		),
			A(
			N(),
				N(nodeid)
			)
		),
			A(
			N(),
				N("flags ( path )")
			)
		),
			A(
			A(
			A(
			N(),
				N("properties")
			),
				A(
				N(),
					N("name \"KeyFramer\"")
				)
			),
				A(
				N(),
					N(propid)
				)
			)
		),
			A(
			A(
			N(),
				N("childlist")
			),
				nextRoot
			)
		)
	);
}

// add a key to either the world file, or to the external key file
void LTAFile::AddKey( const char* name, float time, const Point3& kfPos, const Point3& pos, const Point3& eulerRotation, const char* sound, float soundRadius, const char* command )
{
	if( externalKeys )
		AddKeyExternalFile( name, time, pos - kfPos, eulerRotation, sound, soundRadius, command );
	else
		AddKeyWorldFile( name, time, pos, eulerRotation, sound, soundRadius, command );
}

// add a key to the external key file
void LTAFile::AddKeyExternalFile( const char* name, float time, const Point3& pos, const Point3& eulerRotation, const char* sound, float soundRadius, const char* command )
{
	TSTR keyName, keyPos, keyTime, keyRot, keySound, keySoundRadius, keyCommand;
	keyName.printf( "\"%s\"", name );
	keyPos.printf( "pos %f %f %f", pos.x, pos.y, pos.z );
	keyRot.printf( "rot %f %f %f", eulerRotation.x, eulerRotation.y, eulerRotation.z );
	keyTime.printf( "time %f", time );
	keySound.printf( "sound \"%s\"", sound );
	keySoundRadius.printf( "soundradius %f", soundRadius );
	keyCommand.printf( "command \"%s\"", command );

	A(
	keyList,
		A(
		A(
		A(
		A(
		A(
		 A(
		 A(
		 A(
		 A(
		N(),
			N(keyName)
		),
			A(
			N(),
				N(keyPos)
			)
		),
			A(
			N(),
				N(keyRot)
			)
		),
			A(
			N(),
				N(keyTime)
			)
		),
			A(
			N(),
				N(keySound)
			)
		),
			A(
			N(),
				N(keySoundRadius)
			)
		),
			A(
			N(),
				N(keyCommand)
			)
		),
			A(
			N(),
				N("bezierprev 0.0 0.0 0.0")
			)
		),
			A(
			N(),
				N("beziernext 0.0 0.0 0.0")
			)
		)
	);
}

// add a new key as a child, and sets the root to that object
void LTAFile::AddKeyWorldFile( const char* name, float time, const Point3& pos, const Point3& eulerRotation, const char* sound, float soundRadius, const char* command )
{
	// location in the tree we are adding the container to
	CLTANode* curRoot = nodeList->containerNode;

	// create the root below this one
	CLTANode* nextRoot = new CLTANode;

	// update the lta node list
	LTANodeList* prevNode = nodeList;
	nodeList = new LTANodeList;
	prevNode->next = nodeList;
	nodeList->prev = prevNode;
	nodeList->next = NULL;
	nodeList->containerNode = nextRoot;

	// add the property to the list
	PropertyList* newProp = new PropertyList;
	newProp->name = name;
	newProp->isBrush = false;
	newProp->id = nextPropID++;
	newProp->next = propList;
	propList = newProp;
	curPropID = newProp->id;

	// setup strings for the tree
	TSTR propName, propPos, propRot, propTime, propSound, propSoundRadius, propCommand;
	propName.printf( "string \"Name\" () (data \"%s\")", name );
	propPos.printf( "vector \"Pos\" ( distance ) (data (vector (%f %f %f)))", pos.x, pos.y, pos.z );
	propRot.printf( "rotation \"Rotation\"  () (data (eulerangles (%f %f %f)))", eulerRotation.x, eulerRotation.y, eulerRotation.z );
	propTime.printf( "real \"TimeStamp\" () (data %f)", time );
	propSoundRadius.printf( "real \"SoundRadius\" (radius) (data %f)", soundRadius );

	if( strlen( sound ) )
		propSound.printf( "string \"SoundName\" () (data \"%s\")", sound );
	else
		propSound = "string \"SoundName\" ()";

	if( strlen( command ) )
		propCommand.printf( "string \"Command\" (notifychange) (data \"%s\")", command );
	else
		propCommand = "string \"Command\" (notifychange) ()";

	// add the property to the property tree
	A(
	propRoot,
		A(
		A(
		N(),
			N("proplist")
		),
			A(
			A(
			A(
			A(
			A(
			 A(
			 A(
			 A(
			 A(
			N(),
				A(
				N(),
					N(propName)
				)
			),
				A(
				N(),
					N(propPos)
				)
			),
				A(
				N(),
					N(propRot)
				)
			),
				A(
				N(),
					N(propTime)
				)
			),
				A(
				N(),
					N(propSound)
				)
			),
				A(
				N(),
					N(propSoundRadius)
				)
			),
				A(
				N(),
					N(propCommand)
				)
			),
				A(
				N(),
					N("vector \"BezierPrev\" (bezierprevtangent) (data (vector (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N("vector \"BezierNext\" (beziernexttangent) (data (vector (0.000 0.000 0.000)))")
				)
			)
		)
	);

	// adding a new key node
	TSTR nodeid, propid;
	nodeid.printf( "nodeid %d", curNodeID++ );
	propid.printf( "propid %d", curPropID );

	// add the key node to the tree
	A(
	curRoot,
		A(
		A(
		A(
		A(
		A(
		A(
		N(),
			N("worldnode")
		),
			A(
			N(),
				N("type object")
			)
		),
			A(
			N(),
				N(nodeid)
			)
		),
			A(
			N(),
				N("flags ()")
			)
		),
			A(
			A(
			A(
			N(),
				N("properties")
			),
				A(
				N(),
					N("name \"Key\"")
				)
			),
				A(
				N(),
					N(propid)
				)
			)
		),
			A(
			A(
			N(),
				N("childlist")
			),
				nextRoot
			)
		)
	);
}

// talon: near (declaration count), both builds: size-exact, only the order of two
// talon: loads at the loop head (ecx/edx) differs. A typedef sweep at the top of this
// talon: file matches it at many residues (+1, +3, +7, +14, +27..31, +36..53 mod 64),
// talon: so it moves with the shared headers (ltamgr, sdk/inc); recheck once they settle.
// add a new group object as a child, and sets the root to that object
void LTAFile::AddKeyFramerGroup( const char* name, std::vector<TSTR>& keyFramerNames )
{
	// location in tree we are adding the container to
	CLTANode* curRoot = nodeList->containerNode;

	// create the root below this one
	CLTANode* nextRoot = new CLTANode;

	// create the node that properties are added to
	CLTANode* propNode = new CLTANode;

	// update the lta node list
	LTANodeList* prevNode = nodeList;
	nodeList = new LTANodeList;
	prevNode->next = nodeList;
	nodeList->prev = prevNode;
	nodeList->next = NULL;
	nodeList->containerNode = nextRoot;

	// add the property to the list
	PropertyList* newProp = new PropertyList;
	newProp->name = name;
	newProp->isBrush = false;
	newProp->id = nextPropID++;
	newProp->next = propList;
	propList = newProp;
	curPropID = newProp->id;

	// setup strings for the tree
	TSTR propName;
	propName.printf( "string \"Name\" () (data \"%s\")", name );

	// add the property to the property tree
	A(
	propRoot,
		A(
		A(
		N(),
			N("proplist")
		),
			A(
			A(
			A(
			propNode,
				A(
				N(),
					N(propName)
				)
			),
				A(
				N(),
					N("vector \"Pos\" ( distance ) (data (vector (0.000 0.000 0.000)))")
				)
			),
				A(
				N(),
					N("rotation \"Rotation\" () (data (eulerangles (0.000 0.000 0.000)))")
				)
			)
		)
	);

	TSTR curProp;
	int numKeyFramers = keyFramerNames.size();

	// add the keyframer references
	for( int i = 0; i < 50; i++ )
	{
		if( i < numKeyFramers )
			curProp.printf( "string \"Object%d\" (objectlink) (data \"%s\")", i+1, keyFramerNames[i] );
		else
			curProp.printf( "string \"Object%d\" (objectlink)", i+1 );
	
		// add the property
		A( propNode, A(N(), N(curProp)) );
	}

	// adding a new key node
	TSTR nodeid, propid;
	nodeid.printf( "nodeid %d", curNodeID++ );
	propid.printf( "propid %d", curPropID );

	// add the group node to the tree
	A(
	curRoot,
		A(
		A(
		A(
		A(
		A(
		A(
		A(
		N(),
			N("worldnode")
		),
			A(
			N(),
				N("type object")
			)
		),
			A(
			N(),
				N("label \"Group\"")
			)
		),
			A(
			N(),
				N(nodeid)
			)
		),
			A(
			N(),
				N("flags ()")
			)
		),
			A(
			A(
			A(
			N(),
				N("properties")
			),
				A(
				N(),
					N("name \"Group\"")
				)
			),
				A(
				N(),
					N(propid)
				)
			)
		),
			A(
			A(
			N(),
				N("childlist")
			),
				nextRoot
			)
		)
	);

}

// adds a new object as a child, and sets the root to that object
void LTAFile::AddObject( const char* name, bool asBrush )
{
	// location in tree we are adding the container to
	CLTANode* curRoot = nodeList->containerNode;

	// create the root below this one
	CLTANode* nextRoot = new CLTANode;

	// update the lta node list
	LTANodeList* prevNode = nodeList;
	nodeList = new LTANodeList;
	prevNode->next = nodeList;
	nodeList->prev = prevNode;
	nodeList->next = NULL;
	nodeList->containerNode = nextRoot;

	if( asBrush )
	{
		// adding a new brush node
		TSTR nodeid;
		nodeid.printf( "nodeid %d", curNodeID++ );

		// check for an existing property with this name, add it if not found
		AddProperty( name );
		assert( curPropID );
		TSTR propid;
		propid.printf( "propid %d", curPropID );

		// brush index
		TSTR brushid;
		brushid.printf( "brushindex %d", curBrushID++ );

		// add the brush node to the node tree
		A(
		curRoot,
			A(
			A(
			A(
			A(
			A(
			A(
			A(
			A(
			N(),
				N("worldnode")
			),
				A(
				N(),
					N("type brush")
				)
			),
				A(
				N(),
					N(brushid)
				)
			),
				A(
				N(),
					N("label \"Brush\"")
				)
			),
				A(
				N(),
					N(nodeid)
				)
			),
				A(
				A(
				N(),
					N("flags")
				),
					N()
				)
			),
				A(
				A(
				A(
				N(),
					N("properties")
				),
					A(
					N(),
						N("name \"Brush\"")
					)
				),
					A(
					N(),
						N(propid)
					)
				)
			),
				A(
				A(
				N(),
					N("childlist")
				),
					nextRoot
				)
			)
		);

		// create a new polyhedron and its poly list
		curPolyhedronRoot = new CLTANode;
		curEditPoly = new CLTANode;
		curPointList = new CLTANode;

		// add the new polyhedron to the tree
		A(
		polyRoot,
			A(
			A(
			N(),
				N("polyhedron")
			),
				curPolyhedronRoot
			)
		);

		// set the color of the polyhedron
		A(
		curPolyhedronRoot,
			A(
			N(),
				N("color 255 255 255")
			)
		);

		// add the pointlist to the polyhedron
		A(
		curPolyhedronRoot,
			curPointList
		);

		// add the polylist to the polyhedron
		A(
		curPolyhedronRoot,
			A(
			A(
			N(),
				N("polylist")
			),
				curEditPoly
			)
		);
	}
	else
	{
		// adding a new container node
		TSTR label;
		TSTR nodeid;
		label.printf( "label \"%s\"", name );
		nodeid.printf( "nodeid %d", curNodeID++ );

		// add the container to the node tree
		A(
		curRoot,
			A(
			A(
			A(
			A(
			A(
			A(
			A(
			N(),
				N("worldnode")
			),
				A(
				N(),
					N("type null")
				)
			),
				A(
				N(),
					N(label)
				)
			),
				A(
				N(),
					N(nodeid)
				)
			),
				A(
				A(
				N(),
					N("flags")
				),
					N()
				)
			),
				A(
				A(
				N(),
					N("properties")
				),
					A(
					N(),
						N("propid 0")
					)
				)
			),
				A(
				A(
				N(),
					N("childlist")
				),
					nextRoot
				)
			)
		);
	}
}

void LTAFile::AscendTree( void )
{
	nodeList = nodeList->prev;
	delete nodeList->next;
	nodeList->next = NULL;
}

// add polygon info to the current polyhedron
void LTAFile::AddBrushPolygon( int numVerts, const int* indices, const Point3& n, float d, const Point3& o, const Point3& p, const Point3& q, const TSTR& textureName )
{
	assert( numVerts && indices );

	TSTR sf, sn, sd, so, sp, sq;
	TSTR stex;
	TSTR sbrushindex;
	TSTR snodeid;

	// setup the vert index string
	TSTR sIndex;
	sf.printf( "f" );
	for( int i = 0; i < numVerts; i++ )
	{
		sIndex.printf( " %d", indices[i] );
		sf += sIndex;
	}

	// setup strings for the polygon
	sn.printf( "n %f %f %f", n.x, n.y, n.z );
	sd.printf( "dist %f", d );

	so.printf( "%f %f %f", o.x, o.y, o.z );
	sp.printf( "%f %f %f", p.x, p.y, p.z );
	sq.printf( "%f %f %f", q.x, q.y, q.z );
	stex.printf( "name \"%s\"", textureName );

	// add the editpoly node
	A(
	curEditPoly,
		A(
		A(
		A(
		A(
		A(
		 A(
		 A(
		N(),
			N("editpoly")
		),
			A(
			N(),
				N(sf)
			)
		),
			A(
			N(),
				N(sn)
			)
		),
			A(
			N(),
				N(sd)
			)
		),
			A(
			A(
			A(
			A(
			A(
			A(
			N(),
				N("textureinfo")
			),
				A(
				N(),
					N(so)
				)
			),
				A(
				N(),
					N(sp)
				)
			),
				A(
				N(),
					N(sq)
				)
			),
				A(
				N(),
					N("sticktopoly 1")
				)
			),
				A(
				N(),
					N(stex)
				)
			)
		),
			A(
			N(),
				N("flags")
			)
		),
			A(
			N(),
				N("shade 0 0 0")
			)
		)
	);
}

// add vertex info to the current polyhedron
void LTAFile::AddBrushVertices( int numVerts, const Point3* verts )
{
	// add the pointlist node
	A(
	curPointList,
		N("pointlist")
	);
	
	// add the verts to the pointlist node
	for( int i = 0; i < numVerts; i++ )
	{
		TSTR vertOut;
		vertOut.printf( "%f %f %f", verts[i].x, verts[i].y, verts[i].z );
		
		A(
		curPointList,
			A(
			N(),
				N(vertOut)
			)
		);
	}
}

// save out the lta file.  returns 0 on error
int LTAFile::Output( const TSTR& fileName, const TSTR& keyFileName )
{
	bool success = false;

	success = CLTANodeWriter::SaveNode( root, fileName.data(), CLTAUtil::IsFileCompressed( fileName.data() ) );

	if( success && externalKeys )
		success = CLTANodeWriter::SaveNode( keyRoot, keyFileName.data(), CLTAUtil::IsFileCompressed( keyFileName.data() ) );

	return success;
}

