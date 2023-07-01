#include "physics.h"
#include "physics_object.h"
#include "physics_debug.h"

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif


LOG_REGISTER_CHANNEL( Physics, LogColor::DarkGreen );


extern ConVar phys_dbg;

Phys_DebugFuncs_t gDebugFuncs;
PhysDebugDraw*    gpDebugDraw = nullptr;

CONVAR( phys_fast, 0 );

// aw shit, how can make these a cheat if that's only available in the game
CONVAR( phys_collisionsteps, 1,
	"If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. "
	"Do 1 collision step per 1 / 60th of a second(round up)." );

CONVAR( phys_substeps, 1,
	"If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1." );


// Callback for traces
static void TraceCallback( const char *pFmt, ... )
{ 
	// NOTE: could probably use LogExV here, idk

	va_list list;
	va_start( list, pFmt );
	char buffer[1024];
	vsnprintf( buffer, sizeof(buffer), pFmt, list );

	Log_MsgF( gPhysicsChannel, "%s\n", pFmt );
}


// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedCallback( const char *inExpression, const char *inMessage, const char *inFile, u32 inLine )
{
	HandleAssert( inFile, inLine, inExpression, inMessage );
	return true;
}


// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).

// Function that determines if two object layers can collide
static bool MyObjectCanCollide( JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2 )
{
	switch (inObject1)
	{
		case ObjLayer_Stationary:
			return inObject2 == ObjLayer_Moving; // Non moving only collides with moving

		case ObjLayer_Moving:
			return true; // Moving collides with everything

		case ObjLayer_NoCollide:
			return false; // Nothing collides with this

		default:
			Assert( false );
			return false;
	}
};



// ====================================================================================



// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.

JPH::BroadPhaseLayer BroadPhase_Stationary(0);
JPH::BroadPhaseLayer BroadPhase_Moving(1);
JPH::BroadPhaseLayer BroadPhase_NoCollide(2);


// Function that determines if two broadphase layers can collide
static bool MyBroadPhaseCanCollide( JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2 )
{
	switch (inLayer1)
	{
		case ObjLayer_Stationary:
			return inLayer2 == BroadPhase_Moving;

		case ObjLayer_Moving:
			return true;

		case ObjLayer_NoCollide:
			return false; // Nothing collides with this

		default:
			AssertMsg( false, "Invalid Physics Object Layer" );
			return false;
	}
}


// An example contact listener
class ContactListener : public JPH::ContactListener
{
public:
	// See: ContactListener
	virtual JPH::ValidateResult	OnContactValidate( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::CollideShapeResult &inCollisionResult ) override
	{
		// LogDev( gPhysicsChannel, 1, "Contact validate callback\n" );

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings ) override
	{
		// LogDev( gPhysicsChannel, 1, "A contact was added\n" );
	}

	virtual void OnContactPersisted( const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings ) override
	{
		// LogDev( gPhysicsChannel, 1, "A contact was persisted\n" );
	}

	virtual void OnContactRemoved( const JPH::SubShapeIDPair &inSubShapePair) override
	{ 
		// LogDev( gPhysicsChannel, 1, "A contact was removed\n" );
	}
};


// An example activation listener
class BodyActivationListener : public JPH::BodyActivationListener
{
public:
	virtual void OnBodyActivated( const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData ) override
	{
		// LogDev( gPhysicsChannel, 1, "A body got activated\n" );
	}

	virtual void OnBodyDeactivated( const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData ) override
	{
		// LogDev( gPhysicsChannel, 1, "A body went to sleep\n" );
	}
};


BodyActivationListener    gBodyActivationListener;
ContactListener           gContactListener;


// ====================================================================================


// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
const u32                 gMaxBodies             = 65536;

// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
const u32 gNumBodyMutexes = 0;

// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
const u32                 gMaxBodyPairs          = 65536;

// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
const u32                 gMaxContactConstraints = 65536;


// can jolt physics even do multiple physics environments actually?
// and would you even want one?
// i mean, i know portal does that stuff so idk lol
class Physics : public Ch_IPhysics
{
public:
	bool Init() override
	{
		// Install callbacks
		JPH::Trace = TraceCallback;

		JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = AssertFailedCallback; );

		// Register allocation hook
		JPH::RegisterDefaultAllocator();

		// Create factory
		JPH::Factory::sInstance = new JPH::Factory;

		// Register all Jolt physics types
		JPH::RegisterTypes();

