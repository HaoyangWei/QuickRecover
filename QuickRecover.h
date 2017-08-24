class QuickRecover
{
    enum LockType
    {
        LT_Start = 0,
        LT_Core  = 1,
    };
    enum StartFlag
    {
        SF_Exit    = 0,
        SF_Recover = 1,
    };
    friend void RecoverSigHandler(int iSignal);

public:
    static QuickRecover* GetInstance()
    {
        static QuickRecover soObj;
        return &soObj;
    }
    int Initialize(int iServId);
    int Initialize(const char* szPath);

private:
    int OpenFile(const char* szPath);
    int SetLock(LockType eType, bool bLock);
    int WaitLock(LockType eType);

private:
    int SetFlag(StartFlag eFlag);
    StartFlag GetFlag();
    
private:
    int m_iFile;

private:
    QuickRecover(): m_iFile(-1) {  }
    QuickRecover(const QuickRecover& roOther) { }
};
