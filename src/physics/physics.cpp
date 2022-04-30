#include "physics.h"
#include "physics_object.h"
#include "physics_debug.h"


LOG_REGISTER_CHANNEL( Physics, LogColor::DarkGreen );


extern ConVar phys_dbg;

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

	LogMsg( gPhysicsChannel, "%s\n", pFmt );
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
	virtual void OnBodyActivated( const JPH::BodyID &inBodyID, void* inBodyUserData ) override
	{
		// LogDev( gPhysicsChannel, 1, "A body got activated\n" );
	}

	virtual void OnBodyDeactivated( const JPH::BodyID &inBodyID, void* inBodyUserData ) override
	{
		// LogDev( gPhysicsChannel, 1, "A body went to sleep\n" );
	}
};


BodyActivationListener    gBodyActivationListener;
ContactListener           gContactListener;


// Program entry point
#if 0
int main(int argc, char** argv)
{
	// Install callbacks
	JPH::Trace = TraceCallback;

	JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = AssertFailedCallback; );

	// Register all Jolt physics types
	JPH::RegisterTypes();

	// We need a temp allocator for temporary allocations during the physics update. We're
	// pre-allocating 10 MB to avoid having to do allocations during the physics update. 
	// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	// malloc / free.
	JPH::TempAllocatorImpl temp_allocator(10 * 1024 * 1024);

	// We need a job system that will execute physics jobs on multiple threads. Typically
	// you would implement the JobSystem interface yourself and let Jolt Physics run on top
	// of your own job scheduler. JobSystemThreadPool is an example implementation.
	JPH::JobSystemThreadPool job_system( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1 );

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const u32 cMaxBodies = 1024;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const u32 cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const u32 cMaxBodyPairs = 1024;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const u32 cMaxContactConstraints = 1024;

	// Create mapping table from object layer to broadphase layer
	JPH::ObjectToBroadPhaseLayer object_to_broadphase;
	object_to_broadphase.resize( (size_t)ObjLayer_Count );
	object_to_broadphase[ObjLayer_Stationary] = BroadPhase_Stationary;
	object_to_broadphase[ObjLayer_Moving] = BroadPhase_Moving;

	// Now we can create the actual physics system.
	JPH::PhysicsSystem physics_system;
	physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, object_to_broadphase, MyBroadPhaseCanCollide, MyObjectCanCollide);

	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	MyBodyActivationListener body_activation_listener;
	physics_system.SetBodyActivationListener(&body_activation_listener);

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	MyContactListener contact_listener;
	physics_system.SetContactListener(&contact_listener);

	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	JPH::BodyInterface &body_interface = physics_system.GetBodyInterface();

	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape). 
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	JPH::BoxShapeSettings floor_shape_settings( JPH::Vec3(100.0f, 1.0f, 100.0f) );

	// Create the shape
	JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	JPH::ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

													  // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	JPH::BodyCreationSettings floor_settings( floor_shape, JPH::Vec3(0.0f, -1.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, ObjLayer_Stationary );

	// Create the actual rigid body
	JPH::Body *floor = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

															 // Add it to the world
	body_interface.AddBody( floor->GetID(), JPH::EActivation::DontActivate );

	// Now create a dynamic body to bounce on the floor
	// Note that this uses the shorthand version of creating and adding a body to the world
	JPH::BodyCreationSettings sphere_settings( new JPH::SphereShape(0.5f), JPH::Vec3(0.0f, 2.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, ObjLayer_Moving );
	JPH::BodyID sphere_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

	// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
	// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
	body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, -5.0f, 0.0f));

	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	const float cDeltaTime = 1.0f / 60.0f;

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	physics_system.OptimizeBroadPhase();

	// Now we're ready to simulate the body, keep simulating until it goes to sleep
	JPH::uint step = 0;
	while (body_interface.IsActive(sphere_id))
	{
		// Next step
		++step;

		// Output current position and velocity of the sphere
		JPH::Vec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
		JPH::Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);

		LogDev( gPhysicsChannel, 1,
			"Step %d: Position = ( %d, %d, %d ), Velocity = ( %d, %d, %d )\n",
			step,
			position.GetX(), position.GetY(), position.GetZ(),
			velocity.GetX(), velocity.GetY(), velocity.GetZ()
		);

		// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
		const int cCollisionSteps = 1;

		// If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.
		const int cIntegrationSubSteps = 1;

		// Step the world
		physics_system.Update(cDeltaTime, cCollisionSteps, cIntegrationSubSteps, &temp_allocator, &job_system);
	}

	// Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
	body_interface.RemoveBody(sphere_id);

	// Destroy the sphere. After this the sphere ID is no longer valid.
	body_interface.DestroyBody(sphere_id);

	// Remove and destroy the floor
	body_interface.RemoveBody(floor->GetID());
	body_interface.DestroyBody(floor->GetID());

	return 0;
}
#endif


// ====================================================================================


// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
const u32 gMaxBodies = 1024;

// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
const u32 gNumBodyMutexes = 0;

// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
const u32 gMaxBodyPairs = 1024;

// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
const u32 gMaxContactConstraints = 1024;


