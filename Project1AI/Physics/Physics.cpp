#include "Physics.h"


cPhysics::cPhysics()
{
	//this->m_Gravity = glm::vec3(0.0f, 0.0f, 0.0f);
	this->m_Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
	//this->m_Gravity = glm::vec3(0.0f, -1.0f, 0.0f);
	return;
}

Derivative cPhysics::evaluate(const State& initial, glm::vec3 gravity, float dt, const Derivative& d)
{
	State state;
	state.pos = initial.pos + d.dx * dt;
	state.vel = initial.vel + d.dv * dt;

	Derivative output;
	output.dx = state.vel;
	output.dv = gravity;
	return output;
}

glm::vec3 cPhysics::acceleration(const State& state, double t)
{
	const float k = 15.0f;
	const float b = 0.1f;
	glm::vec3 accl;
	accl.x = -k * state.pos.x - b * state.vel.x;
	accl.y = -k * state.pos.y - b * state.vel.y;
	accl.z = -k * state.pos.z - b * state.vel.z;
	return accl;
}

void cPhysics::integrate(std::vector<cGameObject*>& vec_pGameObjects, float dt)
{
	Derivative a, b, c, d;
	for (int i = 0; i < vec_pGameObjects.size(); i++)
	{

		cGameObject* curObj = vec_pGameObjects[i];

		State state;
		state.pos = curObj->positionXYZ;
		state.vel = curObj->velocity;

		glm::vec3 gravity = curObj->velocity + curObj->accel ; //getGravity();

		if(curObj->inverseMass == 1)
		{
		a = evaluate(state, gravity, 0.0f, Derivative());
		b = evaluate(state, gravity, dt * 0.5f, a);
		c = evaluate(state, gravity, dt * 0.5f, b);
		d = evaluate(state, gravity, dt, c);

		float dxdtX = 1.0f / 6.0f *
			(a.dx.x + 2.0f * (b.dx.x + c.dx.x) + d.dx.x);
		float dxdtY = 1.0f / 6.0f *
			(a.dx.y + 2.0f * (b.dx.y + c.dx.y) + d.dx.y);
		float dxdtZ = 1.0f / 6.0f *
			(a.dx.z + 2.0f * (b.dx.z + c.dx.z) + d.dx.z);

		float dvdtX = 1.0f / 6.0f *
			(a.dv.x + 2.0f * (b.dv.x + c.dv.x) + d.dv.x);
		float dvdtY = 1.0f / 6.0f *
			(a.dv.y + 2.0f * (b.dv.y + c.dv.y) + d.dv.y);
		float dvdtZ = 1.0f / 6.0f *
			(a.dv.z + 2.0f * (b.dv.z + c.dv.z) + d.dv.z);

		state.pos.x = state.pos.x + dxdtX * dt;
		state.pos.y = state.pos.y + dxdtY * dt;
		state.pos.z = state.pos.z + dxdtZ * dt;

		state.vel.x = state.vel.x + dvdtX * dt;
		state.vel.y = state.vel.y + dvdtY * dt;
		state.vel.z = state.vel.z + dvdtZ * dt;

		curObj->positionXYZ = state.pos;
		curObj->velocity = state.vel;
	}
	}
}

void cPhysics::bulletHolder(std::vector<cGameObject*>& vec_pGameObjects)
{
	for (int i = 0; i < vec_pGameObjects.size(); i++)
	{
		if (vec_pGameObjects[i]->friendlyName == "bullet1" || vec_pGameObjects[i]->friendlyName == "bullet2" || vec_pGameObjects[i]->friendlyName == "bullet3")
		{
			if(!vec_pGameObjects[i]->bulletFired)
			vec_pGameObjects[i]->positionXYZ = vec_pGameObjects[4]->positionXYZ;
		}
	}
}


void cPhysics::setGravity(glm::vec3 newGravityValue)
{
	this->m_Gravity = newGravityValue;
	return;
}

glm::vec3 cPhysics::getGravity(void)
{
	return this->m_Gravity;
}


