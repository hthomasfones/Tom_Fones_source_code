// MadDevIoView.cpp : implementation of the CMadDevIoView class
//

#include "stdafx.h"
#include "MadTestApp.h"

#include "MadTestAppDoc.h"
#include "MadTestAppView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern short int gnCurDsplyCnt;
extern char gDisplayBufr[][MAXTEXTLEN];
extern short int gnTextColumn;
extern short int gnTextRow;

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoView

IMPLEMENT_DYNCREATE(CMadDevIoView, CView)

BEGIN_MESSAGE_MAP(CMadDevIoView, CView)
	//{{AFX_MSG_MAP(CMadDevIoView)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoView construction/destruction

CMadDevIoView::CMadDevIoView()
{
	// TODO: add construction code here

}

CMadDevIoView::~CMadDevIoView()
{
}

BOOL CMadDevIoView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoView drawing

void CMadDevIoView::OnDraw(CDC* pDC)
{
	CMadDevIoDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoView printing

BOOL CMadDevIoView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CMadDevIoView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CMadDevIoView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoView diagnostics

#ifdef _DEBUG
void CMadDevIoView::AssertValid() const
{
	CView::AssertValid();
}

void CMadDevIoView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMadDevIoDoc* CMadDevIoView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMadDevIoDoc)));
	return (CMadDevIoDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoView message handlers

void CMadDevIoView::OnPaint() 

{
register int j;
int nLineCnt = 0;
int nVertInc = 20;
int nHorzInit = 5;
int nVertInit = 5;
BOOL bRC;
CPaintDC cDC(this); // device context for painting

    for (j = gnTextRow; j < gnCurDsplyCnt; j++)
        {
        bRC = cDC.TextOut(nHorzInit, ((STDTEXTHEIGHT*nLineCnt)+nVertInit),
                          &gDisplayBufr[j][gnTextColumn]); 
        nLineCnt++; 
        }

    if (gnCurDsplyCnt >= MAXTEXTLINES)
        gnCurDsplyCnt = 0;

    return;
}
