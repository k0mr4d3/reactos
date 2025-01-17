/*
 * PROJECT:     ReactOS Cicero
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Cicero file mapping
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "cicmutex.h"

// class CicFileMappingStatic;
// class CicFileMapping;

class CicFileMappingStatic
{
protected:
    LPCTSTR m_pszName;
    LPVOID m_pView;
    HANDLE m_hMapping;
    BOOL m_bCreated;
    BOOL m_bHasMutex;
    CicMutex *m_pMutex;

    LPVOID _Map();

public:
    CicFileMappingStatic() { }
    ~CicFileMappingStatic() { }

    void Init(LPCTSTR pszName, CicMutex *pMutex);

    LPVOID Create(LPSECURITY_ATTRIBUTES pSA, DWORD dwMaximumSizeLow, LPBOOL pbAlreadyExists);
    LPVOID Open();
    void Close();

    BOOL Enter();
    void Leave();
    BOOL Flush(SIZE_T dwNumberOfBytesToFlush);
    void Finalize();
};

class CicFileMapping : public CicFileMappingStatic
{
public:
    CicFileMapping(LPCTSTR pszName, CicMutex *pMutex);
    virtual ~CicFileMapping() { Finalize(); }
};

/******************************************************************************/

inline
CicFileMapping::CicFileMapping(LPCTSTR pszName, CicMutex *pMutex)
{
    m_pszName = NULL;
    m_pView = NULL;
    m_hMapping = NULL;
    m_bCreated = FALSE;
    m_bHasMutex = FALSE;
    m_pMutex = NULL;
    Init(pszName, pMutex);
}

inline void CicFileMappingStatic::Close()
{
    if (m_pView)
    {
        ::UnmapViewOfFile(m_pView);
        m_pView = NULL;
    }

    if (m_hMapping)
    {
        ::CloseHandle(m_hMapping);
        m_hMapping = NULL;
    }

    m_bCreated = FALSE;
}

inline void CicFileMappingStatic::Init(LPCTSTR pszName, CicMutex *pMutex)
{
    if (pMutex)
        m_pMutex = pMutex;

    m_bCreated = FALSE;
    m_pszName = pszName;
    m_bHasMutex = (pMutex != NULL);
}

inline LPVOID
CicFileMappingStatic::Create(
    LPSECURITY_ATTRIBUTES pSA,
    DWORD dwMaximumSizeLow,
    LPBOOL pbAlreadyExists)
{
    if (!m_pszName)
        return NULL;

    m_hMapping = ::CreateFileMapping(INVALID_HANDLE_VALUE,
                                     pSA,
                                     PAGE_READWRITE,
                                     0,
                                     dwMaximumSizeLow,
                                     m_pszName);
    if (pbAlreadyExists)
        *pbAlreadyExists = (::GetLastError() == ERROR_ALREADY_EXISTS);
    if (!m_hMapping)
        return NULL;

    m_bCreated = TRUE;
    return _Map();
}

inline LPVOID CicFileMappingStatic::Open()
{
    if (!m_pszName)
        return NULL;
    m_hMapping = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, m_pszName);
    if (!m_hMapping)
        return NULL;

    return _Map();
}

inline LPVOID CicFileMappingStatic::_Map()
{
    m_pView = ::MapViewOfFile(m_hMapping, FILE_MAP_WRITE, 0, 0, 0);
    if (!m_pView)
    {
        Close();
        return NULL;
    }
    return m_pView;
}

inline BOOL CicFileMappingStatic::Enter()
{
    if (!m_bHasMutex)
        return TRUE;
    return m_pMutex->Enter();
}

inline void CicFileMappingStatic::Leave()
{
    if (!m_bHasMutex)
        return;
    m_pMutex->Leave();
}

inline BOOL CicFileMappingStatic::Flush(SIZE_T dwNumberOfBytesToFlush)
{
    if (!m_pView)
        return FALSE;
    return ::FlushViewOfFile(m_pView, dwNumberOfBytesToFlush);
}

inline void CicFileMappingStatic::Finalize()
{
    if (!m_bHasMutex)
        return;

    Close();
    Leave();
}
