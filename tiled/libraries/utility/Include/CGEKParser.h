#pragma once

#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>

class CGEKParser
{
private:
    CStringW m_strScript;
    INT32 m_nPosition;

    CStringW m_strToken;

private:
    virtual bool IsBlank(void);
    virtual bool IsSolo(void);

    virtual bool IsCommentLine(void);
    virtual bool IsCommentBegin(void);
    virtual bool IsCommentEnd(void);

public:
    CGEKParser(void);
    virtual ~CGEKParser(void);

    HRESULT Load(LPCWSTR pScript);
    HRESULT LoadFromFile(LPCWSTR pFileName);
    void Reset(void);

    bool IsEOF(void);
    bool NextToken(void);
    CStringW GetToken(void);
};