void cPhysics::IntegrationStep(std::vector<cGameObject*>& vec_pGameObjects, float deltaTime)		
{


	for (unsigned int index = 0;
		index != vec_pGameObjects.size(); index++)
	{
		// Get a pointer to the current object (makes the code a little clearer)
		cGameObject* pCurObj = vec_pGameObjects[index];

		if (pCurObj->inverseMass != 0.0f)
		{

			// Forward Explicit Euler Inetegration
			//NewVelocty += Velocity + ( Ax * DeltaTime )

			// 
			pCurObj->accel = glm::vec3(getGravity());
			


			pCurObj->velocity += pCurObj->accel * deltaTime;
			//		// Or you can do this...
			//		CurObj.velocity += CurObj.accel * deltaTime;

					//NewPosition = Posistion + ( Vx * DeltaTime )

			//pCurObj->prevPositionXYZ = pCurObj->positionXYZ;
			pCurObj->positionXYZ.x += pCurObj->velocity.x * deltaTime;
			pCurObj->positionXYZ.y += pCurObj->velocity.y * deltaTime;
			pCurObj->positionXYZ.z += pCurObj->velocity.z * deltaTime;


			

		}
	}//for (unsigned int index = 0;

	return;
}



// Returns all the triangles and the closest points
void cPhysics::GetClosestTriangleToPoint(Point pointXYZ, cMesh& mesh, glm::vec3& closestPoint, sPhysicsTriangle& closestTriangle)
{

	// Assume the closest distance is REALLY far away
	float closestDistanceSoFar = FLT_MAX;


	for (unsigned int triIndex = 0;
		triIndex != mesh.vecTriangles.size();
		triIndex++)
	{
		sPlyTriangle& curTriangle = mesh.vecTriangles[triIndex];

		// Get the vertices of the triangle
		sPlyVertexXYZ_N_UV triVert1 = mesh.vecVertices[curTriangle.vert_index_1];
		sPlyVertexXYZ_N_UV triVert2 = mesh.vecVertices[curTriangle.vert_index_2];
		sPlyVertexXYZ_N_UV triVert3 = mesh.vecVertices[curTriangle.vert_index_3];

		Point triVertPoint1;
		triVertPoint1.x = triVert1.x;
		triVertPoint1.y = triVert1.y;
		triVertPoint1.z = triVert1.z;

		Point triVertPoint2;
		triVertPoint2.x = triVert2.x;
		triVertPoint2.y = triVert2.y;
		triVertPoint2.z = triVert2.z;

		Point triVertPoint3;
		triVertPoint3.x = triVert3.x;
		triVertPoint3.y = triVert3.y;
		triVertPoint3.z = triVert3.z;

		glm::vec3 curClosetPoint = ClosestPtPointTriangle(pointXYZ,
			triVertPoint1, triVertPoint2, triVertPoint3);



		// Is this the closest so far?
		float distanceNow = glm::distance(curClosetPoint, pointXYZ);

		// is this closer than the closest distance
		if (distanceNow <= closestDistanceSoFar)
		{
			closestDistanceSoFar = distanceNow;

			closestPoint = curClosetPoint;

			// Copy the triangle information over, as well
			closestTriangle.verts[0].x = triVert1.x;
			closestTriangle.verts[0].y = triVert1.y;
			closestTriangle.verts[0].z = triVert1.z;
			closestTriangle.verts[1].x = triVert2.x;
			closestTriangle.verts[1].y = triVert2.y;
			closestTriangle.verts[1].z = triVert2.z;
			closestTriangle.verts[2].x = triVert3.x;
			closestTriangle.verts[2].y = triVert3.y;
			closestTriangle.verts[2].z = triVert3.z;

			// TODO: Copy the normal, too	
			// Quick is to average the normal of all 3 vertices
			glm::vec3 triVert1Norm = glm::vec3(triVert1.nx, triVert1.ny, triVert1.nz);
			glm::vec3 triVert2Norm = glm::vec3(triVert2.nx, triVert2.ny, triVert2.nz);
			glm::vec3 triVert3Norm = glm::vec3(triVert3.nx, triVert3.ny, triVert3.nz);

			// Average of the vertex normals... 
			closestTriangle.normal = (triVert1Norm + triVert2Norm + triVert3Norm) / 3.0f;

		}

	}//for (unsigned int triIndex = 0;

	return;
}

