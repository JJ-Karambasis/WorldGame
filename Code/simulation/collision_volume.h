#ifndef COLLISION_VOLUME_H
#define COLLISION_VOLUME_H

enum collision_volume_type
{    
    COLLISION_VOLUME_TYPE_SPHERE,
    COLLISION_VOLUME_TYPE_CAPSULE,
    COLLISION_VOLUME_TYPE_CONVEX_HULL    
};

struct sphere
{
    v3f CenterP;
    f32 Radius;
};

struct capsule
{
    union
    {
        v3f P[2];
        struct
        {
            v3f P0;
            v3f P1;
        };
    };
    f32 Radius;
    
    inline f32 GetHeight() { return Magnitude(P1-P0); }
    inline v3f GetCenter() { return P0 + (P1-P0)*0.5f; }
};

struct collision_volume
{
    collision_volume_type Type;    
    union
    { 
        sphere Sphere;
        capsule Capsule;        
        convex_hull* ConvexHull;        
    };
    
    collision_volume* Next;
};

struct collision_volume_iter
{
    collision_volume* First;
    collision_volume* Current;
};

collision_volume_iter BeginIter(collision_volume* First)
{
    collision_volume_iter Result = {};
    Result.First = First;
    return Result;
}

collision_volume* GetFirst(collision_volume_iter* Iter)
{
    ASSERT(!Iter->Current);
    Iter->Current = Iter->First;
    return Iter->Current;
}

collision_volume* GetNext(collision_volume_iter* Iter)
{
    collision_volume* Result = Iter->Current->Next;
    Iter->Current = Result;
    return Result;
}

#endif