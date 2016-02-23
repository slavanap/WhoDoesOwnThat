//Copyright © 2006 Shaun Harrington, All Rights Reserved.
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is
// not sold for profit without the authors written consent, and
// providing that this notice and the authors name is included. If
// the source code in this file is used in any commercial application
// then a simple email would be nice.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to your
// computer or anything else for that matter.
//

#include <windows.h>	// For the 'Win32' API.
#include <shlwapi.h>	// For the 'Path' API.
#include <winnetwk.h>	// For the 'WNet' API.

// Function name   : GetOwner
// Description     : Determines the 'Owner' of a given file or folder.
// Return type     : UINT is S_OK if successful; A Win32 'ERROR_' value otherwise.
// Argument        : LPCWSTR szFileOrFolderPathName is the fully qualified path of the file or folder to examine.
// Argument        : LPWSTR pUserNameBuffer is a pointer to a buffer used to contain the resulting 'Owner' string.
// Argument        : int nSizeInBytes is the size of the buffer.
UINT GetOwner(LPCWSTR szFileOrFolderPathName, LPWSTR pUserNameBuffer, int nSizeInBytes) {
	// 1) Validate the path:
	// 1.1) Length should not be 0.
	// 1.2) Path must point to an existing file or folder.
	if(!lstrlen(szFileOrFolderPathName) || !PathFileExists(szFileOrFolderPathName))
		return ERROR_INVALID_PARAMETER;

	// 2) Validate the buffer:
	// 2.1) Size must not be 0.
	// 2.2) Pointer must not be NULL.
	if(nSizeInBytes<=0 || pUserNameBuffer==NULL)
		return ERROR_INVALID_PARAMETER;

	// 3) Convert the path to UNC if it is not already UNC so that we can extract a machine name from it:
	// 3.1) Use a big buffer... some OS's can have a path that is 32768 chars in length.
	WCHAR szUNCPathName[32767] = {0};
	// 3.2) If path is not UNC...
	if(!PathIsUNC(szFileOrFolderPathName)) {
		// 3.3) Mask the big buffer into a UNIVERSAL_NAME_INFO.
		DWORD dwUniSize = 32767*sizeof(WCHAR);
		UNIVERSAL_NAME_INFO* pUNI = (UNIVERSAL_NAME_INFO*)szUNCPathName;
		// 3.4) Attempt to get the UNC version of the path into the big buffer.
		if(!WNetGetUniversalName(szFileOrFolderPathName, UNIVERSAL_NAME_INFO_LEVEL, pUNI, &dwUniSize)) {
			// 3.5) If successful, copy the UNC version into the buffer.
			lstrcpy(szUNCPathName,pUNI->lpUniversalName); // You must extract from this offset as the buffer has UNI overhead.
		} else {
			// 3.6) If not successful, copy the original path into the buffer.
			lstrcpy(szUNCPathName,szFileOrFolderPathName);
		}
	}else {
		// 3.7) Path is already UNC, copy the original path into the buffer.
		lstrcpy(szUNCPathName,szFileOrFolderPathName);
	}

	// 4) If path is UNC (will not be the case for local physical drive paths) we want to extract the machine name:
	// 4.1) Use a buffer bug enough to hold a machine name per Win32.
	WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH+1] = {0};
		// 4.2) If path is UNC...
	if(PathIsUNC(szUNCPathName)) {
		// 4.3) Use PathFindNextComponent() to skip past the double backslashes.
		LPWSTR lpMachineName = PathFindNextComponent(szUNCPathName);
		// 4.4) Walk the the rest of the path to find the end of the machine name.
		int nPos = 0;
		LPWSTR lpNextSlash = lpMachineName;
		while((lpNextSlash[0] != L'\\') && (lpNextSlash[0] != L'\0')) {
			nPos++;
			lpNextSlash++;
		}
		// 4.5) Copyt the machine name into the buffer.
		lstrcpyn(szMachineName, lpMachineName, nPos+1);
	}

    // 5) Derive the 'Owner' by getting the owner's Security ID from a Security Descriptor associated with the file or folder indicated in the path.
	// 5.1) Get a security descriptor for the file or folder that contains the Owner Security Information.
	// 5.1.1) Use GetFileSecurity() with some null params to get the required buffer size.
	// 5.1.2) We don't really care about the return value.
	// 5.1.3) The error code must be ERROR_INSUFFICIENT_BUFFER for use to continue.
    unsigned long   uSizeNeeded = 0;
    GetFileSecurity(szUNCPathName, OWNER_SECURITY_INFORMATION, 0, 0, &uSizeNeeded);
	UINT uRet = GetLastError();
	if(uRet==ERROR_INSUFFICIENT_BUFFER && uSizeNeeded) {
		uRet = S_OK; // Clear the ERROR_INSUFFICIENT_BUFFER

		// 5.2) Allocate the buffer for the Security Descriptor, check for out of memory
        LPBYTE lpSecurityBuffer = (LPBYTE) malloc(uSizeNeeded * sizeof(BYTE));
        if(!lpSecurityBuffer) {
	        return ERROR_NOT_ENOUGH_MEMORY;
        }

		// 5.2) Get the Security Descriptor that contains the Owner Security Information into the buffer, check for errors
        if(!GetFileSecurity(szUNCPathName, OWNER_SECURITY_INFORMATION, lpSecurityBuffer, uSizeNeeded, &uSizeNeeded)) {
	        free(lpSecurityBuffer);
            return GetLastError();
        }

		// 5.3) Get the the owner's Security ID (SID) from the Security Descriptor, check for errors
	    PSID pSID = NULL;
	    BOOL            bOwnerDefaulted = FALSE;
        if(!GetSecurityDescriptorOwner(lpSecurityBuffer, &pSID, &bOwnerDefaulted)) {
	        free(lpSecurityBuffer);
            return GetLastError();
        }
		
		// 5.4) Get the size of the buffers needed for the owner information (domain and name)
		// 5.4.1) Use LookupAccountSid() with buffer sizes set to zero to get the required buffer sizes.
		// 5.4.2) We don't really care about the return value.
		// 5.4.3) The error code must be ERROR_INSUFFICIENT_BUFFER for use to continue.
		LPWSTR			pName = NULL;
		LPWSTR			pDomain = NULL;
		unsigned long   uNameLen = 0;
		unsigned long   uDomainLen = 0;
		SID_NAME_USE    sidNameUse = SidTypeUser;
	    LookupAccountSid(szMachineName, pSID, pName, &uNameLen, pDomain, &uDomainLen, &sidNameUse);
		uRet = GetLastError();
	    if((uRet==ERROR_INSUFFICIENT_BUFFER) && uNameLen && uDomainLen) {
			uRet = S_OK; // Clear the ERROR_INSUFFICIENT_BUFFER

			// 5.5) Allocate the required buffers, check for out of memory
			pName = (LPWSTR) malloc(uNameLen * sizeof(WCHAR));
	        pDomain = (LPWSTR) malloc(uDomainLen * sizeof(WCHAR));
	        if(!pName || !pDomain) {
		        free(lpSecurityBuffer);
		        return ERROR_NOT_ENOUGH_MEMORY;
			}

			// 5.6) Get domain and username
			if(!LookupAccountSid(szMachineName, pSID, pName, &uNameLen, pDomain, &uDomainLen, &sidNameUse)) {
			    free(pName);
				free(pDomain);
				free(lpSecurityBuffer);
				return GetLastError();
			}

			// 5.7) Build the owner string from the domain and username if there is enough room in the buffer.
			if(nSizeInBytes > ((uNameLen + uDomainLen + 2)*sizeof(WCHAR))) {
				lstrcpy(pUserNameBuffer, pDomain);
				lstrcat(pUserNameBuffer, L"\\");
				lstrcat(pUserNameBuffer, pName);
			} else {
				uRet = ERROR_INSUFFICIENT_BUFFER;
			}

			// 5.8) Release memory
			free(pName);
			free(pDomain);
		}
		// 5.9) Release memory
        free(lpSecurityBuffer);
    }
    return uRet;
}