		// We need a temp allocator for temporary allocations during the physics update. We're
		// pre-allocating 10 MB to avoid having to do allocations during the physics update. 
		// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
		// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
		// malloc / free.
		// apAllocator = new JPH::TempAllocatorImpl( 10 * 1024 * 1024 );
		apAllocator = new JPH::TempAllocatorImpl( 70 * 1024 * 1024 );

		// We need a job system that will execute physics jobs on multiple threads. Typically
		// you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		apJobSystem = new JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1 );

		return true;
	}

	~Physics()
	{
		for ( auto env : aPhysEnvs )
		{
			delete env;
		}

		delete apAllocator;
		delete apJobSystem;
	}

	void Update( float sDT ) override
	{
	}

	void SetDebugDrawFuncs( const Phys_DebugFuncs_t& srDebug )override
	{
		gDebugFuncs = srDebug;

		if ( !gpDebugDraw )
		{
			gpDebugDraw = new PhysDebugDraw;
		}

		if ( !gpDebugDraw->aValid )
		{
			gpDebugDraw->Init();
		}
	}

	// -------------------------------------------------------------------

	IPhysicsEnvironment* CreatePhysEnv() override
	{
		PhysicsEnvironment* env = new PhysicsEnvironment;
		aPhysEnvs.push_back( env );
		return env;
	}

	void DestroyPhysEnv( IPhysicsEnvironment* pEnv ) override
	{
		if ( !pEnv )
			return;

		vec_remove_if( aPhysEnvs, pEnv );
		delete pEnv;
	}

	// -------------------------------------------------------------------

	std::vector< IPhysicsEnvironment* >     aPhysEnvs;

	// TODO: replace me with an engine threadpool
	JPH::JobSystemThreadPool*               apJobSystem;

	// also replace me
	JPH::TempAllocatorImpl*                 apAllocator;
};


Physics phys;


