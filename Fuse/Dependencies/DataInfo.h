#pragma once


#include "DependencyInfo.h"


namespace Dependencies
{ 
  
  class DEPENDENCIES_API DataInfo : public DependencyInfo
  {
  public:

    DataInfo();
    DataInfo( const std::string &path, const Finder::FileSpec &fileSpec, const GraphConfigs graphConfigs = ConfigFlags::Default );
    DataInfo( const std::string &path, const Finder::FileSpec &fileSpec, u8* data, const size_t dataSize, 
              const GraphConfigs graphConfigs = ConfigFlags::Default );
    //DataInfo( const DataInfo &rhs );

    ~DataInfo();

    virtual bool IsMD5Valid();
    virtual void GenerateMD5();
    virtual bool WasModified();
    virtual bool AppendToSignature( CryptoPP::HashFilter* hashFilter, V_string& trace );
    virtual void CacheCopy( DependencyInfo& rhs );
    virtual bool Exists(){ return true; }


    virtual void CopyFrom( const DependencyInfo &rhs );
    virtual bool IsEqual( const DependencyInfo &rhs ) const;

  
  public:
    //
    // Members
    //

    u8* m_Data;
    size_t m_DataSize;



  };
  typedef Nocturnal::SmartPtr< DataInfo > DataInfoPtr;
  typedef std::vector< DataInfoPtr > V_DataInfo;
  typedef std::vector< V_DataInfo > VV_DataInfo;
  typedef Nocturnal::OrderedSet< DataInfoPtr > OS_DataInfo;
}