/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/ 
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License 
 * for the specific language governing rights and limitations under the 
 * License. 
 * 
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998. 
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are Copyright (C) 1999
 * Netscape Communications Corporation. All Rights Reserved.  
 * 
 * Contributors:
 *     Samir Gehani <sgehani@netscape.com>
 */

#include "MacInstallWizard.h"


/*-----------------------------------------------------------*
 *   Install Action
 *-----------------------------------------------------------*/

pascal void* Install(void* unused)
{	
	short			vRefNum;
	long			dirID;
	OSErr 			err;
	FSSpec			idiSpec, coreFileSpec, tmpSpec;
	SDISTRUCT		sdistruct;
	Str255			pIDIfname;
	StringPtr		coreFile;
	THz				ourHZ;
	Boolean 		isDir = false;
	GrafPtr			oldPort;
	
#if CORRECT_DL_LOCATION == 1
	/* get "Temporary Items" folder path */
	ERR_CHECK_RET(FindFolder(kOnSystemDisk, kTemporaryFolderType, kCreateFolder, &vRefNum, &dirID), (void*)0);
#else

	vRefNum = gControls->opt->vRefNum;
	err = FSMakeFSSpec( vRefNum, 0, TEMP_DIR, &tmpSpec );
	if (err != noErr)
	{
		err = FSpDirCreate(&tmpSpec, smSystemScript, &dirID);
		if (err != noErr)
		{
			ErrorHandler();
			return (void*)0;
		}
	}
	else
	{
		err = FSpGetDirectoryID( &tmpSpec, &dirID, &isDir );
		if (!isDir || err!=noErr)
		{
			ErrorHandler();
			return (void*)0;
		}
	}
		
#endif /* CORRECT_DL_LOCATION == 1 */
	
	GetIndString(pIDIfname, rStringList, sTempIDIName);
	
	/* generate idi */
	if (!GenerateIDIFromOpt(pIDIfname, dirID, vRefNum, &idiSpec))
	{
		ErrorHandler();
		return (void*) nil;
	}		
	
	/* populate SDI struct */
	sdistruct.dwStructSize 	= sizeof(SDISTRUCT);
	sdistruct.fsIDIFile 	= idiSpec;
	sdistruct.dlDirVRefNum 	= vRefNum;
	sdistruct.dlDirID 		= dirID;
	sdistruct.hwndOwner    	= NULL;
	
	/* call SDI_NetInstall */
	gSDDlg = true;
	ourHZ = GetZone();
	GetPort(&oldPort);
#if SDINST_IS_DLL == 1
	gInstFunc(&sdistruct);
#else
	SDI_NetInstall(&sdistruct);
#endif
	SetPort(oldPort);
	if (gWPtr)
	{
		BeginUpdate(gWPtr);
		DrawControls(gWPtr);
		ShowLogo(true);
		UpdateTerminalWin();
		EndUpdate(gWPtr);
	}
	SetZone(ourHZ);
	gSDDlg = false;
	
	FSpDelete(&idiSpec);
	
	/* check if coreFile was downloaded */
	HLock(gControls->cfg->coreFile);
	if (*gControls->cfg->coreFile != NULL)
	{
		coreFile = CToPascal(*gControls->cfg->coreFile);
		if (!coreFile)
			return (void*) memFullErr;
	}
	HUnlock(gControls->cfg->coreFile);
		
	if (coreFile != NULL && *coreFile > 0) /* core file was specified */
	{
		err = FSMakeFSSpec(vRefNum, dirID, coreFile, &coreFileSpec);
		if (err==noErr) /* core file was downloaded */
		{
			InitProgressBar();
			
			/* extract contents of downloaded core file */
			err = ExtractCoreFile(vRefNum, dirID);
			if (err!=noErr) 
			{
				ErrorHandler();
				return (void*) nil;
			}
				
			/* run all .xpi's through XPInstall */
			err = RunAllXPIs(vRefNum, dirID);
			if (err!=noErr)
				ErrorHandler();

			CleanupExtractedFiles(vRefNum, dirID);
		}
	
		if (coreFile)
			DisposePtr((Ptr)coreFile);
	}
	 
	/* wind down app */
	gDone = true;
	
	return (void*) 0;
}