static ModuleInterface_t gInterfaces[] = {
	{ &phys, IPHYSICS_NAME, IPHYSICS_HASH }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* cframework_GetInterfaces( size_t& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}


// =========================================================


// KEEP IN SAME ORDER AS ShapeType ENUM
static const char* gShapeTypeStr[] = {
	"Invalid",
	"Sphere",
	"Box",
	"Capsule",
	"TaperedCapsule",
	"Cylinder",
	"Convex",
	"Mesh",
	"StaticCompound",
	"MutableCompound",
	"HeightField",
};


const char* PhysShapeType2Str( PhysShapeType type )
{
	if ( type > PhysShapeType::Max || type <= PhysShapeType::Invalid )
		return gShapeTypeStr[0];

	return gShapeTypeStr[(size_t)type];
}


#if 0
void TickCallback( btDynamicsWorld* world, btScalar timeStep )
{
	// process manifolds
	for ( uint32_t i = 0; i < physenv->apWorld->getDispatcher()->getNumManifolds(); i++ ) {
		const auto manifold = physenv->apWorld->getDispatcher()->getManifoldByIndexInternal(i);

		PhysicsObject* firstCollider = (PhysicsObject*)manifold->getBody0()->getUserPointer();
		PhysicsObject* secondCollider = (PhysicsObject*)manifold->getBody1()->getUserPointer();

		if ((!firstCollider->aPhysInfo.callbacks && !secondCollider->aPhysInfo.callbacks) ||
			(!manifold->getBody0()->isActive() && !manifold->getBody1()->isActive()))
			continue;

		ContactEvent contactEvent;
		contactEvent.first = firstCollider;
		contactEvent.second = secondCollider;

		ReadManifold( manifold, &contactEvent );

		float impulse = ReadManifold(manifold, &contactEvent);

		if ( impulse == 0 )
			continue;

		//eventsPtr->emit<ContactEvent>(contactEvent);
	}

	//eventsPtr->emit<PhysicsUpdateEvent>(PhysicsUpdateEvent{timeStep});
}
#endif


PhysicsEnvironment::PhysicsEnvironment()
{
}

PhysicsEnvironment::~PhysicsEnvironment()
{
	if ( apPhys )
		delete apPhys;
}


class ObjectToBroadphase : public JPH::BroadPhaseLayerInterface
{
public:
	/// Return the number of broadphase layers there are
	JPH::uint GetNumBroadPhaseLayers() const override
	{
		return 3;
	}

	/// Convert an object layer to the corresponding broadphase layer
	JPH::BroadPhaseLayer GetBroadPhaseLayer( JPH::ObjectLayer inLayer ) const override
	{
		switch( inLayer )
		{
			default:
			case ObjLayer_Stationary:
				return BroadPhase_Stationary;

			case ObjLayer_Moving:
				return BroadPhase_Moving;

			case ObjLayer_NoCollide:
				return BroadPhase_NoCollide;
		}
	}

#if 1 // defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	/// Get the user readable name of a broadphase layer (debugging purposes)
	virtual const char* GetBroadPhaseLayerName( JPH::BroadPhaseLayer inLayer ) const // override
	{
		if ( inLayer == BroadPhase_Stationary )
			return "Stationary";

		if ( inLayer == BroadPhase_Moving )
			return "Moving";

		if ( inLayer == BroadPhase_NoCollide )
			return "NoCollide";

		return "Unknown";
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
};


// Create mapping table from object layer to broadphase layer
ObjectToBroadphase gObjectToBroadphase;


void PhysicsEnvironment::Init()
{
	// Now we can create the actual physics system.
	apPhys = new JPH::PhysicsSystem;
	apPhys->Init( gMaxBodies, gNumBodyMutexes, gMaxBodyPairs, gMaxContactConstraints, gObjectToBroadphase, MyBroadPhaseCanCollide, MyObjectCanCollide );

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	apPhys->SetBodyActivationListener( &gBodyActivationListener );

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	apPhys->SetContactListener( &gContactListener );
}


void PhysicsEnvironment::Shutdown(  )
{
}


CONVAR( phys_dbg_wireframe, 1 );


void PhysicsEnvironment::Simulate( float sDT )
{
	PROF_SCOPE();

	// um
	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	// const float cDeltaTime = 1.0f / 60.0f;

	apPhys->Update( sDT, phys_collisionsteps, phys_substeps, phys.apAllocator, phys.apJobSystem );

	if ( !phys_dbg || !gpDebugDraw || !gpDebugDraw->aValid )
		return;

	for ( auto physObj : aPhysObjs )
	{
		if ( !physObj->aAllowDebugDraw )
			continue;

		physObj->apShape->aShape->Draw(
			gpDebugDraw,
			JPH::Mat44::sRotationTranslation( physObj->GetRotationJolt(), physObj->GetPositionJolt() ),
			// physObj->GetWorldTransformJolt(),
			{1, 1, 1},
			{1, 0, 0},
			false,
			phys_dbg_wireframe.GetBool()
		);
	}

	// if ( phys_fast )
	// 	apWorld->stepSimulation( sDT );
	// else
	// 	apWorld->stepSimulation( sDT, 100, 1 / 240.0 );
}


IPhysicsShape* PhysicsEnvironment::CreateShape( const PhysicsShapeInfo& physInfo )
{
	// The ShapeSettings object is only required for building the shape, all information is copied into the Shape class
	JPH::ShapeSettings* shapeSettings = nullptr;

	// also, all shape settings have a physics material option to go with it

	switch ( physInfo.aShapeType )
	{
		case PhysShapeType::Sphere:
			shapeSettings = new JPH::SphereShapeSettings( physInfo.aBounds.x * .5f );  // maybe don't multiply by .5f? idk
			break;

		case PhysShapeType::Box:
			shapeSettings = new JPH::BoxShapeSettings( toJolt(physInfo.aBounds) );  // float inConvexRadius
			break;

		case PhysShapeType::Capsule:
			shapeSettings = new JPH::CapsuleShapeSettings( physInfo.aBounds.x * .5f, physInfo.aBounds.y );
			break;

		// case PhysShapeType::TaperedCapsule:
		// 	break;

		case PhysShapeType::Cylinder:
			shapeSettings = new JPH::CylinderShapeSettings( physInfo.aBounds.x * .5f, physInfo.aBounds.y );  // float inConvexRadius
			break;

		case PhysShapeType::Convex:
		case PhysShapeType::Mesh:
			shapeSettings = LoadModel( physInfo );
			break;

		// case PhysShapeType::StaticCompound:
		// 	break;

		// case PhysShapeType::MutableCompound:
		// 	break;

		// case PhysShapeType::HeightField:
		// 	break;

		default:
			Log_ErrorF( gPhysicsChannel, "Physics Shape Type Not Currently Supported: %s\n", PhysShapeType2Str( physInfo.aShapeType ) );
			return nullptr;
	}

	if ( shapeSettings == nullptr )
		return nullptr;

	// Create shape
	JPH::Shape::ShapeResult result = shapeSettings->Create();
	if ( !result.IsValid() )
	{
		if ( result.HasError() )
		{
			Log_ErrorF( gPhysicsChannel, "Failed to create \"%s\" Physics Shape - %s\n",
				PhysShapeType2Str( physInfo.aShapeType ),
				result.GetError().c_str()
			);
		}
		else
		{
			Log_ErrorF( gPhysicsChannel, "Failed to create \"%s\" Physics Shape\n", PhysShapeType2Str( physInfo.aShapeType ) );
		}

		delete shapeSettings;
		return nullptr;
	}

	PhysicsShape* shape    = new PhysicsShape( physInfo.aShapeType );
	shape->apShapeSettings = shapeSettings;
	shape->aShape          = result.Get();

	return shape;
}


void PhysicsEnvironment::DestroyShape( IPhysicsShape *spShape )
{
	Assert( spShape );

	if ( !spShape )
		return;

	PhysicsShape* shape = (PhysicsShape*)spShape;

	if ( shape->apShapeSettings )
		delete shape->apShapeSettings;

	delete spShape;
}


IPhysicsObject* PhysicsEnvironment::CreateObject( IPhysicsShape* spShape, const PhysicsObjectInfo& physInfo )
{
	Assert( spShape );

	if ( !spShape )
		return nullptr;

	JPH::BodyInterface &bodyInterface = apPhys->GetBodyInterface();

	PhysicsShape* physShape = (PhysicsShape*)spShape;

	JPH::EMotionType motionType = (JPH::EMotionType)physInfo.aMotionType;

	JPH::ObjectLayer layer = physInfo.aMotionType == PhysMotionType::Static ? ObjLayer_Stationary : ObjLayer_Moving;

	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	JPH::BodyCreationSettings bodySettings(
		physShape->aShape,
		toJolt( physInfo.aPos ),
		toJoltRot( physInfo.aAng ),
		motionType,
		layer
	);

	bodySettings.mAllowSleeping      = physInfo.aAllowSleeping;
	bodySettings.mIsSensor           = physInfo.aIsSensor;
	bodySettings.mMaxLinearVelocity  = physInfo.aMaxLinearVelocity;
	bodySettings.mMaxAngularVelocity = physInfo.aMaxAngularVelocity;
	bodySettings.mMotionQuality      = (JPH::EMotionQuality)physInfo.aMotionQuality;

	if ( physInfo.aCustomMass )
	{
		bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		bodySettings.mMassPropertiesOverride.mMass = physInfo.aMass;
	}

	// TODO: check for any mass override

	// Create the actual rigid body
	JPH::Body *body = bodyInterface.CreateBody( bodySettings ); // Note that if we run out of bodies this can return nullptr

	if ( body == nullptr )
	{
		Log_Error( gPhysicsChannel, "Failed to create physics body\n" );
		return nullptr;
	}

	PhysicsObject* phys = new PhysicsObject;
	phys->aPhysInfo     = physInfo;
	phys->apShape       = physShape;
	phys->apBody        = body;
	phys->apEnv         = this;
	phys->aLayer        = layer;
	phys->aOrigLayer    = layer;

	// Add it to the world
	bodyInterface.AddBody( body->GetID(), physInfo.aStartActive ? JPH::EActivation::Activate : JPH::EActivation::DontActivate );

	// REMOVE ME:
	aPhysObjs.push_back( phys );

	return phys;
}


void PhysicsEnvironment::DestroyObject( IPhysicsObject *spPhysObj )
{
	Assert( spPhysObj );

	if ( !spPhysObj )
		return;

	PhysicsObject* physObj = (PhysicsObject*)spPhysObj;

	if ( physObj->apBody )
	{
		JPH::BodyInterface &bodyInterface = apPhys->GetBodyInterface();
		bodyInterface.RemoveBody( physObj->apBody->GetID() );
		bodyInterface.DestroyBody( physObj->apBody->GetID() );
	}

	vec_remove( aPhysObjs, physObj );

	delete physObj;
}


#if 0
// this is stupid, graphics2 will do this better, or i might just try doing it better in graphics1
void GetModelVerts( Model* spModel, std::vector< JPH::Vec3 >& srVertices )
{
	PROF_SCOPE();

	for ( size_t s = 0; s < spModel->GetSurfaceCount(); s++ )
	{
		auto& ind = spModel->GetSurfaceIndices( s );
		auto& vertData = spModel->GetSurfaceVertexData( s );

		float* data = nullptr;
		for ( auto& attrib : vertData.aData )
		{
			if ( attrib.aAttrib == VertexAttribute_Position )
			{
				data = (float*)attrib.apData;
				break;
			}
		}

		if ( ind.size() )
		{
			for ( size_t i = 0; i < ind.size(); i++ )
			{
				size_t i0 = ind[i] * 3;
				srVertices.emplace_back( data[i0], data[i0 + 1], data[i0 + 2] );
			}
		}
		else
		{
			for ( size_t i = 0; i < vertData.aCount * 3; )
			{
				srVertices.emplace_back( data[i++], data[i++], data[i++] );
			}
		}
	}
}


void GetModelTris( Model* spModel, std::vector< JPH::Triangle >& srTris )
{
	PROF_SCOPE();

	for ( size_t s = 0; s < spModel->GetSurfaceCount(); s++ )
	{
		auto& ind = spModel->GetSurfaceIndices( s );
		auto& vertData = spModel->GetSurfaceVertexData( s );

		float* data = nullptr;
		for ( auto& attrib : vertData.aData )
		{
			if ( attrib.aAttrib == VertexAttribute_Position )
			{
				data = (float*)attrib.apData;
				break;
			}
		}

		// shouldn't be using this function if we have indices lol
		// could even calculate them in GetModelInd as well
		if ( ind.size() )
		{
			for ( size_t i = 0; i < ind.size(); i += 3)
			{
				size_t i0 = ind[i + 0] * 3;
				size_t i1 = ind[i + 1] * 3;
				size_t i2 = ind[i + 2] * 3;

				JPH::Vec3 vec0 = {data[i0], data[i0 + 1], data[i0 + 2]};
				JPH::Vec3 vec1 = {data[i1], data[i1 + 1], data[i1 + 2]};
				JPH::Vec3 vec2 = {data[i2], data[i2 + 1], data[i2 + 2]};

				srTris.emplace_back( vec0, vec1, vec2 );
			}
		}
		else
		{
			for ( size_t i = 0; i < vertData.aCount * 3; )
			{
				JPH::Vec3 vec0 = {data[i++], data[i++], data[i++]};
				JPH::Vec3 vec1 = {data[i++], data[i++], data[i++]};
				JPH::Vec3 vec2 = {data[i++], data[i++], data[i++]};

				srTris.emplace_back( vec0, vec1, vec2 );
			}
		}
	}
}


void GetModelInd( Model* spModel, std::vector< JPH::Float3 >& srVerts, std::vector< JPH::IndexedTriangle >& srInd )
{
	PROF_SCOPE();

	JPH::uint32 origSize = (JPH::uint32)srVerts.size();

	for ( size_t s = 0; s < spModel->GetSurfaceCount(); s++ )
	{
		auto& ind = spModel->GetSurfaceIndices( s );
		auto& vertData = spModel->GetSurfaceVertexData( s );

		float* data = nullptr;
		for ( auto& attrib : vertData.aData )
		{
			if ( attrib.aAttrib == VertexAttribute_Position )
			{
				data = (float*)attrib.apData;
				break;
			}
		}

		origSize = (JPH::uint32)srVerts.size();

		for ( size_t i = 0; i < vertData.aCount * 3; i += 3 )
		{
			srVerts.emplace_back( data[i+0], data[i+1], data[i+2] );
		}

		for ( size_t i = 0; i < ind.size(); i += 3 )
		{
			srInd.emplace_back(
				origSize + ind[i + 0],
				origSize + ind[i + 1],
				origSize + ind[i + 2]
			);
		}
	}
}
#endif


JPH::ShapeSettings* PhysicsEnvironment::LoadModel( const PhysicsShapeInfo& physInfo )
{
#if 1
	// Assert( physInfo.aMeshData.apModel != nullptr );

	switch ( physInfo.aShapeType )
	{
		case PhysShapeType::Convex:
		{
			if ( physInfo.aConvexData.apVertices == nullptr )
				return nullptr;

			// Create an array of vertices
			JPH::Array< JPH::Vec3 > vertices( physInfo.aConvexData.aVertCount );

			// same thing
			memcpy( vertices.data(), physInfo.aConvexData.apVertices, physInfo.aConvexData.aVertCount * sizeof( glm::vec3 ) );

			// GetModelVerts( physInfo.aMeshData.apModel, vertices );

			if ( vertices.empty() )
			{
				// Log_Warn( gPhysicsChannel, "No vertices in model? - \"%s\"\n", physInfo.aMeshData.apModel->aPath.c_str() );
				Log_Warn( gPhysicsChannel, "No vertices in model?\n" );
				return nullptr;
			}

			// NOTE: is a hull the best idea? it's what they used on the main wiki page so idk
			return new JPH::ConvexHullShapeSettings( vertices, JPH::cDefaultConvexRadius );
		}

		case PhysShapeType::Mesh:
		{
#if 1
			if ( physInfo.aConcaveData.aTris == nullptr || physInfo.aConcaveData.apVertices == nullptr )
				return nullptr;

			JPH::Array< JPH::Float3 >          verts( physInfo.aConcaveData.aVertCount );
			JPH::Array< JPH::IndexedTriangle > ind( physInfo.aConcaveData.aTriCount );

			// same thing
			memcpy( verts.data(), physInfo.aConcaveData.apVertices, physInfo.aConcaveData.aVertCount * sizeof( glm::vec3 ) );

			for ( u32 i = 0; i < physInfo.aConcaveData.aTriCount; i++ )
			{
				if ( physInfo.aConcaveData.aTris[ i ].aPos[ 0 ] > physInfo.aConcaveData.aVertCount )
				{
					Log_Error( "AAA\n" );
				}

				ind[ i ] = {
					physInfo.aConcaveData.aTris[ i ].aPos[ 0 ],
					physInfo.aConcaveData.aTris[ i ].aPos[ 1 ],
					physInfo.aConcaveData.aTris[ i ].aPos[ 2 ]
				};
			}

			// GetModelInd( physInfo.aMeshData.apModel, verts, ind );

			// if ( ind.empty() )
			// {
			// 	Log_WarnF( gPhysicsChannel, "No vertices in model? - \"%s\"\n", graphics->GetModelPath( physInfo.aMeshData.apModel ).c_str() );
			// 	return nullptr;
			// }

			return new JPH::MeshShapeSettings( verts, ind );
#else
			std::vector< JPH::Triangle > tris;

			GetModelTris( physInfo.aMeshData.apModel, tris );

			if ( tris.empty() )
			{
				Log_Warn( gPhysicsChannel, "No vertices in model? - \"%s\"\n", graphics->GetModelPath( physInfo.aMeshData.apModel ).c_str() );
				return nullptr;
			}

			return new JPH::MeshShapeSettings( tris );
#endif
		}

		default:
			Log_ErrorF( gPhysicsChannel, "Invalid Shape Type Being Passed into PhysicsEnvironment::LoadModel: %s\n", PhysShapeType2Str( physInfo.aShapeType ) );
			return nullptr;
	}
#endif
	return nullptr;
}


void PhysicsEnvironment::SetGravity( const glm::vec3& gravity )
{
	apPhys->SetGravity( toJolt( gravity ) );
}


void PhysicsEnvironment::SetGravityY( float gravity )
{
	apPhys->SetGravity( JPH::Vec3( 0, gravity, 0 ) );
}

void PhysicsEnvironment::SetGravityZ( float gravity )
{
	apPhys->SetGravity( JPH::Vec3( 0, 0, gravity ) );
}


glm::vec3 PhysicsEnvironment::GetGravity()
{
	return fromJolt( apPhys->GetGravity() );
}


// UNFINISHED !!!!
#if 0
void PhysicsEnvironment::RayTest( const glm::vec3& from, const glm::vec3& to, std::vector< RayHit* >& hits )
{
	btVector3 btFrom( toBt(from) );
	btVector3 btTo( toBt(to) );
	btCollisionWorld::AllHitsRayResultCallback res( btFrom, btTo );

	// res.m_flags |= btTriangleRaycastCallback::kF_KeepUnflippedNormal;
	// res.m_flags |= btTriangleRaycastCallback::kF_UseSubSimplexConvexCastRaytest;

	apWorld->rayTest( btFrom, btTo, res );

	hits.reserve( res.m_collisionObjects.size() );

	for ( uint32_t i = 0; i < res.m_collisionObjects.size(); i++ )
	{
		// PhysicsObject* physObj = (PhysicsObject*)res.m_collisionObjects[i]->getUserPointer();
		// RayHit* hit = new RayHit;

		//hits.push_back( hit );
	}
}
#endif

