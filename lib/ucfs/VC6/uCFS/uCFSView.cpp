// uCFSView.cpp : implementation of the CUCFSView class
//

#include "stdafx.h"
#include "uCFS.h"

#include "uCFSDoc.h"
#include "uCFSView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef __cplusplus
extern "C" {
#endif

void SDCard_insert (int dev, const char *sdmmc_file_name, __int64 sdmmc_size);
void SDCard_remove (int dev);
int FS_FormatSD(const char *sVolumeName);
void __cdecl XM_printf (const char *fmt, ...);
int _fs_sd_function_test (unsigned int sd_dev_index);
int os_main (void);


#ifdef __cplusplus
}
#endif


/////////////////////////////////////////////////////////////////////////////
// CUCFSView

IMPLEMENT_DYNCREATE(CUCFSView, CView)

BEGIN_MESSAGE_MAP(CUCFSView, CView)
	//{{AFX_MSG_MAP(CUCFSView)
	ON_COMMAND(ID_SDMMC_2GB, OnSdmmc2gb)
	ON_UPDATE_COMMAND_UI(ID_SDMMC_2GB, OnUpdateSdmmc2gb)
	ON_COMMAND(ID_SDMMC_8GB, OnSdmmc8gb)
	ON_UPDATE_COMMAND_UI(ID_SDMMC_8GB, OnUpdateSdmmc8gb)
	ON_COMMAND(ID_SDMMC_UNPLUG, OnSdmmcUnplug)
	ON_UPDATE_COMMAND_UI(ID_SDMMC_UNPLUG, OnUpdateSdmmcUnplug)
	ON_COMMAND(ID_SDMMC_FORMAT, OnSdmmcFormat)
	ON_COMMAND(ID_TEST_FS_SD_TEST, OnTestFsSdTest)
	ON_COMMAND(ID_SDMMC_TEST_MAIN, OnSdmmcTestMain)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUCFSView construction/destruction

CUCFSView::CUCFSView()
{
	// TODO: add construction code here

}

CUCFSView::~CUCFSView()
{
}

BOOL CUCFSView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CUCFSView drawing

void CUCFSView::OnDraw(CDC* pDC)
{
	CUCFSDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CUCFSView diagnostics

#ifdef _DEBUG
void CUCFSView::AssertValid() const
{
	CView::AssertValid();
}

void CUCFSView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CUCFSDoc* CUCFSView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CUCFSDoc)));
	return (CUCFSDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUCFSView message handlers

void CUCFSView::OnSdmmc2gb() 
{
	// TODO: Add your command handler code here
	__int64 sd_size = 2 * 1024 * 1024;
	sd_size *= 1024;

	SDCard_insert (0, "D:\\SDMMC_2GB.BIN", sd_size);
}

void CUCFSView::OnUpdateSdmmc2gb(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CUCFSView::OnSdmmc8gb() 
{
	// TODO: Add your command handler code here
	__int64 sd_size = 8 * 1024 * 1024;
	sd_size *= 1024;

	SDCard_insert (0, "D:\\SDMMC_8GB.BIN", sd_size);
	
}

void CUCFSView::OnUpdateSdmmc8gb(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CUCFSView::OnSdmmcUnplug() 
{
	// TODO: Add your command handler code here
	SDCard_remove (0);
}

void CUCFSView::OnUpdateSdmmcUnplug(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CUCFSView::OnSdmmcFormat() 
{
	// TODO: Add your command handler code here
	if(FS_FormatSD ("mmc:0:") == 0)
		XM_printf ("Format success\n");
	else
		XM_printf ("Format failure\n");
}

void CUCFSView::OnTestFsSdTest() 
{
	// TODO: Add your command handler code here
	_fs_sd_function_test (0);
}

void CUCFSView::OnSdmmcTestMain() 
{
	// TODO: Add your command handler code here
	os_main ();
}