// Will return the closest triangles that are within the range "distanceRange".
// This can be used as a "closest triangles to sphere"
void cPhysics::GetClosestTrianglesToSphere(cGameObject& testSphere, float distanceRange, cMesh& mesh, std::vector<sPhysicsTriangle>& vecClosestTriangles)
{



}

void cPhysics::CheckIfCrossedEndBound(std::vector<cGameObject*>& vec_pGameObjects)		
{
	float randX = randInRange<float>(-50.0f, 50.0f);
	float randY = randInRange<float>(5.0f, 10.0f);
	float randZ = randInRange<float>(-50.0f, 50.0f);

	for (int index = 0; index != vec_pGameObjects.size(); index++)
	{
		
		if (vec_pGameObjects[index]->positionXYZ.x > 256 || vec_pGameObjects[index]->positionXYZ.x < -256)
		{
			vec_pGameObjects[index]->positionXYZ = glm::vec3(randX, randY, randZ);
			vec_pGameObjects[index]->velocity = glm::vec3(0, 0, 0);
		}
		if (vec_pGameObjects[index]->positionXYZ.z > 256 || vec_pGameObjects[index]->positionXYZ.z < -256)
		{
			vec_pGameObjects[index]->positionXYZ = glm::vec3(randX, randY, randZ);
			vec_pGameObjects[index]->velocity = glm::vec3(0, 0, 0);
		}
	}
}

// Test each object with every other object
void cPhysics::TestForCollisions(std::vector<cGameObject*>& vec_pGameObjects)			
{
	// This will store all the collisions in this frame
	std::vector<sCollisionInfo> vecCollisions;

	sCollisionInfo collisionInfo;

	for (unsigned int outerLoopIndex = 0;
		outerLoopIndex < vec_pGameObjects.size(); outerLoopIndex++)
	{
		for (unsigned int innerLoopIndex = outerLoopIndex;
			innerLoopIndex < vec_pGameObjects.size(); innerLoopIndex++)
		{
			cGameObject* pA = vec_pGameObjects[outerLoopIndex];
			cGameObject* pB = vec_pGameObjects[innerLoopIndex];



			// Note that if you don't respond to the 
			// collision here, then you will get the same
			// result twice (Object "A" with "B" and later, 
			//   object "B" with "A" - but it's the same collison

			// Compare the two objects:
			// Either a sphere-sphere or sphere-mesh
			// An I testing the object with itself? 
			//if (pA == pB)
			if (pA->getUniqueID() == pB->getUniqueID())
			{
				// It's the same object
				// Do nothing
			}
			else if (pA->physicsShapeType == SPHERE &&
				pB->physicsShapeType == SPHERE)
			{
				if (DoSphereSphereCollisionTest(pA, pB, collisionInfo))			
				{
					vecCollisions.push_back(collisionInfo);
				}
			}
			else if ((pA->physicsShapeType == SPHERE &&
				pB->physicsShapeType == MESH)/*||(pA->physicsShapeType == MESH &&pB->physicsShapeType==SPHERE)*/)
			{
				if (DoShphereMeshCollisionTest(pA, pB, collisionInfo))			
				{
					vecCollisions.push_back(collisionInfo);
				}
			}


		}//for (unsigned int innerLoopIndex = 0;
	}//for (unsigned int outerLoopIndex = 0;

}

