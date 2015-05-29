#include "Windows/Windows.h"
#include "Windows/Thread.h"
#include "BuilderStats.h"

#include "AppUtils/AppUtils.h"
#include "Console/Console.h"

#include <mysql/include/mysql.h>

#include "Common/Config.h"
#include "Common/Environment.h"
#include "Common/Version.h"
#include "Common/Environment.h"

#include "FileSystem/FileSystem.h"

// max storage size for a query string
#define MAX_QUERY_LENGTH  2048
#define MAX_INSERT_LENGTH 2048

#define SQL_MYSQL_DEFAULT_TIMEOUT (30)

// The return code for mysql success
#define MYSQL_OK 0
//const u64 s_InvalidRowID = 0;

#define STATS_DB_NAME   "AssetBuilder"
#define TOOLS_DB_NAME   "Tools"
#define USERS_DB_NAME   "Users"

#include "FileSystem/FileSystem.h"

// max storage size for a query string
#define MAX_QUERY_LENGTH  2048
#define MAX_INSERT_LENGTH 2048

#define SQL_MYSQL_DEFAULT_TIMEOUT (30)

// The return code for mysql success
#define MYSQL_OK 0
//const u64 s_InvalidRowID = 0;

#define STATS_DB_NAME   "AssetBuilder"
#define TOOLS_DB_NAME   "Tools"
#define USERS_DB_NAME   "Users"

// for the sql statistics
static MYSQL*      g_MySQL          = NULL;
static u64         g_ComputerID     = 0;
static u64         g_VersionID      = 0;
static u64         g_AssetBranchID  = 0;
static u64         g_ProjectID      = 0;
static u64         g_ReleaseID      = 0;
static u64         g_BuildConfigID  = 0;
static u64         g_CodeBranchID   = 0;
static bool        g_IsSymbolBuilder = false;
static bool        g_IsToolsBuilder  = false;

static const u32   g_Port     = 3306;
static const char* g_UserName = "tools";
static const char* g_PassWord = "t0o1z7e4m!";
static const char* g_DataBase = STATS_DB_NAME;
static const i32   g_Timeout  = SQL_MYSQL_DEFAULT_TIMEOUT;

static const char* g_EngineTypesEnumName = "enum Asset::EngineTypes::EngineType";

// User DB
static const char* s_SelectUsersComputerIDSQL = "SELECT `id` FROM `"USERS_DB_NAME"`.`computers` WHERE `name`='%s';";
static const char* s_InsertUsersComputerSQL = "INSERT INTO `"USERS_DB_NAME"`.`computers` (`name`) VALUES ('%s');";

// Tools DB
static const char* s_SelectToolsProjectIDSQL = "SELECT `id` FROM `"TOOLS_DB_NAME"`.`projects` WHERE `name`='%s';";
static const char* s_InsertToolsProjectSQL = "INSERT INTO `"TOOLS_DB_NAME"`.`projects` (`name`) VALUES ('%s');";

static const char* s_SelectToolsBranchIDSQL = "SELECT `id` FROM `"TOOLS_DB_NAME"`.`branches` WHERE `name`='%s';";
static const char* s_InsertToolsBranchSQL = "INSERT INTO `"TOOLS_DB_NAME"`.`branches` (`name`) VALUES ('%s');";

static const char* s_SelectToolsBuildConfigIDSQL = "SELECT `id` FROM `"TOOLS_DB_NAME"`.`build_configs` WHERE `name`='%s';";
static const char* s_InsertToolsBuildConfigSQL = "INSERT INTO `"TOOLS_DB_NAME"`.`build_configs` (`name`) VALUES ('%s');";

static const char* s_SelectToolsVersionIDSQL = "SELECT `id` FROM `"TOOLS_DB_NAME"`.`versions` WHERE `version`='%s';";
static const char* s_InsertToolsVersionSQL   = "INSERT INTO `"TOOLS_DB_NAME"`.`versions` (`version`) VALUES ('%s');";

static const char* s_SelectToolsReleaseIDSQL = "SELECT `id` FROM `"TOOLS_DB_NAME"`.`releases` WHERE `name`='%s';";
static const char* s_InsertToolsReleaseSQL = "INSERT INTO `"TOOLS_DB_NAME"`.`releases` (`name`) VALUES ('%s');";

