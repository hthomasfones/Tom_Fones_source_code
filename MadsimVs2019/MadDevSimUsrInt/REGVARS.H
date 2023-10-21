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
/*                 MadEnum.exe, MadMonitor.exe, MadWmi.exe                     */
/*                                                                             */
/*  Module  NAME : RegVars.h                                                   */
/*                                                                             */
/*  DESCRIPTION  : Function prototypes, structures, classes, etc.              */
/*                                                                             */
/*******************************************************************************/

class RegistryVar
	{
	public:
    	static void		setInt(UINT id, int value, HKEY root = HKEY_CURRENT_USER);
	    static int		getInt(UINT id, int def = 0, HKEY root = HKEY_CURRENT_USER);
	    static void		setString(UINT id, LPCTSTR s,
		HKEY root = HKEY_CURRENT_USER);
    	static CString	getString(UINT id, LPCTSTR def = NULL,
		HKEY root = HKEY_CURRENT_USER);

	protected:
	    UINT			id;
	    HKEY			root;
	};

class RegistryInt: public RegistryVar
	{
	public:
	    RegistryInt(UINT n, HKEY r = HKEY_CURRENT_USER)

	{
		id = n; root = r;
	}
	int				value; // the value
	virtual int		load(int def = 0);
	virtual void	store();
	};


class RegistryString: public RegistryVar
	{
	public:
	    RegistryString(UINT n, HKEY r = HKEY_CURRENT_USER)
		{
		id = n; root = r;
		}

		CString			value; // the value
	    virtual CString	load(LPCTSTR def = NULL);
	    virtual void	store();
	};



