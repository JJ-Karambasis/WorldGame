#ifndef CONTACTS_H
#define CONTACTS_H

struct contact
{
    v3f Position;
    v3f Normal;
    f32 Penetration;    
};

struct manifold
{
    m3 InvInertiaTensorWorldA;
    m3 InvInertiaTensorWorldB;    
    
    entity_id BodyA;
    entity_id BodyB;
    
    list<contact> Contacts;
};

typedef pool<list_entry<contact>> contact_storage;
typedef pool<manifold> manifold_storage;

#endif