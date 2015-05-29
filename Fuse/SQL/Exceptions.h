#pragma once

#include "API.h"
#include "DBManager.h"
#include "Common/Exception.h"

namespace SQL
{
  DEFINE_EXCEPTION_CLASS

  class DBManagerException : public Exception
  {
  protected:
    int m_ErrorCode;

  protected:
    DBManagerException()
    {
    }

  public:
    DBManagerException( DBManager* dbManager, const char* function )
    {
      m_ErrorCode = dbManager->GetLastErrCode();
      m_Message = dbManager->GetLastErrMsg();

      m_Message += "\n  Database: ";
      m_Message += dbManager->GetDBFilename();
      m_Message += "\n  Function: ";
      m_Message += function;
    }

    DBManagerException( DBManager* dbManager, const char* function, const char *msgFormat, ... )
    {
      m_ErrorCode = dbManager->GetLastErrCode();
      m_Message = dbManager->GetLastErrMsg();

      char msgBuffer[Nocturnal::ERROR_STRING_BUF_SIZE];
      if ( strcmp( msgFormat, "" ) != 0 )
      {
        // reuse of msgBuffer requires clear (why?)
        memset( msgBuffer, '\0', sizeof(msgBuffer)); 

        va_list msgArgs;
        va_start( msgArgs, msgFormat );
        vsnprintf_s( msgBuffer, sizeof(msgBuffer), _TRUNCATE, msgFormat, msgArgs );
        msgBuffer[sizeof(msgBuffer) - 1] = 0; 
        va_end( msgArgs );


        m_Message += "; ";
        m_Message += msgBuffer;
      }

      m_Message += "\n  Database: ";
      m_Message += dbManager->GetDBFilename();
      m_Message += "\n  Function: ";
      m_Message += function;
    }
  };


  class StmtHandleNotFoundException : public SQL::Exception
  {
  public:
    StmtHandleNotFoundException( DBManager* dbManager, const char* function )
      : Exception( "Statement handle not found!" )
    {
      m_Message += "\n  Database: ";
      m_Message += dbManager->GetDBFilename();
      m_Message += "\n  Function: ";
      m_Message += function;
    }
  };

}