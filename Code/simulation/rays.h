#ifndef RAYS_H
#define RAYS_H

struct ray_mesh_intersection_result
{
    b32 FoundCollision;
    f32 t;
    f32 u;
    f32 v;
};

#endif