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
/*  Module  NAME : SpinnerButton.cpp                                           */
/*                                                                             */
/*  DESCRIPTION  : Function definition(s)                                      */
/*                                                                             */
/*******************************************************************************/

// SpinnerButton.cpp : implementation file
//

#include "stdafx.h"
#include "MadDevSimUsrInt.h"
#include "SpinnerButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBrush CSpinnerButton::green(RGB(0, 128, 0));
CBrush CSpinnerButton::red(RGB(255, 0, 0));

/////////////////////////////////////////////////////////////////////////////
// CSpinnerButton

CSpinnerButton::CSpinnerButton()

{
	pos = 0;
	mode = TRUE;  // "Green" mode
}

CSpinnerButton::~CSpinnerButton()

{
}


BEGIN_MESSAGE_MAP(CSpinnerButton, CButton)
//{{AFX_MSG_MAP(CSpinnerButton)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpinnerButton message handlers

void CSpinnerButton::DrawItem(LPDRAWITEMSTRUCT dis)

{
	CPoint a;
	CPoint b;
	CRect r;
	CRect client;
	CPoint p1;
	CPoint p2;
	int width = ::GetSystemMetrics(SM_CXBORDER) * 4;

	GetClientRect(&client);
	r = client;
	r.InflateRect(-1, -1);
	CDC* dc = CDC::FromHandle(dis->hDC);

	switch (pos)
		{
			/* pos */
		case 0:
			//   a    
			//  /|\
			//p1 | p2
			//   |   
			//   b 
			a.x = r.Width() / 2;
			a.y = r.top;

			b.x = a.x;
			b.y = r.bottom;

			p1.x = a.x - width;
			p1.y = a.y + width;
			p2.x = a.x + width + 1;
			p2.y = a.y + width + 1;
			break;
		case 1:
			// p1_ a
			//    /|
			//   / p2
			//  /
			// b
			a.x = r.right;
			a.y = r.top;

			b.x = r.left;
			b.y = r.bottom;

			p1.x = a.x - width;
			p1.y = a.y;
			p2.x = a.x;
			p2.y = a.y + width + 1;
			break;
		case 2:
			//  	p1
			//  b ___\ a
			//  	 /
			//  	p2

			a.x = r.right;
			a.y = r.Height() / 2;

			b.x = r.left;
			b.y = a.y;

			p1.x = a.x - width;
			p1.y = a.y - width;
			p2.x = p1.x - 1;
			p2.y = a.y + width + 1;
			break;
		case 3:
			//   b
			//    \
			//     \ p1
			//   p2_\|
			//  	 a
			a.x = r.right;
			a.y = r.bottom;

			b.x = r.left;
			b.y = r.top;

			p1.x = a.x;
			p1.y = a.y - width;
			p2.x = a.x - width - 1;
			p2.y = a.y;
			break;
		case 4:
			//     b
			//     |
			//   p1| p2
			//    \|/
			//     a
			a.x = r.Width() / 2;
			a.y = r.bottom;
			b.x = a.x;
			b.y = r.top;

			p1.x = a.x - width;
			p1.y = a.y - width;
			p2.x = a.x + width + 1;
			p2.y = p1.y - 1;
			break;
		case 5:
			//  	  b
			//  	 /
			//   p1 /
			//    |/_ p2
			//    a
			a.x = r.left;
			a.y = r.bottom;
			b.x = r.right;
			b.y = r.top;
			p1.x = a.x;
			p1.y = a.y - width;
			p2.x = a.x + width + 1;
			p2.y = a.y;
			break;
		case 6:
			//     p1
			//   a/___ b
			//    \
			//     p2
			a.x = r.left;
			a.y = r.Height() / 2;
			b.x = r.right;
			b.y = a.y;
			p1.x = a.x + width;
			p1.y = a.y - width;
			p2.x = p1.x + 1;
			p2.y = a.y + width + 1;
			break;
		case 7:
			//
			// a _ p2
			//  |\
			// p1 \
			//     \
			//  	b
			a.x = r.left;
			a.y = r.top;
			b.x = r.right;
			b.y = r.bottom;
			p1.x = a.x;
			p1.y = a.y + width;
			p2.x = a.x + width + 1;
			p2.y = a.y;
			break;
		} /* pos */

	dc->FillRect(&client, mode ? &green : &red);
	dc->SelectStockObject(WHITE_PEN);
	dc->MoveTo(a);
	dc->LineTo(b);
	dc->MoveTo(p1);
	dc->LineTo(a);
	dc->LineTo(p2);
}
