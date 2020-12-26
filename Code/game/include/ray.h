#ifndef RAY_H
#define RAY_H

struct ray
{
    ak_v3f Origin;
    ak_v3f Direction;
};

struct ray_cast
{
    ak_bool Intersected;
    ak_f32 t;
};

ak_v3f Ray_PixelToWorld(ak_v2i PixelCoords, ak_v2i PixelDim, ak_m4f Perspective, ak_m4f View);
ray_cast Ray_SphereCast(ak_v3f Origin, ak_v3f Direction, ak_v3f CenterP, ak_f32 Radius);
ray_cast Ray_CapsuleCast(ak_v3f Origin, ak_v3f Direction, ak_v3f P0, ak_v3f P1, ak_f32 Radius);
ray_cast Ray_TriangleCast(ak_v3f Origin, ak_v3f Direction, ak_v3f P0, ak_v3f P1, ak_v3f P2, ak_f32* uOut=NULL, ak_f32* vOut=NULL);
ray_cast Ray_TriangleCastNoCull(ak_v3f Origin, ak_v3f Direction, ak_v3f P0, ak_v3f P1, 
                                ak_v3f P2, ak_f32* uOut=NULL, ak_f32* vOut=NULL);
ray_cast Ray_TriangleMeshCast(ak_v3f RayOrigin, ak_v3f RayDirection, 
                              ak_v3f* Vertices, ak_u16* Indices, ak_u32 IndexCount, 
                              ak_m4f MeshTransform, ak_f32* uOut=NULL, ak_f32* vOut=NULL);
ray_cast Ray_TriangleMeshCastNoCull(ak_v3f RayOrigin, ak_v3f RayDirection, 
                                    ak_v3f* Vertices, ak_u16* Indices, ak_u32 IndexCount, 
                                    ak_m4f MeshTransform, ak_f32* uOut=NULL, ak_f32* vOut=NULL);
ray_cast Ray_TriangleMeshCast(ak_v3f RayOrigin, ak_v3f RayDirection, 
                              ak_v3f* Vertices, ak_u32* Indices, ak_u32 IndexCount, 
                              ak_m4f MeshTransform, ak_f32* uOut=NULL, ak_f32* vOut=NULL);
ray_cast Ray_TriangleMeshCastNoCull(ak_v3f RayOrigin, ak_v3f RayDirection, 
                                    ak_v3f* Vertices, ak_u32* Indices, ak_u32 IndexCount, 
                                    ak_m4f MeshTransform, ak_f32* uOut=NULL, ak_f32* vOut=NULL);
ray_cast Ray_PlaneCast(ak_v3f Origin, ak_v3f Direction, ak_v3f PlaneNormal, ak_v3f PlanePoint);
ray_cast LineSegment_SphereCast(ak_v3f* LineSegment, ak_v3f CenterP, ak_f32 Radius);
ray_cast LineSegment_CapsuleCast(ak_v3f* LineSegment, ak_v3f P0, ak_v3f P1, ak_f32 Radius);

#endif
