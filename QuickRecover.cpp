#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "QuickRecover.h"

static int CoreSignals[] = {SIGILL, SIGQUIT, SIGABRT, SIGSEGV};

//////////////////////////////////////////////////////////////////////////

void RecoverSigHandler(int iSignal)
{
    bool bGetLock = false;
    if (QuickRecover::GetInstance()->SetLock(QuickRecover::LT_Core, true) == 0)
    {
        bGetLock = true;
    }
    QuickRecover::GetInstance()->SetFlag(QuickRecover::SF_Recover);
    QuickRecover::GetInstance()->SetLock(QuickRecover::LT_Start, false);
    if (bGetLock == false)
    {
        return;
    } 
    int  iPID = getpid();
    char szCommand[64];
    snprintf(szCommand, sizeof(szCommand), "ionice -c3 -p %d", iPID);
    system(szCommand);
    signal(iSignal, SIG_DFL);
    kill(iPID, iSignal);
}

//////////////////////////////////////////////////////////////////////////

int QuickRecover::Initialize(int iServId)
{
    char szPath[256];
    snprintf(szPath, sizeof(szPath), "/tmp/recover_%d", iServId);
    return Initialize(szPath);
}

int QuickRecover::Initialize(const char* szPath)
{
    if (szPath == NULL)
    {
        return -1;
    }
    if (OpenFile(szPath) != 0)
    {
        return -2;
    }
    if (SetLock(LT_Start, true) != 0)
    {
        exit(0);
    }
    if (SetFlag(SF_Recover) != 0)
    {
        return -4;
    }    
    int iSignalNum = (int)(sizeof(CoreSignals) / sizeof(CoreSignals[0]));
    for (int i = 0; i < iSignalNum; ++i)
    {
        signal(CoreSignals[i], RecoverSigHandler);
    }
    
    while (true)
    {
        if (GetFlag() == SF_Exit)
        {
            exit(0);
        }
        if (fork() != 0)
        {
            break;
        }
        if (WaitLock(LT_Start) != 0)
        {
            return -5;
        }
    }
    if (SetFlag(SF_Exit) != 0)
    {
        return -6;
    }    
    return 0;
}

int QuickRecover::OpenFile(const char* szPath)
{
    umask(S_IWGRP | S_IWOTH);
    m_iFile = open(szPath, (O_RDWR | O_CREAT | O_SYNC), 0666);
    if (m_iFile < 0)
    {
        return -1;
    }
    return 0;
}

int QuickRecover::SetLock(LockType eType, bool bLock)
{
    struct flock stLock;
    stLock.l_type   = bLock == true ? F_WRLCK : F_UNLCK;
    stLock.l_pid    = getpid();
    stLock.l_whence = SEEK_SET;
    stLock.l_start  = eType;
    stLock.l_len    = 1;
    while (fcntl(m_iFile, F_SETLK, &stLock) == -1)
    {
        if (errno == EAGAIN)
        {
            continue;
        }
        return -1;
    }
    return 0;
}

int QuickRecover::WaitLock(LockType eType)
{
    struct flock stLock;
    stLock.l_type   = F_WRLCK;
    stLock.l_pid    = getpid();
    stLock.l_whence = SEEK_SET;
    stLock.l_start  = eType;
    stLock.l_len    = 1;
    while (fcntl(m_iFile, F_SETLKW, &stLock) == -1)
    {
        if (errno == EAGAIN || errno == EACCES)
        {
            continue;
        }
        return -1;
    }
    return 0;
}

int QuickRecover::SetFlag(StartFlag eFlag)
{
    char chFlag = eFlag == SF_Recover ? '1' : '0';
    while (pwrite(m_iFile, (void*)&chFlag , 1, 0) == -1)
    {
        if (errno == EAGAIN)
        {
            continue;
        }
        return -2;
    }
    return 0;
}

QuickRecover::StartFlag QuickRecover::GetFlag()
{
    char chFlag = '0';
    while (pread(m_iFile, (void*)&chFlag , 1, 0) == -1)
    {
        if (errno == EAGAIN)
        {
            continue;
        }
        return SF_Exit;
    }
    return chFlag != '0' ? SF_Recover : SF_Exit;
}
