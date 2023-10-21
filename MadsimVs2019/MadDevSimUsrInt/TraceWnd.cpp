/********1*********2*********3*********4*********5**********6*********7*********/
/*                                                                             */
/*  PRODUCT      : MAD Device Simulation Framework                             */
/*  COPYRIGHT    : (c) 2022 HTF Consulting                                     */
/*                                                                             */
/* This source code is provided by exclusive license for non-commercial use to */
/* XYZ Company                                                                 */
/*                                                                             */
/*******************************************************************************/
/*                                                                             */
/*  Exe file ID  : MadSimUI.exe                                                */
/*                                                                             */
/*  Module  NAME : TraceWnd.cpp                                                */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

// TraceWnd.cpp : implementation file
//
#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include <sys\timeb.h>
#include "TraceWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTraceWnd

CTraceWnd::CTraceWnd()

{
	timeBase = 0; // clear Time Base for normalization
}

CTraceWnd::~CTraceWnd()

{
}


BEGIN_MESSAGE_MAP(CTraceWnd, CListBox)
//{{AFX_MSG_MAP(CTraceWnd)
// NOTE - the ClassWizard will add and remove mapping macros here.
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTraceWnd message handlers

/*****************************************************************************
*					COLORS
*****************************************************************************/

#define COLOR_TIMESTAMP 	  RGB(  0,   0,   0) // 000 Black (timestamp)
#define COLOR_OUT_COMMAND     RGB(  0,   0, 128) // 001 Dk Blue
#define COLOR_ANNOTATION	  RGB(  0,   0, 255) // 001 Blue
#define COLOR_GENERAL   	  RGB(  0, 128,   0) // 010 DK Green
#define COLOR_COMMENT   	  RGB(  0, 128,   0)
#define COLOR_OUT_STATUS	  RGB(  0, 128, 128) // 011 Cyan
#define COLOR_ERROR 		  RGB(128,   0,   0)
#define COLOR_IN_COMMAND	  RGB(255,   0,   0) // 100 Red
#define COLOR_IN_STATUS 	  RGB(255,   0, 255) // 101 Magenta
// 110 Yellow (ugly)
// 111 White (useless)

/****************************************************************************
*   						  CTraceWnd::DrawItem
* Inputs:
*   	LPDRAWITEMSTRUCT dis:
* Result: void
*   	
* Effect: 
*   	Draws the items.
* Notes:
*	Uses the serialization subroutines to get the display string
****************************************************************************/

void CTraceWnd::DrawItem(LPDRAWITEMSTRUCT dis)