Boolean 	
GenerateIDIFromOpt(Str255 idiName, long dirID, short vRefNum, FSSpec *idiSpec)
{
	Boolean bSuccess = true;
	OSErr 	err;
	short	refNum, instChoice;
	long 	count, compsDone, i, j;
	char 	*buf, *fnum, *keybuf, ch;
	Str255	pfnum, pkeybuf;

	err = FSMakeFSSpec(vRefNum, dirID, idiName, idiSpec);
	if ((err != noErr) && (err != fnfErr))
	{
		ErrorHandler();
		return false;
	}
	err = FSpCreate(idiSpec, 'NSCP', 'TEXT', smSystemScript);
	if ( (err != noErr) && (err != dupFNErr))
	{
		ErrorHandler();
		return false;
	}
	ERR_CHECK_RET(FSpOpenDF(idiSpec, fsRdWrPerm, &refNum), false);
	
	// setup buffer to at least 8K
	buf = NULL;
	buf = NewPtrClear(kGenIDIFileSize);
	if (!buf)
	{
		ErrorHandler();
		return false;
	}
	compsDone = 0;
	instChoice = gControls->opt->instChoice-1;
	
	// loop through 0 to kMaxComponents
	for(i=0; i<kMaxComponents; i++)
	{
		// general test: if component in setup type
		if ( (gControls->cfg->st[instChoice].comp[i] == kInSetupType) &&
			 (compsDone < gControls->cfg->st[instChoice].numComps) )
		{ 
			// if custom and selected, or not custom setup type
			// add file to buffer
			if ( ((instChoice == gControls->cfg->numSetupTypes-1) && 
				  (gControls->opt->compSelected[i] == kSelected)) ||
				 (instChoice < gControls->cfg->numSetupTypes-1) )
			{
				// get file number from STR# resource
				GetIndString(pfnum, rIndices, compsDone+1);
				fnum = PascalToC(pfnum);
				
				// construct through concatenation [File<num>]\r
				GetIndString(pkeybuf, rIDIKeys, sFile);
				keybuf = PascalToC(pkeybuf);
				ch = '[';
				strncat(buf, &ch, 1);
				strncat(buf, keybuf, strlen(keybuf));
				strncat(buf, fnum, strlen(fnum));
				DisposePtr(keybuf);
				
				keybuf = NewPtrClear(2);
				keybuf = "]\r";
				strncat(buf, keybuf, strlen(keybuf));
				if (keybuf)
					DisposePtr(keybuf);

				// write out \tdesc=
				GetIndString(pkeybuf, rIDIKeys, sDesc);
				keybuf = PascalToC(pkeybuf);
				ch = '\t';								
				strncat(buf, &ch, 1);					// \t
				strncat(buf, keybuf, strlen(keybuf));	// \tdesc
				ch = '=';								// \tdesc=
				strncat(buf, &ch, 1);
				if (keybuf)
					DisposePtr(keybuf);
				
				// write out gControls->cfg->comp[i].shortDesc\r
				HLock(gControls->cfg->comp[i].shortDesc);
				strncat(buf, *gControls->cfg->comp[i].shortDesc, strlen(*gControls->cfg->comp[i].shortDesc));				
				HUnlock(gControls->cfg->comp[i].shortDesc);
				ch = '\r';
				strncat(buf, &ch, 1);
			
				// iterate over gControls->cfg->comp[i].numURLs
				for (j=0; j<gControls->cfg->comp[i].numURLs; j++)
				{
					// write out \tnumURLs+1= from STR# resource
					GetIndString(pkeybuf, rIndices, j+2); // j+2 since 1-based idx, not 0-based
					keybuf = PascalToC(pkeybuf);
					ch = '\t';
					strncat(buf, &ch, 1);					// \t
					strncat(buf, keybuf, strlen(keybuf));	// \t<n>
					ch = '=';
					strncat(buf, &ch, 1);					// \t<n>=
					if (keybuf)
						DisposePtr(keybuf);
					
					// write out gControls->cfg->comp[i].url[j]+archive\r
					HLock(gControls->cfg->comp[i].url[j]);					
					strncat(buf, *gControls->cfg->comp[i].url[j], strlen(*gControls->cfg->comp[i].url[j]));
					HUnlock(gControls->cfg->comp[i].url[j]);
					HLock(gControls->cfg->comp[i].archive);
					strncat(buf, *gControls->cfg->comp[i].archive, strlen(*gControls->cfg->comp[i].archive));
					HUnlock(gControls->cfg->comp[i].archive);
					ch = '\r';
					strncat(buf, &ch, 1);
				}
				if (fnum)
					DisposePtr(fnum);
				compsDone++;
			}
		}
		else if (compsDone == gControls->cfg->st[instChoice].numComps)
			break;  
	}			
	
	// terminate by entering Netscape Install section
	GetIndString(pkeybuf, rIDIKeys, sNSInstall);
	keybuf = PascalToC(pkeybuf);
	ch = '[';
	strncat(buf, &ch, 1);					// [
	strncat(buf, keybuf, strlen(keybuf));	// [Netscape Install
	if (keybuf)
		DisposePtr(keybuf);
	
	keybuf = NewPtrClear(2);
	keybuf = "]\r";				
	strncat(buf, keybuf, strlen(keybuf));	// [Netscape Install]\r
	if (keybuf)
		DisposePtr(keybuf);
	
	// write out \tcore_file=<filename>\r
	AddKeyToIDI( sCoreFile, gControls->cfg->coreFile, buf );
	
	// write out \tcore_dir=<dirname>\r
	AddKeyToIDI( sCoreDir, gControls->cfg->coreDir, buf );
	
	// write out \tno_ads=<boolean>\r
	AddKeyToIDI( sNoAds, gControls->cfg->noAds, buf );
	
	// write out \tsilent=<boolean>\r
	AddKeyToIDI( sSilent, gControls->cfg->silent, buf );
	
	// write out \texecution=<boolean>\r
	AddKeyToIDI( sExecution, gControls->cfg->execution, buf );
	
	// write out \tconfirm_install=<boolean>
	AddKeyToIDI( sConfirmInstall, gControls->cfg->confirmInstall, buf );
	
	// write buf to disk
	count = strlen(buf);
	ERR_CHECK_RET(FSWrite(refNum, &count, buf), false);
	
	// close file
	ERR_CHECK_RET(FSClose(refNum), false)
	
	if (buf)
		DisposePtr(buf);
	return bSuccess;
}