// BuilderStatistics DB
static const char* s_SelectBuilderIDSQL = "SELECT `id` FROM `"STATS_DB_NAME"`.`builders` WHERE `name`='%s';";
static const char* s_InsertBuilderSQL   = "INSERT INTO `"STATS_DB_NAME"`.`builders` (`name`) VALUES ('%s');";

static const char* s_SelectEngineTypeIDSQL = "SELECT `id` FROM `"STATS_DB_NAME"`.`engine_types` WHERE `name`='%s';";
static const char* s_InsertEngineTypeSQL = "INSERT INTO `"STATS_DB_NAME"`.`engine_types` (`name`) VALUES ('%s');";

static const char* s_SelectBuiltAssetRowIDSQL = "SELECT `id` FROM `"STATS_DB_NAME"`.`built_assets` WHERE file_id='%I64u' AND project_id='%I64u' AND asset_branch_id='%I64u' AND engine_type_id='%I64u';";
static const char* s_InsertBuiltAssetSQL = "INSERT INTO `"STATS_DB_NAME"`.`built_assets` ( file_id, project_id, asset_branch_id, engine_type_id ) VALUES( %I64u, %I64u, %I64u, %I64u );";

static const char* s_InsertBuildSQL = 
"INSERT INTO `"STATS_DB_NAME"`.`builds` \
    SET `built_asset_id`=%I64u, \
    `computer_id`=%I64u, \
    `tools_version_id`=%I64u, \
    `tools_release_id`=%I64u, \
    `build_config_id`=%I64u, \
    `is_tools_builder`=%d, \
    `code_branch_id`=%I64u, \
    `builder_id`=%f, \
    `duration`=%f";

static const char* s_InsertTopLevelBuildSQL = 
"INSERT INTO `"STATS_DB_NAME"`.`top_level_builds` \
    SET `built_asset_id`=%I64u, \
    `computer_id`=%I64u, \
    `tools_version_id`=%I64u, \
    `tools_release_id`=%I64u, \
    `build_config_id`=%I64u, \
    `is_tools_builder`=%d, \
    `code_branch_id`=%I64u, \
    `check_depends_duration`=%f, \
    `download_duration`=%f, \
    `upload_duration`=%f, \
    `build_duration`=%f, \
    `initialize_duration`=%f, \
    `gather_jobs_duration`=%f, \
    `create_sigs_duration`=%f, \
    `unaccounted_duration`=%f, \
    `total_duration`=%f;";

Windows::CriticalSection g_Section;


/////////////////////////////////////////////////////////////////////////////
static inline bool InsertIfNotFound( const char* select, const char* insert, u64& rowID )
{
  NOC_ASSERT( select );

  if ( g_MySQL == NULL )
  {
    return false;
  }

  bool haveID = false;
  rowID = 0;

  char queryStr[MAX_QUERY_LENGTH];
  _snprintf( queryStr, sizeof(queryStr), "%s", select );
  queryStr[ sizeof(queryStr) - 1] = 0; 
  int execResult = mysql_query( g_MySQL, queryStr );

  if ( execResult == MYSQL_OK )
  {
    MYSQL_RES* sqlResult = mysql_store_result( g_MySQL );
    if ( sqlResult )
    { 
      int numFields = mysql_num_fields( sqlResult );
      if ( numFields > 0 )
      {
        MYSQL_ROW sqlRow = mysql_fetch_row( sqlResult );
        if ( sqlRow && sqlRow[0] != NULL )
        {
          rowID = (u64) _atoi64( sqlRow[0] );
          haveID = true;
        }
      }

      // free the result
      mysql_free_result( sqlResult );
    }
    else
    {
      Console::Warning( "No result for: %s", select );
      return false;
    }
  }
  else
  {
    Console::Warning( "Failed to execute SQL: %s", select );
    return false;
  }

  if ( !haveID && insert )
  {
    memset( queryStr, '\0', MAX_QUERY_LENGTH );
    _snprintf( queryStr, sizeof(queryStr), "%s", insert );
    queryStr[ sizeof(queryStr) - 1] = 0; 
    execResult = mysql_query( g_MySQL, queryStr );

    if ( execResult == MYSQL_OK )
    {
      // query was successful - the auto incremented value is the result
      rowID = ( u64 ) mysql_insert_id( g_MySQL );
      if ( rowID > 0 )
      {
        haveID = true;
      }
    }
    else
    {
      Console::Warning( "Failed to execute SQL: %s", insert );
      return false;
    }
  }

  return haveID;
}

