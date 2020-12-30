#ifndef EDIT_RECORDINGS_H
#define EDIT_RECORDINGS_H


enum edit_entry_type
{
    EDIT_ENTRY_TYPE_CREATE,
    EDIT_ENTRY_TYPE_DELETE, 
    EDIT_ENTRY_TYPE_TRANSFORM,
    EDIT_ENTRY_TYPE_PROPERTIES
};

struct dev_entity_pair
{
    ak_u32 Count;
    dev_entity Entity[2];
};

struct dev_point_light_pair
{
    ak_u32 Count;
    dev_point_light PointLight[2];
};

struct create_delete_edit_entry
{
    object_type Type;
    ak_u32 WorldIndex;
    
    union
    {
        dev_entity_pair EntityPair;
        dev_point_light_pair PointLightPair;
    };
};

struct transform_edit_entry
{
    object Object;
    ak_u32 WorldIndex;
    ak_v3f Translation;
    ak_v3f Scale;
    ak_quatf Orientation;
    ak_v3f Euler;
};

struct property_edit_entry
{
    object_type Type;
    ak_u32 WorldIndex;
    
    union
    {
        dev_entity Entities[2];
        dev_point_light PointLights[2];
    };
};

struct edit_entry
{
    edit_entry_type Type;
    union
    {
        create_delete_edit_entry CreateDelete;
        transform_edit_entry Transform;
        property_edit_entry Properties;
    };
};

struct edit_recordings
{
    ak_array<edit_entry> UndoStack;
    ak_array<edit_entry> RedoStack;
    
    void ClearUndo();
    void ClearRedo();
    void Clear();
    
    void PushCreateEntry(ak_u32 WorldIndex, dev_entity* Entity);
    void PushCreateEntry(ak_u32 WorldIndex, dev_entity* EntityA, dev_entity* EntityB);
    void PushCreateEntry(ak_u32 WorldIndex, dev_point_light* PointLight);
    void PushCreateEntry(ak_u32 WorldIndex, dev_point_light* PointLightA, dev_point_light* PointLightB);
    
    void PushDeleteEntry(ak_u32 WorldIndex, dev_entity* Entity);
    void PushDeleteEntry(ak_u32 WorldIndex, dev_entity* EntityA, dev_entity* EntityB);
    void PushDeleteEntry(ak_u32 WorldIndex, dev_point_light* PointLight);
    void PushDeleteEntry(ak_u32 WorldIndex, dev_point_light* PointLightA, dev_point_light* PointLightB);
    
    void PushTransformEntry(ak_u32 WorldIndex, object Object, ak_v3f Translation, ak_v3f Scale, ak_quatf Orientation, 
                            ak_v3f Euler);
    
    void PushPropertyEntry(ak_u32 WorldIndex, dev_entity* OldState, dev_entity* NewState);
    void PushPropertyEntry(ak_u32 WorldIndex, dev_point_light* OldState, dev_point_light* NewState);
    
    void Undo(world_management* WorldManagement);
    void Redo(world_management* WorldManagement);
    
};

#endif