void
AddKeyToIDI(short key, Handle val, char *ostream)
{
	Str255	pkeybuf;
	char 	*keybuf, *cval, ch;
	
	HLock(val);
	cval = *val;
	
	GetIndString(pkeybuf, rIDIKeys, key);
	keybuf = PascalToC(pkeybuf);
	ch = '\t';
	strncat(ostream, &ch, 1);					// \t
	strncat(ostream, keybuf, strlen(keybuf));	// \t<key>
	ch = '=';
	strncat(ostream, &ch, 1);					// \t<key>=
	strncat(ostream, cval, strlen(cval));		// \t<key>=<val>
	ch = '\r';
	strncat(ostream, &ch, 1);					// \t<key>=<val>\r
	
	HUnlock(val);
	
	if (keybuf)
		DisposePtr(keybuf);
}

void
InitProgressBar(void)
{
	Boolean	indeterminateFlag = true;
	Rect	r;
	Str255	extractingStr;
	GrafPtr	oldPort;
	GetPort(&oldPort);
	
	if (gWPtr)
	{
		SetPort(gWPtr);
		
		gControls->tw->progressBar = NULL;
		gControls->tw->progressBar = GetNewControl(rInstProgBar, gWPtr);
		if (gControls->tw->progressBar)
		{
			SetControlData(gControls->tw->progressBar, kControlNoPart, kControlProgressBarIndeterminateTag,
							sizeof(indeterminateFlag), (Ptr) &indeterminateFlag);
			Draw1Control(gControls->tw->progressBar);
			
			gControls->tw->progressMsg = NULL;
			HLock((Handle)gControls->tw->progressBar);
			SetRect(&r, (*gControls->tw->progressBar)->contrlRect.left,
						(*gControls->tw->progressBar)->contrlRect.top - 42,
						(*gControls->tw->progressBar)->contrlRect.right,
						(*gControls->tw->progressBar)->contrlRect.top - 26 );
			HUnlock((Handle)gControls->tw->progressBar);
			gControls->tw->progressMsg = TENew(&r, &r);
			if (gControls->tw->progressMsg)
			{
				GetIndString(extractingStr, rStringList, sExtracting);
				TEInsert(&extractingStr[1], extractingStr[0], gControls->tw->progressMsg);	
			}
			
			TextFace(normal);
			TextSize(9);
			TextFont(applFont);	
			
			gControls->tw->xpiProgressMsg = NULL;	/* used by XPInstall progress callback */
			HLock((Handle)gControls->tw->progressBar);
			SetRect(&r, (*gControls->tw->progressBar)->contrlRect.left,
						(*gControls->tw->progressBar)->contrlRect.top - 21,
						(*gControls->tw->progressBar)->contrlRect.right,
						(*gControls->tw->progressBar)->contrlRect.top - 5 );
			HUnlock((Handle)gControls->tw->progressBar);
			gControls->tw->xpiProgressMsg = TENew(&r, &r);
			
			TextFont(systemFont);	/* restore systemFont */
			TextSize(12);
		}
	}
	
	SetPort(oldPort);
}

