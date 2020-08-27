void manifold::AddContactConstraints(simulation* Simulation, contact_list* List)
{
    for(u32 ContactIndex = 0; ContactIndex < List->Count; ContactIndex++)
    {
        contact* Contact = List->Ptr + ContactIndex;
        list_entry<contact_constraint>* ConstraintLink = Simulation->ContactStorage.Get(Simulation->ContactStorage.Allocate());        
        contact_constraint* Constraint = &ConstraintLink->Entry;        
        
        Constraint->WorldPosition = Contact->Position;
        Constraint->Normal = Contact->Normal;
        Constraint->Penetration = Contact->Penetration;
        
        Contacts.Add(ConstraintLink);
    }
}

contact_list GetSphereCapsuleContacts(sphere* Sphere, capsule* Capsule)
{
    contact_list Result = {};
    
    v3f P0 = Sphere->CenterP;
    v3f P1 = PointLineSegmentClosestPoint(P0, Capsule->P);
    
    v3f Normal = P1-P0;
    f32 NormalLength = SquareMagnitude(Normal);  
    
    //TODO(JJ): This assertion is a degenerate case. We probably should support it at some point. Happens when the sphere center point 
    //is on the capsule line thus a contact normal cannot be computed. Can probably just use an arbitrary normal
    ASSERT(!IsFuzzyZero(NormalLength)); 
    
    f32 Radius = Sphere->Radius+Capsule->Radius;    
    f32 Diff = NormalLength-Square(Radius);
    if(Diff < 0 || IsFuzzyZero(Diff))
    {
        NormalLength = Sqrt(NormalLength);
        Normal /= NormalLength;
        
        Result.Count = 1;
        Result.Ptr = PushArray(Result.Count, contact, Clear, 0);
        
        v3f PointOnSphere  = P0 + Normal*Sphere->Radius;
        v3f PointOnCapsule = P1 - Normal*Capsule->Radius;
        
        Result.Ptr[0].Normal = Normal;        
        Result.Ptr[0].Position = PointOnSphere + ((PointOnCapsule-PointOnSphere)*0.5f);
        Result.Ptr[0].Penetration = Radius - NormalLength;
    }
    
    return Result;    
}

contact_list GetSphereHullContacts(sphere* Sphere, convex_hull* ConvexHull, sqt ConvexHullTransform)
{
    contact_list Result = {};
    
    point_support PointGJK = {Sphere->CenterP};
    convex_hull_support ConvexHullGJK = {ConvexHull, ConvexHullTransform};        
    gjk_distance Distance = GJKDistance(&PointGJK, &ConvexHullGJK);
    
    if(Distance.SquareDistance <= Square(Sphere->Radius))
    {
        v3f P0, P1;
        Distance.GetClosestPoints(&P0, &P1);
        
        v3f Normal = P1-P0;
        f32 NormalLength = SquareMagnitude(Normal);
        ASSERT(!IsFuzzyZero(NormalLength));
        
        NormalLength = Sqrt(NormalLength);
        Normal /= NormalLength;
        
        Result.Count = 1;
        Result.Ptr = PushArray(Result.Count, contact, Clear, 0);
        
        Result.Ptr[0].Normal = Normal;
        Result.Ptr[0].Position = P1;
        Result.Ptr[0].Penetration = Sphere->Radius - NormalLength;
    }
    
    return Result;
}

void AddSphereCapsuleContacts(simulation* Simulation, sim_entity_rigid_body_volume_pair* A, sim_entity_rigid_body_volume_pair* B)
{    
    sphere Sphere = TransformSphere(&A->Volume->Sphere, A->SimEntity->Transform);    
    capsule Capsule = TransformCapsule(&B->Volume->Capsule, B->SimEntity->Transform);
    
    contact_list Contacts = GetSphereCapsuleContacts(&Sphere, &Capsule);
    manifold* Manifold  = Simulation->GetManifold(A->RigidBody, B->RigidBody);
    Manifold->AddContactConstraints(Simulation, &Contacts);    
    
    penetration Penetration = FindMaxPenetration(&Contacts);
    if(!IsInvalidPenetration(&Penetration))
        Simulation->AddCollisionEvent(A->SimEntity, B->SimEntity, Penetration);
}

void AddSphereConvexHullContacts(simulation* Simulation, sim_entity_rigid_body_volume_pair* A, sim_entity_rigid_body_volume_pair* B)
{
    sphere Sphere = TransformSphere(&A->Volume->Sphere, A->SimEntity->Transform);    
    convex_hull* ConvexHull = B->Volume->ConvexHull;
    sqt ConvexHullTransform = ToParentCoordinates(ConvexHull->Header.Transform, B->SimEntity->Transform);
    
    contact_list Contacts = GetSphereHullContacts(&Sphere, ConvexHull, ConvexHullTransform);            
    manifold* Manifold = Simulation->GetManifold(A->RigidBody, B->RigidBody);
    Manifold->AddContactConstraints(Simulation, &Contacts);                                                                             
        
    penetration Penetration = FindMaxPenetration(&Contacts);
    if(!IsInvalidPenetration(&Penetration))
        Simulation->AddCollisionEvent(A->SimEntity, B->SimEntity, Penetration);
}

void AddContacts(simulation* Simulation, sim_entity_rigid_body_volume_pair* A, sim_entity_rigid_body_volume_pair* B)
{
    collision_volume* VolumeA = A->Volume;
    collision_volume* VolumeB = B->Volume;
    
    switch(VolumeA->Type)
    {
        case COLLISION_VOLUME_TYPE_SPHERE:
        {            
            switch(VolumeB->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    NOT_IMPLEMENTED;
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {                       
                    AddSphereCapsuleContacts(Simulation, A, B);                    
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {                    
                    AddSphereConvexHullContacts(Simulation, A, B);                                                                    
                } break;
            }
        } break;
        
        case COLLISION_VOLUME_TYPE_CAPSULE:
        {
            switch(VolumeB->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    AddSphereCapsuleContacts(Simulation, B, A);
                } break;
                
                INVALID_DEFAULT_CASE;
            }
        } break;
        
        case COLLISION_VOLUME_TYPE_CONVEX_HULL:
        {
            switch(VolumeB->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    AddSphereConvexHullContacts(Simulation, B, A);
                } break;
            }
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}
