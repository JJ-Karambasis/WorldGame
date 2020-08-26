#ifndef CONTACTS_H
#define CONTACTS_H

struct rigid_body;

struct contact
{    
    v3f Position;
    v3f Normal;
    f32 Penetration;    
};

struct contact_list
{
    contact* Ptr;
    u32 Count;
};

struct contact_constraint
{
    v3f WorldPosition;
    v3f Normal;
    f32 Penetration;
    
    v3f LocalPositionA;            
    v3f LocalPositionB;
    f32 NormalMass;
    f32 NormalLambda;
    
    f32 Bias;
};

struct manifold
{   
    rigid_body* BodyA;
    rigid_body* BodyB;    
    list<contact_constraint> Contacts;
    
    void AddContactConstraints(simulation* Game, contact_list* Contacts);
};

typedef pool<list_entry<contact_constraint>> contact_storage;
typedef pool<manifold> manifold_storage;

#endif