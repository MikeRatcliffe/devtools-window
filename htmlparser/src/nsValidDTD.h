
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/**
 * MODULE NOTES:
 * @update  gess 4/8/98
 * 
 *         
 */

#ifndef __NS_VALID_DTD
#define __NS_VALID_DTD

#include "nsIDTD.h"
#include "nsISupports.h"
#include "nsHTMLTokens.h"
#include "nshtmlpars.h"
#include "nsVoidArray.h"
#include "nsDeque.h"

#define NS_VALID_DTD_IID      \
  {0xa39c6bfe, 0x15f0,    0x11d2, \
  {0x80, 0x41, 0x0, 0x10, 0x4b, 0x98, 0x3f, 0xd4}}


class nsIHTMLContentSink;
class nsIDTDDebug;
class nsIParserNode;
class CITokenHandler;
class nsParser;



class CValidDTD : public nsIDTD {
            
  public:

    NS_DECL_ISUPPORTS


    /**
     *  
     *  
     *  @update  gess 4/9/98
     *  @param   
     *  @return  
     */
    CValidDTD();

    /**
     *  
     *  
     *  @update  gess 4/9/98
     *  @param   
     *  @return  
     */
    virtual ~CValidDTD();

    /**
     * This method is called to determine if the given DTD can parse
     * a document in a given source-type. 
     * NOTE: Parsing always assumes that the end result will involve
     *       storing the result in the main content model.
     * @update	gess6/24/98
     * @param   
     * @return  TRUE if this DTD can satisfy the request; FALSE otherwise.
     */
    virtual PRBool CanParse(nsString& aContentType, PRInt32 aVersion);


    /**
     * This method is called to determine if the given DTD can perform
     * a document conversion from a given source-type to a given target-type.
     * NOTE: conversion always assumes an input stream and an outputstream.
     * @update	gess6/24/98
     * @param   
     * @return  TRUE if this DTD can satisfy the request; FALSE otherwise.
     */
    virtual PRBool CanConvert(nsString& aSourceType, nsString& aTargetType, PRInt32 aVersion);

    /**
     * 
     * @update	gess7/7/98
     * @param 
     * @return
     */
    virtual eAutoDetectResult AutoDetectContentType(nsString& aBuffer,nsString& aType);

    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    virtual PRInt32 WillBuildModel(const char* aFilename=0);

    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    virtual PRInt32 DidBuildModel(PRInt32 anErrorCode);

    /**
     *  
     *  @update  gess 3/25/98
     *  @param   aToken -- token object to be put into content model
     *  @return  0 if all is well; non-zero is an error
     */
    virtual PRInt32 HandleToken(CToken* aToken);

    /**
     * 
     *  
     *  @update  gess 3/25/98
     *  @param   
     *  @return 
     */
    virtual void SetParser(nsIParser* aParser);

    /**
     *  Cause the tokenizer to consume the next token, and 
     *  return an error result.
     *  
     *  @update  gess 3/25/98
     *  @param   anError -- ref to error code
     *  @return  new token or null
     */
    virtual PRInt32 ConsumeToken(CToken*& aToken);


    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    virtual void WillResumeParse(void);

    /**
     * 
     * @update	gess5/18/98
     * @param 
     * @return
     */
    virtual void WillInterruptParse(void);

   /**
     * Select given content sink into parser for parser output
     * @update	gess5/11/98
     * @param   aSink is the new sink to be used by parser
     * @return  old sink, or NULL
     */
    virtual nsIContentSink* SetContentSink(nsIContentSink* aSink);

    /**
     * 
     * @update	jevering6/23/98
     * @param 
     * @return
     */
	  virtual void SetDTDDebug(nsIDTDDebug * aDTDDebug);

    /**
     *  This method is called to determine whether or not a tag
     *  of one type can contain a tag of another type.
     *  
     *  @update  gess 3/25/98
     *  @param   aParent -- int tag of parent container
     *  @param   aChild -- int tag of child container
     *  @return  PR_TRUE if parent can contain child
     */
    virtual PRBool CanContain(PRInt32 aParent,PRInt32 aChild);
	
protected:

    
    nsParser*           mParser;
    nsIHTMLContentSink* mSink;
    char*               mFilename;
};

extern NS_HTMLPARS nsresult NS_NewValid_DTD(nsIDTD** aInstancePtrResult);

#endif 