/////////////////////////////////////////////////////////////////////////////
static inline bool SelectID( const char* select, const char* insert, const char* value, u64& rowID )
{
  char valueBuff[MAX_QUERY_LENGTH];
  mysql_real_escape_string( g_MySQL, valueBuff, value, (unsigned long) strlen( value ) );

  char selectBuff[MAX_QUERY_LENGTH];
  sprintf_s( selectBuff, sizeof( selectBuff ), select, valueBuff );

  char* insertDumbPtr = NULL;
  if ( insert )
  {
    char insertBuff[MAX_INSERT_LENGTH];
    sprintf_s( insertBuff, sizeof( insertBuff ), insert, value );
    insertDumbPtr = &insertBuff[0];
  }

  return InsertIfNotFound( &selectBuff[0], insertDumbPtr, rowID );
}


///////////////////////////////////////////////////////////////////////////////
static bool InitializeRowIDs()
{
  /////////////////////////////////////
  // Computer Name
  DWORD nameLength = MAX_COMPUTERNAME_LENGTH + 1;
  char computerName[MAX_COMPUTERNAME_LENGTH + 1];
  ::GetComputerName( computerName, &nameLength );

  if ( !SelectID( s_SelectUsersComputerIDSQL, s_InsertUsersComputerSQL, computerName, g_ComputerID ) )
  {
    return false;
  }

  /////////////////////////////////////
  // Tools Release Version
  if ( !SelectID( s_SelectToolsVersionIDSQL, s_InsertToolsVersionSQL, NOCTURNAL_VERSION_STRING, g_VersionID ) )
  {
    return false;
  }

  /////////////////////////////////////
  // Tools build config
  if ( !SelectID( s_SelectToolsBuildConfigIDSQL, s_InsertToolsBuildConfigSQL, NOCTURNAL_BUILD_CONFIG_STRING, g_BuildConfigID ) )
  {
    return false;
  }
 
  /////////////////////////////////////
  // Tools Release name
  std::string toolsRelease;
  if ( !Nocturnal::GetEnvVar( NOCTURNAL_STUDIO_PREFIX"TOOLS_RELEASE_NAME", toolsRelease ) )
  {
    Console::Warning( "Environment variable not set: %s.\n", NOCTURNAL_STUDIO_PREFIX"TOOLS_RELEASE_NAME" );
    return false;
  }

  if ( !SelectID( s_SelectToolsReleaseIDSQL, s_InsertToolsReleaseSQL, toolsRelease.c_str(), g_ReleaseID ) )
  {
    return false;
  }
  
  /////////////////////////////////////
  // is tools builder
  g_IsToolsBuilder = false;
  std::string toolsBuilder;
  if ( Nocturnal::GetEnvVar( NOCTURNAL_STUDIO_PREFIX"TOOLS_BUILDER", toolsBuilder )
    && stricmp( toolsBuilder.c_str(), "1" ) == 0 )
  {
    g_IsToolsBuilder = true;
  }

  /////////////////////////////////////
  // is symbole builder
  g_IsSymbolBuilder = false;
  std::string symbolMode;
  if ( Nocturnal::GetEnvVar( NOCTURNAL_STUDIO_PREFIX"SYMBOL_MODE", symbolMode )
    && stricmp( symbolMode.c_str(), "BUILD" ) == 0 )
  {
    g_IsSymbolBuilder = true;
  }

  /////////////////////////////////////
  // Project Name
  std::string projectName;
  if ( !Nocturnal::GetEnvVar( NOCTURNAL_STUDIO_PREFIX"PROJECT_NAME", projectName ) )
  {
    Console::Warning( "Environment variable not set: %s.\n", NOCTURNAL_STUDIO_PREFIX"PROJECT_NAME" );
    return false;
  }

  if ( !SelectID( s_SelectToolsProjectIDSQL, s_InsertToolsProjectSQL, projectName.c_str(), g_ProjectID ) )
  {
    return false;
  }

  /////////////////////////////////////
  // Asset Branch Name
  std::string assetBranch;
  if ( !Nocturnal::GetEnvVar( NOCTURNAL_STUDIO_PREFIX"ASSETS_BRANCH_NAME", assetBranch ) )
  {
    Console::Warning( "Environment variable not set: %s.\n", NOCTURNAL_STUDIO_PREFIX"ASSETS_BRANCH_NAME" );
    return false;
  }
  FileSystem::CleanName( assetBranch );

  if ( !SelectID( s_SelectToolsBranchIDSQL, s_InsertToolsBranchSQL, assetBranch.c_str(), g_AssetBranchID ) )
  {
    return false;
  }

  /////////////////////////////////////
  // Code Branch Name
  std::string codeBranch;
  if ( !Nocturnal::GetEnvVar( NOCTURNAL_STUDIO_PREFIX"CODE_BRANCH_NAME", codeBranch ) )
  {
    Console::Warning( "Environment variable not set: %s.\n", NOCTURNAL_STUDIO_PREFIX"CODE_BRANCH_NAME" );
    return false;
  }
  FileSystem::CleanName( codeBranch );

  if ( !SelectID( s_SelectToolsBranchIDSQL, s_InsertToolsBranchSQL, codeBranch.c_str(), g_CodeBranchID ) )
  {
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
void BuilderStats::Initialize()
{
  Windows::TakeSection sec( g_Section );

  static const Reflect::Enumeration* info = Reflect::Registry::GetInstance()->GetEnumeration( g_EngineTypesEnumName );
  if ( !info )
  {
    Console::Warning( "Could not locate EngineTypes enum, build statistics reporting will not work properly, please report this error to the tools team." );
  }

  std::string host;
  if ( !Nocturnal::GetEnvVar( NOCTURNAL_STUDIO_PREFIX"DB_HOST", host ) )
  {
    Console::Warning( "No database host set in environment (%s).  Build statistics will not be reported.\n", NOCTURNAL_STUDIO_PREFIX"DB_HOST" );
    return;
  }

  // Database connection logic
  MYSQL* handle = mysql_init( NULL );
  if ( handle == NULL )
  {
    Console::Warning( "Call to mysql_init failed, could not init MySQL DB." );
    g_MySQL = NULL;
  }
  else
  {
    if ( g_Timeout >= 0 )
    {
      handle->options.connect_timeout = handle->options.read_timeout = handle->options.write_timeout = g_Timeout;
    }

    // connect to the g_DataBase engine
    g_MySQL = mysql_real_connect( handle,
      host.c_str(), g_UserName,
      g_PassWord, g_DataBase,
      g_Port, NULL, 0 );

    // if a failure occured when connecting, then cleanup
    if ( g_MySQL == NULL )
    {
      Console::Warning( "Call to mysql_real_connect failed, could not init MySQL DB." );
      mysql_close( handle );
      handle = NULL;
    }
    else if ( !InitializeRowIDs() )
    {
      BuilderStats::Cleanup();
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void BuilderStats::Cleanup()
{
  Windows::TakeSection sec( g_Section );

  if ( g_MySQL )
  {
    mysql_close( g_MySQL );
    g_MySQL = NULL;
  }
}

///////////////////////////////////////////////////////////////////////////////
bool GetBuiltAssetRowID( tuid fileID, Asset::EngineType engineType, u64& assetRowID )
{
  Windows::TakeSection sec( g_Section );

  u64 engineTypeID = 0;
  std::string engineTypeLabel;
  static const Reflect::Enumeration* info = Reflect::Registry::GetInstance()->GetEnumeration( g_EngineTypesEnumName );
  if ( !info 
    || !info->GetElementLabel( engineType, engineTypeLabel )
    || !SelectID( s_SelectEngineTypeIDSQL, s_InsertEngineTypeSQL, engineTypeLabel.c_str(), engineTypeID ) )
  {
    return false;
  }

  static char insertBuff[MAX_QUERY_LENGTH] = { '\0' };
  memset( insertBuff, '\0', MAX_QUERY_LENGTH );
  sprintf_s( insertBuff,
    sizeof( insertBuff ),
    s_InsertBuiltAssetSQL,
    (u64) fileID,
    (u64) g_ProjectID,
    (u64) g_AssetBranchID,
    (u64) engineTypeID );

  static char selectBuff[MAX_INSERT_LENGTH] = { '\0' };
  memset( selectBuff, '\0', MAX_QUERY_LENGTH );
  sprintf_s( selectBuff,
    sizeof( selectBuff ),
    s_SelectBuiltAssetRowIDSQL,
    (u64) fileID,
    (u64) g_ProjectID,
    (u64) g_AssetBranchID,
    (u64) engineTypeID );

  return InsertIfNotFound( selectBuff, insertBuff, assetRowID );
}        


///////////////////////////////////////////////////////////////////////////////
bool BuilderStats::AddBuild( tuid fileID, Asset::EngineType engineType, const std::string& builderName, f32 duration )
{
  Windows::TakeSection sec( g_Section );

  BUILDER_SCOPE_TIMER((""));

  if ( g_MySQL == NULL )
    return false;

  if ( fileID == TUID::Null )
    return false;

  u64 assetRowID;
  if ( !GetBuiltAssetRowID( fileID, engineType, assetRowID ) )
    return false;

  u64 builderRowID = 0;
  if ( !SelectID( s_SelectBuilderIDSQL, s_InsertBuilderSQL, builderName.c_str(), builderRowID ) )
    return false;

  static char insertBuildBuff[MAX_INSERT_LENGTH] = { '\0' };
  memset( insertBuildBuff, '\0', MAX_QUERY_LENGTH );
  sprintf_s( insertBuildBuff,
    sizeof( insertBuildBuff ),
    s_InsertBuildSQL,
    (u64) assetRowID,
    (u64) g_ComputerID,
    (u64) g_VersionID,
    (u64) g_ReleaseID, 
    (u64) g_BuildConfigID, 
    ( g_IsToolsBuilder ? 1 : 0 ), 
    (u64) g_CodeBranchID, 
    (u64) builderRowID,
    duration );

  char queryStr[MAX_QUERY_LENGTH];
  _snprintf( queryStr, sizeof(queryStr), "%s", insertBuildBuff );
  queryStr[ sizeof(queryStr) - 1] = 0; 

  int execResult = mysql_query( g_MySQL, queryStr );
  if ( execResult != MYSQL_OK )
  {
    Console::Warning( "Failed to report builder statistics, AddBuild failed.\n" );
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool BuilderStats::AddTopLevelBuild( tuid fileID, Asset::EngineType engineType, TopLevelBuild& topLevelBuild )
{
  Windows::TakeSection sec( g_Section );

  BUILDER_SCOPE_TIMER((""));

  if ( g_MySQL == NULL )
    return false;

  if ( fileID == TUID::Null )
    return false;

  u64 assetRowID;
  if ( !GetBuiltAssetRowID( fileID, engineType, assetRowID ) )
    return false;

  static char insertBuildBuff[MAX_INSERT_LENGTH] = { '\0' };
  memset( insertBuildBuff, '\0', MAX_QUERY_LENGTH );
  sprintf_s( insertBuildBuff,
    sizeof( insertBuildBuff ),
    s_InsertTopLevelBuildSQL,
    (u64) assetRowID,
    (u64) g_ComputerID,
    (u64) g_VersionID,
    (u64) g_ReleaseID, 
    (u64) g_BuildConfigID, 
    ( g_IsToolsBuilder ? 1 : 0 ), 
    (u64) g_CodeBranchID, 
    topLevelBuild.m_DependencyCheckTime,
    topLevelBuild.m_DownloadTime,
    topLevelBuild.m_UploadTime,
    topLevelBuild.m_BuildTime,
    topLevelBuild.m_InitializationTime,
    topLevelBuild.m_JobGatheringTime,
    topLevelBuild.m_SignatureCreationTime,
    topLevelBuild.m_UnaccountedTime,
    topLevelBuild.m_TotalTime );

  char queryStr[MAX_QUERY_LENGTH];
  _snprintf( queryStr, sizeof(queryStr), "%s", insertBuildBuff );
  queryStr[ sizeof(queryStr) - 1] = 0; 

  int execResult = mysql_query( g_MySQL, queryStr );
  if ( execResult != MYSQL_OK )
  {
    Console::Warning( "Failed to report builder statistics, AddTopLevelBuild failed.\n" );
    return false;
  }

  return true;
}
