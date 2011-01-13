#include "Foundation/Reflect/Data/PointerData.h"

using namespace Helium;
using namespace Helium::Reflect;

REFLECT_DEFINE_CLASS( PointerData );

PointerData::PointerData()
{

}

PointerData::~PointerData()
{

}

void PointerData::ConnectData(Helium::HybridPtr<void> data)
{
    m_Data.Connect( Helium::HybridPtr<DataType> (data.Address(), data.State()) );
}

bool PointerData::Set(const Data* s, uint32_t flags)
{
    const PointerData* rhs = SafeCast<PointerData>(s);
    if (!rhs)
    {
        return false;
    }

    if (flags & DataFlags::Shallow)
    {
        m_Data.Set( rhs->m_Data.Get() );
    }
    else
    {
        m_Data.Set( rhs->m_Data.Get().ReferencesObject() ? rhs->m_Data.Get()->Clone() : NULL );
    }

    return true;
}

bool PointerData::Equals(const Object* object) const
{
    const PointerData* rhs = SafeCast<PointerData>(object);
    
    if (!rhs)
    {
        return false;
    }

    // if the pointers are equal we are done
    if ( m_Data.Get() == rhs->m_Data.Get() )
    {
        return true;
    }
    // if they are not equal but one is null we are done
    else if (!m_Data.Get().ReferencesObject() || !rhs->m_Data.Get().ReferencesObject())
    {
        return false;
    }

    // pointers aren't equal so we have to do deep equality test
    return m_Data.Get()->Equals( rhs->m_Data.Get() );
}

void PointerData::Accept(Visitor& visitor)
{
    if (!visitor.VisitPointer(*(ObjectPtr*)(m_Data.Ptr())))
    {
        return;
    }

    if ( m_Data.Get() )
    {
        m_Data.Get()->Accept( visitor );
    }
}

void PointerData::Serialize(Archive& archive) const
{
    archive.Serialize(m_Data.Get());
}

void PointerData::Deserialize(Archive& archive)
{
    m_Data.Set( NULL );

    archive.Deserialize(m_Data.Ref());
}