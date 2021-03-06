 /* 
 * Copyright (C) 2002 Markus Eriksson, marre@renameit.hypermart.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

#ifndef _SECURE_ATL
# define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
# define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif


// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.

# ifndef WINVER					// Specifies that the minimum required platform is Windows XP.
#  define WINVER 0x0501			// Change this to the appropriate value to target other versions of Windows.
# endif

# ifndef _WIN32_WINNT			// Specifies that the minimum required platform is Windows XP.
#  define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
# endif

# ifndef _WIN32_WINDOWS			// Specifies that the minimum required platform is Windows 98.
#  define _WIN32_WINDOWS 0x0410	// Change this to the appropriate value to target Windows Me or later.
# endif

# ifndef _WIN32_IE				// Specifies that the minimum required platform is Internet Explorer 6.0.
#  define _WIN32_IE 0x0600		// Change this to the appropriate value to target other versions of IE.
# endif


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes

#ifndef _AFX_NO_OLE_SUPPORT
# include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
# include <afxcmn.h>		// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcoll.h>
#include <afxdlgs.h>
#include <afxtempl.h>

#if defined _M_IX86
# pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
# pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
# pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
# pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// General stuff
#include <math.h>
#include <Winerror.h>

#ifndef ERROR_RM_NOT_ACTIVE
// MessageText: Transaction support within the specified file system resource manager is not started or was shutdown due to an error.
# define ERROR_RM_NOT_ACTIVE       6801L
#endif

// STL
#include <vector>
#include <list>
#include <set>
#include <map>
#include <stdexcept>
#include <fstream>

// Boost
#ifdef _DEBUG
#  define BOOST_LIB_DIAGNOSTIC	// Optional: when set the header will print out the name
								// of the library selected (useful for debugging).
#endif
//#include <boost/bind.hpp>	// Now part of TR1 too
#include <boost/cast.hpp>
#include <boost/exception/info.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
//#include <boost/shared_ptr.hpp>	// Now part of TR1 too
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/signal.hpp>
#include <boost/static_assert.hpp>
#include <boost/throw_exception.hpp>

using namespace std;
using namespace std::placeholders;
//using namespace boost;

inline long int lrintf(float x) {
	return (long int)(x + 0.5f);
}