{
	CDC* dc = CDC::FromHandle(dis->hDC);
	static DWORD indent = 0;

	if (indent == 0)
		{
		/* compute indent */
		indent = dc->GetTextExtent(CString(_T("000000.000 "))).cx;
		} /* compute indent */

	if (dis->itemID == -1)
		{
		/* no items */
		CBrush bg(::GetSysColor(COLOR_WINDOW));
		dc->FillRect(&dis->rcItem, & bg);
		if (dis->itemState & ODS_FOCUS)
			{
			/* selected */
			dc->DrawFocusRect(&dis->rcItem);
			} /* selected */
		return;  // nothing to draw
		} /* no items */

	TraceItem* item = (TraceItem*) dis->itemData;

	if (dis->itemState & ODA_FOCUS)
		{
		/* focus only */
		dc->DrawFocusRect(&dis->rcItem);
		return;
		} /* focus only */

	int saved = dc->SaveDC();

	CBrush bg(::GetSysColor(COLOR_WINDOW));
	dc->FillRect(&dis->rcItem, & bg);
	dc->SetBkMode(TRANSPARENT);


	CString s;
	s = item->timeString(timeBase);

	int width = dc->GetTextExtent(s).cx + dc->GetTextExtent(_T(" "), 1).cx;

	dc->SetTextColor(COLOR_TIMESTAMP);
	dc->TextOut(dis->rcItem.left + indent - width, dis->rcItem.top, s);

	int x = dis->rcItem.left + indent;
	int y = dis->rcItem.top;

	s = item->getString();

	switch (item->type)
		{
			/* type */
			//----------------------------------------------------------------
			// Miscellaneous status
			//----------------------------------------------------------------

		case TRACE_TYPE_UNKNOWN:
			break;

		case TRACE_TYPE_SEND:
			dc->SetTextColor(COLOR_IN_COMMAND);
			break;

		case TRACE_TYPE_THREAD:
			dc->SetTextColor(COLOR_GENERAL);
			break;

		case TRACE_TYPE_INT:
			dc->SetTextColor(COLOR_GENERAL);
			break;

		case TRACE_TYPE_INPUT_DATA:
			dc->SetTextColor(COLOR_IN_COMMAND);
			break;

		case TRACE_TYPE_OUTPUT_DATA:
			dc->SetTextColor(COLOR_GENERAL);
			break;

		case TRACE_TYPE_ANNOTATION:
			dc->SetTextColor(COLOR_ANNOTATION);
			break;

		case TRACE_TYPE_COMMENT:
			dc->SetTextColor(COLOR_COMMENT);
			break;

		case TRACE_TYPE_ERROR:
			dc->SetTextColor(COLOR_ERROR);
			break;
			//----------------------------------------------------------------
			// Input command
			//----------------------------------------------------------------

		case TRACE_TYPE_GO_IN:
		case TRACE_TYPE_IE_IN:
		case TRACE_TYPE_IACK_IN:
		case TRACE_TYPE_RST_IN:
			dc->SetTextColor(COLOR_IN_COMMAND);
			break;

			//----------------------------------------------------------------
			// Input status
			//----------------------------------------------------------------

		case TRACE_TYPE_DONE_IN:
		case TRACE_TYPE_EOP_IN:
		case TRACE_TYPE_INT_IN:
		case TRACE_TYPE_OVR_IN:
		case TRACE_TYPE_ERR_IN:
			dc->SetTextColor(COLOR_IN_STATUS);
			break;

			//----------------------------------------------------------------
			// Output Command
			//----------------------------------------------------------------

		case TRACE_TYPE_GO_OUT:
		case TRACE_TYPE_IE_OUT:
		case TRACE_TYPE_IACK_OUT:
		case TRACE_TYPE_RST_OUT:
			dc->SetTextColor(COLOR_OUT_COMMAND);
			dc->SetTextColor(COLOR_OUT_COMMAND);
			break;

			//----------------------------------------------------------------
			// Output status
			//----------------------------------------------------------------

		case TRACE_TYPE_DONE_OUT:
		case TRACE_TYPE_BUSY_OUT:
		case TRACE_TYPE_INT_OUT:
		case TRACE_TYPE_UND_OUT:
		case TRACE_TYPE_ERR_OUT:
			dc->SetTextColor(COLOR_OUT_STATUS);
			break;

			//----------------------------------------------------------------
		} /* type */


	dc->TextOut(dis->rcItem.left + indent, dis->rcItem.top, s);

	if (dis->itemState & ODS_FOCUS)
		dc->DrawFocusRect(&dis->rcItem);

	dc->RestoreDC(saved);
}

void CTraceWnd::DeleteItem(LPDELETEITEMSTRUCT dis)

{
	if (dis->itemData != NULL)
		delete (TraceItem *) dis->itemData;
	CListBox::DeleteItem(dis);
}

/****************************************************************************
*   						 CTraceWnd::operator<<
* Inputs:
*   	CArchive & ar: Archive reference
* Result: CArchive &
*   	As per convention of << operator
* Effect: 
*   	Writes the items to the archive
****************************************************************************/
#pragma warning(suppress: 4800) 

CArchive& CTraceWnd::operator<<(CArchive& ar)

