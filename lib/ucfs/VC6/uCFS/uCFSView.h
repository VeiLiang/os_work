// uCFSView.h : interface of the CUCFSView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_UCFSVIEW_H__0282B5F9_98D4_4775_938B_1DAF3D8E8A35__INCLUDED_)
#define AFX_UCFSVIEW_H__0282B5F9_98D4_4775_938B_1DAF3D8E8A35__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CUCFSView : public CView
{
protected: // create from serialization only
	CUCFSView();
	DECLARE_DYNCREATE(CUCFSView)

// Attributes
public:
	CUCFSDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUCFSView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUCFSView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CUCFSView)
	afx_msg void OnSdmmc2gb();
	afx_msg void OnUpdateSdmmc2gb(CCmdUI* pCmdUI);
	afx_msg void OnSdmmc8gb();
	afx_msg void OnUpdateSdmmc8gb(CCmdUI* pCmdUI);
	afx_msg void OnSdmmcUnplug();
	afx_msg void OnUpdateSdmmcUnplug(CCmdUI* pCmdUI);
	afx_msg void OnSdmmcFormat();
	afx_msg void OnTestFsSdTest();
	afx_msg void OnSdmmcTestMain();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in uCFSView.cpp
inline CUCFSDoc* CUCFSView::GetDocument()
   { return (CUCFSDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UCFSVIEW_H__0282B5F9_98D4_4775_938B_1DAF3D8E8A35__INCLUDED_)
