#ifndef PENETRATION_H
#define PENETRATION_H

struct penetration
{    
    v3f Normal;
    f32 Distance;
};

inline penetration InvalidPenetration()
{
    penetration Result = {V3(), INFINITY};
    return Result;
}

inline b32 IsInvalidPenetration(penetration* Penetration)
{
    b32 Result = (Penetration->Normal == V3()) || (Penetration->Distance == INFINITY);
    return Result;
}

#endif