{
CString s;

	for (int i = 0; i < GetCount(); i++)
		{
		/* write each */
		TraceItem* item = (TraceItem*) GetItemDataPtr(i);
		s = item->timeString(timeBase);
		ar << (LPCTSTR)s;
		ar << _T(" ");

		s = item->getString();
		ar << (LPCTSTR)s;

		ar << _T("\r\n");
		} /* write each */

	return ar;
}

/****************************************************************************
*   						 CTraceWnd::AddString
* Inputs:
*   	TraceItem * item: Item to add
* Result: int
*   	The index of the added item
* Effect: 
*   	Adds the item to the box.  If the previous last item was visible and
*	the new item will be even partially obscured, the window is scrolled
*	by 1 to make the new item visible.  BUT if the last item was already
*	invisible, no such scrolling takes place
****************************************************************************/

int CTraceWnd::AddString(TraceItem* item)

{
CRect body;
BOOL visible = TRUE;
int result;

//    if (!m_bDisplayTrace)
//		return;

	int count = GetCount();
	GetClientRect(&body);

	if (count > 0)
		{
		/* handle scrolling */
		CRect r;
		CListBox::GetItemRect(count - 1, & r);
		if (r.top > body.bottom)
			visible = FALSE;
		} /* handle scrolling */

	modified = TRUE;  // indicate we have modified the list box
	result = CListBox::AddString((LPCTSTR) item); 

	if (visible)
		{
		/* may need to scroll */
		// The previous last item was visible.  See if the new item
		// is fully visible
		CRect r;

		// Use GetCount() here because we've added a new string!
		CListBox::GetItemRect(GetCount() - 1, & r);
		if (r.bottom > body.bottom)
			{
			/* force scrolling */
			int n = GetTopIndex();
			SetTopIndex(n + 1);  // scroll it
			} /* force scrolling */
		} /* may need to scroll */

	return result;
}

/****************************************************************************
*   						  CTraceWnd::display
* Result: CString
*   	The displayable form of the string
* Effect: 
*   	Constructs a displayable string
****************************************************************************/

CString TraceItem::display(DWORD timeBase)

{
	CString s;
	s = timeString(timeBase);
	s += _T(" ");

	s += getString();
	s += _T("\r\n");
	return s;
}

/****************************************************************************
*   						 TraceItem::timeString
* Inputs:
*   	TraceItem * item: Item whose time is to be formatted
* Result: CString
*   	Formatted time string
****************************************************************************/

CString TraceItem::timeString(DWORD timeBase)

{
	CString s;
	DWORD ms = timestamp.getMS() - timeBase;
	s.Format(_T("%6d.%03d"), ms / 1000, ms % 1000);
	return s;
}

/****************************************************************************
*   						 TraceItem::getString
* Inputs:
*   	TraceItem * item: Item to display
* Result: CString
*   	Display string for item
****************************************************************************/

CString TraceItem::getString()