Boolean
InitSDLib(void)
{
	Str255			libName;
	FSSpec			libSpec;
	short			vRefNum;
	long			dirID;
	OSErr 			err;
	
	ERR_CHECK_RET(GetCWD(&dirID, &vRefNum), false);

	/* initialize SDI lib and struct */
	GetIndString(libName, rStringList, sSDLib);
	ERR_CHECK_RET(FSMakeFSSpec(vRefNum, dirID, libName, &libSpec), false);
	if (!LoadSDLib(libSpec, &gInstFunc, &gSDIEvtHandler, &gConnID))
	{
		ErrorHandler();
		return false;
	}

	return true;
}

Boolean		
LoadSDLib(FSSpec libSpec, SDI_NETINSTALL *outSDNFunc, EventProc *outEvtProc, CFragConnectionID *outConnID)
{
	OSErr		err;
	Str255		errName;
	Ptr			mainAddr;
	Ptr			symAddr;
	CFragSymbolClass	symClass;
	
	ERR_CHECK_RET(GetDiskFragment(&libSpec, 0, kCFragGoesToEOF, nil, kReferenceCFrag, outConnID, &mainAddr, errName), false);
	
	if (*outConnID != NULL)
	{
		ERR_CHECK_RET(FindSymbol(*outConnID, "\pSDI_NetInstall", &symAddr, &symClass), false);
		*outSDNFunc = (SDI_NETINSTALL) symAddr;
		
		ERR_CHECK_RET(FindSymbol(*outConnID, "\pSDI_HandleEvent", &symAddr, &symClass), false);			
		*outEvtProc = (EventProc) symAddr;
	}
	
	return true;
}

Boolean		
UnloadSDLib(CFragConnectionID *connID)
{
	if (*connID != NULL)
	{
		CloseConnection(connID);
		*connID = NULL;
	}
	else
		return false;
	
	return true;
}