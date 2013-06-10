#pragma once

#include "Platform/Types.h"
#include "Platform/Locks.h"

#include "Foundation/Log.h"
#include "Foundation/SmartPtr.h"

#include "Application/API.h"

namespace Helium
{
    namespace IPC
    {
        class Message;
        class Connection;
    }

    namespace Worker
    {
        // This is the allotted time (in millis) that:
        //  - Manager's Process::Create() will wait for IPC connection from the running Worker process
        //  - Worker's Worker::Initialize function will wait for a connection with the Manager process
        const static int DefaultWorkerTimeout = 10000;

        struct HELIUM_APPLICATION_API Args
        {
            static const char* Worker;
            static const char* Debug;
            static const char* Wait;
        };

#pragma warning ( disable: 4200 )
        struct ConsoleOutput
        {
            Log::Stream m_Stream;
            Log::Level  m_Level;
            int         m_Indent;
            char       m_String[0];
        };

        const static uint32_t ConsoleOutputMessage = 0;
#pragma warning ( default: 4200 )

        class HELIUM_APPLICATION_API Process : public Helium::RefCountBase<Process>
        {
        private:
            // the file to execute
            std::string m_Executable;

            // the process handle
            void* m_Handle;

            // the process IPC connection
            IPC::Connection* m_Connection;

            // mutex to handles killing the process
            Helium::Mutex m_KillMutex;

            // this process was killed
            bool m_Killed;
            bool m_Debug; // we're debugging?
            bool m_Wait;

        public:
            // process tracking support
            static Process* Create( const std::string& executable, bool debug, bool wait );
            static void Release( Process*& worker );
            static void ReleaseAll();

        private:
            // protect constructor so all worker processes are tracked
            Process( const std::string& executable, bool debug, bool wait );

        public:
            // will destroy process if its not over or killed
            ~Process();

            // create process
            bool Start( int timeout = DefaultWorkerTimeout );

            // you must delete the object this returns, if non-null
            IPC::Message* Receive( bool wait = true );

            // a copy is made into the IPC connection system
            bool Send(uint32_t id, uint32_t size = -1, const uint8_t* data = NULL);

            // test to see if its still running
            bool Running();

            // wait for exit and return success/fail
            int Finish();

            // destroy the process
            void Kill();
        };
    }
}