bool cPhysics::DoSphereSphereCollisionTest(cGameObject* pA, cGameObject* pB,
	sCollisionInfo& collisionInfo)
{
	////new code
	//glm::vec3 U1x, U1y, U2x, U2y, V1x, V1y, V2x, V2y;

	//float m1, m2, x1, x2;
	//glm::vec3 v1temp, v1, v2, v1x, v2x, v1y, v2y, x(pA->positionXYZ - pB->positionXYZ);

	//glm::normalize(x);
	//v1 = pA->velocity;
	//x1 = dot(x, v1);
	//v1x = x * x1;
	//v1y = v1 - v1x;
	//m1 = 1.0f; //mass of 1

	//x = x * -1.0f;
	//v2 = pB->velocity;
	//x2 = dot(x, v2);
	//v2x = x * x2;
	//v2y = v2 - v2x;
	//m2 = 1.0f; //mass of 1

	//	//new code


	sPhysicsTriangle closestTriangle;

	GetClosestTriangleToPoint(pA->positionXYZ, pB->GameObjectMesh, collisionInfo.closestPoint, closestTriangle);

	glm::vec3 centreOfTriangle = (closestTriangle.verts[0] +
		closestTriangle.verts[1] +
		closestTriangle.verts[2]) / 3.0f;		// Average

	float distanceBetweenSpheres = glm::length(pA->positionXYZ - pB->positionXYZ);
	
	
	float distance = glm::length(pA->positionXYZ - collisionInfo.closestPoint);

	if (/*distance <= pA->SPHERE_radius*/distanceBetweenSpheres <= (pA->SPHERE_radius +pB->SPHERE_radius))
	{

		
		//// 1. Calculate vector from centre of sphere to closest point
		//glm::vec3 vecSphereToClosestPoint = collisionInfo.closestPoint - pA->positionXYZ;
		//
		//// 2. Get the length of this vector
		//float centreToContractDistance = glm::length(vecSphereToClosestPoint);
		//
		//// 3. Create a vector from closest point to radius
		//float lengthPositionAdjustment = pA->SPHERE_radius - centreToContractDistance;
		//
		//// 4. Sphere is moving in the direction of the velocity, so 
		////    we want to move the sphere BACK along this velocity vector
		//glm::vec3 vecDirection = glm::normalize(pA->velocity);
		//
		//glm::vec3 vecPositionAdjust = (-vecDirection) * lengthPositionAdjustment;
		//
		//// 5. Reposition sphere 
		//pA->positionXYZ += (vecPositionAdjust);
		//
		///*	 Calculate the response vector off the triangle. */
		//glm::vec3 velocityVector = glm::normalize(pA->velocity);
		//
		//// closestTriangle.normal
		//glm::vec3 reflectionVec = glm::reflect(velocityVector, closestTriangle.normal);
		//reflectionVec = glm::normalize(reflectionVec);
		//
		//
		//// Get lenght of the velocity vector
		//float speed = glm::length(pA->velocity);
		//
		//pA->velocity = reflectionVec * speed;
		////set the position of the spheres to their previous non contact positions to unstick them.
		//pA->positionXYZ = pA->prevPositionXYZ;
		//pB->positionXYZ = pB->prevPositionXYZ;
		//pA->velocity = glm::vec3(v1x * (m1 - m2) / (m1 + m2) + v2x * (2 * m2) / (m1 + m2) + v1y) / 4.0f;
		//pB->velocity = glm::vec3(v1x * (2 * m1) / (m1 + m2) + v2x * (m2 - m1) / (m1 + m2) + v2y) / 4.0f;
		pA->velocity = glm::vec3(0);
		pB->velocity = glm::vec3(0);

		


		return true;
	}

	return false;
}
bool cPhysics::DoShphereMeshCollisionTest(cGameObject* pA, cGameObject* pB,
	sCollisionInfo& collisionInfo)
{

	sPhysicsTriangle closestTriangle;

	GetClosestTriangleToPoint(pA->positionXYZ,pB->GameObjectMesh , collisionInfo.closestPoint, closestTriangle);


	glm::vec3 centreOfTriangle = (closestTriangle.verts[0] +
		closestTriangle.verts[1] +
		closestTriangle.verts[2]) / 3.0f;		// Average

	float distance = glm::length(pA->positionXYZ - collisionInfo.closestPoint);

	if (distance <= pA->SPHERE_radius)
	{

		// 1. Calculate vector from centre of sphere to closest point
		glm::vec3 vecSphereToClosestPoint = collisionInfo.closestPoint - pA->positionXYZ;

		// 2. Get the length of this vector
		float centreToContractDistance = glm::length(vecSphereToClosestPoint);

		// 3. Create a vector from closest point to radius
		float lengthPositionAdjustment = pA->SPHERE_radius - centreToContractDistance;

		// 4. Sphere is moving in the direction of the velocity, so 
		//    we want to move the sphere BACK along this velocity vector
		glm::vec3 vecDirection = glm::normalize(pA->velocity);

		glm::vec3 vecPositionAdjust = (-vecDirection) * lengthPositionAdjustment;

		// 5. Reposition sphere 
		pA->positionXYZ += (vecPositionAdjust);

		/*	 Calculate the response vector off the triangle. */
		glm::vec3 velocityVector = glm::normalize(pA->velocity);

		// closestTriangle.normal
		glm::vec3 reflectionVec = glm::reflect(velocityVector, closestTriangle.normal);
		reflectionVec = glm::normalize(reflectionVec);


		// Get lenght of the velocity vector
		float speed = glm::length(pA->velocity);

		pA->velocity = reflectionVec * speed;


		return true;
	}
}
// Takes a mesh in "model space" and converts it into "world space"
void cPhysics::CalculateTransformedMesh(cMesh& originalMesh, glm::mat4 matWorld,
	cMesh& mesh_transformedInWorld)
{
	mesh_transformedInWorld = originalMesh;


	for (std::vector<sPlyVertexXYZ_N_UV>::iterator itVert = mesh_transformedInWorld.vecVertices.begin();
		itVert != mesh_transformedInWorld.vecVertices.end(); itVert++)
	{
		glm::vec4 vertex = glm::vec4(itVert->x, itVert->y, itVert->z, 1.0f);

		glm::vec4 vertexWorldTransformed = matWorld * vertex;

		// Update 
		itVert->x = vertexWorldTransformed.x;
		itVert->y = vertexWorldTransformed.y;
		itVert->z = vertexWorldTransformed.z;


		// CALCAULTE THE NORMALS for the this mesh, too (for the response)
		// for the normal, do the inverse transpose of the world matrix
		glm::mat4 matWorld_Inv_Transp = glm::inverse(glm::transpose(matWorld));


		glm::vec4 normal = glm::vec4(itVert->nx, itVert->ny, itVert->nz, 1.0f);

		glm::vec4 normalWorldTransformed = matWorld_Inv_Transp * normal;


		// Update 
		itVert->nx = normalWorldTransformed.x;
		itVert->ny = normalWorldTransformed.y;
		itVert->nz = normalWorldTransformed.z;
	}

	return;
}

