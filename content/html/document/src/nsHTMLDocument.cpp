/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */ 
#define NS_IMPL_IDS
#include "nsICharsetAlias.h"
#undef NS_IMPL_IDS

#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsHTMLDocument.h"
#include "nsIParser.h"
#include "nsIParserFilter.h"
#include "nsIHTMLContentSink.h"
#include "nsHTMLParts.h"
#include "nsIHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"
#include "nsIStyleSet.h"
#include "nsHTMLAtoms.h"
#include "nsIPresShell.h" 
#include "nsIPresContext.h" 
#include "nsIHTMLContent.h"
#include "nsIDOMNode.h" // for Find
#include "nsIDOMNodeList.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIDOMComment.h" 
#include "nsIDOMWindow.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsNeckoUtil.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIWebShell.h"
#include "nsIWebShellServices.h"
#include "nsIDocumentLoader.h"
#include "CNavDTD.h"
#include "nsIScriptGlobalObject.h"
#include "nsContentList.h"
#include "nsDOMError.h"
#include "nsICodebasePrincipal.h"
#include "nsIScriptSecurityManager.h"

#include "nsIIOService.h"
#include "nsICookieService.h"

#include "nsIParserService.h"
#include "nsIServiceManager.h"
#include "nsIFormManager.h"
#include "nsIComponentManager.h"
#include "nsParserCIID.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsINameSpaceManager.h"
#include "nsGenericHTMLElement.h"
#include "nsGenericDOMNodeList.h"
#include "nsICSSLoader.h"
#include "nsIHTTPChannel.h"
#include "nsIFileChannel.h"
#include "nsIEventListenerManager.h"
#include "nsISelectElement.h"

#include "nsICharsetDetector.h"
#include "nsICharsetDetectionAdaptor.h"
#include "nsCharsetDetectionAdaptorCID.h"
#include "nsICharsetAlias.h"
#include "nsIPref.h"
#define DETECTOR_PROGID_MAX 127
static char g_detector_progid[DETECTOR_PROGID_MAX + 1];
static PRBool gInitDetector = PR_FALSE;
static PRBool gPlugDetector = PR_FALSE;


#ifdef PCB_USE_PROTOCOL_CONNECTION
// beard: how else would we get the referrer to a URL?
#include "nsIProtocolConnection.h"
#include "net.h"
#endif

#include "prmem.h"
#include "prtime.h"

// Find/Serach Includes
const PRInt32 kForward  = 0;
const PRInt32 kBackward = 1;


//#define rickgdebug 1
#ifdef rickgdebug
#include "nsHTMLContentSinkStream.h"
#endif

// XXX Used to control whether we implement document.layers
//#define NS_IMPLEMENT_DOCUMENT_LAYERS

static NS_DEFINE_IID(kIWebShellIID, NS_IWEB_SHELL_IID);
static NS_DEFINE_IID(kIDocumentIID, NS_IDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
static NS_DEFINE_IID(kIDOMNodeListIID, NS_IDOMNODELIST_IID);
static NS_DEFINE_IID(kIHTMLDocumentIID, NS_IHTMLDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMHTMLDocumentIID, NS_IDOMHTMLDOCUMENT_IID);
static NS_DEFINE_IID(kIDOMNSHTMLDocumentIID, NS_IDOMNSHTMLDOCUMENT_IID);

static NS_DEFINE_IID(kIIOServiceIID, NS_IIOSERVICE_IID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_IID(kCookieServiceCID, NS_COOKIESERVICE_CID);




static NS_DEFINE_IID(kIHTMLContentContainerIID, NS_IHTMLCONTENTCONTAINER_IID);
static NS_DEFINE_IID(kIDOMHTMLBodyElementIID, NS_IDOMHTMLBODYELEMENT_IID);

static NS_DEFINE_IID(kIParserFilterIID, NS_IPARSERFILTER_IID);
static int
MyPrefChangedCallback(const char*aPrefName, void* instance_data)
{
        nsresult rv;
        NS_WITH_SERVICE(nsIPref, prefs, "component://netscape/preferences", &rv);
        char* detector_name = nsnull;
        if(NS_SUCCEEDED(rv) && NS_SUCCEEDED(
             rv = prefs->CopyCharPref("intl.charset.detector",
                                     &detector_name)))
        {
			if(nsCRT::strlen(detector_name) > 0) {
				PL_strncpy(g_detector_progid, NS_CHARSET_DETECTOR_PROGID_BASE,DETECTOR_PROGID_MAX);
				PL_strncat(g_detector_progid, detector_name,DETECTOR_PROGID_MAX);
				gPlugDetector = PR_TRUE;
			} else {
				g_detector_progid[0]=0;
				gPlugDetector = PR_FALSE;
			}
            PR_FREEIF(detector_name);
        }
	return 0;
}
// ==================================================================
// =
// ==================================================================
NS_LAYOUT nsresult
NS_NewHTMLDocument(nsIDocument** aInstancePtrResult)
{
  nsHTMLDocument* doc = new nsHTMLDocument();
  if(doc)
    return doc->QueryInterface(kIDocumentIID, (void**) aInstancePtrResult);
  return NS_ERROR_OUT_OF_MEMORY;
}

nsHTMLDocument::nsHTMLDocument()
  : nsMarkupDocument(),
    mAttrStyleSheet(nsnull),
    mStyleAttrStyleSheet(nsnull),
    mBaseURL(nsnull),
    mBaseTarget(nsnull),
    mLastModified(nsnull),
    mReferrer(nsnull),
    mIsWriting(0)
{
  mImages = nsnull;
  mApplets = nsnull;
  mEmbeds = nsnull;
  mLinks = nsnull;
  mAnchors = nsnull;
  mLayers = nsnull;
  mNamedItems = nsnull;
  mParser = nsnull;
  mDTDMode = eDTDMode_Nav;
  mCSSLoader = nsnull;
  mDocTypeStr=nsnull;

  // Find/Search Init
  mSearchStr             = nsnull;
  mLastBlockSearchOffset = 0;
  mAdjustToEnd           = PR_FALSE;
  mShouldMatchCase       = PR_FALSE;
  mIsPreTag              = PR_FALSE;

  mHoldBlockContent      = nsnull;

  // These will be converted to a nsDeque
  mStackInx    = 0;
  mParentStack = (nsIDOMNode**) new PRUint32[32];
  mChildStack  = (nsIDOMNode**) new PRUint32[32];
  mBodyContent = nsnull;
  mForms = nsnull;
  mIsWriting = 0;
  mWriteLevel = 0;
}

nsHTMLDocument::~nsHTMLDocument()
{
  PRInt32 i;

  DeleteNamedItems();
  NS_IF_RELEASE(mImages);
  NS_IF_RELEASE(mApplets);
  NS_IF_RELEASE(mEmbeds);
  NS_IF_RELEASE(mLinks);
  NS_IF_RELEASE(mAnchors);
  NS_IF_RELEASE(mLayers);
  if (nsnull != mAttrStyleSheet) {
    mAttrStyleSheet->SetOwningDocument(nsnull);
    NS_RELEASE(mAttrStyleSheet);
  }
  if (nsnull != mStyleAttrStyleSheet) {
    mStyleAttrStyleSheet->SetOwningDocument(nsnull);
    NS_RELEASE(mStyleAttrStyleSheet);
  }
  NS_IF_RELEASE(mBaseURL);
  if (nsnull != mBaseTarget) {
    delete mBaseTarget;
    mBaseTarget = nsnull;
  }
  if (nsnull != mLastModified) {
    nsString::Recycle(mLastModified);
    mLastModified = nsnull;
  }
  if (nsnull != mReferrer) {
    nsString::Recycle(mReferrer);
    mReferrer = nsnull;
  }
  NS_IF_RELEASE(mParser);
  for (i = 0; i < mImageMaps.Count(); i++) {
    nsIDOMHTMLMapElement* map = (nsIDOMHTMLMapElement*)mImageMaps.ElementAt(i);
    NS_RELEASE(map);
  }
  NS_IF_RELEASE(mForms);
  if (mCSSLoader) {
    mCSSLoader->DropDocumentReference();  // release weak ref
    NS_RELEASE(mCSSLoader);
  }
  if (nsnull != mDocTypeStr) {
    delete mDocTypeStr;
    mDocTypeStr = nsnull;
  }

  // These will be converted to a nsDeque
  delete[] mParentStack;
  delete[] mChildStack;

  delete mSearchStr;
  NS_IF_RELEASE(mBodyContent);
}

NS_IMETHODIMP nsHTMLDocument::QueryInterface(REFNSIID aIID,
                                             void** aInstancePtr)
{
  NS_PRECONDITION(nsnull != aInstancePtr, "null ptr");
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  if (aIID.Equals(kIHTMLDocumentIID)) {
    NS_ADDREF_THIS();
    *aInstancePtr = (void**) (nsIHTMLDocument *)this;
    return NS_OK;
  }
  if (aIID.Equals(kIDOMHTMLDocumentIID)) {
    NS_ADDREF_THIS();
    *aInstancePtr = (void**) (nsIDOMHTMLDocument *)this;
    return NS_OK;
  }
  if (aIID.Equals(kIDOMNSHTMLDocumentIID)) {
    NS_ADDREF_THIS();
    *aInstancePtr = (void**) (nsIDOMNSHTMLDocument *)this;
    return NS_OK;
  }
  if (aIID.Equals(kIHTMLContentContainerIID)) {
    NS_ADDREF_THIS();
    *aInstancePtr = (void**) (nsIHTMLContentContainer *)this;
    return NS_OK;
  }
  return nsDocument::QueryInterface(aIID, aInstancePtr);
}

nsrefcnt nsHTMLDocument::AddRef()
{
  return nsDocument::AddRef();
}

nsrefcnt nsHTMLDocument::Release()
{
  return nsDocument::Release();
}

nsresult 
nsHTMLDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
  nsresult result = nsDocument::Reset(aChannel, aLoadGroup);
  nsCOMPtr<nsIURI> aURL;
  result = aChannel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(result)) return result;
  if (NS_FAILED(result)) {
    return result;
  }

  PRInt32 i;

  DeleteNamedItems();
  NS_IF_RELEASE(mImages);
  NS_IF_RELEASE(mApplets);
  NS_IF_RELEASE(mEmbeds);
  NS_IF_RELEASE(mLinks);
  NS_IF_RELEASE(mAnchors);
  NS_IF_RELEASE(mLayers);

  for (i = 0; i < mImageMaps.Count(); i++) {
    nsIDOMHTMLMapElement* map = (nsIDOMHTMLMapElement*)mImageMaps.ElementAt(i);
    NS_RELEASE(map);
  }
  NS_IF_RELEASE(mForms);

  if (nsnull == mAttrStyleSheet) {
    result = NS_NewHTMLStyleSheet(&mAttrStyleSheet, aURL, this);
  }
  else {
    result = mAttrStyleSheet->Reset(aURL);
  }
  if (NS_OK == result) {
    AddStyleSheet(mAttrStyleSheet); // tell the world about our new style sheet

    if (nsnull == mStyleAttrStyleSheet) {
      result = NS_NewHTMLCSSStyleSheet(&mStyleAttrStyleSheet, aURL, this);
    }
    else {
      result = mStyleAttrStyleSheet->Reset(aURL);
    }
    if (NS_OK == result) {
      AddStyleSheet(mStyleAttrStyleSheet); // tell the world about our new style sheet
    }
  }

  return result;
}