// can jolt physics even do multiple physics environments actually?
// and would you even want one?
// i mean, i know portal does that stuff so idk lol
class Physics : public Ch_IPhysics
{
public:
	void Init() override
	{
		GET_SYSTEM_ASSERT( graphics, BaseGraphicsSystem );

		// Install callbacks
		JPH::Trace = TraceCallback;

		JPH_IF_ENABLE_ASSERTS( JPH::AssertFailed = AssertFailedCallback; );

		// Register all Jolt physics types
		JPH::RegisterTypes();

		// We need a temp allocator for temporary allocations during the physics update. We're
		// pre-allocating 10 MB to avoid having to do allocations during the physics update. 
		// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
		// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
		// malloc / free.
		apAllocator = new JPH::TempAllocatorImpl( 10 * 1024 * 1024 );

		// We need a job system that will execute physics jobs on multiple threads. Typically
		// you would implement the JobSystem interface yourself and let Jolt Physics run on top
		// of your own job scheduler. JobSystemThreadPool is an example implementation.
		apJobSystem = new JPH::JobSystemThreadPool( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1 );
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

	// -------------------------------------------------------------------

	IPhysicsEnvironment* CreatePhysEnv() override
	{
		return aPhysEnvs.emplace_back( new PhysicsEnvironment );
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


extern "C" {
	DLL_EXPORT void* cframework_get() {
		return &phys;
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


// Create mapping table from object layer to broadphase layer
JPH::ObjectToBroadPhaseLayer gObjectToBroadphase = {
	BroadPhase_Stationary,
	BroadPhase_Moving,
	BroadPhase_NoCollide,
};


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

	apDebugDraw = new PhysDebugDraw;
	// apDebugDraw->setDebugMode( btIDebugDraw::DBG_NoDebug );
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
	const float cDeltaTime = 1.0f / 60.0f;

	apPhys->Update( sDT, phys_collisionsteps, phys_substeps, phys.apAllocator, phys.apJobSystem );

	if ( phys_dbg )
	{
		for ( auto physObj : aPhysObjs )
		{
			physObj->apShape->aShape->Draw(
				apDebugDraw,
				JPH::Mat44::sRotationTranslation( physObj->GetRotationJolt(), physObj->GetPositionJolt() ),
				{1, 1, 1},
				{1, 0, 0},
				false,
				phys_dbg_wireframe.GetBool()
			);
		}

		// shape->Draw(mDebugRenderer, shape_transform, Vec3::sReplicate(1.0f), had_hit? Color::sGreen : Color::sGrey, false, false);
	}

	// if ( phys_fast )
	// 	apWorld->stepSimulation( sDT );
	// else
	// 	apWorld->stepSimulation( sDT, 100, 1 / 240.0 );

	// if ( phys_dbg )
	//	apWorld->debugDrawWorld();
}


IPhysicsShape* PhysicsEnvironment::CreateShape( const PhysicsShapeInfo& physInfo )
{
	PROF_SCOPE();

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
			LogError( gPhysicsChannel, "Physics Shape Type Not Currently Supported: %s\n", PhysShapeType2Str( physInfo.aShapeType ) );
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
			LogError( gPhysicsChannel, "Failed to create \"%s\" Physics Shape - %s\n",
				PhysShapeType2Str( physInfo.aShapeType ),
				result.GetError().c_str()
			);
		}
		else
		{
			LogError( gPhysicsChannel, "Failed to create \"%s\" Physics Shape\n", PhysShapeType2Str( physInfo.aShapeType ) );
		}
		// TODO: use HasError() and GetError() on the result
		delete shapeSettings;
		return nullptr;
	}

	PhysicsShape* shape = new PhysicsShape( physInfo );
	shape->apShapeSettings = shapeSettings;
	shape->aShape = result.Get();

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
	PROF_SCOPE();

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

	bodySettings.mAllowSleeping             = physInfo.aAllowSleeping;
	bodySettings.mIsSensor                  = physInfo.aIsSensor;
	bodySettings.mMaxLinearVelocity         = physInfo.aMaxLinearVelocity;
	bodySettings.mMaxAngularVelocity        = physInfo.aMaxAngularVelocity;
	bodySettings.mMotionQuality             = (JPH::EMotionQuality)physInfo.aMotionQuality;

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
		LogError( gPhysicsChannel, "Failed to create physics body\n" );
		return nullptr;
	}

	PhysicsObject* phys = new PhysicsObject;
	phys->aPhysInfo = physInfo;
	phys->apShape = physShape;
	phys->apBody = body;
	phys->apEnv = this;
	phys->aLayer = layer;
	phys->aOrigLayer = layer;

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


// this is stupid, graphics2 will do this better, or i might just try doing it better in graphics1
void GetModelVerts( Model* spModel, std::vector< JPH::Vec3 >& srVertices )
{
	PROF_SCOPE();

	auto& verts = spModel->GetVertices();
	auto& ind = spModel->GetIndices();

	size_t origSize = srVertices.size();
	srVertices.resize( srVertices.size() + ind.size() );
			
	for ( size_t i = 0; i < ind.size(); i++ )
	{
		srVertices[origSize + i] = toJolt( verts[i].pos );
	}
}


void GetModelTris( Model* spModel, std::vector< JPH::Triangle >& srTris )
{
	PROF_SCOPE();

	auto& verts = spModel->GetVertices();
	auto& ind = spModel->GetIndices();

	// size_t origSize = srTris.size();
	// srTris.reserve( srTris.size() + ind.size() );

	for ( size_t i = 0; i < ind.size(); i += 3 )
	{
		srTris.emplace_back(
			toJolt( verts[ind[i]  ].pos ),
			toJolt( verts[ind[i+1]].pos ),
			toJolt( verts[ind[i+2]].pos )
		);
	}

#if 0
	for (int i = 0; i < physInfo.indices.size() / 3; i++)
		//for (int i = 0; i < model.aIndices.size() / 3; i++)
	{
		//glm::vec3 v0 = model.aVertices[ model.aIndices[i * 3]     ].pos;
		//glm::vec3 v1 = model.aVertices[ model.aIndices[i * 3 + 1] ].pos;
		//glm::vec3 v2 = model.aVertices[ model.aIndices[i * 3 + 2] ].pos;

		glm::vec3 v0 = physInfo.vertices[ physInfo.indices[i * 3]     ].pos;
		glm::vec3 v1 = physInfo.vertices[ physInfo.indices[i * 3 + 1] ].pos;
		glm::vec3 v2 = physInfo.vertices[ physInfo.indices[i * 3 + 2] ].pos;

		meshInterface->addTriangle(btVector3(v0[0], v0[1], v0[2]),
			btVector3(v1[0], v1[1], v1[2]),
			btVector3(v2[0], v2[1], v2[2]));
	}
#endif
}


// something is very broken here
void GetModelInd( Model* spModel, std::vector< JPH::Float3 >& srVerts, std::vector< JPH::IndexedTriangle >& srInd )
{
	PROF_SCOPE();

	auto& verts = spModel->GetVertices();
	auto& ind = spModel->GetIndices();

	JPH::uint32 origSize = (JPH::uint32)srVerts.size();

	// TODO: do these faster, memcpy the indices maybe?
	srVerts.resize( origSize + verts.size() );
	// std::memcpy( srVerts.data() + origSize, verts.data(), verts.size() * sizeof( );

	for ( size_t i = 0; i < verts.size(); i++ )
	{
		// srVerts.emplace_back( toJoltFl( verts[i].pos ) );
		srVerts[origSize + i] = toJoltFl( verts[i].pos );
	}

	for ( size_t i = 0; i < ind.size(); i += 3 )
	{
		srInd.emplace_back(
			origSize + ind[i],
			origSize + ind[i + 1],
			origSize + ind[i + 2]
		);
	}
}


JPH::ShapeSettings* PhysicsEnvironment::LoadModel( const PhysicsShapeInfo& physInfo )
{
	PROF_SCOPE();

	Assert( physInfo.aMeshData.apModel != nullptr );

	if ( physInfo.aMeshData.apModel == nullptr )
		return nullptr;

	switch ( physInfo.aShapeType )
	{
		case PhysShapeType::Convex:
		{
			// Create an array of vertices
			std::vector< JPH::Vec3 > vertices;
			GetModelVerts( physInfo.aMeshData.apModel, vertices );

			if ( vertices.empty() )
			{
				// LogWarn( gPhysicsChannel, "No vertices in model? - \"%s\"\n", physInfo.aMeshData.apModel->aPath.c_str() );
				LogWarn( gPhysicsChannel, "No vertices in model?\n" );
				return nullptr;
			}

			// NOTE: is a hull the best idea? it's what they used on the main wiki page so idk
			return new JPH::ConvexHullShapeSettings( vertices, JPH::cDefaultConvexRadius );
		}

		case PhysShapeType::Mesh:
		{
#if 1
			std::vector< JPH::Float3 > verts;
			std::vector< JPH::IndexedTriangle > ind;

			GetModelInd( physInfo.aMeshData.apModel, verts, ind );

			if ( ind.empty() )
			{
				// LogWarn( gPhysicsChannel, "No vertices in model? - \"%s\"\n", physInfo.aMeshData.apModel->aPath.c_str() );
				LogWarn( gPhysicsChannel, "No vertices in model?\n" );
				return nullptr;
			}

			return new JPH::MeshShapeSettings( verts, ind );
#else
			std::vector< JPH::Triangle > tris;

			GetModelTris( physInfo.aMeshData.apModel, tris );

			if ( tris.empty() )
			{
				LogWarn( gPhysicsChannel, "No vertices in model? - \"%s\"\n", physInfo.aMeshData.apModel->aPath.c_str() );
				return nullptr;
			}

			return new JPH::MeshShapeSettings( tris );
#endif
		}

		default:
			LogError( gPhysicsChannel, "Invalid Shape Type Being Passed into PhysicsEnvironment::LoadModel: %s\n", PhysShapeType2Str( physInfo.aShapeType ) );
			return nullptr;
	}
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