void cPhysics::seek(cGameObject* target, cGameObject* aiObj,double deltatime)
{
	glm::vec3 desieredVelocity = target->positionXYZ - aiObj->positionXYZ;

	float dist = desieredVelocity.length();

	glm::vec3 direction = glm::normalize(desieredVelocity);

	if(dist < slowingRadius)
	{
		desieredVelocity = direction * maxVelocity *(dist/slowingRadius);
	}
	else
	{
		desieredVelocity = direction* maxVelocity;
	}

	glm::vec3 steer = desieredVelocity - aiObj->velocity;

	aiObj->velocity += steer * (float)deltatime;

	if(aiObj->velocity.length() > maxVelocity)
	{
		aiObj->velocity = normalize(aiObj->velocity) * maxVelocity;
	}
	
}
void cPhysics::seek(glm::vec3 targetPos, cGameObject* aiObj, double deltatime)
{
	glm::vec3 desieredVelocity = targetPos - aiObj->positionXYZ;

	float dist = desieredVelocity.length();

	glm::vec3 direction = glm::normalize(desieredVelocity);

	if (dist < slowingRadius)
	{
		desieredVelocity = direction * maxVelocity * (dist / slowingRadius);
	}
	else
	{
		desieredVelocity = direction * maxVelocity;
	}

	glm::vec3 steer = desieredVelocity - aiObj->velocity;

	aiObj->velocity += steer * (float)deltatime;

	if (aiObj->velocity.length() > maxVelocity)
	{
		aiObj->velocity = normalize(aiObj->velocity) * maxVelocity;
	}
}
void cPhysics::flee(cGameObject* target, cGameObject* aiObj, double deltatime)
{
	glm::vec3 desieredVelocity =  aiObj->positionXYZ- target->positionXYZ;

	float dist = desieredVelocity.length();

	glm::vec3 direction = glm::normalize(desieredVelocity);

	if (dist < slowingRadius)
	{
		desieredVelocity = direction * maxVelocity * (dist / slowingRadius);
	}
	else
	{
		desieredVelocity = direction * maxVelocity;
	}

	glm::vec3 steer = desieredVelocity - aiObj->velocity;

	aiObj->velocity += steer * (float)deltatime;

	if (aiObj->velocity.length() > maxVelocity)
	{
		aiObj->velocity = normalize(aiObj->velocity) * maxVelocity;
	}

}