{
	CString s;
	switch (type)
		{
			/* type */
			//----------------------------------------------------------------
			// Miscellaneous status
			//----------------------------------------------------------------

		case TRACE_TYPE_UNKNOWN:
#pragma warning(suppress: 6031)
			s.LoadString(IDS_ITEM_UNKNOWN);
			return s;

		case TRACE_TYPE_SEND:
			s.Format(_T("Sending %d bytes \"%s\""),
			  	data.GetLength(),
			  	(LPCTSTR) data);
			return s;

		case TRACE_TYPE_THREAD:
			s.Format(_T("Thread: %s"), (LPCTSTR) data);
			return s;

		case TRACE_TYPE_INT:
			s.Format(_T("Interrupt %s %s"), _T("Requested"), (LPCTSTR) data);
			return s;					   

		case TRACE_TYPE_INPUT_DATA:
			// TODO: not really Unicode-aware
			if (status < _T(' '))
				s.Format(_T("DATA IN = '\\x%02x' %s"),
				  	(BYTE) status,
				  	(LPCTSTR) data);
			else
				s.Format(_T("DATA IN = '%c'=='\\x%02x' %s"),
				  	(BYTE) status,
				  	(BYTE) status,
				  	(LPCTSTR) data);
			return s;

		case TRACE_TYPE_OUTPUT_DATA:
			// TODO: not really Unicode-aware
			if (status < _T(' '))
				s.Format(_T("DATA OUT '\\x%02x' %s"),
				  	(BYTE) status,
				  	(LPCTSTR) data);
			else
				s.Format(_T("DATA OUT '%c'=='\\x%02x' %s"),
				  	(BYTE) status,
				  	(BYTE) status,
				  	(LPCTSTR) data);
			return s;

		case TRACE_TYPE_ANNOTATION:
			s.Format(_T("%s %s"),
			  	(LPCTSTR) data,
			  	status ? _T("ON") : _T("OFF"));
			return s;

		case TRACE_TYPE_COMMENT:
			return data;

		case TRACE_TYPE_ERROR:
			return data;
			//----------------------------------------------------------------
			// Input command
			//----------------------------------------------------------------

		case TRACE_TYPE_GO_IN:
			return logEvent(_T("GO IN"));

		case TRACE_TYPE_IE_IN:
			return logEvent(_T("IE IN"));

		case TRACE_TYPE_IACK_IN:
			return logEvent(_T("IACK IN"));

		case TRACE_TYPE_RST_IN:
			return logEvent(_T("RST IN"));

			//----------------------------------------------------------------
			// Input status
			//----------------------------------------------------------------

		case TRACE_TYPE_DONE_IN:
			return logEvent(_T("DONE IN"));

		case TRACE_TYPE_EOP_IN:
			return logEvent(_T("EOP IN"));

		case TRACE_TYPE_INT_IN:
			return logEvent(_T("INT IN"));

		case TRACE_TYPE_OVR_IN:
			return logEvent(_T("OVR IN"));

		case TRACE_TYPE_ERR_IN:
			return logEvent(_T("ERR IN"));

			//----------------------------------------------------------------
			// Output Command
			//----------------------------------------------------------------

		case TRACE_TYPE_GO_OUT:
			return logEvent(_T("GO OUT"));

		case TRACE_TYPE_IE_OUT:
			return logEvent(_T("IE OUT"));

		case TRACE_TYPE_IACK_OUT:
			return logEvent(_T("IACK OUT"));

		case TRACE_TYPE_RST_OUT:
			return logEvent(_T("RST OUT"));
			//----------------------------------------------------------------
			// Output status
			//----------------------------------------------------------------

		case TRACE_TYPE_DONE_OUT:
			return logEvent(_T("DONE OUT"));

		case TRACE_TYPE_BUSY_OUT:
			return logEvent(_T("BUSY OUT"));

		case TRACE_TYPE_INT_OUT:
			return logEvent(_T("INT OUT"));

		case TRACE_TYPE_UND_OUT:
			return logEvent(_T("UND OUT"));

		case TRACE_TYPE_ERR_OUT:
			return logEvent(_T("ERR OUT"));

		case TRACE_TYPE_BIT_X:
			return logEvent(_T("Bit X changed"));

			//----------------------------------------------------------------
		default:
			s.Format(_T("Unimplemented item code %d"), type);
			return s;
		} /* type */
}

/****************************************************************************
*   						  TraceItem::logEvent
* Inputs:
*	LPCTSTR id: String ID
* Result: CString
*   	
* Effect: 
*   	Returns a string to log the event
****************************************************************************/

CString TraceItem::logEvent(LPCTSTR id)

{
	CString s;

	s = id;
	if (status)
		s += _T(" set");
	else
		s += _T(" cleared");
	if (data.GetLength() > 0)
		{
		/* annotation */
		s += _T(" ");
		s += data;
		} /* annotation */
	return s;
}
