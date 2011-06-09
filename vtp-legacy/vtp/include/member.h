/*
 * Copyright (c) 2003 MIT, Haystack Observatory
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction,including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

/*
 * $Source: /usr/local/cvsroot/mit/vtp/include/member.h,v $
 * $Revision: 1.1 $
 * $Date: 2003/09/20 16:48:40 $
 * $State: Exp $
 * $Author: davidlapsley $
 *
 * $Log: member.h,v $
 * Revision 1.1  2003/09/20 16:48:40  davidlapsley
 * *** empty log message ***
 *
 * Revision 1.1  2003/09/19 21:52:19  dlapsley
 * *** empty log message ***
 *
 * Revision 1.2  2003/09/10 06:19:26  davidlapsley
 * *** empty log message ***
 *
 * Revision 1.1  2003/09/10 00:32:46  davidlapsley
 * *** empty log message ***
 *
 * Revision 1.6  2003/09/10 00:21:16  davidlapsley
 * Modified build system.
 *
 * Revision 1.5  2003/09/09 22:30:15  davidlapsley
 * - Added XML parser to the main branch. Converted configuration from
 * command line driven to configuration file driven (too many parameters
 * for the command line).
 * - Integrated server with RTP/RTCP logic.
 *
 * Revision 1.4  2003/09/09 18:01:37  davidlapsley
 * Initial integration of rtp- and rtcp-related modules with client-related
 * modules.
 *
 * Revision 1.3  2003/09/09 05:57:41  davidlapsley
 * - Removed members and senders from rtcp_member_t (this logic already
 * maintained in OnReceive).
 * - Completed rtcp_send_report(), rtcp_send_bye() functions.
 * - Made buffer sizes in rtcp_session_t an initialization parameter.
 * - client.c: integrating rtcp module
 *
 * Revision 1.2  2003/09/08 21:54:27  davidlapsley
 * - Added random module for generating random 32 bit integers.
 * - Replaced u_intXX style typedefs with standard u_intXX_t types.
 * - Added variables to rtcp_t session member.
 * - Added code file template: template.h which includes the MIT license
 * header.
 *
 * Revision 1.1  2003/09/04 22:07:52  davidlapsley
 * Added new module: members to encapsulate the member tree. The member
 * tree consists of a struct: members_t that encapsulates a glib balances
 * tree. This tree is used to store a list of session members and their
 * state.
 *
 *
 *
 *
 */

#include <glib.h>
#include <time.h>
#include <rtp.h>

#ifndef _MEMBER_H_
#define _MEMBER_H_

typedef struct {
  u_int32_t sender;
  u_int32_t ssrc;
  rtcp_rr_t rr;
  FILE* out;
  source src;
} member_t;

void member_init(member_t* m, u_int32_t ssrc);
void member_destroy(member_t* m);
void member_close(member_t* m);
int member_write(member_t* m, u_int8_t* buf, u_int32_t sz);


#endif


