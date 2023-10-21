// MadDevIoDoc.cpp : implementation of the CMadDevIoDoc class
//

#include "stdafx.h"
#include "MadTestApp.h"

#include "MadTestAppDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoDoc

IMPLEMENT_DYNCREATE(CMadDevIoDoc, CDocument)

BEGIN_MESSAGE_MAP(CMadDevIoDoc, CDocument)
	//{{AFX_MSG_MAP(CMadDevIoDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoDoc construction/destruction

CMadDevIoDoc::CMadDevIoDoc()
{
	// TODO: add one-time construction code here

}

CMadDevIoDoc::~CMadDevIoDoc()
{
}

BOOL CMadDevIoDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CMadDevIoDoc serialization

void CMadDevIoDoc::Serialize(CArchive& ar)
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
// CMadDevIoDoc diagnostics

#ifdef _DEBUG
void CMadDevIoDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMadDevIoDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMadDevIoDoc commands
