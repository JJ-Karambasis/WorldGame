enum pole_cell_index
{
    POLE_CELL_INDEX_BOTTOM_LEFT,
    POLE_CELL_INDEX_BOTTOM_RIGHT,
    POLE_CELL_INDEX_TOP_LEFT,
    POLE_CELL_INDEX_TOP_RIGHT
};

enum pole_surrounding_index
{
    POLE_SURROUNDING_INDEX_BOTTOM_LEFT,
    POLE_SURROUNDING_INDEX_BOTTOM,
    POLE_SURROUNDING_INDEX_BOTTOM_RIGHT,
    POLE_SURROUNDING_INDEX_LEFT,
    POLE_SURROUNDING_INDEX_RIGHT,
    POLE_SURROUNDING_INDEX_TOP_LEFT,
    POLE_SURROUNDING_INDEX_TOP,
    POLE_SURROUNDING_INDEX_TOP_RIGHT
};

enum grid_method_type
{
    GRID_METHOD_TYPE_UNKNOWN,
    GRID_METHOD_TYPE_NONE,
    GRID_METHOD_TYPE_CORNER,
    GRID_METHOD_TYPE_WALL,
    GRID_METHOD_TYPE_TRIANGLE,
    GRID_METHOD_TYPE_SURFACE
};

enum 
{
    WALKABLE_POLE_FLAG_NONE = 0,
    WALKABLE_POLE_FLAG_WALKABLE = BIT_SET(0),
    WALKABLE_POLE_FLAG_WALK_EDGE = BIT_SET(1),    
};

struct surface_edge_test
{
    f32 Distance;
    v2f Point;
    b32 IsPoleWalkEdge;
};

struct surface_test
{
    b32 HasIntersected;
    v2f Point;
};

struct walkable_pole
{
    u32 Flags;
    union
    {
        v3f IntersectionPoint;
        struct
        {
            v2f Position2D;
            f32 ZIntersection;
        };
    };
    
    struct static_entity* HitEntity;
    b32 RadiusCheck;        
    surface_test SurfaceTests[8];
};

struct walkable_grid
{
    v2f Min;
    v2i CellCount;
    v2i PoleCount;
    
    
    walkable_pole* Poles;
};

struct walkable_triangle_ring
{ 
    v3f P[3];    
    walkable_triangle_ring* Next;
};

struct walkable_triangle_ring_list
{
    walkable_triangle_ring* Head;
};

inline surface_edge_test 
CreateDefaultSurfaceEdgeTest()
{
    surface_edge_test Result;
    Result.Distance = -FLT_MAX;
    Result.Point = InvalidV2();
    Result.IsPoleWalkEdge = false;
    return Result;
}

inline surface_test
CreateSurfaceTest(v2f Point)
{
    surface_test Result;
    Result.HasIntersected = true;
    Result.Point = Point;
    return Result;
}