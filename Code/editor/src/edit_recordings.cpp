void edit_recordings::ClearUndo()
{
    UndoStack.Clear();
}

void edit_recordings::ClearRedo()
{
    RedoStack.Clear();
}

void edit_recordings::Clear()
{
    ClearUndo();
    ClearRedo();
}

void edit_recordings::Push(edit_entry_type Type, ak_u32 WorldIndex, dev_entity* Entity)
{
    AK_Assert(Type == EDIT_ENTRY_TYPE_CREATE || Type == EDIT_ENTRY_TYPE_DELETE, 
              "Invalid edit entry type");
    AK_Assert(AK_StringIsNullOrEmpty(Entity->LinkName), "If object has a linked entity, please pass it into this function");
    
    edit_entry Entry = {};
    Entry.Type = Type;
    Entry.CreateDelete.Type = OBJECT_TYPE_ENTITY;
    Entry.CreateDelete.WorldIndex = WorldIndex;
    Entry.CreateDelete.EntityPair.Count = 1;
    Entry.CreateDelete.EntityPair.Entity[0] = *Entity;
    UndoStack.Add(Entry);
}

void edit_recordings::Push(edit_entry_type Type, ak_u32 WorldIndex, 
                           dev_entity* EntityA, dev_entity* EntityB)
{
    AK_Assert(Type == EDIT_ENTRY_TYPE_CREATE || Type == EDIT_ENTRY_TYPE_DELETE, 
              "Invalid edit entry type");
    
    edit_entry Entry = {};
    Entry.Type = Type;
    Entry.CreateDelete.Type = OBJECT_TYPE_ENTITY;
    Entry.CreateDelete.WorldIndex = WorldIndex;
    Entry.CreateDelete.EntityPair.Entity[0] = *EntityA;
    Entry.CreateDelete.EntityPair.Entity[1] = *EntityB;
    
    if(!AK_StringIsNullOrEmpty(EntityA->LinkName))
    {
        AK_Assert(AK_StringEquals(EntityA->LinkName, EntityB->Name), "If EntityA has a linked entity, then that entity must be EntityB");
        AK_Assert(AK_StringEquals(EntityB->LinkName, EntityA->Name), "If EntityB has a linked entity, then that entity must be EntityA");
        Entry.CreateDelete.EntityPair.Count = 1;
    }
    else
    {
        AK_Assert(AK_StringIsNullOrEmpty(EntityB->LinkName), "EntityB must not have a linked entity if entity A does not have any");
        Entry.CreateDelete.EntityPair.Count = 2;
        
    }
    
    UndoStack.Add(Entry);
}


void edit_recordings::Undo(world_management* WorldManagement)
{
    edit_entry* Entry = UndoStack.Pop(); 
    if(Entry)
    {
        switch(Entry->Type)
        {
            case EDIT_ENTRY_TYPE_CREATE:
            {
                create_delete_edit_entry* CreatedEntry = &Entry->CreateDelete;
                
                switch(CreatedEntry->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        WorldManagement->DeleteDevEntity(CreatedEntry->WorldIndex, CreatedEntry->EntityPair.Entity[0].Name);
                        
                        if(CreatedEntry->EntityPair.Count > 1)
                        {
                            WorldManagement->DeleteDevEntity(!CreatedEntry->WorldIndex, 
                                                             CreatedEntry->EntityPair.Entity[1].Name);
                        }
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        WorldManagement->DeleteDevPointLight(CreatedEntry->WorldIndex, CreatedEntry->PointLightPair.PointLight[0].Name);
                        if(CreatedEntry->PointLightPair.Count > 1)
                        {
                            WorldManagement->DeleteDevPointLight(!CreatedEntry->WorldIndex, 
                                                                 CreatedEntry->PointLightPair.PointLight[1].Name);
                        }
                    } break;
                    
                    AK_INVALID_DEFAULT_CASE;
                }
                
            } break;
            
            case EDIT_ENTRY_TYPE_DELETE:
            {
                AK_NotImplemented();
            } break;
            
            AK_INVALID_DEFAULT_CASE;
        }
        
        RedoStack.Add(*Entry);
    }
}

void edit_recordings::Redo(world_management* WorldManagement)
{
    edit_entry* Entry = RedoStack.Pop();
    if(Entry)
    {
        switch(Entry->Type)
        {
            case EDIT_ENTRY_TYPE_CREATE:
            {
                create_delete_edit_entry* CreatedEntry = &Entry->CreateDelete;
                
                switch(CreatedEntry->Type)
                {
                    case OBJECT_TYPE_ENTITY:
                    {
                        WorldManagement->CopyDevEntity(CreatedEntry->EntityPair.Entity, CreatedEntry->WorldIndex);
                        if(CreatedEntry->EntityPair.Count > 1)
                        {
                            WorldManagement->CopyDevEntity(&CreatedEntry->EntityPair.Entity[1], 
                                                           !CreatedEntry->WorldIndex);
                        }
                    } break;
                    
                    case OBJECT_TYPE_LIGHT:
                    {
                        WorldManagement->CopyDevPointLight(CreatedEntry->PointLightPair.PointLight, 
                                                           CreatedEntry->WorldIndex);
                        if(CreatedEntry->PointLightPair.Count > 1)
                        {
                            WorldManagement->CopyDevPointLight(&CreatedEntry->PointLightPair.PointLight[1], 
                                                               !CreatedEntry->WorldIndex);
                        }
                    } break;
                }
                
            } break;
            
            case EDIT_ENTRY_TYPE_DELETE:
            {
                AK_NotImplemented();
            } break;
        }
        
        UndoStack.Add(*Entry);
    }
}