NS_IMETHODIMP 
nsHTMLDocument::GetContentType(nsString& aContentType) const
{
  aContentType.SetString("text/html");
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::CreateShell(nsIPresContext* aContext,
                            nsIViewManager* aViewManager,
                            nsIStyleSet* aStyleSet,
                            nsIPresShell** aInstancePtrResult)
{
  nsresult result = nsMarkupDocument::CreateShell(aContext,
                                                  aViewManager,
                                                  aStyleSet,
                                                  aInstancePtrResult);

  if (NS_SUCCEEDED(result)) {
    aContext->SetCompatibilityMode(((eDTDMode_NoQuirks == mDTDMode) ? 
                                    eCompatibility_Standard : 
                                    eCompatibility_NavQuirks));
  }
  return result;
}

NS_IMETHODIMP
nsHTMLDocument::StartDocumentLoad(const char* aCommand,
                                  nsIChannel* aChannel,
                                  nsILoadGroup* aLoadGroup,
                                  nsISupports* aContainer,
                                  nsIStreamListener **aDocListener)
{
  nsresult rv = nsDocument::StartDocumentLoad(aCommand,
                                              aChannel, aLoadGroup,
                                              aContainer, 
                                              aDocListener);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIWebShell> webShell;
  nsAutoString charset = "ISO-8859-1"; // fallback value in case webShell return error
  nsCharsetSource charsetSource = kCharsetFromWeakDocTypeDefault;

  nsCOMPtr<nsIURI> aURL;
  rv = aChannel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;
  
  nsAutoString lastModified;
  nsCOMPtr<nsIHTTPChannel> httpChannel = do_QueryInterface(aChannel);
  if (httpChannel) {
    nsXPIDLCString lastModHeader;
    nsIAtom* lastModKey = NS_NewAtom("last-modified");

    rv = httpChannel->GetResponseHeader(lastModKey, 
                                        getter_Copies(lastModHeader));

    NS_RELEASE(lastModKey);
    if (NS_SUCCEEDED(rv)) {
      lastModified = lastModHeader;
      SetLastModified(lastModified);
    }

    nsXPIDLCString referrerHeader;
    nsAutoString referrer;
    // The misspelled key 'referer' is as per the HTTP spec
    nsIAtom* referrerKey = NS_NewAtom("referer");
    
    rv = httpChannel->GetRequestHeader(referrerKey, 
                                        getter_Copies(referrerHeader));

    NS_RELEASE(referrerKey);
    if (NS_SUCCEEDED(rv)) {
      referrer = referrerHeader;
      SetReferrer(referrer);
    }

    if(kCharsetFromHTTPHeader > charsetSource)
    {
      nsIAtom* contentTypeKey = NS_NewAtom("content-type");
      nsXPIDLCString contenttypeheader;
      rv = httpChannel->GetResponseHeader(contentTypeKey, getter_Copies(contenttypeheader));
      NS_RELEASE(contentTypeKey);
      if (NS_SUCCEEDED(rv)) {
        nsAutoString contentType;
        contentType = contenttypeheader;
        PRInt32 start = contentType.RFind("charset=", PR_TRUE ) ;
        if(kNotFound != start)
        {
          start += 8; // 8 = "charset=".length
          PRInt32 end = contentType.FindCharInSet(";\n\r ", start  );
          if(kNotFound == end )
            end = contentType.Length();
          nsAutoString theCharset;
          contentType.Mid(theCharset, start, end - start);
          nsICharsetAlias* calias = nsnull;
          rv = nsServiceManager::GetService(
                                            kCharsetAliasCID,
                                            nsICharsetAlias::GetIID(),
                                            (nsISupports**) &calias);
          if(NS_SUCCEEDED(rv) && (nsnull != calias) )
          {
            nsAutoString preferred;
            rv = calias->GetPreferred(theCharset, preferred);
            if(NS_SUCCEEDED(rv))
            {
              charset = preferred;
              charsetSource = kCharsetFromHTTPHeader;
            }
            nsServiceManager::ReleaseService(kCharsetAliasCID, calias);
          }
        }
      }
    }

    // Don't propogate the result code beyond here, since it
    // could just be that the response header wasn't found.
    rv = NS_OK;
  }

  nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(aChannel);
  if (fileChannel) {
    PRTime modDate;
    
    rv = fileChannel->GetModDate(&modDate);
    if (NS_SUCCEEDED(rv)) {
      PRExplodedTime prtime;
      char buf[100];
      PRInt64 prusec, scale;

      LL_I2L(scale, 1000000L);
      LL_MUL(prusec, modDate, scale);
      PR_ExplodeTime(prusec, PR_LocalTimeParameters, &prtime);
      PR_FormatTime(buf, sizeof buf, "%#c", &prtime);
      lastModified.SetString(buf);
      SetLastModified(lastModified);
    }
  }

  static NS_DEFINE_IID(kCParserIID, NS_IPARSER_IID);
  static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);

  if (rv == NS_OK)
    rv = nsComponentManager::CreateInstance(kCParserCID, 
                                      nsnull, 
                                      kCParserIID, 
                                      (void **)&mParser);

  if (NS_OK == rv) { 
    nsIHTMLContentSink* sink;

    PRUnichar* requestCharset = nsnull;
    nsCharsetSource requestCharsetSource = kCharsetUninitialized;
    
    nsIParserFilter *cdetflt = nsnull;

#ifdef rickgdebug
    nsString outString;   // added out. Redirect to stdout if desired -- gpk 04/01/99
    rv = NS_New_HTML_ContentSinkStream(&sink,&outString,0);
#else
    NS_PRECONDITION(nsnull != aContainer, "No content viewer container");
    aContainer->QueryInterface(kIWebShellIID, getter_AddRefs(webShell));
    rv = NS_NewHTMLContentSink(&sink, this, aURL, webShell);

    if (NS_SUCCEEDED(rv)) 
    {
      nsCOMPtr<nsIMarkupDocumentViewer> muCV;
      nsCOMPtr<nsIContentViewer> cv;
      webShell->GetContentViewer(getter_AddRefs(cv));
      if (cv) {
         muCV = do_QueryInterface(cv);            
      }
		  if(kCharsetFromUserDefault > charsetSource) 
      {
		    PRUnichar* defaultCharsetFromWebShell = NULL;
        if (muCV) {
  		    rv = muCV->GetDefaultCharacterSet(&defaultCharsetFromWebShell);
        }
        if(NS_SUCCEEDED(rv)) {
          charset = defaultCharsetFromWebShell;
          Recycle(defaultCharsetFromWebShell);
          charsetSource = kCharsetFromUserDefault;
        }
        // for html, we need to find out the Meta tag from the hint.
        if (muCV) {
          rv = muCV->GetHintCharacterSet(&requestCharset);
          if(NS_SUCCEEDED(rv)) {
            rv = muCV->GetHintCharacterSetSource((PRInt32*)(&requestCharsetSource));
          }
        }
        if(NS_SUCCEEDED(rv)) 
        {
          if(requestCharsetSource > charsetSource) 
          {
            charsetSource = requestCharsetSource;
            charset = requestCharset;
            Recycle(requestCharset);
          }
        }
	      if(kCharsetFromPreviousLoading > charsetSource)
        {
          PRUnichar* forceCharsetFromWebShell = NULL;
          if (muCV) {
		        rv = muCV->GetForceCharacterSet(&forceCharsetFromWebShell);
          }
		      if(NS_SUCCEEDED(rv) && (nsnull != forceCharsetFromWebShell)) 
          {
				    charset = forceCharsetFromWebShell;
            Recycle(forceCharsetFromWebShell);
				    //TODO: we should define appropriate constant for force charset
				    charsetSource = kCharsetFromPreviousLoading;  
          }
        }
        nsresult rv_detect = NS_OK;
        if(! gInitDetector)
        {
          nsIPref* pref = nsnull;
          if(NS_SUCCEEDED(webShell->GetPrefs(pref)) && pref)
          {
            char* detector_name = nsnull;
            if(NS_SUCCEEDED(
                 rv_detect = pref->CopyCharPref("intl.charset.detector",
                                     &detector_name)))
            {
              PL_strncpy(g_detector_progid, NS_CHARSET_DETECTOR_PROGID_BASE,DETECTOR_PROGID_MAX);
              PL_strncat(g_detector_progid, detector_name,DETECTOR_PROGID_MAX);
              gPlugDetector = PR_TRUE;
              PR_FREEIF(detector_name);
            }
            pref->RegisterCallback("intl.charset.detector", MyPrefChangedCallback, nsnull);
          }
          NS_IF_RELEASE(pref);
          gInitDetector = PR_TRUE;
        }
   
        if((kCharsetFromAutoDetection > charsetSource )  && gPlugDetector)
        {
          // we could do charset detection
          nsICharsetDetector *cdet = nsnull;
          nsIWebShellServices *wss = nsnull;
          nsICharsetDetectionAdaptor *adp = nsnull;

          if(NS_SUCCEEDED( rv_detect = 
                nsComponentManager::CreateInstance(g_detector_progid, nsnull,
                               nsICharsetDetector::GetIID(), (void**)&cdet)))
          {
            if(NS_SUCCEEDED( rv_detect = 
                nsComponentManager::CreateInstance(
                               NS_CHARSET_DETECTION_ADAPTOR_PROGID, nsnull,
                               kIParserFilterIID, (void**)&cdetflt)))
            {
              if(cdetflt && 
                    NS_SUCCEEDED( rv_detect=
                         cdetflt->QueryInterface(
                            nsICharsetDetectionAdaptor::GetIID(),(void**) &adp)))
              {
                if( NS_SUCCEEDED( rv_detect=
                         webShell->QueryInterface(
                            nsIWebShellServices::GetIID(),(void**) &wss)))
                {
                  rv_detect = adp->Init(wss, cdet, aCommand);
                }
              }
            }
          }
          else 
          {
            // IF we cannot create the detector, don't bother to 
            // create one next time.
            gPlugDetector = PR_FALSE;
          }
          NS_IF_RELEASE(wss);
          NS_IF_RELEASE(cdet);
          NS_IF_RELEASE(adp);
          // NO NS_IF_RELEASE(cdetflt); here, do it after mParser->SetParserFilter
        }
      }
    }
#endif

    if (NS_SUCCEEDED(rv)) {
       rv = this->SetDocumentCharacterSet(charset);
    }

    if (NS_OK == rv) {
      // Set the parser as the stream listener for the document loader...
      static NS_DEFINE_IID(kIStreamListenerIID, NS_ISTREAMLISTENER_IID);
      rv = mParser->QueryInterface(kIStreamListenerIID, (void**)aDocListener);

      if (NS_OK == rv) {

        //The following lines were added by Rick.
        //These perform "dynamic" DTD registration, allowing
        //the caller total control over process, and decoupling
        //parser from any given grammar.

//        nsIDTD* theDTD=0;
//        NS_NewNavHTMLDTD(&theDTD);
//        mParser->RegisterDTD(theDTD);

        nsIParserFilter *oldFilter = nsnull;
        if(cdetflt)
           oldFilter = mParser->SetParserFilter(cdetflt);
        NS_IF_RELEASE(oldFilter);
        NS_IF_RELEASE(cdetflt);

        mParser->SetDocumentCharset( charset, charsetSource);
        mParser->SetCommand(aCommand);
        mParser->SetContentSink(sink); 
        mParser->Parse(aURL, nsnull, PR_FALSE, (void *)this);
      }
      NS_RELEASE(sink);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLDocument::EndLoad()
{
  NS_IF_RELEASE(mParser);
  return nsDocument::EndLoad();
}

NS_IMETHODIMP nsHTMLDocument::SetTitle(const nsString& aTitle)
{
  if (nsnull == mDocumentTitle) {
    mDocumentTitle = new nsString(aTitle);
  }
  else {
    *mDocumentTitle = aTitle;
  }

  // Pass on to any interested containers
  PRInt32 i, n = mPresShells.Count();
  for (i = 0; i < n; i++) {
    nsIPresShell* shell = (nsIPresShell*) mPresShells.ElementAt(i);
    nsCOMPtr<nsIPresContext> cx;
    shell->GetPresContext(getter_AddRefs(cx));
    nsISupports* container;
    if (NS_OK == cx->GetContainer(&container)) {
      if (nsnull != container) {
        nsCOMPtr<nsIWebShell> webShell = do_QueryInterface(container);
        if (webShell) {
          webShell->SetTitle(aTitle.GetUnicode());
        }
        NS_RELEASE(container);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::AddImageMap(nsIDOMHTMLMapElement* aMap)
{
  NS_PRECONDITION(nsnull != aMap, "null ptr");
  if (nsnull == aMap) {
    return NS_ERROR_NULL_POINTER;
  }
  if (mImageMaps.AppendElement(aMap)) {
    NS_ADDREF(aMap);
    return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP 
nsHTMLDocument::RemoveImageMap(nsIDOMHTMLMapElement* aMap)
{
  NS_PRECONDITION(nsnull != aMap, "null ptr");
  if (nsnull == aMap) {
    return NS_ERROR_NULL_POINTER;
  }
  if (mImageMaps.RemoveElement((void*)aMap)) {
    NS_RELEASE(aMap);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetImageMap(const nsString& aMapName,
                            nsIDOMHTMLMapElement** aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  nsAutoString name;
  PRInt32 i, n = mImageMaps.Count();
  for (i = 0; i < n; i++) {
    nsIDOMHTMLMapElement* map = (nsIDOMHTMLMapElement*)mImageMaps.ElementAt(i);
    if (NS_OK == map->GetName(name)) {
      if (name.EqualsIgnoreCase(aMapName)) {
        *aResult = map;
        NS_ADDREF(map);
        return NS_OK;
      }
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsHTMLDocument::GetAttributeStyleSheet(nsIHTMLStyleSheet** aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = mAttrStyleSheet;
  if (nsnull == mAttrStyleSheet) {
    return NS_ERROR_NOT_AVAILABLE;  // probably not the right error...
  }
  else {
    NS_ADDREF(mAttrStyleSheet);
  }
  return NS_OK;
}

NS_IMETHODIMP nsHTMLDocument::GetInlineStyleSheet(nsIHTMLCSSStyleSheet** aResult)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = mStyleAttrStyleSheet;
  if (nsnull == mStyleAttrStyleSheet) {
    return NS_ERROR_NOT_AVAILABLE;  // probably not the right error...
  }
  else {
    NS_ADDREF(mStyleAttrStyleSheet);
  }
  return NS_OK;
}

void nsHTMLDocument::InternalAddStyleSheet(nsIStyleSheet* aSheet)  // subclass hook for sheet ordering
{
  if (aSheet == mAttrStyleSheet) {  // always first
    mStyleSheets.InsertElementAt(aSheet, 0);
  }
  else if (aSheet == mStyleAttrStyleSheet) {  // always last
    mStyleSheets.AppendElement(aSheet);
  }
  else {
    if (mStyleAttrStyleSheet == mStyleSheets.ElementAt(mStyleSheets.Count() - 1)) {
      // keep attr sheet last
      mStyleSheets.InsertElementAt(aSheet, mStyleSheets.Count() - 1);
    }
    else {
      mStyleSheets.AppendElement(aSheet);
    }
  }
}

void
nsHTMLDocument::InternalInsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex)
{
  mStyleSheets.InsertElementAt(aSheet, aIndex + 1); // offset one for the attr style sheet
}


NS_IMETHODIMP
nsHTMLDocument::GetBaseURL(nsIURI*& aURL) const
{
  if (nsnull != mBaseURL) {
    NS_ADDREF(mBaseURL);
    aURL = mBaseURL;
  }
  else {
    aURL = GetDocumentURL();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument:: SetBaseURL(const nsString& aURLSpec)
{
  nsresult result = NS_OK;

  NS_IF_RELEASE(mBaseURL);
  if (0 < aURLSpec.Length()) {
    {
        result = NS_NewURI(&mBaseURL, aURLSpec, mDocumentURL);
    }
  }
  return result;
}

NS_IMETHODIMP
nsHTMLDocument:: GetBaseTarget(nsString& aTarget) const
{
  if (nsnull != mBaseTarget) {
    aTarget = *mBaseTarget;
  }
  else {
    aTarget.Truncate();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument:: SetBaseTarget(const nsString& aTarget)
{
  if (0 < aTarget.Length()) {
    if (nsnull != mBaseTarget) {
      *mBaseTarget = aTarget;
    }
    else {
      mBaseTarget = aTarget.ToNewString();
    }
  }
  else {
    if (nsnull != mBaseTarget) {
      delete mBaseTarget;
      mBaseTarget = nsnull;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLDocument::SetLastModified(const nsString& aLastModified)
{
  if (0 < aLastModified.Length()) {
    if (nsnull != mLastModified) {
      *mLastModified = aLastModified;
    }
    else {
      mLastModified = aLastModified.ToNewString();
    }
  }
  else if (nsnull != mLastModified) {
    nsString::Recycle(mLastModified);
    mLastModified = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLDocument::SetReferrer(const nsString& aReferrer)
{
  if (0 < aReferrer.Length()) {
    if (nsnull != mReferrer) {
      *mReferrer = aReferrer;
    }
    else {
      mReferrer = aReferrer.ToNewString();
    }
  }
  else if (nsnull != mReferrer) {
    nsString::Recycle(mReferrer);
    mReferrer = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetCSSLoader(nsICSSLoader*& aLoader)
{
  nsresult result = NS_OK;
  if (! mCSSLoader) {
    result = NS_NewCSSLoader(this, &mCSSLoader);
  }
  if (mCSSLoader) {
    mCSSLoader->SetCaseSensitive(PR_FALSE);
    mCSSLoader->SetQuirkMode(PRBool(eDTDMode_NoQuirks != mDTDMode));
  }
  aLoader = mCSSLoader;
  NS_IF_ADDREF(aLoader);
  return result;
}


NS_IMETHODIMP
nsHTMLDocument::GetDTDMode(nsDTDMode& aMode)
{
  aMode = mDTDMode;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetDTDMode(nsDTDMode aMode)
{
  mDTDMode = aMode;
  if (mCSSLoader) {
    mCSSLoader->SetQuirkMode(PRBool(eDTDMode_NoQuirks != mDTDMode));
  }

  nsIPresShell* shell = (nsIPresShell*) mPresShells.ElementAt(0);
  if (nsnull != shell) {
    nsCOMPtr<nsIPresContext> pc;
    shell->GetPresContext(getter_AddRefs(pc));
     if (pc) {
        pc->SetCompatibilityMode(((eDTDMode_NoQuirks == mDTDMode) ? 
                                    eCompatibility_Standard : 
                                    eCompatibility_NavQuirks));
     }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetDocTypeStr(nsString& aDocTypeString)
{
  if(mDocTypeStr) aDocTypeString = *mDocTypeStr;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::AddDocTypeDecl(const nsString& aDocTypeString, nsDTDMode aMode)
{
  nsresult result=NS_OK;
   
  result=SetDTDMode(aMode);
  if(result==NS_OK) {
    if (mDocTypeStr == nsnull) {
      if(aDocTypeString.Length()>0) {
        mDocTypeStr = new nsString(aDocTypeString);
        if (mDocTypeStr==nsnull) result=NS_ERROR_OUT_OF_MEMORY;
      }
    } 
    else {
      mDocTypeStr->SetLength(0);
      mDocTypeStr->Append(aDocTypeString);
    }
  }
  return result;
}

NS_IMETHODIMP
nsHTMLDocument::SetHeaderData(nsIAtom* aHeaderField, const nsString& aData)
{
  nsresult result = nsMarkupDocument::SetHeaderData(aHeaderField, aData);

  if (NS_SUCCEEDED(result)) {
    if (aHeaderField == nsHTMLAtoms::headerDefaultStyle) {
      // switch alternate style sheets based on default
      nsAutoString type;
      nsAutoString title;
      nsAutoString textHtml("text/html");
      PRInt32 index;
      PRInt32 count = mStyleSheets.Count();
      for (index = 0; index < count; index++) {
        nsIStyleSheet* sheet = (nsIStyleSheet*)mStyleSheets.ElementAt(index);
        sheet->GetType(type);
        if (PR_FALSE == type.Equals(textHtml)) {
          sheet->GetTitle(title);
          if (0 < title.Length()) {  // if sheet has title
            PRBool disabled = ((0 == aData.Length()) || 
                               (PR_FALSE == aData.EqualsIgnoreCase(title)));
            SetStyleSheetDisabledState(sheet, disabled);
          }
        }
      }
    }
  }
  return result;
}


NS_IMETHODIMP 
nsHTMLDocument::ContentAppended(nsIContent* aContainer,
                                PRInt32 aNewIndexInContainer)
{
  if (nsnull != mNamedItems) {
    nsIContent* child;
    nsIAtom *name;

    aContainer->GetTag(name);
    aContainer->ChildAt(aNewIndexInContainer, child);
    RegisterNamedItems(aContainer, name == nsHTMLAtoms::form);
    NS_IF_RELEASE(child);
    NS_IF_RELEASE(name);
  }

  return nsDocument::ContentAppended(aContainer, aNewIndexInContainer);
}

NS_IMETHODIMP 
nsHTMLDocument::ContentInserted(nsIContent* aContainer,
                                nsIContent* aChild,
                                PRInt32 aIndexInContainer)
{
  if (nsnull != mNamedItems) {
    nsIAtom *name;

    aContainer->GetTag(name);
    RegisterNamedItems(aChild, name == nsHTMLAtoms::form);
    NS_IF_RELEASE(name);
  }

  return nsDocument::ContentInserted(aContainer, aChild, aIndexInContainer);
}

NS_IMETHODIMP
nsHTMLDocument::ContentReplaced(nsIContent* aContainer,
                                nsIContent* aOldChild,
                                nsIContent* aNewChild,
                                PRInt32 aIndexInContainer)
{
  if (nsnull != mNamedItems) {
    nsIAtom *name;

    aContainer->GetTag(name);
    UnregisterNamedItems(aOldChild, name == nsHTMLAtoms::form);
    RegisterNamedItems(aNewChild, name == nsHTMLAtoms::form);

    NS_IF_RELEASE(name);
  }

  return nsDocument::ContentReplaced(aContainer, aOldChild, 
                                     aNewChild, aIndexInContainer);
}

NS_IMETHODIMP 
nsHTMLDocument::ContentRemoved(nsIContent* aContainer,
                               nsIContent* aChild,
                               PRInt32 aIndexInContainer)
{
  if ((nsnull != mNamedItems) && (nsnull != aContainer)) {
    nsIAtom *name;

    aContainer->GetTag(name);
    UnregisterNamedItems(aChild, name == nsHTMLAtoms::form);

    NS_IF_RELEASE(name);
  }

  return nsDocument::ContentRemoved(aContainer, aChild, aIndexInContainer);
}

//
// nsIDOMDocument interface implementation
//
NS_IMETHODIMP    
nsHTMLDocument::CreateElement(const nsString& aTagName, 
                              nsIDOMElement** aReturn)
{
  nsCOMPtr<nsIHTMLContent> content;
  nsresult rv = NS_CreateHTMLElement(getter_AddRefs(content), aTagName);
  if (NS_SUCCEEDED(rv)) {
    rv = content->QueryInterface(kIDOMElementIID, (void**)aReturn);
  }
  return rv;
}

NS_IMETHODIMP    
nsHTMLDocument::CreateProcessingInstruction(const nsString& aTarget, 
                                            const nsString& aData, 
                                            nsIDOMProcessingInstruction** aReturn)
{
  // There are no PIs for HTML
  *aReturn = nsnull;
  
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP    
nsHTMLDocument::CreateCDATASection(const nsString& aData, 
                                   nsIDOMCDATASection** aReturn)
{
  // There are no CDATASections in HTML
  *aReturn = nsnull;

  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}
 
NS_IMETHODIMP    
nsHTMLDocument::CreateEntityReference(const nsString& aName, 
                                      nsIDOMEntityReference** aReturn)
{
  // There are no EntityReferences in HTML
  *aReturn = nsnull;

  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}


NS_IMETHODIMP    
nsHTMLDocument::GetDoctype(nsIDOMDocumentType** aDocumentType)
{
  // There's no document type for a HTML document
  *aDocumentType = nsnull;
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetImplementation(nsIDOMDOMImplementation** aImplementation)
{ 
  return nsDocument::GetImplementation(aImplementation); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetDocumentElement(nsIDOMElement** aDocumentElement)
{ 
  return nsDocument::GetDocumentElement(aDocumentElement); 
}

NS_IMETHODIMP    
nsHTMLDocument::CreateDocumentFragment(nsIDOMDocumentFragment** aReturn)
{ 
  return nsDocument::CreateDocumentFragment(aReturn); 
}

NS_IMETHODIMP    
nsHTMLDocument::CreateComment(const nsString& aData, nsIDOMComment** aReturn)
{ 
  return nsDocument::CreateComment(aData, aReturn); 
}

NS_IMETHODIMP    
nsHTMLDocument::CreateAttribute(const nsString& aName, nsIDOMAttr** aReturn)
{ 
  return nsDocument::CreateAttribute(aName, aReturn); 
}

NS_IMETHODIMP    
nsHTMLDocument::CreateTextNode(const nsString& aData, nsIDOMText** aReturn)
{ 
  return nsDocument::CreateTextNode(aData, aReturn); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetElementsByTagName(const nsString& aTagname, nsIDOMNodeList** aReturn)
{ 
  return nsDocument::GetElementsByTagName(aTagname, aReturn); 
}

//
// nsIDOMNode interface implementation
//
NS_IMETHODIMP    
nsHTMLDocument::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  return nsDocument::GetChildNodes(aChildNodes);
}

NS_IMETHODIMP    
nsHTMLDocument::GetFirstChild(nsIDOMNode** aFirstChild)
{
  return nsDocument::GetFirstChild(aFirstChild);
}

NS_IMETHODIMP    
nsHTMLDocument::GetLastChild(nsIDOMNode** aLastChild)
{
  return nsDocument::GetLastChild(aLastChild);
}

NS_IMETHODIMP    
nsHTMLDocument::InsertBefore(nsIDOMNode* aNewChild, 
                             nsIDOMNode* aRefChild, 
                             nsIDOMNode** aReturn)
{
  return nsDocument::InsertBefore(aNewChild, aRefChild, aReturn);
}

NS_IMETHODIMP    
nsHTMLDocument::ReplaceChild(nsIDOMNode* aNewChild, 
                             nsIDOMNode* aOldChild, 
                             nsIDOMNode** aReturn)
{
  return nsDocument::ReplaceChild(aNewChild, aOldChild, aReturn);
}

NS_IMETHODIMP    
nsHTMLDocument::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  return nsDocument::RemoveChild(aOldChild, aReturn);
}

NS_IMETHODIMP    
nsHTMLDocument::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return nsDocument::AppendChild(aNewChild, aReturn);
}

NS_IMETHODIMP    
nsHTMLDocument::HasChildNodes(PRBool* aReturn)
{
  return nsDocument::HasChildNodes(aReturn);
}

NS_IMETHODIMP    
nsHTMLDocument::GetNodeName(nsString& aNodeName)
{ 
  return nsDocument::GetNodeName(aNodeName); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetNodeValue(nsString& aNodeValue)
{ 
  return nsDocument::GetNodeValue(aNodeValue); 
}

NS_IMETHODIMP    
nsHTMLDocument::SetNodeValue(const nsString& aNodeValue)
{ 
  return nsDocument::SetNodeValue(aNodeValue); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetNodeType(PRUint16* aNodeType)
{ 
  return nsDocument::GetNodeType(aNodeType); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetParentNode(nsIDOMNode** aParentNode)
{ 
  return nsDocument::GetParentNode(aParentNode); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{ 
  return nsDocument::GetPreviousSibling(aPreviousSibling); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetNextSibling(nsIDOMNode** aNextSibling)
{ 
  return nsDocument::GetNextSibling(aNextSibling); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{ 
  return nsDocument::GetAttributes(aAttributes); 
}

NS_IMETHODIMP    
nsHTMLDocument::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{ 
  return nsDocument::GetOwnerDocument(aOwnerDocument); 
}

NS_IMETHODIMP    
nsHTMLDocument::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{ 
  return nsDocument::CloneNode(aDeep, aReturn);
}


//
// nsIDOMHTMLDocument interface implementation
//
// see http://www.w3.org/TR/1998/REC-DOM-Level-1-19981001/level-one-html.html#ID-1006298752
// for full specification.
//
NS_IMETHODIMP
nsHTMLDocument::GetTitle(nsString& aTitle)
{
  if (nsnull != mDocumentTitle) {
    aTitle=*mDocumentTitle;
  }
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetReferrer(nsString& aReferrer)
{
  if (nsnull != mReferrer) {
    aReferrer = *mReferrer;
  }
  else {
    aReferrer.Truncate();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetDomainURI(nsIURI **uri) 
{
  nsCOMPtr<nsIPrincipal> principal = GetDocumentPrincipal();
  if (!principal)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsICodebasePrincipal> codebase = do_QueryInterface(principal);
  if (!codebase)
    return NS_ERROR_FAILURE;
  return codebase->GetURI(uri);
}


NS_IMETHODIMP    
nsHTMLDocument::GetDomain(nsString& aDomain)
{
  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(GetDomainURI(getter_AddRefs(uri))))
    return NS_ERROR_FAILURE;

  char *hostName;
  if (NS_FAILED(uri->GetHost(&hostName)))
    return NS_ERROR_FAILURE;
  aDomain.SetString(hostName);
  nsCRT::free(hostName);

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::SetDomain(const nsString& aDomain)
{
  // Check new domain
  nsAutoString current;
  if (NS_FAILED(GetDomain(current)))
    return NS_ERROR_FAILURE;
  PRBool ok = PR_FALSE;
  if (current.Equals(aDomain)) {
    ok = PR_TRUE;
  } else if (aDomain.Length() < current.Length()) {
    nsAutoString suffix;
    current.Right(suffix, aDomain.Length());
    PRUnichar c = current.CharAt(current.Length() - aDomain.Length() - 1);
    if (aDomain.EqualsIgnoreCase(suffix) && (c == '.' || c == '/'))
      ok = PR_TRUE;
  }
  if (!ok) {
    // Error: illegal domain
    return NS_ERROR_DOM_BAD_DOCUMENT_DOMAIN;
  }

  // Create new URI
  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(GetDomainURI(getter_AddRefs(uri))))
    return NS_ERROR_FAILURE;
  nsXPIDLCString scheme;
  if (NS_FAILED(uri->GetScheme(getter_Copies(scheme))))
    return NS_ERROR_FAILURE;
  nsXPIDLCString path;
  if (NS_FAILED(uri->GetPath(getter_Copies(path))))
    return NS_ERROR_FAILURE;
  nsAutoString newURIString = (const char *)scheme;
  newURIString += "://";
  newURIString += aDomain;
  newURIString += path;
  nsIURI *newURI;
  if (NS_FAILED(NS_NewURI(&newURI, newURIString)))
    return NS_ERROR_FAILURE;

  // Get codebase principal
  nsresult rv;
  NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager, 
                 NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
  if (NS_FAILED(rv)) 
    return NS_ERROR_FAILURE;
  return securityManager->GetCodebasePrincipal(newURI, &mPrincipal);
}

NS_IMETHODIMP    
nsHTMLDocument::GetURL(nsString& aURL)
{
  if (nsnull != mDocumentURL) {
    char* str;
    mDocumentURL->GetSpec(&str);
    aURL = str;
    nsCRT::free(str);
  }
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetBody(nsIDOMHTMLElement** aBody)
{
  if (mBodyContent == nsnull && PR_FALSE == GetBodyContent()) {
    return NS_ERROR_FAILURE;
  }
  return mBodyContent->QueryInterface(kIDOMHTMLElementIID, (void **)aBody);
}

NS_IMETHODIMP    
nsHTMLDocument::SetBody(nsIDOMHTMLElement* aBody)
{
  nsresult result = NS_OK;
  nsIDOMElement * root = nsnull;
  result = GetDocumentElement(&root);
  if (NS_OK != result) {  
    return result;
  }

  nsAutoString bodyStr("BODY");
  nsIDOMNode * child;
  root->GetFirstChild(&child);

  while (child != nsnull) {
    nsIDOMElement* domElement;
    result = child->QueryInterface(kIDOMElementIID,(void **)&domElement);
    if (NS_OK == result) {
      nsString tagName;
      domElement->GetTagName(tagName);
      if (bodyStr.EqualsIgnoreCase(tagName)) {
        nsIDOMNode* ret;
       
        result = root->ReplaceChild(aBody, child, &ret);
        NS_IF_RELEASE(ret);
        NS_IF_RELEASE(mBodyContent);

        NS_RELEASE(child);
        NS_RELEASE(root);
        NS_RELEASE(domElement);
        return result;
      }
      NS_RELEASE(domElement);
    }
    nsIDOMNode * node = child;
    NS_RELEASE(child);
    node->GetNextSibling(&child);
  }
  NS_RELEASE(root);
  return PR_FALSE;
}

NS_IMETHODIMP    
nsHTMLDocument::GetImages(nsIDOMHTMLCollection** aImages)
{
  if (nsnull == mImages) {
    mImages = new nsContentList(this, nsHTMLAtoms::img, kNameSpaceID_HTML);
    if (nsnull == mImages) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mImages);
  }

  *aImages = (nsIDOMHTMLCollection *)mImages;
  NS_ADDREF(mImages);

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetApplets(nsIDOMHTMLCollection** aApplets)
{
  if (nsnull == mApplets) {
    mApplets = new nsContentList(this, nsHTMLAtoms::applet, kNameSpaceID_HTML);
    if (nsnull == mApplets) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mApplets);
  }

  *aApplets = (nsIDOMHTMLCollection *)mApplets;
  NS_ADDREF(mApplets);

  return NS_OK;
}

PRBool
nsHTMLDocument::MatchLinks(nsIContent *aContent, nsString* aData)
{
  nsIAtom *name;
  aContent->GetTag(name);
  nsAutoString attr;
  PRBool result = PR_FALSE;
  
  if ((nsnull != name) && 
      ((nsHTMLAtoms::area == name) || (nsHTMLAtoms::a == name)) &&
      (NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::href, attr))) {
      result = PR_TRUE;
  }

  NS_IF_RELEASE(name);
  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::GetLinks(nsIDOMHTMLCollection** aLinks)
{
  if (nsnull == mLinks) {
    mLinks = new nsContentList(this, MatchLinks, nsnull);
    if (nsnull == mLinks) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mLinks);
  }

  *aLinks = (nsIDOMHTMLCollection *)mLinks;
  NS_ADDREF(mLinks);

  return NS_OK;
}

PRBool
nsHTMLDocument::MatchAnchors(nsIContent *aContent, nsString* aData)
{
  nsIAtom *name;
  aContent->GetTag(name);
  nsAutoString attr;
  PRBool result = PR_FALSE;
  
  if ((nsnull != name) && 
      (nsHTMLAtoms::a == name) &&
      (NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::name, attr))) {
      result = PR_TRUE;
  }

  NS_IF_RELEASE(name);
  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::GetAnchors(nsIDOMHTMLCollection** aAnchors)
{
  if (nsnull == mAnchors) {
    mAnchors = new nsContentList(this, MatchAnchors, nsnull);
    if (nsnull == mAnchors) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mAnchors);
  }

  *aAnchors = (nsIDOMHTMLCollection *)mAnchors;
  NS_ADDREF(mAnchors);

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetCookie(nsString& aCookie)
{
  nsresult result = NS_OK;
  NS_WITH_SERVICE(nsICookieService, service, kCookieServiceCID, &result);
  if ((NS_OK == result) && (nsnull != service) && (nsnull != mDocumentURL)) {
    result = service->GetCookieString(mDocumentURL, aCookie);
  }
  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::SetCookie(const nsString& aCookie)
{
  nsresult result = NS_OK;
  NS_WITH_SERVICE(nsICookieService, service, kCookieServiceCID, &result);
  if ((NS_OK == result) && (nsnull != service) && (nsnull != mDocumentURL)) {
    result = service->SetCookieString(mDocumentURL, aCookie);
  }
  return result;
}

nsresult
nsHTMLDocument::GetSourceDocumentURL(JSContext* cx,
                                     nsIURI** sourceURL)
{
  // XXX Tom said this reminded him of the "Six Degrees of
  // Kevin Bacon" game. We try to get from here to there using
  // whatever connections possible. The problem is that this
  // could break if any of the connections along the way change.
  // I wish there were a better way.
  nsresult result = NS_OK;
  *sourceURL = nsnull;

  nsIScriptContext* context = (nsIScriptContext*)JS_GetContextPrivate(cx);
  
  if (nsnull != context) {
    nsCOMPtr<nsIScriptGlobalObject> global;

    global = dont_AddRef(context->GetGlobalObject());
    if (global) {
      nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(global, &result));

      if (window) {
        nsCOMPtr<nsIDOMDocument> document;
        
        result = window->GetDocument(getter_AddRefs(document));
        if (NS_SUCCEEDED(result)) {
          nsCOMPtr<nsIDocument> doc(do_QueryInterface(document, &result));
          if (doc) { 
            *sourceURL = doc->GetDocumentURL(); 
            result = sourceURL ? NS_OK : NS_ERROR_FAILURE; 
          } 
        }
      }
    }
  }

  return result;
}


// XXX TBI: accepting arguments to the open method.
nsresult
nsHTMLDocument::OpenCommon(nsIURI* aSourceURL)
{
  nsresult result = NS_OK;
  // The open occurred after the document finished loading.
  // So we reset the document and create a new one.
  if (nsnull == mParser) {
    nsCOMPtr<nsIChannel> channel;
    nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);

    result = NS_OpenURI(getter_AddRefs(channel), aSourceURL, group);
    if (NS_FAILED(result)) return result;
    result = Reset(channel, group);
    if (NS_FAILED(result)) return result;
    if (NS_OK == result) {
      static NS_DEFINE_IID(kCParserIID, NS_IPARSER_IID);
      static NS_DEFINE_IID(kCParserCID, NS_PARSER_IID);
      
      result = nsComponentManager::CreateInstance(kCParserCID, 
                                                  nsnull, 
                                                  kCParserIID, 
                                                  (void **)&mParser);
      mIsWriting = 1;
      
      if (NS_OK == result) { 
        nsIHTMLContentSink* sink;
        nsIWebShell* webShell = nsnull;
        
        // Get the webshell of our primary presentation shell
        nsIPresShell* shell = (nsIPresShell*) mPresShells.ElementAt(0);
        if (nsnull != shell) {
          nsCOMPtr<nsIPresContext> cx;
          shell->GetPresContext(getter_AddRefs(cx));
          nsISupports* container;
          if (NS_OK == cx->GetContainer(&container)) {
            if (nsnull != container) {
              container->QueryInterface(kIWebShellIID, (void**) &webShell);
            }
          }
        }

        result = NS_NewHTMLContentSink(&sink, this, aSourceURL, webShell);
        NS_IF_RELEASE(webShell);
        
        if (NS_OK == result) {
          nsIDTD* theDTD=0;
          NS_NewNavHTMLDTD(&theDTD);
          mParser->RegisterDTD(theDTD);
          mParser->SetContentSink(sink); 
          NS_RELEASE(sink);
        }
      }
    }
  }

  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::Open()
{
  nsresult result = NS_OK;
  nsIURI* sourceURL;

  // XXX For the non-script Open case, we have to make
  // up a URL.
  result = NS_NewURI(&sourceURL, "about:blank");

  if (NS_SUCCEEDED(result)) {
    result = OpenCommon(sourceURL);
    NS_RELEASE(sourceURL);
  }

  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::Open(JSContext *cx, jsval *argv, PRUint32 argc)
{
  nsresult result = NS_OK;
  nsIURI* sourceURL;

  // XXX The URL of the newly created document will match
  // that of the source document. Is this right?
  result = GetSourceDocumentURL(cx, &sourceURL);
  // Recover if we had a problem obtaining the source URL
  if (nsnull == sourceURL) {
      result = NS_NewURI(&sourceURL, "about:blank");
  }

  if (NS_SUCCEEDED(result)) {
    result = OpenCommon(sourceURL);
    NS_RELEASE(sourceURL);
  }

  return result;
}

#define NS_GENERATE_PARSER_KEY() (void*)((mIsWriting << 31) | (mWriteLevel & 0x7fffffff))

NS_IMETHODIMP    
nsHTMLDocument::Close()
{
  nsresult result = NS_OK;

  if ((nsnull != mParser) && mIsWriting) {
    nsAutoString emptyStr("</HTML>");
    mWriteLevel++;
    result = mParser->Parse(emptyStr, NS_GENERATE_PARSER_KEY(), 
                            "text/html", PR_FALSE, PR_TRUE);
    mWriteLevel--;
    mIsWriting = 0;
  }

  return NS_OK;
}

nsresult
nsHTMLDocument::WriteCommon(const nsString& aText,
                            PRBool aNewlineTerminate)
{
  nsresult result = NS_OK;

  if (nsnull == mParser) {
    result = Open();
    if (NS_OK != result) {
      return result;
    }
  }
  
  nsAutoString str(aText);

  if (aNewlineTerminate) {
    str.Append('\n');
  }

  mWriteLevel++;
  result = mParser->Parse(str, NS_GENERATE_PARSER_KEY(), 
                          "text/html", PR_FALSE, 
                          (!mIsWriting || (mWriteLevel > 1)));
  mWriteLevel--;
  if (NS_OK != result) {
    return result;
  }
  
  return result;
}

NS_IMETHODIMP
nsHTMLDocument::Write(const nsString& aText)
{
  return WriteCommon(aText, PR_FALSE);
}

NS_IMETHODIMP    
nsHTMLDocument::Writeln(const nsString& aText)
{
  return WriteCommon(aText, PR_TRUE);
}

nsresult
nsHTMLDocument::ScriptWriteCommon(JSContext *cx, 
                                  jsval *argv, 
                                  PRUint32 argc,
                                  PRBool aNewlineTerminate)
{
  nsresult result = NS_OK;

  if (nsnull == mParser) {
    result = Open(cx, argv, argc);
    if (NS_OK != result) {
      return result;
    }
  }
  
  if (argc > 0) {
    PRUint32 index;
    nsAutoString str;
    str.Truncate();
    for (index = 0; index < argc; index++) {
      JSString *jsstring = JS_ValueToString(cx, argv[index]);
      
      if (nsnull != jsstring) {
        str.Append(JS_GetStringChars(jsstring));
      }
    }

    if (aNewlineTerminate) {
      str.Append('\n');
    }

    mWriteLevel++;
    result = mParser->Parse(str, NS_GENERATE_PARSER_KEY(), 
                            "text/html", PR_FALSE, 
                            (!mIsWriting || (mWriteLevel > 1)));
    mWriteLevel--;
    if (NS_OK != result) {
      return result;
    }
  }
  
  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::Write(JSContext *cx, jsval *argv, PRUint32 argc)
{
  return ScriptWriteCommon(cx, argv, argc, PR_FALSE);
}

NS_IMETHODIMP    
nsHTMLDocument::Writeln(JSContext *cx, jsval *argv, PRUint32 argc)
{
  return ScriptWriteCommon(cx, argv, argc, PR_TRUE);
}

nsIContent *
nsHTMLDocument::MatchName(nsIContent *aContent, const nsString& aName)
{
  nsAutoString value;
  nsIContent *result = nsnull;

  if ((NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::id, value)) &&
      aName.Equals(value)) {
    return aContent;
  }
  else if ((NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::name, value)) &&
           aName.Equals(value)) {
    return aContent;
  }
  
  PRInt32 i, count;
  aContent->ChildCount(count);
  for (i = 0; i < count && result == nsnull; i++) {
    nsIContent *child;
    aContent->ChildAt(i, child);
    result = MatchName(child, aName);
    NS_RELEASE(child);
  }  

  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::GetElementById(const nsString& aElementId, nsIDOMElement** aReturn)
{
  nsIContent *content;

  // XXX For now, we do a brute force search of the content tree.
  // We should come up with a more efficient solution.
  content = MatchName(mRootContent, aElementId);

  if (nsnull != content) {
    return content->QueryInterface(kIDOMElementIID, (void **)aReturn);
  }
  else {
    *aReturn = nsnull;
  }
  
  return NS_OK;
}

PRBool
nsHTMLDocument::MatchNameAttribute(nsIContent* aContent, nsString* aData)
{
  nsresult result;
  nsAutoString name;

  result = aContent->GetAttribute(kNameSpaceID_None, 
                                  nsHTMLAtoms::name,
                                  name);
  if ((result == NS_OK) && (nsnull != aData) && name.Equals(*aData)) {
    return PR_TRUE;
  }
  else {
    return PR_FALSE;
  }
}

NS_IMETHODIMP    
nsHTMLDocument::GetElementsByName(const nsString& aElementName, 
                                  nsIDOMNodeList** aReturn)
{
  nsContentList* elements = new nsContentList(this, 
                                              MatchNameAttribute, 
                                              &aElementName);
  if (nsnull == elements) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return elements->QueryInterface(kIDOMNodeListIID, (void**)aReturn);
}

NS_IMETHODIMP    
nsHTMLDocument::GetAlinkColor(nsString& aAlinkColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  aAlinkColor.Truncate();
  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->GetALink(aAlinkColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nscolor color;
    result = mAttrStyleSheet->GetActiveLinkColor(color);
    if (NS_OK == result) {
      nsHTMLValue value(color);
      nsGenericHTMLElement::ColorToString(value, aAlinkColor);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::SetAlinkColor(const nsString& aAlinkColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->SetALink(aAlinkColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nsHTMLValue value;
  
    if (nsGenericHTMLElement::ParseColor(aAlinkColor, this, value)) {
      mAttrStyleSheet->SetActiveLinkColor(value.GetColorValue());
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetLinkColor(nsString& aLinkColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  aLinkColor.Truncate();
  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->GetLink(aLinkColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nscolor color;
    result = mAttrStyleSheet->GetLinkColor(color);
    if (NS_OK == result) {
      nsHTMLValue value(color);
      nsGenericHTMLElement::ColorToString(value, aLinkColor);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::SetLinkColor(const nsString& aLinkColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->SetLink(aLinkColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nsHTMLValue value;
    if (nsGenericHTMLElement::ParseColor(aLinkColor, this, value)) {
      mAttrStyleSheet->SetLinkColor(value.GetColorValue());
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetVlinkColor(nsString& aVlinkColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  aVlinkColor.Truncate();
  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->GetVLink(aVlinkColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nscolor color;
    result = mAttrStyleSheet->GetVisitedLinkColor(color);
    if (NS_OK == result) {
      nsHTMLValue value(color);
      nsGenericHTMLElement::ColorToString(value, aVlinkColor);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::SetVlinkColor(const nsString& aVlinkColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->SetVLink(aVlinkColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nsHTMLValue value;
    if (nsGenericHTMLElement::ParseColor(aVlinkColor, this, value)) {
      mAttrStyleSheet->SetVisitedLinkColor(value.GetColorValue());
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetBgColor(nsString& aBgColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  aBgColor.Truncate();
  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->GetBgColor(aBgColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nscolor color;
    result = mAttrStyleSheet->GetDocumentBackgroundColor(color);
    if (NS_OK == result) {
      nsHTMLValue value(color);
      nsGenericHTMLElement::ColorToString(value, aBgColor);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::SetBgColor(const nsString& aBgColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->SetBgColor(aBgColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nsHTMLValue value;
    if (nsGenericHTMLElement::ParseColor(aBgColor, this, value)) {
      mAttrStyleSheet->SetDocumentBackgroundColor(value.GetColorValue());
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetFgColor(nsString& aFgColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  aFgColor.Truncate();
  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->GetText(aFgColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nscolor color;
    result = mAttrStyleSheet->GetDocumentForegroundColor(color);
    if (NS_OK == result) {
      nsHTMLValue value(color);
      nsGenericHTMLElement::ColorToString(value, aFgColor);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::SetFgColor(const nsString& aFgColor)
{
  nsresult result = NS_OK;
  nsIDOMHTMLBodyElement* body;

  result = GetBodyElement(&body);
  if (NS_OK == result) {
    result = body->SetText(aFgColor);
    NS_RELEASE(body);
  }
  else if (nsnull != mAttrStyleSheet) {
    nsHTMLValue value;
  
    if (nsGenericHTMLElement::ParseColor(aFgColor, this, value)) {
      mAttrStyleSheet->SetDocumentForegroundColor(value.GetColorValue());
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetLastModified(nsString& aLastModified)
{
  if (nsnull != mLastModified) {
    aLastModified = *mLastModified;
  }
  else {
    aLastModified.SetString("January 1, 1970 GMT");
  }

  return NS_OK;
}


NS_IMETHODIMP    
nsHTMLDocument::GetEmbeds(nsIDOMHTMLCollection** aEmbeds)
{
  if (nsnull == mEmbeds) {
    mEmbeds = new nsContentList(this, nsHTMLAtoms::embed, kNameSpaceID_HTML);
    if (nsnull == mEmbeds) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mEmbeds);
  }

  *aEmbeds = (nsIDOMHTMLCollection *)mEmbeds;
  NS_ADDREF(mEmbeds);

  return NS_OK;
}

PRBool
nsHTMLDocument::MatchLayers(nsIContent *aContent, nsString* aData)
{
  nsIAtom *name;
  aContent->GetTag(name);
  nsAutoString attr;
  PRBool result = PR_FALSE;
  
  if ((nsnull != name) && 
      ((nsHTMLAtoms::layer == name) || (nsHTMLAtoms::ilayer == name))) {
    result = PR_TRUE;
  }

  NS_IF_RELEASE(name);
  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::GetLayers(nsIDOMHTMLCollection** aLayers)
{
#ifdef NS_IMPLEMENT_DOCUMENT_LAYERS
  if (nsnull == mLayers) {
    mLayers = new nsContentList(this, MatchLayers, nsnull);
    if (nsnull == mLayers) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mLayers);
  }

  *aLayers = (nsIDOMHTMLCollection *)mLayers;
  NS_ADDREF(mLayers);
#else
  *aLayers = nsnull;
#endif

  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetPlugins(nsIDOMHTMLCollection** aPlugins)
{
  //XXX TBImplemented
  *aPlugins = nsnull;
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetSelection(nsString& aReturn)
{
  //XXX TBImplemented
  aReturn.Truncate();
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::CaptureEvents(PRInt32 aEventFlags)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    //mManager->CaptureEvent(aListener);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP    
nsHTMLDocument::ReleaseEvents(PRInt32 aEventFlags)
{
  if (nsnull != mListenerManager) {
    //mListenerManager->ReleaseEvent(aListener);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP    
nsHTMLDocument::RouteEvent(nsIDOMEvent* aEvt)
{
  //XXX Not the best solution -joki
  return NS_OK;
}

PRIntn 
nsHTMLDocument::RemoveStrings(PLHashEntry *he, PRIntn i, void *arg)
{
  char *str = (char *)he->key;

  Recycle(str);
  return HT_ENUMERATE_REMOVE;
}

void
nsHTMLDocument::DeleteNamedItems()
{
  if (nsnull != mNamedItems) {
    PL_HashTableEnumerateEntries(mNamedItems, RemoveStrings, nsnull);
    PL_HashTableDestroy(mNamedItems);
    mNamedItems = nsnull;
  }
}

static PRBool
IsNamedItem(nsIContent* aContent, nsIAtom *aTag,
            PRBool aInForm, nsString& aName)
{
  // Only the content types reflected in Level 0 with a NAME
  // attribute are registered. Images, layers and forms always get 
  // reflected up to the document. Applets and embeds only go
  // to the closest container (which could be a form).
  if ((aTag == nsHTMLAtoms::img) || (aTag == nsHTMLAtoms::form) ||
      (!aInForm && ((aTag == nsHTMLAtoms::applet) || 
                    (aTag == nsHTMLAtoms::embed)))) {
    if ((NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::name, aName)) ||
        (NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::id, aName))) {
      return PR_TRUE;
    }
  }

  if ((aTag == nsHTMLAtoms::layer) || (aTag == nsHTMLAtoms::ilayer)) {
    if ((NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::name, aName)) ||
        (NS_CONTENT_ATTR_HAS_VALUE == aContent->GetAttribute(kNameSpaceID_HTML, nsHTMLAtoms::id, aName))) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

void
nsHTMLDocument::UnregisterNamedItems(nsIContent *aContent, PRBool aInForm)
{
  nsAutoString value;
  nsIAtom *tag;
  aContent->GetTag(tag);
  PRBool inForm;

  if (IsNamedItem(aContent, tag, aInForm, value)) {
      char *nameStr = value.ToNewCString();
      // XXX What about the string held in the hash table entry
      PL_HashTableRemove(mNamedItems, nameStr);
      Recycle(nameStr);
  }
  
  inForm = aInForm || (tag == nsHTMLAtoms::form);
  NS_IF_RELEASE(tag);
  
  PRInt32 i, count;
  aContent->ChildCount(count);
  for (i = 0; i < count; i++) {
    nsIContent *child;
    aContent->ChildAt(i, child);
    UnregisterNamedItems(child, inForm);
    NS_RELEASE(child);
  }    
}

void
nsHTMLDocument::RegisterNamedItems(nsIContent *aContent, PRBool aInForm)
{
  nsAutoString value;
  nsIAtom *tag;
  aContent->GetTag(tag);
  PRBool inForm;

  if (IsNamedItem(aContent, tag, aInForm, value)) {
    char *nameStr = value.ToNewCString();
    PL_HashTableAdd(mNamedItems, nameStr, aContent);
  }

  inForm = aInForm || (tag == nsHTMLAtoms::form);
  NS_IF_RELEASE(tag);
  
  PRInt32 i, count;
  aContent->ChildCount(count);
  for (i = 0; i < count; i++) {
    nsIContent *child;
    aContent->ChildAt(i, child);
    RegisterNamedItems(child, inForm);
    NS_RELEASE(child);
  }  
}

nsIContent*
nsHTMLDocument::FindNamedItem(nsIContent *aContent, 
                              const nsString& aName,
                              PRBool aInForm)
{
  nsAutoString value;
  nsIAtom *tag;
  aContent->GetTag(tag);
  PRBool inForm;

  if (IsNamedItem(aContent, tag, aInForm, value)) {
    if (aName.Equals(value)) {
      NS_IF_RELEASE(tag);
      return aContent;
    }
  }

  inForm = aInForm || (tag == nsHTMLAtoms::form);
  NS_IF_RELEASE(tag);
  
  PRInt32 i, count;
  nsIContent *result = nsnull;
  aContent->ChildCount(count);
  for (i = 0; (i < count) && (nsnull == result); i++) {
    nsIContent *child;
    aContent->ChildAt(i, child);
    result = FindNamedItem(child, aName, inForm);
    NS_RELEASE(child);
  }  

  return result;
}

NS_IMETHODIMP    
nsHTMLDocument::NamedItem(const nsString& aName, nsIDOMElement** aReturn)
{
  nsresult result = NS_OK;
  nsIContent *content = nsnull;

  // XXX If we have a parser, it means that we're still loading the
  // document. Since there's still content coming in (and not all
  // may yet have been explicitly added to the document), we do
  // a depth-first search rather than build up a table.
  // Obviously, this may be inefficient for large documents.
  if (nsnull != mParser) {
    content = FindNamedItem(mRootContent, aName, PR_FALSE);
  }
  else {
    // If the document has completed loading, we build a table and
    // cache the named items. The table will be updated as content
    // is added and removed.
    if (nsnull == mNamedItems) {
      mNamedItems = PL_NewHashTable(10, PL_HashString, PL_CompareStrings, 
                                    PL_CompareValues, nsnull, nsnull);
      RegisterNamedItems(mRootContent, PR_FALSE);
    }

    char *str = aName.ToNewCString();
    content = (nsIContent *)PL_HashTableLookup(mNamedItems, str);
    Recycle(str);
  }

  if (nsnull != content) {
    result = content->QueryInterface(kIDOMElementIID, (void **)aReturn);
  }
  else {
    *aReturn = nsnull;
  }

  return result;
}

NS_IMETHODIMP
nsHTMLDocument::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
  nsresult res = NS_OK;
  nsCOMPtr<nsIScriptGlobalObject> global;

  if (nsnull == mScriptObject) {
    // XXX We make the (possibly erroneous) assumption that the first
    // presentation shell represents the "primary view" of the document
    // and that the JS parent chain should incorporate just that view.
    // This is done for lack of a better model when we have multiple
    // views.
    nsIPresShell* shell = (nsIPresShell*) mPresShells.ElementAt(0);
    if (shell) {
      nsCOMPtr<nsIPresContext> cx;
      shell->GetPresContext(getter_AddRefs(cx));
      nsCOMPtr<nsISupports> container;
      
      res = cx->GetContainer(getter_AddRefs(container));
      if (NS_SUCCEEDED(res) && container) {
        nsCOMPtr<nsIScriptContextOwner> sco = do_QueryInterface(container);
        if (sco) {
          res = sco->GetScriptGlobalObject(getter_AddRefs(global));
        }
      }
    }
    // XXX If we can't find a view, parent to the calling context's
    // global object. This may not be right either, but we need
    // something.
    else {
      global = getter_AddRefs(aContext->GetGlobalObject());
    }
    
    if (NS_SUCCEEDED(res)) {
      res = NS_NewScriptHTMLDocument(aContext, 
                                     (nsISupports *)(nsIDOMHTMLDocument *)this,
                                     (nsISupports *)global, 
                                     (void**)&mScriptObject);
    }
  }
  
  *aScriptObject = mScriptObject;

  return res;
}

PRBool    
nsHTMLDocument::Resolve(JSContext *aContext, jsval aID)
{
  nsCOMPtr<nsIDOMElement> element;
  char* str = JS_GetStringBytes(JS_ValueToString(aContext, aID));
  nsAutoString name(str); 
  nsresult result = NS_OK;
  PRBool ret = PR_TRUE;

  result = NamedItem(name, getter_AddRefs(element));
  if (NS_SUCCEEDED(result) && element) {
    nsCOMPtr<nsIScriptObjectOwner> owner = do_QueryInterface(element);
    
    if (owner) {
      nsCOMPtr<nsIScriptContext> scriptContext;
      nsCOMPtr<nsIScriptContextOwner> contextOwner;

      contextOwner = getter_AddRefs(GetScriptContextOwner());
      if (contextOwner) {
        result = contextOwner->GetScriptContext(getter_AddRefs(scriptContext));
      }

      if (!scriptContext) {
        scriptContext = dont_AddRef((nsIScriptContext*)JS_GetContextPrivate(aContext));
      }

      JSObject* obj;
      if (scriptContext) {
        result = owner->GetScriptObject(scriptContext, (void**)&obj);
        if (NS_SUCCEEDED(result) && (nsnull != obj)) {
          JSObject* myObj;
          result = GetScriptObject(scriptContext, (void**)&myObj);
          ret = ::JS_DefineProperty(aContext, myObj,
                                    str, OBJECT_TO_JSVAL(obj),
                                    nsnull, nsnull, 0);
        }
      }
    }
  }

  if (NS_FAILED(result)) {
    ret = PR_FALSE;
  }

  return ret;
}

//----------------------------
static PRBool IsInline(eHTMLTags aTag)
{
  PRBool  result = PR_FALSE;

  switch (aTag)
  {
    case  eHTMLTag_a:
    case  eHTMLTag_address:
    case  eHTMLTag_big:
    case  eHTMLTag_blink:
    case  eHTMLTag_b:
    case  eHTMLTag_br:
    case  eHTMLTag_cite:
    case  eHTMLTag_code:
    case  eHTMLTag_dfn:
    case  eHTMLTag_em:
    case  eHTMLTag_font:
    case  eHTMLTag_img:
    case  eHTMLTag_i:
    case  eHTMLTag_kbd:
    case  eHTMLTag_keygen:
    case  eHTMLTag_nobr:
    case  eHTMLTag_samp:
    case  eHTMLTag_small:
    case  eHTMLTag_spacer:
    case  eHTMLTag_span:      
    case  eHTMLTag_strike:
    case  eHTMLTag_strong:
    case  eHTMLTag_sub:
    case  eHTMLTag_sup:
    case  eHTMLTag_td:
    case  eHTMLTag_textarea:
    case  eHTMLTag_tt:
    case  eHTMLTag_var:
    case  eHTMLTag_wbr:
           
      result = PR_TRUE;
      break;

    default:
      break;

  }
  return result;
}

//----------------------------
static PRBool IsBlockLevel(eHTMLTags aTag, PRBool &isPreTag)
{
  isPreTag = (aTag == eHTMLTag_pre);

  return !IsInline(aTag);
}

//----------------------------
class SubText {
public:
  nsIDOMNode * mContentNode;
  PRInt32      mOffset;
  PRInt32      mLength;
};

//----------------------------
class BlockText {
  nsString   mText;
  SubText  * mSubTexts[512];
  PRInt32    mNumSubTexts;
public:
  BlockText() : mNumSubTexts(0) {  PRInt32 i; for (i=0;i<512;i++) mSubTexts[i] = 0; } // debug only
  virtual ~BlockText();

  char * GetText()               { return mText.ToNewCString(); }
  const nsString & GetNSString() { return mText; }

  void    ClearBlock();
  PRInt32 GetNumItems() { return mNumSubTexts; }

  void AddSubText(nsIDOMNode * aNode, nsString & aStr, PRInt32 aDirection, PRInt32 & aOffset);
  PRInt32 GetStartEnd(PRInt32 anIndex, PRInt32 aLength,
                       nsIDOMNode ** aStartNode, PRInt32 & aStartOffset,
                       nsIDOMNode ** aEndNode,   PRInt32 & aEndOffset,
                       PRInt32       aDirection);
};

//----------------------------
BlockText::~BlockText() 
{
  ClearBlock();
}

//----------------------------
void BlockText::ClearBlock() 
{
  PRInt32 i;
  for (i=0;i<mNumSubTexts;i++) {
    delete mSubTexts[i];
  }
  mNumSubTexts = 0;
  mText.SetLength(0);
}

//----------------------------
void BlockText::AddSubText(nsIDOMNode * aNode, nsString & aStr, PRInt32 aDirection, PRInt32 & aOffset) 
{
  SubText* subTxt     = new SubText();
  if(subTxt) {
    subTxt->mContentNode = aNode;
    subTxt->mLength      = aStr.Length();
    if (aDirection == kForward) {
      subTxt->mOffset = mText.Length();
      mText.Append(aStr);
      mSubTexts[mNumSubTexts++] = subTxt;
    } 
    else {
      subTxt->mOffset = 0;
      // Shift them all down one slot
      PRInt32 i;
      for (i=mNumSubTexts;i>0;i--) {
        mSubTexts[i] = mSubTexts[i-1];
        mSubTexts[i]->mOffset += aStr.Length();
      }
      mNumSubTexts++;
      mText.Insert(aStr, 0, aStr.Length());
      mSubTexts[0] = subTxt;
    }
  }
}

//-----------------------------------------------
PRInt32 BlockText::GetStartEnd(PRInt32 anIndex, PRInt32 aLength,
                               nsIDOMNode ** aStartNode, PRInt32 & aStartOffset,
                               nsIDOMNode ** aEndNode,   PRInt32 & aEndOffset,
                               PRInt32       aDirection)
{
  
  PRInt32 i      = 0;
  PRInt32 endPos = anIndex + aLength;
  while (anIndex > mSubTexts[i]->mOffset+mSubTexts[i]->mLength) {
    i++;
  }
  aStartOffset  = anIndex - mSubTexts[i]->mOffset;
  *aStartNode = mSubTexts[i]->mContentNode;
  if (endPos <= mSubTexts[i]->mOffset+mSubTexts[i]->mLength) {
    aEndOffset = aStartOffset + aLength;
    *aEndNode = *aStartNode;
  } else {

    while (endPos > mSubTexts[i]->mOffset+mSubTexts[i]->mLength) {
      i++;
    }
    aEndOffset   = endPos - mSubTexts[i]->mOffset;
    *aEndNode = mSubTexts[i]->mContentNode;
  }

  if (aDirection == kBackward) {
    endPos -= aLength;
  }
  return endPos;

}

//-----------------------------------------------
PRBool nsHTMLDocument::SearchBlock(BlockText  & aBlockText, 
                                   nsString   & aStr,
                                   nsIDOMNode * aCurrentBlock) 
{
  PRBool found = PR_FALSE;

  PRInt32 lastBlockSearchOffset; 
  PRBool  adjustToEnd;

  if (aCurrentBlock == mHoldBlockContent || mHoldBlockContent == nsnull) {
    lastBlockSearchOffset = mLastBlockSearchOffset;
    adjustToEnd           = mAdjustToEnd;
  } else {
    lastBlockSearchOffset = 0;
    adjustToEnd           = PR_TRUE;
  }

  char * searchStr;
  char * contentStr;

  if (!mShouldMatchCase) {
    nsString searchTxt(aStr);
    nsString blockTxt(aBlockText.GetNSString());

    searchTxt.ToLowerCase();
    blockTxt.ToLowerCase();
    searchStr  = searchTxt.ToNewCString();
    contentStr = blockTxt.ToNewCString();
  } else {
    searchStr  = aStr.ToNewCString();
    contentStr = aBlockText.GetText();
  }

  char * adjustedContent;
  char * str = nsnull;

  if (mSearchDirection == kForward) {
    adjustedContent = contentStr + lastBlockSearchOffset;
    str = strstr(adjustedContent, searchStr);
  } else {
    adjustedContent = contentStr;
    size_t adjLen;
    size_t srchLen = strlen(searchStr);
    if (adjustToEnd) {
      adjLen = strlen(adjustedContent);
      if (srchLen > adjLen) {
        str = nsnull;
      } else if (adjLen == srchLen && !strncmp(adjustedContent, searchStr, srchLen)) {
        str = adjustedContent;
      } else {
        str = adjustedContent + adjLen - srchLen;
        while (strncmp(str, searchStr, srchLen)) {
          str--;
          if (str < adjustedContent) {
            str = nsnull;
            break;
          }
        }
      }
    } else {
      adjLen = lastBlockSearchOffset;
      if (lastBlockSearchOffset > 0) {
        str = adjustedContent + adjLen - 1;
        while (strncmp(str, searchStr, srchLen)) {
          str--;
          if (str < adjustedContent) {
            str = nsnull;
            break;
          }
        }
      }
    }
  }

#if 0
//DEBUG MJUDGE
  if (str) {
    PRInt32 inx = str - contentStr;

    nsIDOMNode * startNode;
    PRInt32      startOffset;
    nsIDOMNode * endNode;
    PRInt32      endOffset;

    mLastBlockSearchOffset      = aBlockText.GetStartEnd(inx, aStr.Length(), &startNode, startOffset, &endNode, endOffset, mSearchDirection);
    mHoldBlockContent           = aCurrentBlock;
    nsSelectionRange * range    = mSelection->GetRange();
    nsSelectionPoint * startPnt = range->GetStartPoint();
    nsSelectionPoint * endPnt   = range->GetEndPoint();

    nsIContent* content;
    nsresult rv = startNode->QueryInterface(kIContentIID,(void **)&content);
    if (NS_OK == rv) {
      startPnt->SetPoint(content, startOffset, PR_TRUE);
      NS_RELEASE(content);
    }
    rv = endNode->QueryInterface(kIContentIID,(void **)&content);
    if (NS_OK == rv) {
      endPnt->SetPoint(content, endOffset, PR_FALSE);
      NS_RELEASE(content);
    }

    found = PR_TRUE;
  }
#endif //0
  delete[] searchStr;
  delete[] contentStr;

  return found;
}

static NS_DEFINE_CID(kParserServiceCID, NS_PARSERSERVICE_CID);

////////////////////////////////////////////////////
// Check to see if a Content node is a block tag
////////////////////////////////////////////////////
PRBool nsHTMLDocument::NodeIsBlock(nsIDOMNode * aNode) 
{
  PRBool isBlock = PR_FALSE;

  nsIDOMElement* domElement;
  nsresult rv = aNode->QueryInterface(kIDOMElementIID,(void **)&domElement);
  if (NS_OK != rv) {
    return isBlock;
  }
  nsAutoString tagName;
  domElement->GetTagName(tagName);
  NS_RELEASE(domElement);

  // XXX Should be done at a higher level than this routine
  // since getting the service is not the cheapest operation.
  // Waiting for mjudge to tell me where since it looks like
  // this code is still in development.

  NS_WITH_SERVICE(nsIParserService, service, kParserServiceCID, &rv);

  if (NS_SUCCEEDED(rv)) {
    PRInt32 id;
    
    service->HTMLStringTagToId(tagName, &id);
    isBlock = IsBlockLevel(nsHTMLTag(id), mIsPreTag);
  }

  return isBlock;
}

/////////////////////////////////////////////
// This function moves up the parent hierarchy 
// looking for a parent that is a "block"
/////////////////////////////////////////////
nsIDOMNode * nsHTMLDocument::FindBlockParent(nsIDOMNode * aNode, 
                                             PRBool       aSkipThisContent)
{
  // Clear up stack and release content nodes on it
  PRInt32 i;
  for (i=0;i<mStackInx;i++) {
    NS_RELEASE(mParentStack[i]);
    NS_RELEASE(mChildStack[i]);
  }
  mStackInx = 0;
  nsIDOMNode * parent;
  aNode->GetParentNode(&parent);
  nsIDOMNode * child;

  if (parent == nsnull) {
    return nsnull;
  }
  NS_ADDREF(aNode);

  // This method enables the param "aNode" be part of the "path"
  // on the stack as the it look for the block parent.
  //
  // There are times when we don't want to include the aNode
  // as part of the path so we skip to its prev/next sibling. If it was
  // the first/last sibling then we jump up to it's parent.
  if (aSkipThisContent) {
    child = aNode;
    nsIDOMNode * nextChild;

    PRBool done = PR_FALSE;
    while (!done) {

      if (mSearchDirection == kForward) {
        child->GetNextSibling(&nextChild);
      } else {
        child->GetPreviousSibling(&nextChild);
      }

      if (nextChild == nsnull) {
        NS_RELEASE(child);
        child = parent;
        child->GetParentNode(&parent);
        if (parent == nsnull) {
          NS_RELEASE(child);
          return nsnull;
        }
      } else {
        NS_RELEASE(child);
        child = nextChild;
        done  = PR_TRUE;
      }
    } // while

  } else {
    child = aNode;
  }

  // This travels up through the parents looking for the parent who
  // is a block tag. We place the child/parent pairs onto a stack 
  // so we know what nodes to skip as we work our way back down into
  // the block
  do {

    NS_ADDREF(parent);
    NS_ADDREF(child);
    mParentStack[mStackInx]  = parent;
    mChildStack[mStackInx++] = child;

    if (parent == mBodyContent) {
      break;
    }

    nsIDOMNode * oldChild = child;
    child  = parent;
    child->GetParentNode(&parent);
    NS_RELEASE(oldChild);
  } while (parent != nsnull);


  NS_RELEASE(child);
  return parent;
}

//-----------------------------------------------
PRBool nsHTMLDocument::BuildBlockFromContent(nsIDOMNode * aNode,
                                             BlockText     & aBlockText,
                                             nsIDOMNode * aCurrentBlock)
{
  // First check to see if it is a new Block node
  // If it is then we need to check the current block text (aBlockText)
  // to see if it holds the search string
  if (NodeIsBlock(aNode)) {

    // Search current block of text
    if (SearchBlock(aBlockText, *mSearchStr, aCurrentBlock)) {
      return PR_TRUE;
    }

    // Clear the text we have already searched
    aBlockText.ClearBlock();

    // Start new search here on down with a new block
    BlockText blockText;
    if (!BuildBlockTraversing(aNode, blockText, aNode)) {
      // down inside the search string wasn't found check the full block text 
      // for the search text
      if (SearchBlock(blockText, *mSearchStr, aNode)) {
        return PR_TRUE;
      }
    } else {
      // search text was found down inside, so leave
      return PR_TRUE;
    }

  } else {
    if (BuildBlockTraversing(aNode, aBlockText, aCurrentBlock)) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

//----------------------------------
// This function moves down through
// the hiearchy and build the block of text
//-----------------------------------------
PRBool nsHTMLDocument::BuildBlockTraversing(nsIDOMNode * aParent,
                                            BlockText  & aBlockText,
                                            nsIDOMNode * aCurrentBlock) 
{
  nsIDOMText* textContent;
  nsresult rv = aParent->QueryInterface(kIDOMTextIID,(void **)&textContent);
  if (NS_OK == rv) {
    nsString stringBuf;
    textContent->GetData(stringBuf);

    if (aCurrentBlock == mHoldBlockContent || mHoldBlockContent == nsnull) {
      if (mSearchDirection == kBackward && aBlockText.GetNumItems() > 0) {
        mLastBlockSearchOffset += stringBuf.Length();
      }
    }

    aBlockText.AddSubText(aParent, stringBuf, mSearchDirection, mLastBlockSearchOffset);
    NS_RELEASE(textContent);
  } else {
    PRBool hasChildNode;
    aParent->HasChildNodes(&hasChildNode);
    if (hasChildNode) {
      nsIDOMNode * child;
      if (mSearchDirection == kForward) {
        aParent->GetFirstChild(&child);
      } else {
        aParent->GetLastChild(&child);
      }
      while (nsnull != child) {
        PRBool found = BuildBlockFromContent(child, aBlockText, aCurrentBlock);
        nsIDOMNode * sibling = child;
        NS_IF_RELEASE(child);
        if (found) {
          return PR_TRUE;
        }
        if (mSearchDirection == kForward) {
          sibling->GetNextSibling(&child);
        } else {
          sibling->GetPreviousSibling(&child);
        }
      }
    }
  }

  return PR_FALSE;
}

//----------------------------------
// This function moves down through
// the hiearchy and build the block of text
//-----------------------------------------
PRBool nsHTMLDocument::BuildBlockFromStack(nsIDOMNode * aParent,
                                           BlockText  & aBlockText,
                                           PRInt32      aStackInx,
                                           nsIDOMNode * aCurrentBlock) 
{
  nsIDOMNode * stackChild  = mChildStack[aStackInx];

  PRBool hasChildNode;
  aParent->HasChildNodes(&hasChildNode);
  if (hasChildNode) {
    nsIDOMNode * child = stackChild;
    NS_ADDREF(child);

    while (nsnull != child) {

      if (child == stackChild && aStackInx > 0) {
        if (NodeIsBlock(child)) {
          if (SearchBlock(aBlockText, *mSearchStr, aCurrentBlock)) {
            NS_IF_RELEASE(child);
            return PR_TRUE;
          } else {
            aBlockText.ClearBlock();
          }
          BlockText blockText;
          if (BuildBlockFromStack(child, blockText, aStackInx-1, child)) {
            NS_IF_RELEASE(child);
            return PR_TRUE;
          }
          if (SearchBlock(blockText, *mSearchStr, child)) {
            return PR_TRUE;
          }
        } else {
          if (BuildBlockFromStack(child, aBlockText, aStackInx-1, aCurrentBlock)) {
            NS_IF_RELEASE(child);
            return PR_TRUE;
          }
        }
      } else {
        if (BuildBlockFromContent(child, aBlockText, aCurrentBlock)) {
          NS_IF_RELEASE(child);
          return PR_TRUE;
        }
      }

      nsIDOMNode * sibling = child;
      NS_RELEASE(child);
      if (mSearchDirection == kForward) {
        sibling->GetNextSibling(&child);
      } else {
        sibling->GetPreviousSibling(&child);
      }
    }
  }

  return PR_FALSE;

}


#if 0 // debug
void printDOMRefs(nsIDOMNode * aNode, PRInt32 aLevel)
{
  char * cStr = nsnull;

  PRInt32 i;
  for (i=0;i<aLevel;i++) {
    printf(".");
  }

  nsIDOMElement* domElement;
  nsresult rv = aNode->QueryInterface(kIDOMElementIID,(void **)&domElement);
  if (NS_OK == rv) {
    nsString tagName;
    domElement->GetTagName(tagName);
    cStr = tagName.ToNewCString();
    NS_RELEASE(domElement);
  }

  static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
  nsIDOMText* textContent;
  rv = aNode->QueryInterface(kIDOMTextIID,(void **)&textContent);
  if (NS_OK == rv) {
    nsString stringBuf;
    textContent->GetData(stringBuf);
    cStr = stringBuf.ToNewCString();
    NS_RELEASE(textContent);
  }

  if (cStr) {
    for (i=0;i<PRInt32(strlen(cStr));i++) {
      if (cStr[i] < 15) {
        cStr[i] =  ' ';
      }
    }
  }

  printf("[%s] (0x%x)\n", (cStr?cStr:"<?>"), aNode);
  delete[] cStr;

  PRBool hasChildren;
  aNode->HasChildNodes(&hasChildren);
  if (hasChildren) {
    nsIDOMNode * childNode;
    aNode->GetFirstChild(&childNode);
    while (childNode != nsnull) {
      printDOMRefs(childNode, aLevel+2);
      nsIDOMNode * oldChild = childNode;
      oldChild->GetNextSibling(&childNode);
      NS_RELEASE(oldChild);
    }
  }
}
#endif



static nsIDOMNode * FindDOMNode(nsIDOMNode * aNode, nsIContent * aContent)
{
  nsIContent* content;
  nsresult rv = aNode->QueryInterface(kIContentIID,(void **)&content);
  if (NS_OK == rv) {
    if (content == aContent) {
      return aNode;
    }
  }

  PRBool hasChildren;
  aNode->HasChildNodes(&hasChildren);
  if (hasChildren) {
    nsIDOMNode * childNode;
    aNode->GetFirstChild(&childNode);
    while (childNode != nsnull) {
      nsIDOMNode * node = FindDOMNode(childNode, aContent);
      if (node != nsnull) {
        return node;
      }
      nsIDOMNode * oldChild = childNode;
      oldChild->GetNextSibling(&childNode);
      NS_RELEASE(oldChild);
    }
  }
  return nsnull;
}

/**
  * Finds text in content
 */
NS_IMETHODIMP nsHTMLDocument::FindNext(const nsString &aSearchStr, PRBool aMatchCase, PRBool aSearchDown, PRBool &aIsFound)
{
#if 0
//DEBUG MJUDGE
  aIsFound         = PR_FALSE;
  mShouldMatchCase = aMatchCase;

  if (mSearchStr == nsnull) {
    mSearchStr = new nsString(aSearchStr);
  } else {
    mSearchStr->SetLength(0);
    mSearchStr->Append(aSearchStr);
  }

  // Temporary
  PRBool   doReplace = PR_FALSE;
  nsString replacementStr("xxxx");
  PRInt32 inx = mSearchStr->FindChar('/');
  if (inx > -1) {
    if (inx == mSearchStr->Length()-1) {
      replacementStr.SetLength(0);
    } else {
      replacementStr = *mSearchStr;
      replacementStr.Cut(0, inx+1);
    }
    mSearchStr->Truncate(inx);

    doReplace = PR_TRUE;
  }

  nsIDOMElement * root = nsnull;
  if (NS_OK != GetDocumentElement(&root)) {  
    return PR_FALSE;
  }

  // DEBUG printDOMRefs(root, 0);

  nsIDOMNode * start = nsnull;
  nsIDOMNode * end   = nsnull;
  nsIDOMNode * child;

  if (mBodyContent == nsnull && PR_FALSE == GetBodyContent()) {
    return NS_OK;
  }

  //printRefs(mBodyContent, 0);

  start = mBodyContent;
  NS_ADDREF(start);

  // Find very first Piece of Content
  while (1) {
    start->GetFirstChild(&child);
    if (child == nsnull) {
      break;
    }
    NS_RELEASE(start);
    start = child;
  }

  end = mBodyContent;
  NS_ADDREF(end);

  // Find very last piece of Content
  while (1) {
    end->GetLastChild(&child);
    if (child == nsnull) {
      break;
    }
    NS_RELEASE(end);
    end = child;
  }

  nsSelectionRange * range        = mSelection->GetRange();
  nsSelectionPoint * startPnt     = range->GetStartPoint();
  nsSelectionPoint * endPnt       = range->GetEndPoint();
  nsIContent       * endContent   = range->GetEndContent();
  nsIContent       * startContent;

  mSearchDirection = aSearchDown? kForward:kBackward;
  nsIDOMNode * searchNode = nsnull;

  // This is a short circut for starting to search 
  // at the beginning of the doocument forward
  if (endContent == nsnull && mSearchDirection == kForward) {
    searchNode = mBodyContent;
    BlockText blockText;
    if (!BuildBlockTraversing(searchNode, blockText, mBodyContent)) {
      if (SearchBlock(blockText, *mSearchStr, mBodyContent)) {
        aIsFound = PR_TRUE;
      }
    } else {
      aIsFound = PR_TRUE;
    }
  } else {

    mAdjustToEnd = PR_FALSE;

    // Convert the nsICOntent Node in the Selection Points 
    // to their DOM Node counter part
    if (mSearchDirection == kForward) {
      nsIDOMNode * endNode;
      nsresult rv = endContent->QueryInterface(kIDOMNodeIID,(void **)&endNode);
      if (NS_OK == rv) {
        searchNode = endNode;
      }
      mLastBlockSearchOffset = endPnt->GetOffset();
    } else {
      nsIContent * startContent = range->GetStartContent();
      if (startContent == nsnull) {
        searchNode   = end;
        mAdjustToEnd = PR_TRUE;
      } else {
        nsIDOMNode * startNode;
        nsresult rv = startContent->QueryInterface(kIDOMNodeIID,(void **)&startNode);
        if (NS_OK == rv) {
          searchNode = startNode;
        }
        mLastBlockSearchOffset = startPnt->GetOffset();
        NS_IF_RELEASE(startContent);
      }
    }

    nsIDOMNode * blockContent = FindBlockParent(searchNode); // this ref counts blockContent
    while (blockContent != nsnull) {

      BlockText blockText;
      if (BuildBlockFromStack(blockContent, blockText, mStackInx-1, blockContent)) {
        aIsFound = PR_TRUE;
        break;
      }
      if (SearchBlock(blockText, *mSearchStr, blockContent)) {
        return PR_TRUE;
      }

      mLastBlockSearchOffset = 0;
      mAdjustToEnd           = PR_TRUE;

      nsIDOMNode * blockChild = blockContent;
      blockContent = FindBlockParent(blockChild, PR_TRUE);
      NS_RELEASE(blockChild);
    }

    // Clear up stack and release content nodes on it
    PRInt32 i;
    for (i=0;i<mStackInx;i++) {
      NS_RELEASE(mParentStack[i]);
      NS_RELEASE(mChildStack[i]);
    }
    mStackInx = 0;
  }

  NS_IF_RELEASE(endContent);
  NS_IF_RELEASE(searchNode);
  NS_IF_RELEASE(start);
  NS_IF_RELEASE(end);

  SetDisplaySelection(PR_TRUE); 


  if (aIsFound && doReplace) {
    range        = mSelection->GetRange();
    startContent = range->GetStartContent();
    endContent   = range->GetEndContent();
    startPnt     = range->GetStartPoint();
    endPnt       = range->GetEndPoint();

    nsIDOMElement * root = nsnull;
    if (NS_OK == GetDocumentElement(&root)) {  
      nsIDOMNode * node = FindDOMNode(root, startContent);

      nsString contentStr;
      //static NS_DEFINE_IID(kIDOMTextIID, NS_IDOMTEXT_IID);
      nsIDOMText* textContent;
      nsresult rv = node->QueryInterface(kIDOMTextIID,(void **)&textContent);
      if (NS_OK == rv) {
        textContent->GetData(contentStr);
        NS_RELEASE(textContent);
      }

      //PRInt32 offset = contentStr.Find(*mSearchStr, !mShouldMatchCase);
      PRInt32 offset;
      // temporary
      if (mShouldMatchCase) {
        offset = contentStr.Find(*mSearchStr);
        if (offset == -1) {
          offset = contentStr.RFind(*mSearchStr);
        }
      } else {
        nsString cs(contentStr);
        nsString ss(*mSearchStr);
        cs.ToLowerCase();
        ss.ToLowerCase();
        offset = cs.Find(ss);
        if (offset == -1) {
          offset = cs.RFind(ss);        
        }
      } 
      // end temporary

      if (offset > -1) {
        contentStr.Cut(offset, mSearchStr->Length());
        contentStr.Insert(replacementStr, offset, replacementStr.Length());

        nsIDOMNode * parent;
        node->GetParentNode(&parent);

          //nsIDOMNode * delNode;
          //parent->ReplaceChild(newNode, node, &delNode);

        PRBool       nodeWasAdded = PR_FALSE;
        nsIDOMNode * nextNode;
        nsIDOMNode * prevNode;
        nsIDOMNode * delNode;

        node->GetPreviousSibling(&prevNode);
        node->GetNextSibling(&nextNode);

        parent->RemoveChild(node, &delNode);
        NS_IF_RELEASE(delNode);

        nsIDOMNode * contentNode = nsnull;
        PRInt32      newOffset;

        if (contentStr.Length() > 0) {
          nsIDOMNode * retNode;
          nsIDOMText * newNode;
          CreateTextNode(contentStr, &newNode);
          if (nsnull != nextNode) {
            parent->InsertBefore(newNode, nextNode, &retNode);
          } else {
            parent->AppendChild(newNode, &retNode);
          }

          newOffset    = offset + replacementStr.Length();
          contentNode  = newNode;
          nodeWasAdded = PR_TRUE;

          NS_IF_RELEASE(newNode);
          NS_IF_RELEASE(retNode);
        } else {
          newOffset   = 0;
          contentNode = (nextNode == nsnull ? prevNode : nextNode);
          // XXX Bummer if previous is also null then this was the only child
          // now we have to go find a different node to set the content to for selection
          // So we will use the parent node for now, but this needs to be changed
          if (contentNode == nsnull) {
            contentNode = parent;
          }
        }

        NS_ADDREF(contentNode);

        NS_IF_RELEASE(nextNode);
        NS_IF_RELEASE(prevNode);

        if (contentNode != nsnull) {
          static NS_DEFINE_IID(kIContentIID, NS_ICONTENT_IID);
          nsIContent* content;
          rv = contentNode->QueryInterface(kIContentIID,(void **)&content);
          if (NS_OK == rv) {
            //range    = mSelection->GetRange();
            //startPnt = range->GetStartPoint();
            //endPnt   = range->GetEndPoint();

            startPnt->SetContent(content);
            startPnt->SetOffset(newOffset);
            endPnt->SetContent(content);
            endPnt->SetOffset(newOffset);
            range->SetStartPoint(startPnt);
            range->SetEndPoint(endPnt);
            NS_RELEASE(content);
          }
        }
        NS_IF_RELEASE(contentNode);
        NS_IF_RELEASE(parent);
        NS_IF_RELEASE(node);
        NS_IF_RELEASE(root);
      }
    }
    NS_RELEASE(startContent);
    NS_RELEASE(endContent);
  }
#endif //0
  return NS_OK;
}

PRBool
nsHTMLDocument::GetBodyContent()
{
  nsIDOMElement * root = nsnull;
  if (NS_OK != GetDocumentElement(&root)) {  
    return PR_FALSE;
  }

  nsAutoString bodyStr("BODY");
  nsIDOMNode * child;
  root->GetFirstChild(&child);

  while (child != nsnull) {
    nsIDOMElement* domElement;
    nsresult rv = child->QueryInterface(kIDOMElementIID,(void **)&domElement);
    if (NS_OK == rv) {
      nsString tagName;
      domElement->GetTagName(tagName);
      if (bodyStr.EqualsIgnoreCase(tagName)) {
        mBodyContent = child;
        NS_RELEASE(root);
        NS_RELEASE(domElement);
        return PR_TRUE;
      }
      NS_RELEASE(domElement);
    }
    nsIDOMNode * node = child;
    NS_RELEASE(child);
    node->GetNextSibling(&child);
  }
  NS_RELEASE(root);
  return PR_FALSE;
}

nsresult
nsHTMLDocument::GetBodyElement(nsIDOMHTMLBodyElement** aBody)
{
  if (mBodyContent == nsnull && PR_FALSE == GetBodyContent()) {
    return NS_ERROR_FAILURE;
  }
  
  return mBodyContent->QueryInterface(kIDOMHTMLBodyElementIID, 
                                      (void**)aBody);
}

// forms related stuff

NS_IMETHODIMP 
nsHTMLDocument::AddForm(nsIDOMHTMLFormElement *aForm)
{
#if 0
  // Not necessary anymore since forms are real content now
  NS_PRECONDITION(nsnull != aForm, "null ptr");
  if (nsnull == aForm) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIContent* iContent = nsnull;
  nsresult result = aForm->QueryInterface(kIContentIID, (void**)&iContent);
  if ((NS_OK == result) && iContent) {
    nsIDOMHTMLCollection* forms = nsnull;
    
    // Initialize mForms if necessary...
    if (nsnull == mForms) {
      mForms = new nsContentList(this, nsHTMLAtoms::form, kNameSpaceID_HTML);
      NS_ADDREF(mForms);
    }

    mForms->Add(iContent);
    NS_RELEASE(iContent);
  }
  return result;
#endif
  return NS_OK;
}

NS_IMETHODIMP    
nsHTMLDocument::GetForms(nsIDOMHTMLCollection** aForms)
{
  if (nsnull == mForms) {
    mForms = new nsContentList(this, nsHTMLAtoms::form, kNameSpaceID_HTML);
    if (nsnull == mForms) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mForms);
  }

  *aForms = (nsIDOMHTMLCollection *)mForms;
  NS_ADDREF(mForms);

  return NS_OK;
}



