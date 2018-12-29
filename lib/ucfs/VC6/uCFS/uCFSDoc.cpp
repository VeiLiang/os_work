// uCFSDoc.cpp : implementation of the CUCFSDoc class
//

#include "stdafx.h"
#include "uCFS.h"

#include "uCFSDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUCFSDoc

IMPLEMENT_DYNCREATE(CUCFSDoc, CDocument)

BEGIN_MESSAGE_MAP(CUCFSDoc, CDocument)
	//{{AFX_MSG_MAP(CUCFSDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUCFSDoc construction/destruction

CUCFSDoc::CUCFSDoc()
{
	// TODO: add one-time construction code here

}

CUCFSDoc::~CUCFSDoc()
{
}

BOOL CUCFSDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CUCFSDoc serialization

void CUCFSDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CUCFSDoc diagnostics

#ifdef _DEBUG
void CUCFSDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CUCFSDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUCFSDoc commands
