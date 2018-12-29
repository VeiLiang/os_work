// uCFSDoc.h : interface of the CUCFSDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_UCFSDOC_H__C0E4D944_171B_484E_A65B_1EA53ACF02C2__INCLUDED_)
#define AFX_UCFSDOC_H__C0E4D944_171B_484E_A65B_1EA53ACF02C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CUCFSDoc : public CDocument
{
protected: // create from serialization only
	CUCFSDoc();
	DECLARE_DYNCREATE(CUCFSDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUCFSDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUCFSDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CUCFSDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UCFSDOC_H__C0E4D944_171B_484E_A65B_1EA53ACF02C2__INCLUDED_)
