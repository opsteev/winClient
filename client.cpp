#define _WIN32_DCOM
#include <windows.h>
#include <conio.h>
#include <objbase.h>
#include <rpcsal.h>
#include <objbase.h>
#include <msxml6.h>
#include <atlbase.h>
#include <iostream>
#include <iomanip>
// headers needed to use WLAN APIs 
#include <wlanapi.h>
#include <sstream>
#pragma comment(lib,"wlanapi.lib")
#pragma comment(lib,"Rpcrt4.lib")
#pragma comment(lib,"msxml6.lib")
using namespace std;

// get win32 error from HRESULT
#define WIN32_FROM_HRESULT(hr) (SUCCEEDED(hr) ? ERROR_SUCCESS :(HRESULT_FACILITY(hr) == FACILITY_WIN32 ? HRESULT_CODE(hr) : (hr)))


//
// Utility functions
//

LPWSTR GetInterfaceStateString(__in WLAN_INTERFACE_STATE wlanInterfaceState)
{
    LPWSTR strRetCode;

    switch(wlanInterfaceState)
    {
        case wlan_interface_state_not_ready:
            strRetCode = L"\"not ready\"";
            break;
        case wlan_interface_state_connected:
            strRetCode = L"\"connected\"";
            break;
        case wlan_interface_state_ad_hoc_network_formed:
            strRetCode = L"\"ad hoc network formed\"";
            break;
        case wlan_interface_state_disconnecting:
            strRetCode = L"\"disconnecting\"";
            break;
        case wlan_interface_state_disconnected:
            strRetCode = L"\"disconnected\"";
            break;
        case wlan_interface_state_associating:
            strRetCode = L"\"associating\"";
            break;
        case wlan_interface_state_discovering:
            strRetCode = L"\"discovering\"";
            break;
        case wlan_interface_state_authenticating:
            strRetCode = L"\"authenticating\"";
            break;
        default:
            strRetCode = L"\"invalid interface state\"";
    }

    return strRetCode;
}




// get SSID from the WCHAR string
DWORD StringWToSsid(__in LPCWSTR strSsid,__out PDOT11_SSID pSsid)
{
    DWORD dwRetCode = ERROR_SUCCESS;
    BYTE pbSsid[DOT11_SSID_MAX_LENGTH + 1] = {0};

    if (strSsid == NULL || pSsid == NULL)
    {
        dwRetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
        pSsid->uSSIDLength = WideCharToMultiByte (CP_ACP,
                                                   0,
                                                   strSsid,
                                                   -1,
                                                   (LPSTR)pbSsid,
                                                   sizeof(pbSsid),
                                                   NULL,
                                                   NULL);

        pSsid->uSSIDLength--;
        memcpy(&pSsid->ucSSID, pbSsid, pSsid->uSSIDLength);
    }

    return dwRetCode;
}


// copy SSID to a null-terminated WCHAR string
// count is the number of WCHAR in the buffer.
LPWSTR SsidToStringW(__out_ecount(count) LPWSTR   buf,__in ULONG   count,__in PDOT11_SSID pSsid)
{
    ULONG   bytes, i;

    bytes = min( count-1, pSsid->uSSIDLength);
    for( i=0; i<bytes; i++)
        mbtowc( &buf[i], (const char *)&pSsid->ucSSID[i], 1);
    buf[bytes] = '\0';

    return buf;
}




// the max lenght of the reason string in characters
#define WLSAMPLE_REASON_STRING_LEN 256

// print the reason string
VOID PrintReason(__in WLAN_REASON_CODE reason)
{
    WCHAR strReason[WLSAMPLE_REASON_STRING_LEN];
    if (WlanReasonCodeToString(
            reason,
            WLSAMPLE_REASON_STRING_LEN,
            strReason,
            NULL            // reserved
            ) == ERROR_SUCCESS)
    {
        wcout << L" The reason is \"" << strReason << L"\"." << endl;
    }
    else
    {
        wcout << L" The reason code is " << reason << L"." << endl;
    }
}

