/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

#ifndef MIME_H
#define MIME_H

#include "ntypes.h"
#if 0
#include "mimeenc.h"
#endif

XP_BEGIN_PROTOS

extern PUBLIC const char *FE_UsersOrganization(void);


/* Returns the appropriate contents of a From: field of a mail message
   originating from the current user.  This calls FE_UsersFullName()
   and FE_UsersMailAddress() and correctly munges the values, using
   MSG_MakeFullAddress()

   A new string is returned, which you must free when you're done with it.
   */
extern PUBLIC char *MIME_MakeFromField (PRInt16 csid);

/* Convert a block of text to the MIME quoted-printable encoding.
   Returns a new string and its size, or NULL if it couldn't allocate.
 */
extern PUBLIC void MIME_EncodeQuotedPrintableString(const unsigned char *input,
													PRUint32 input_size,
													unsigned char **output,
													PRUint32 *output_size);

/* Convert a block of text to the MIME base64 encoding.
   Returns a new string and its size, or NULL if it couldn't allocate.
 */
extern PUBLIC void MIME_EncodeBase64String(const unsigned char *input,
										   PRUint32 input_size,
										   unsigned char **output,
										   PRUint32 *output_size);

/* build a mailto: url address given a to field
 *
 * returns a malloc'd string
 */
extern char *
MIME_BuildMailtoURLAddress(const char * to);

/* build a news: url address to post to, given a partial news-post
 * URL and the newsgroups line
 *
 * returns a malloc'd string
 */
extern char *
MIME_BuildNewspostURLAddress(const char *partial_newspost_url,
                             const char *newsgroups);

/* If the argument is true, we use quoted-printable encoding on mail and
   news messages which have 8bit characters in them.  Otherwise, we assume
   that the mail and news transport will not strip the eighth bit (a bad
   assumption.) */
extern void MIME_ConformToStandard (PRBool conform_p);

/* for right now, only the XFE uses this */
extern PRBool Get_MIME_ConformToStandard(); 

/* Generate headers for a form post to a mailto: URL.
   This lets the URL specify additional headers, but is careful to
   ignore headers which would be dangerous.  It may modify the URL
   (because of CC) so a new URL to actually post to is returned.
 */
extern int MIME_GenerateMailtoFormPostHeaders (const char *old_post_url,
											   const char *referer,
											   char **new_post_url_return,
											   char **headers_return);

#define	IMAP_EXTERNAL_CONTENT_HEADER "X-Mozilla-IMAP-Part"

XP_END_PROTOS

#endif /* mime.h */
