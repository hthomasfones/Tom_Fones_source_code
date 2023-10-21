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
/*  Module  NAME : TraceWnd.h                                                  */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, structures, classes, etc.              */
/*                                                                             */
/*******************************************************************************/

// TraceWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTraceWnd window

//----------------------------------------------------------------
// Miscellaneous status
//----------------------------------------------------------------
#define TRACE_TYPE_UNKNOWN  	0
#define TRACE_TYPE_SEND 		1
#define TRACE_TYPE_THREAD   	2 // 'data' is name of thread running
#define TRACE_TYPE_INT  		3 // interrupt generated or suppressed
#define TRACE_TYPE_INPUT_DATA   4
#define TRACE_TYPE_OUTPUT_DATA  5
#define TRACE_TYPE_ANNOTATION   6
#define TRACE_TYPE_COMMENT  	7
#define TRACE_TYPE_ERROR		8
//----------------------------------------------------------------
// Input Command
//----------------------------------------------------------------
#define TRACE_TYPE_GO_IN	   10 
#define TRACE_TYPE_IE_IN	   11
#define TRACE_TYPE_IACK_IN     12
#define TRACE_TYPE_RST_IN      13
//----------------------------------------------------------------
// Input Status
//----------------------------------------------------------------
#define	TRACE_TYPE_DONE_IN     20
#define TRACE_TYPE_EOP_IN      21
#define TRACE_TYPE_INT_IN      22
#define TRACE_TYPE_OVR_IN      23
#define TRACE_TYPE_ERR_IN      24
//----------------------------------------------------------------
// Output Command
//----------------------------------------------------------------
#define TRACE_TYPE_GO_OUT      30
#define TRACE_TYPE_IE_OUT      31
#define TRACE_TYPE_IACK_OUT    32
#define TRACE_TYPE_RST_OUT     33
//----------------------------------------------------------------
// Output Status
//----------------------------------------------------------------
#define TRACE_TYPE_DONE_OUT    40
#define TRACE_TYPE_BUSY_OUT    41
#define TRACE_TYPE_INT_OUT     42
#define TRACE_TYPE_UND_OUT     43
#define TRACE_TYPE_ERR_OUT     44

#define TRACE_TYPE_BIT_X       45
//----------------------------------------------------------------

/*****************************************************************************
				   TimeStamp
*****************************************************************************/

class TimeStamp
	{
	public:
	TimeStamp()

	{
		ms = 0;
	}
	TimeStamp(BOOL value)

	{
		if (value)
			reset();
		else
			ms = 0;
	}
	DWORD getMS()

	{
		return ms;
	}
	void reset()

	{
		struct _timeb	t;
		_ftime_s(&t); 
		ms = (DWORD)(t.time * 1000 + t.millitm);
	}
	protected:
	DWORD	ms;
	};

/*****************************************************************************
				   TraceItem
 TraceItem()
 TraceItem(int type)			   
 TraceItem(int type, LPCTSTR d)
 TraceItem(int type, CString & d)
 TraceItem(int type, int status)
 TraceItem(int type, int status, LPCTSTR d)
 TraceItem(int type, int status, CString & d)
*****************************************************************************/
class TraceItem
	{
	public:
	TraceItem()

	{
		type = TRACE_TYPE_UNKNOWN; 
		status = 0; 
		timestamp.reset();
	}
	TraceItem(int t)

	{
		type = t;
		status = 0;
		timestamp.reset();
	}
	TraceItem(int t, LPCTSTR d)

	{
		type = t; 
		status = 0; 
		timestamp.reset(); 
		if (d != NULL)
			data = d;
	}
	TraceItem(int t, CString& d)

	{
		type = t; 
		status = 0; 
		timestamp.reset(); 
		data = d;
	}
	TraceItem(int t, int st, LPCTSTR d)

	{
		type = t; 
		status = st; 
		timestamp.reset(); 
		if (d != NULL)
			data = d;
	}
	TraceItem(int t, int st, CString& d)

	{
		type = t; 
		status = st; 
		timestamp.reset(); 
		data = d;
	}
	TraceItem(int t, int st)

	{
		type = t; 
		status = st; 
		timestamp.reset();
	}
	virtual ~TraceItem()

	{
	}
	CString		display(DWORD timeBase);
	CString		timeString(DWORD timeBase);
	CString		getString();
	CString		logEvent(LPCTSTR id);

	int			type;
	TimeStamp	timestamp;

	int			status;    // State information (per-type specific)
	CString		data;  // Annotations
	};

/*****************************************************************************
				   CTraceWnd
*****************************************************************************/

class CTraceWnd: public CListBox
	{
	// Construction
	public:
		CTraceWnd();

	// Attributes
	public:

	// Operations
	public:
	int	AddString(TraceItem* item); 
	void ResetContent()

	{
		modified = TRUE; CListBox::ResetContent();
	}
	BOOL GetModified()

	{
		return modified;
	}
	void SetModified(BOOL val)

	{
		modified = val;
	}

	CArchive&	operator <<(CArchive& ar);
	void setTimeBase(DWORD ms)

	{
		timeBase = ms;
	}
	DWORD getTimeBase()

	{
		return timeBase;
	}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTraceWnd)
	public:
	virtual void	DrawItem(LPDRAWITEMSTRUCT dis);
	virtual void	DeleteItem(LPDELETEITEMSTRUCT dis);
	//}}AFX_VIRTUAL

	// Implementation
	public:
	virtual			~CTraceWnd();

	// Generated message map functions
	protected:
	BOOL			modified;
	DWORD			timeBase;

	//{{AFX_MSG(CTraceWnd)
	// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	};

/////////////////////////////////////////////////////////////////////////////
