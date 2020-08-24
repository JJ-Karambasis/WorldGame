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
    
    if(NormalLength <= Square(Radius))
    {
        NormalLength = Sqrt(NormalLength);
        Normal /= NormalLength;
        
        Result.Count = 1;
        Result.Ptr = PushArray(Result.Count, contact, Clear, 0);
        
        v3f PointOnSphere  = P0 + Normal*Sphere->Radius;
        v3f PointOnCapsule = P1 - Normal*Capsule->Radius;
        
        Result.Ptr[0].Normal = Normal;        
        Result.Ptr[0].Position = P0 + ((PointOnCapsule-PointOnSphere)*0.5f);
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
        
        Normal /= Sqrt(NormalLength);
        
        Result.Count = 1;
        Result.Ptr = PushArray(Result.Count, contact, Clear, 0);
        
        Result.Ptr[0].Normal = Normal;
        Result.Ptr[0].Position = P1;
        Result.Ptr[0].Penetration = Sphere->Radius - NormalLength;
    }
    
    return Result;
}

void AddContinuousContacts(simulation* Simulation, sim_entity_volume_pair* A, sim_entity_volume_pair* B)
{
    collision_volume* VolumeA = A->Volume;
    collision_volume* VolumeB = B->Volume;
    
    switch(VolumeA->Type)
    {
        case COLLISION_VOLUME_TYPE_SPHERE:
        {
            sphere Sphere = TransformSphere(&VolumeA->Sphere, A->SimEntity->Transform);
            
            switch(VolumeB->Type)
            {
                case COLLISION_VOLUME_TYPE_SPHERE:
                {
                    NOT_IMPLEMENTED;
                } break;
                
                case COLLISION_VOLUME_TYPE_CAPSULE:
                {                                    
                    capsule Capsule = TransformCapsule(&VolumeB->Capsule, B->SimEntity->Transform);
                    
                    f32 t = SphereCapsuleTOI(&Sphere, A->SimEntity->MoveDelta, &Capsule, B->SimEntity->MoveDelta);
                    if(t != INFINITY)
                    {
                        A->SimEntity->MoveDelta *= t;
                        B->SimEntity->MoveDelta *= t;
                        
                        TranslateSphere(&Sphere, A->SimEntity->MoveDelta);
                        TranslateCapsule(&Capsule, B->SimEntity->MoveDelta);
                        
                        contact_list Contacts = GetSphereCapsuleContacts(&Sphere, &Capsule);
                        manifold* Manifold  = Simulation->GetManifold(A->SimEntity->RigidBody, B->SimEntity->RigidBody);
                        Manifold->AddContactConstraints(Simulation, &Contacts);
                    }
                    
                } break;
                
                case COLLISION_VOLUME_TYPE_CONVEX_HULL:
                {                    
                    convex_hull* ConvexHull = VolumeB->ConvexHull;
                    sqt ConvexHullTransform = ToParentCoordinates(ConvexHull->Header.Transform, B->SimEntity->Transform);
                    
                    f32 t = SphereHullTOI(&Sphere, A->SimEntity->MoveDelta, ConvexHull, ConvexHullTransform, B->SimEntity->MoveDelta);
                    if(t != INFINITY)
                    {
                        A->SimEntity->MoveDelta *= t;
                        B->SimEntity->MoveDelta *= t;
                        
                        TranslateSphere(&Sphere, A->SimEntity->MoveDelta);
                        ConvexHullTransform.Translation += B->SimEntity->MoveDelta;
                        
                        contact_list Contacts = GetSphereHullContacts(&Sphere, ConvexHull, ConvexHullTransform);
                        manifold* Manifold = Simulation->GetManifold(A->SimEntity->RigidBody, B->SimEntity->RigidBody);
                        Manifold->AddContactConstraints(Simulation, &Contacts);
                    }                                                
                } break;
            }
        } break;
        
        INVALID_DEFAULT_CASE;
    }
}
