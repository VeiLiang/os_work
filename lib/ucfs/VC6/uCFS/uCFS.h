// uCFS.h : main header file for the UCFS application
//

#if !defined(AFX_UCFS_H__421A1076_3131_4BB3_9F5E_14925059C44E__INCLUDED_)
#define AFX_UCFS_H__421A1076_3131_4BB3_9F5E_14925059C44E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CUCFSApp:
// See uCFS.cpp for the implementation of this class
//

class CUCFSApp : public CWinApp
{
public:
	CUCFSApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUCFSApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CUCFSApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UCFS_H__421A1076_3131_4BB3_9F5E_14925059C44E__INCLUDED_)
