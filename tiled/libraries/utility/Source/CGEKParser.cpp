#include "CGEKParser.h"
#include "GEKUtility.h"

CGEKParser::CGEKParser(void)
    : m_nPosition(0)
{
}

CGEKParser::~CGEKParser(void)
{
}

HRESULT CGEKParser::Load(LPCWSTR pScript)
{
    m_strScript = pScript;
    m_nPosition = 0;
    return S_OK;
}

HRESULT CGEKParser::LoadFromFile(LPCWSTR pFileName)
{
    CStringW strBuffer;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, strBuffer);
    if (FAILED(hRetVal))
    {
        return hRetVal;
    }

    return Load(strBuffer);
}

void CGEKParser::Reset(void)
{
    m_nPosition = 0;
}

bool CGEKParser::IsSolo(void)
{
    if (m_strScript[m_nPosition] == '(') return true;
    if (m_strScript[m_nPosition] == ')') return true;
    if (m_strScript[m_nPosition] == '{') return true;
    if (m_strScript[m_nPosition] == '}') return true;
    if (m_strScript[m_nPosition] == ',') return true;
    return false;
}

bool CGEKParser::IsBlank(void)
{
    if (m_strScript[m_nPosition] == ' ') return true;
    if (m_strScript[m_nPosition] == '\r') return true;
    if (m_strScript[m_nPosition] == '\n') return true;
    if (m_strScript[m_nPosition] == '\t') return true;
    return false;
}

bool CGEKParser::IsCommentLine(void)
{
    if (m_nPosition > (m_strScript.GetLength() - 2)) return false;
    if ((m_strScript[m_nPosition] == '/')&&(m_strScript[m_nPosition + 1] == '/')) return true;
    return false;
}

bool CGEKParser::IsCommentBegin(void)
{
    if (m_nPosition > (m_strScript.GetLength() - 2)) return false;
    if ((m_strScript[m_nPosition] == '/')&&(m_strScript[m_nPosition + 1] == '*')) return true;
    return false;
}

bool CGEKParser::IsCommentEnd(void)
{
    if (m_nPosition > (m_strScript.GetLength() - 2)) return false;
    if ((m_strScript[m_nPosition] == '*')&&(m_strScript[m_nPosition + 1] == '/')) return true;
    return false;
}

bool CGEKParser::IsEOF(void)
{
    return (m_nPosition >= m_strScript.GetLength());
}

bool CGEKParser::NextToken(void)
{
    m_strToken.Empty();
    while (!IsEOF() && IsBlank())
    {
        m_nPosition++;
    };

    if (IsEOF())
    {
        return false;
    }

    if (IsSolo())
    {
        m_strToken += m_strScript[m_nPosition];
        m_nPosition++;
    }
    else if (m_strScript[m_nPosition] == L'"')
    {
        m_nPosition++;
        while (!IsEOF() && m_strScript[m_nPosition] != L'"')
        {
            m_strToken += m_strScript[m_nPosition];
            m_nPosition++;
        };

        m_nPosition++;
    }
    else
    {
        while (!IsEOF())
        {
            if (IsEOF() || IsBlank() || IsSolo())
            {
                break;
            }
            else if (IsCommentBegin())
            {
                while (!IsEOF() && !IsCommentEnd())
                {
                    m_nPosition++;
                };

                m_nPosition += 2;
                if (!m_strToken.IsEmpty())
                {
                    break;
                }
                else
                {
                    return NextToken();
                }
            }
            else if (IsCommentLine())
            {
                while (!IsEOF() && (m_strScript[m_nPosition] != L'\r' && m_strScript[m_nPosition] != L'\n'))
                {
                    m_nPosition++;
                };

                while (!IsEOF() && (m_strScript[m_nPosition] == L'\r' || m_strScript[m_nPosition] == L'\n'))
                {
                    m_nPosition++;
                };

                if (!m_strToken.IsEmpty())
                {
                    break;
                }
                else
                {
                    return NextToken();
                }
            }
            else
            {
                m_strToken += m_strScript[m_nPosition];
                m_nPosition++;
            }
        };
    }

    return (!m_strToken.IsEmpty());
}

CStringW CGEKParser::GetToken(void)
{
    return m_strToken;
}