// print the error message
VOID PrintErrorMsg(__in LPWSTR strCommand,__in DWORD dwError)
{
    if (strCommand != NULL)
    {
        if (dwError == ERROR_SUCCESS)
        {
            wcout << L"Command \"" << strCommand << L"\" completed successfully." << endl;
        }
        else if (dwError == ERROR_INVALID_PARAMETER)
        {
            wcout << L"The parameter for \"" << strCommand << L"\" is not correct. ";
            wcout << L"Please use \"help " << strCommand << L"\" to check the usage of the command." << endl;
        }
        else if (dwError == ERROR_BAD_PROFILE)
        {
            wcout << L"The given profile is not valid." << endl;
        }
        else if (dwError == ERROR_NOT_SUPPORTED)
        {
            wcout << L"Command \"" << strCommand << L"\" is not supported." << endl;
        }
        else
        {
            wcout << L"Got error " << dwError << L" for command \"" << strCommand << L"\"" << endl;
        }
    }
}

// open a WLAN client handle and check version
DWORD OpenHandleAndCheckVersion(PHANDLE phClient)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwServiceVersion;
    HANDLE hClient = NULL;

    __try
    {
        *phClient = NULL;

        // open a handle to the service
        if ((dwError = WlanOpenHandle(
                            WLAN_API_VERSION,
                            NULL,               // reserved
                            &dwServiceVersion,
                            &hClient
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }

        // check service version
        if (WLAN_API_VERSION_MAJOR(dwServiceVersion) < WLAN_API_VERSION_MAJOR(WLAN_API_VERSION_2_0))
        {
            // No-op, because the version check is for demonstration purpose only.
            // You can add your own logic here.
        }

        *phClient = hClient;

        // set hClient to NULL so it will not be closed
        hClient = NULL;
    }
    __finally
    {
        if (hClient != NULL)
        {
            // clean up
            WlanCloseHandle(
                hClient,
                NULL            // reserved
                );
        }
    }

    return dwError;
}

//
// Functions that demonstrate how to use WLAN APIs
//
VOID GetInterfaceState(__in int argc,__in_ecount(argc) LPWSTR argv[])
{
	DWORD dwError = ERROR_SUCCESS;
    HANDLE hClient = NULL;
	PWLAN_INTERFACE_INFO_LIST pIntfList = NULL;
	__try
    {
		
        if (argc != 1)
        {
            dwError = ERROR_INVALID_PARAMETER;
            __leave;
        }
		
		// open handle
        if ((dwError = OpenHandleAndCheckVersion(
                            &hClient
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		
		// enumerate wireless interfaces
        if ((dwError = WlanEnumInterfaces(
                            hClient,
                            NULL,               // reserved
                            &pIntfList
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		
		if (pIntfList->dwNumberOfItems !=1) {
			wcout << L"There are " << pIntfList->dwNumberOfItems << L" interfaces in the system." << endl;
			wcout << L"This program does not support..."<<endl;
			__leave;
		}
		
		
		wcout<<GetInterfaceStateString(pIntfList->InterfaceInfo[0].isState)<<endl;
		if (pIntfList != NULL)
        {
            WlanFreeMemory(pIntfList);
        }
    }
	
    __finally
    {
        // clean up
        if (hClient != NULL)
        {
            WlanCloseHandle(hClient,NULL);
        }
    }


    PrintErrorMsg(argv[0], dwError);
}


// set profile
DWORD SetProfile(__in HANDLE hClient,__in GUID guidIntf,__in LPWSTR pProfilePath)
{
    DWORD dwError;
    HRESULT hr;
    CComPtr<IXMLDOMDocument2> pXmlDoc;
    CComBSTR bstrXml;
    VARIANT_BOOL vbSuccess;
    DWORD dwReason;

	// __try and __leave cannot be used here because of COM object
    do
    {
        // create a COM object to read the XML file
        hr = CoCreateInstance(
                CLSID_DOMDocument60,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IXMLDOMDocument2,
                (void**)&pXmlDoc
                );
        if (hr != S_OK)
        {
            dwError = WIN32_FROM_HRESULT(hr);
            break;
        }

		// load the file into the COM object
		hr = pXmlDoc->load((CComVariant)pProfilePath, &vbSuccess);
        if (hr != S_OK || vbSuccess != VARIANT_TRUE)
        {
			wcout<<L"test1";
            dwError = ERROR_BAD_PROFILE;
            break;
        }

        // get XML string out from the file
        hr = pXmlDoc->get_xml(&bstrXml);
        if (hr != S_OK)
        {
			wcout<<L"test2";
            dwError = ERROR_BAD_PROFILE;
            break;
        }
        // set profile
        dwError = WlanSetProfile(hClient,&guidIntf,0,bstrXml,NULL,TRUE,NULL,&dwReason);
        if (dwError != ERROR_SUCCESS)
        {
            wcout << L"The profile is bad.";
            PrintReason(dwReason);
        }
    } while (FALSE);
	return dwError;
}

DWORD SetProfileUserData(__in HANDLE hClient,__in GUID guidIntf,__in LPWSTR Profilename,__in LPWSTR EapUserProfile)
{
    DWORD dwError;
    HRESULT hr;
    CComPtr<IXMLDOMDocument2> pXmlDoc;
    CComBSTR bstrXml;
    VARIANT_BOOL vbSuccess;

	// __try and __leave cannot be used here because of COM object
    do
    {
		wcout << L"Enter SetProfileUserData"<<endl;
        // create a COM object to read the XML file
        hr = CoCreateInstance(
                CLSID_DOMDocument60,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IXMLDOMDocument2,
                (void**)&pXmlDoc
                );
		wcout << L"Created a COM object"<<endl;
        if (hr != S_OK)
        {
            dwError = WIN32_FROM_HRESULT(hr);
            break;
        }

		// load the file into the COM object
		hr = pXmlDoc->load((CComVariant)EapUserProfile, &vbSuccess);
		wcout << L"loaded the file"<<endl;
        if (hr != S_OK || vbSuccess != VARIANT_TRUE)
        {
            dwError = ERROR_BAD_PROFILE;
            break;
        }

        // get XML string out from the file
        hr = pXmlDoc->get_xml(&bstrXml);
        if (hr != S_OK)
        {
            dwError = ERROR_BAD_PROFILE;
            break;
        }
		wcout << L"Got the xml string"<<endl;
        // set profile
		wcout << L"The profilename is: "<<Profilename<<endl;
        dwError = WlanSetProfileEapXmlUserData(hClient,&guidIntf,Profilename,0,bstrXml,NULL);
        if (dwError != ERROR_SUCCESS)
        {
            break;
        }
		wcout << L"Setted profile"<<endl;
    } while (FALSE);
	
	return dwError;
}



// delete profile
VOID DeleteProfile(__in int argc,__in_ecount(argc) LPWSTR argv[])
{
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hClient = NULL;
    GUID guidIntf;
	PWLAN_INTERFACE_INFO_LIST pIntfList = NULL;
	LPWSTR Profilename;

    __try
    {
        if (argc != 2)
        {
            dwError = ERROR_INVALID_PARAMETER;
            __leave;
        }
		Profilename = argv[1];
        // open handle
        if ((dwError = OpenHandleAndCheckVersion(
                            &hClient
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		
		// enumerate wireless interfaces
        if ((dwError = WlanEnumInterfaces(
                            hClient,
                            NULL,               // reserved
                            &pIntfList
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		
		if (pIntfList->dwNumberOfItems !=1) {
			wcout << L"There are " << pIntfList->dwNumberOfItems << L" interfaces in the system." << endl;
			wcout << L"This program does not support..."<<endl;
			__leave;
		}
		
		guidIntf = pIntfList->InterfaceInfo[0].InterfaceGuid;
		if (pIntfList != NULL)
        {
            WlanFreeMemory(pIntfList);
        }
        // delete profile
        dwError = WlanDeleteProfile(
                        hClient,
                        &guidIntf,
                        Profilename,        // profile name
                        NULL            // reserved
                        );

    }
    __finally
    {
        // clean up
        if (hClient != NULL)
        {
            WlanCloseHandle(
                hClient,
                NULL            // reserved
                );
        }
    }

    PrintErrorMsg(argv[0], dwError);
}

VOID
PrintBssInfo(
    __in PWLAN_BSS_ENTRY pBss
)
{
    WCHAR strSsid[DOT11_SSID_MAX_LENGTH+1];
    UINT i;
    PBYTE pIe = NULL;

    if (pBss != NULL)
    {
        // MAC address
        wcout << L"MAC address: ";
        for (i = 0; i < 6; i++)
        {
            wcout << setw(2) << setfill(L'0') << hex << (UINT)pBss->dot11Bssid[i] <<L" ";
        }
        wcout << endl;

        // SSID
        wcout << L"SSID: " << SsidToStringW(strSsid, sizeof(strSsid)/sizeof(WCHAR), &pBss->dot11Ssid) << endl;

        // Beacon period
        //wcout << L"\tBeacon period: " << dec << pBss->usBeaconPeriod << L" TU" << endl;

        // IE
        /*wcout << L"\tIE";
        i = 0;
        pIe = (PBYTE)(pBss) + pBss->ulIeOffset;

        // print 8 byte per line
        while (i < pBss->ulIeSize)
        {
            if (i % 8 == 0)
            {
                wcout << endl << L"\t\t";
            }
            wcout << setw(2) << setfill(L'0') << hex << (UINT)pIe[i] << L" ";
            i++;
        }

        wcout << endl;*/
    }

}

// connect to a network using a saved profile
UCHAR CharToHex(UCHAR tchar)
{
	UCHAR result; 
	if(tchar>='0'&&tchar<='9')
	result=tchar-'0';
	else if (tchar>='a'&&tchar<='f')
	result=tchar-'a'+10;
	else if (tchar>='A'&&tchar<='F')
	result=tchar-'A'+10;
	else return -1;
	return result;
}
VOID StringToMAC(UCHAR* strmac,DOT11_MAC_ADDRESS bssid)
{
	for(int i=0;i!=6;i++)
	{
		bssid[i]=(CharToHex(strmac[i*2])<<4)+CharToHex(strmac[i*2+1]);
	}
}



VOID Connect(__in int argc,__in_ecount(argc) LPWSTR argv[])
{
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hClient = NULL;
	PWLAN_INTERFACE_INFO_LIST pIntfList = NULL;
    GUID guidIntf;
    DOT11_SSID dot11Ssid = {0};
    WLAN_CONNECTION_PARAMETERS wlanConnPara;
    LPWSTR SSID;
	LPWSTR Profilename;
	LPWSTR pProfilePath;
	LPWSTR EapUserProfile = L"";
	DOT11_MAC_ADDRESS bssid ;
	UCHAR tbssid[13];
	const UCHAR dummyBssid[13]="000000000000";
	bool setBssid = false;

    __try
    {
		
        if (argc != 5 && argc !=6)
        {
            dwError = ERROR_INVALID_PARAMETER;
            __leave;
        }
		
		SSID = argv[1];
		Profilename = argv[2];
		pProfilePath = argv[3];
		WideCharToMultiByte(CP_ACP,0,argv[4],-1,(LPSTR)tbssid,sizeof(tbssid),NULL,NULL);
		StringToMAC(tbssid,bssid);
		if (argc == 6)
		{
			EapUserProfile = argv[5];
		}
		
		// open handle
        if ((dwError = OpenHandleAndCheckVersion(
                            &hClient
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		
		// enumerate wireless interfaces
        if ((dwError = WlanEnumInterfaces(
                            hClient,
                            NULL,               // reserved
                            &pIntfList
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		
		if (pIntfList->dwNumberOfItems !=1) {
			wcout << L"There are " << pIntfList->dwNumberOfItems << L" interfaces in the system." << endl;
			wcout << L"This program does not support..."<<endl;
			__leave;
		}
		
		guidIntf = pIntfList->InterfaceInfo[0].InterfaceGuid;
		if (pIntfList != NULL)
        {
            WlanFreeMemory(pIntfList);
        }
		
        // get SSID
        if ((dwError = StringWToSsid(SSID, &dot11Ssid)) != ERROR_SUCCESS)
        {
            __leave;
        }

		PWLAN_BSS_LIST pWlanBssList = NULL;
		DOT11_BSS_TYPE dot11BssType = dot11_BSS_type_infrastructure;
		if ((dwError = WlanGetNetworkBssList(
                            hClient,
                            &guidIntf,
                            &dot11Ssid,
                            dot11BssType,
                            FALSE,
                            NULL,                       // reserved
                            &pWlanBssList
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		for (UINT i = 0; i < pWlanBssList->dwNumberOfItems; i++)
        {
            PrintBssInfo(&pWlanBssList->wlanBssEntries[i]);
        }
		WlanFreeMemory(pWlanBssList);


		//set profile
		dwError = SetProfile(hClient,guidIntf,pProfilePath);
		if (dwError != ERROR_SUCCESS)
		{
			__leave;
		}

		if (_wcsicmp(EapUserProfile,L"") != 0)
		{
			dwError = SetProfileUserData(hClient,guidIntf,Profilename,EapUserProfile);
			if (dwError != ERROR_SUCCESS)
			{
				__leave;
			}
		}

        // set the connection mode (connecting using a profile)
        wlanConnPara.wlanConnectionMode = wlan_connection_mode_profile;
        // set the profile name
        wlanConnPara.strProfile = Profilename;
        // set the SSID
        wlanConnPara.pDot11Ssid = &dot11Ssid;

        // set BSS type
        wlanConnPara.dot11BssType = dot11_BSS_type_infrastructure;
        // the desired BSSID list is empty
        //wlanConnPara.pDesiredBssidList = NULL;
		for(int t=0; t < 12; t++) {
			if(tbssid[t]!=dummyBssid[t]) {
				setBssid=true;
				break;
			}
		}
		if(!setBssid) {
			cout<<"Desiredbssid is NULL"<<endl;
			wlanConnPara.pDesiredBssidList = NULL;
		} else {
			cout<<"Desiredbssid is "<<bssid<<endl;
			wlanConnPara.pDesiredBssidList=new DOT11_BSSID_LIST;
			wlanConnPara.pDesiredBssidList->Header.Type=NDIS_OBJECT_TYPE_DEFAULT;
			wlanConnPara.pDesiredBssidList->Header.Revision=DOT11_BSSID_LIST_REVISION_1;
			wlanConnPara.pDesiredBssidList->Header.Size=sizeof(DOT11_BSSID_LIST);
			wlanConnPara.pDesiredBssidList->uNumOfEntries=1;
			wlanConnPara.pDesiredBssidList->uTotalNumOfEntries=1;
			wlanConnPara.pDesiredBssidList->BSSIDs[0][0]=bssid[0];
			wlanConnPara.pDesiredBssidList->BSSIDs[0][1]=bssid[1];
			wlanConnPara.pDesiredBssidList->BSSIDs[0][2]=bssid[2];
			wlanConnPara.pDesiredBssidList->BSSIDs[0][3]=bssid[3];
			wlanConnPara.pDesiredBssidList->BSSIDs[0][4]=bssid[4];
			wlanConnPara.pDesiredBssidList->BSSIDs[0][5]=bssid[5];
		}
        // no connection flags
        wlanConnPara.dwFlags = 0;

        
		
		
        
        if ((dwError = WlanConnect(hClient,&guidIntf,&wlanConnPara,NULL)) != ERROR_SUCCESS)
		{
			__leave;
		}
		delete []wlanConnPara.pDesiredBssidList;
    }
	
    __finally
    {
		//
        // clean up
        if (hClient != NULL)
        {
            WlanCloseHandle(hClient,NULL);
        }
    }
    PrintErrorMsg(argv[0], dwError);
}

// disconnect from the current network
VOID Disconnect(__in int argc,__in_ecount(argc) LPWSTR argv[])
{
    DWORD dwError = ERROR_SUCCESS;
    HANDLE hClient = NULL;
    GUID guidIntf;
	PWLAN_INTERFACE_INFO_LIST pIntfList = NULL;

    __try
    {
        if (argc != 1)
        {
            dwError = ERROR_INVALID_PARAMETER;
            __leave;
        }

        // open handle
        if ((dwError = OpenHandleAndCheckVersion(
                            &hClient
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		// enumerate wireless interfaces
        if ((dwError = WlanEnumInterfaces(
                            hClient,
                            NULL,               // reserved
                            &pIntfList
                            )) != ERROR_SUCCESS)
        {
            __leave;
        }
		
		if (pIntfList->dwNumberOfItems !=1) {
			wcout << L"There are " << pIntfList->dwNumberOfItems << L" interfaces in the system." << endl;
			wcout << L"This program does not support..."<<endl;
			__leave;
		}
		
		guidIntf = pIntfList->InterfaceInfo[0].InterfaceGuid;
		if (pIntfList != NULL)
        {
            WlanFreeMemory(pIntfList);
        }

        dwError = WlanDisconnect(
                        hClient,
                        &guidIntf,
                        NULL            // reserved
                        );

    }
    __finally
    {
        // clean up
        if (hClient != NULL)
        {
            WlanCloseHandle(
                hClient,
                NULL            // reserved
                );
        }
    }

    PrintErrorMsg(argv[0], dwError);
}

// show help messages
VOID Help(__in int argc,__in_ecount(argc) LPWSTR argv[]);

typedef VOID (*WLSAMPLE_FUNCTION) (int argc, LPWSTR argv[]);

typedef struct _WLSAMPLE_COMMAND {
    LPWSTR strCommandName;           // command name
    LPWSTR strShortHand;             // a shorthand for the command
    WLSAMPLE_FUNCTION Func;         // pointer to the function
    LPWSTR strHelpMessage;          // help message
    LPWSTR strParameters;           // parameters for the command
    BOOL bRemarks;                  // whether have remarks for the command
    LPWSTR strRemarks;              // remarks
} WLSAMPLE_COMMAND, *PWLSAMPLE_COMMAND;

WLSAMPLE_COMMAND g_Commands[] = {
    // profile releated commands
    {
        L"DeleteProfile",
        L"dp",
        DeleteProfile,
        L"Delete a saved profile.",
        L"<profile name>",
        FALSE,
        L""
    },
    // connection related commands
    {
        L"Connect",
        L"conn",
        Connect,
        L"Connect to a wireless network using a saved profile.",
        L"<SSID> <Profile name> <Profile path> <bssid> <EapUserProfile path(optional)>",
        FALSE,
        L""
    },
    {
        L"Disconnect",
        L"dc",
        Disconnect,
        L"Disconnect from the current network.",
        L"",
        FALSE,
        L""
    },
	{
		L"GetInterfaceState",
		L"gs",
		GetInterfaceState,
		L"Get the interface state",
		L"",
		FALSE,
		L""
	},
    // other commands
    /*{
        L"RegisterNotif",
        L"r",
        RegisterNotification,
        L"Register ACM and MSM notifications.",
        L"",
        FALSE,
        L""
    },*/
    {
        L"help",
        L"?",
        Help,
        L"Print this help message.",
        L"[<command>]",
        FALSE,
        L""
    }
};

// show help messages
VOID Help(__in int argc,__in_ecount(argc) LPWSTR argv[])
{
    UINT i;

    if (argc == 1)
    {
        // show all commands
        wcout << L"Use \"help xyz\" to show the description of command xyz." << endl;
        for (i=0; i < sizeof(g_Commands)/sizeof(WLSAMPLE_COMMAND); i++)
        {
                wcout << L"\t"<< g_Commands[i].strCommandName;
                wcout << L"(" << g_Commands[i].strShortHand << L")" << endl;
        }
    }
    else if (argc == 2)
    {
        // show the description of a command
        for (i=0; i < sizeof(g_Commands)/sizeof(WLSAMPLE_COMMAND); i++)
        {
            if (_wcsicmp(argv[1], g_Commands[i].strCommandName) == 0 ||
                    _wcsicmp(argv[1], g_Commands[i].strShortHand) == 0)
            {
                wcout << L"Command: " << g_Commands[i].strCommandName;
                wcout << L"(" << g_Commands[i].strShortHand << L")" << endl;
                wcout << L"Description: " << g_Commands[i].strHelpMessage << endl;
                wcout << L"Usage: " << g_Commands[i].strCommandName;
                wcout << L"(" << g_Commands[i].strShortHand << L") ";
                wcout << g_Commands[i].strParameters << endl;
                if (g_Commands[i].bRemarks)
                {
                    wcout << L"Remarks: " << g_Commands[i].strRemarks << endl;
                }
                break;
            }
        }
    }
    else
    {
        PrintErrorMsg(argv[0], ERROR_INVALID_PARAMETER);
    }
}

// command is stored in the global variable
void ExecuteCommand(__in int argc,__in_ecount(argc) LPWSTR argv[])
{
    UINT i = 0;

    for (i=0; i < sizeof(g_Commands)/sizeof(WLSAMPLE_COMMAND); i++)
    {
        // find the command and call the function
        if (_wcsicmp(argv[0], g_Commands[i].strCommandName) == 0 ||
            _wcsicmp(argv[0], g_Commands[i].strShortHand) == 0)
        {
            g_Commands[i].Func(argc, argv);
            break;
        }
    }

    if (i == sizeof(g_Commands)/sizeof(WLSAMPLE_COMMAND))
    {
        wcerr << L"Invalid command " << argv[0] << L"!" << endl;
    }
}

// the main program
int _cdecl wmain(__in int argc,__in_ecount(argc) LPWSTR argv[])
{
    DWORD dwRetCode = ERROR_SUCCESS;

    if (argc <= 1)
    {
        wcout << L"Please type \"" << argv[0] << L" ?\" for help." << endl;
        dwRetCode = ERROR_INVALID_PARAMETER;
    }
    else
    {
		CoInitialize(NULL);
        // don't pass in the first parameter
        ExecuteCommand(argc-1, argv+1);
		CoUninitialize();
    }

    return dwRetCode;
}