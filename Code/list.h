#ifndef LIST_H
#define LIST_H

template <typename list, typename entry>
inline void AddToList(list* List, entry* Entry)
{
    if(List->Count == 0)
    {
        List->Last = List->First = Entry;
    }
    else if(List->Count == 1)
    {
        List->Last = Entry;
        List->Last->Prev = List->First;
        List->First->Next = List->Last;
    }
    else
    {
        Entry->Prev = List->Last;
        List->Last = Entry;
        Entry->Prev->Next = List->Last;
    }
    List->Count++;
}

template <typename list, typename result>
result* RemoveEndOfList(list* List)
{
    ASSERT(List->Last && (List->Count > 0)); 
    
    result* Result = NULL;
    if(List->Count == 1)
    {
        Result = List->Last;
        List->Last = List->First = NULL;                
    }
    else
    {        
        result* Prev = List->Last->Prev;
        Result = List->Last;
        ASSERT(!Result->Next);
        List->Last = Prev;
        Result->Prev = NULL;            
    }
    
    List->Count--;
    return Result;
}

template <typename list, typename entry>
void RemoveFromList(list* List, entry* Entry)
{
    ASSERT(List->Count > 0);
    if(List->Count == 1)
    {
        ASSERT(!Entry->Prev && !Entry->Next);
        List->First = List->Last = NULL;        
    }
    else
    {
        entry* Prev = Entry->Prev;
        entry* Next = Entry->Next;
        
        if(!Prev)
        {
            ASSERT(List->First == Entry);
            List->First = Next;
            List->First->Prev = NULL;
        }        
        else if(!Next)
        {
            ASSERT(List->Last == Entry);
            List->Last = Prev;
            List->Last->Next = NULL;
        }
        else
        {
            Prev->Next = Next;
            Next->Prev = Prev;
        }
        
        Entry->Prev = Entry->Next = NULL;
    }
    
    List->Count--;
}

#endif