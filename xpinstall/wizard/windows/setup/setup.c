/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code,
 * released March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Sean Su <ssu@netscape.com>
 */

#include "setup.h"
#include "extra.h"
#include "dialogs.h"
#include "ifuncns.h"

/* global variables */
HINSTANCE       hInst;
HINSTANCE       hSetupRscInst;
HINSTANCE       hSDInst;
HINSTANCE       hXPIStubInst;

HBITMAP         hbmpBoxChecked;
HBITMAP         hbmpBoxCheckedDisabled;
HBITMAP         hbmpBoxUnChecked;

HANDLE          hAccelTable;

HWND            hDlgCurrent;
HWND            hDlgMessage;
HWND            hWndMain;

LPSTR           szEGlobalAlloc;
LPSTR           szEStringLoad;
LPSTR           szEDllLoad;
LPSTR           szEStringNull;
LPSTR           szTempSetupPath;
LPSTR           szEOutOfMemory;

LPSTR           szSetupDir;
LPSTR           szTempDir;
LPSTR           szOSTempDir;
LPSTR           szFileIniConfig;
LPSTR           szFileIniInstall;

LPSTR           szSiteSelectorDescription;

DWORD           dwWizardState;
DWORD           dwSetupType;

DWORD           dwTempSetupType;
DWORD           gdwUpgradeValue;
DWORD           gdwSiteSelectorStatus;

BOOL            bSDUserCanceled;
BOOL            bIdiArchivesExists;
BOOL            bCreateDestinationDir;
BOOL            bReboot;
BOOL            gbILUseTemp;
BOOL            gbPreviousUnfinishedDownload;
BOOL            gbPreviousUnfinishedInstallXpi;
BOOL            gbIgnoreRunAppX;
BOOL            gbIgnoreProgramFolderX;
BOOL            gbRestrictedAccess;
BOOL            gbDownloadTriggered;
BOOL            gbAllowMultipleInstalls = FALSE;
BOOL            gbForceInstall = FALSE;
/* XXX For now, we want to always force the installation of GRE until
 * GRE is appropriately versioned.  See bug 180383
 */
BOOL            gbForceInstallGre = TRUE;
BOOL            gShowBannerImage = TRUE;

setupGen        sgProduct;
diS             diSetup;
diW             diWelcome;
diL             diLicense;
diQL            diQuickLaunch;
diST            diSetupType;
diSC            diSelectComponents;
diSC            diSelectAdditionalComponents;
diWI            diWindowsIntegration;
diPF            diProgramFolder;
diDO            diAdditionalOptions;
diAS            diAdvancedSettings;
diSI            diStartInstall;
diD             diDownload;
diR             diReboot;
siSD            siSDObject;
siCF            siCFXpcomFile;
siC             *siComponents;
ssi             *ssiSiteSelector;
installGui      sgInstallGui;
sems            gErrorMessageStream;
sysinfo         gSystemInfo;
dsN             *gdsnComponentDSRequirement = NULL;


/* do not add setup.exe to the list because we figure out the filename
 * by calling GetModuleFileName() */
char *SetupFileList[] = {"setuprsc.dll",
                         "config.ini",
                         "setup.ini",
                         "install.ini",
                         "license.txt",
                         ""};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
  /***********************************************************************/
  /* HANDLE hInstance;       handle for this instance                    */
  /* HANDLE hPrevInstance;   handle for possible previous instances      */
  /* LPSTR  lpszCmdLine;     long pointer to exec command line           */
  /* int    nCmdShow;        Show code for main window display           */
  /***********************************************************************/

  MSG   msg;
  char  szBuf[MAX_BUF];
  int   iRv = WIZ_OK;
  HWND  hwndFW;

  if(!hPrevInstance)
  {
    if(InitSetupGeneral())
      PostQuitMessage(1);
    else if(ParseForStartupOptions(lpszCmdLine))
      PostQuitMessage(1);
    else if((hwndFW = FindWindow(CLASS_NAME_SETUP_DLG, NULL)) != NULL && !gbAllowMultipleInstalls)
    {
    /* Allow only one instance of setup to run.
     * Detect a previous instance of setup, bring it to the 
     * foreground, and quit current instance */

      ShowWindow(hwndFW, SW_RESTORE);
      SetForegroundWindow(hwndFW);
      iRv = WIZ_SETUP_ALREADY_RUNNING;
      PostQuitMessage(1);
    }
    else if(Initialize(hInstance))
      PostQuitMessage(1);
    else if(!InitApplication(hInstance, hSetupRscInst))
    {
      char szEFailed[MAX_BUF];

      if(GetPrivateProfileString("Messages", "ERROR_FAILED", "", szEFailed, sizeof(szEFailed), szFileIniInstall))
      {
        wsprintf(szBuf, szEFailed, "InitApplication().");
        PrintError(szBuf, ERROR_CODE_SHOW);
      }
      PostQuitMessage(1);
    }
    else if(!InitInstance(hInstance, nCmdShow))
    {
      char szEFailed[MAX_BUF];

      if(GetPrivateProfileString("Messages", "ERROR_FAILED", "", szEFailed, sizeof(szEFailed), szFileIniInstall))
      {
        wsprintf(szBuf, szEFailed, "InitInstance().");
        PrintError(szBuf, ERROR_CODE_SHOW);
      }
      PostQuitMessage(1);
    }
    else if(GetInstallIni())
    {
      PostQuitMessage(1);
    }
    else if(ParseInstallIni())
    {
      PostQuitMessage(1);
    }
    else if(GetConfigIni())
    {
      PostQuitMessage(1);
    }
    else if(ParseConfigIni(lpszCmdLine))
    {
      PostQuitMessage(1);
    }
    else
    {
      DlgSequence(NEXT_DLG);
    }
  }

  while(GetMessage(&msg, NULL, 0, 0))
  {
    if((!IsDialogMessage(hDlgCurrent, &msg)) && (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  if(iRv != WIZ_SETUP_ALREADY_RUNNING)
    /* Do clean up before exiting from the application */
    DeInitialize();

  /* garbage collection */
  DeInitSetupGeneral();

  return(msg.wParam);
} /*  End of WinMain */

