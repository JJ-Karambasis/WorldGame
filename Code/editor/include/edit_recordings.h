#ifndef EDIT_RECORDINGS_H
#define EDIT_RECORDINGS_H


enum edit_entry_type
{
    EDIT_ENTRY_TYPE_CREATE,
    EDIT_ENTRY_TYPE_DELETE 
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

struct edit_entry
{
    edit_entry_type Type;
    union
    {
        create_delete_edit_entry CreateDelete;
    };
};

struct edit_recordings
{
    ak_array<edit_entry> UndoStack;
    ak_array<edit_entry> RedoStack;
    
    void ClearUndo();
    void ClearRedo();
    void Clear();
    
    void Push(edit_entry_type Type, ak_u32 WorldIndex, dev_entity* Entity);
    void Push(edit_entry_type Type, ak_u32 WorldIndex, 
              dev_entity* EntityA, dev_entity* EntityB);
    
    void Undo(world_management* WorldManagement);
    void Redo(world_management* WorldManagement);
    
};

#endif
