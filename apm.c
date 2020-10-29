/* DtApm: apm.c
	This is derived from apm.c from OpenBSD. It has been transmogrified
	into a Motif graphical applet, although the underlying code is much
	the same. There is a lot of dead code in here; that is intentional.
*/

/*
 *  Copyright (c) 2020 vmlinuz719
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*	$OpenBSD: apm.c,v 1.37 2020/09/23 05:50:26 jca Exp $	*/

/*
 *  Copyright (c) 1996 John T. Kohl
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <machine/apmvar.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/LabelG.h>
#include <Xm/MwmUtil.h>
#include "pathnames.h"
#include "apm-proto.h"

#define FALSE 0
#define TRUE 1

extern char *__progname;

static int		do_zzz(int, enum apm_action);
static int		open_socket(const char *);
static int		send_command(int, struct apm_command *,
			    struct apm_reply *);
static __dead void	usage(void);
static __dead void	zzusage(void);

static __dead void
usage(void)
{
	fprintf(stderr,"usage: %s [-AabHLlmPSvZz] [-f sockname]\n",
	    __progname);
	exit(1);
}

static __dead void
zzusage(void)
{
	fprintf(stderr,"usage: %s [-SZz] [-f sockname]\n",
	    __progname);
	exit(1);
}

static int
send_command(int fd, struct apm_command *cmd, struct apm_reply *reply)
{
	/* send a command to the apm daemon */
	cmd->vno = APMD_VNO;

	if (send(fd, cmd, sizeof(*cmd), 0) == sizeof(*cmd)) {
		if (recv(fd, reply, sizeof(*reply), 0) != sizeof(*reply)) {
			warn("invalid reply from APM daemon");
			return (1);
		}
	} else {
		warn("invalid send to APM daemon");
		return (1);
	}
	return (0);
}

static int
do_zzz(int fd, enum apm_action action)
{
	struct apm_command command;
	struct apm_reply reply;
	char *msg;

	switch (action) {
	case NONE:
	case SUSPEND:
		command.action = SUSPEND;
		msg = "Suspending system";
		break;
	case STANDBY:
		command.action = STANDBY;
		msg = "System standing by";
		break;
	case HIBERNATE:
		command.action = HIBERNATE;
		msg = "Hibernating system";
		break;
	default:
		zzusage();
	}

	printf("%s...\n", msg);
	exit(send_command(fd, &command, &reply));
}

static int
open_socket(const char *sockname)
{
	int sock, errr;
	struct sockaddr_un s_un;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
		err(1, "cannot create local socket");

	s_un.sun_family = AF_UNIX;
	strlcpy(s_un.sun_path, sockname, sizeof(s_un.sun_path));
	if (connect(sock, (struct sockaddr *)&s_un, sizeof(s_un)) == -1) {
		errr = errno;
		close(sock);
		errno = errr;
		sock = -1;
	}
	return (sock);
}

struct apm_reply getStatus() {
	const char *sockname = _PATH_APM_SOCKET;
	int ch, fd, rval;
	struct apm_command command;
	struct apm_reply reply;
	int cpuspeed_mib[] = { CTL_HW, HW_CPUSPEED }, cpuspeed;
	size_t cpuspeed_sz = sizeof(cpuspeed);

	if (sysctl(cpuspeed_mib, 2, &cpuspeed, &cpuspeed_sz, NULL, 0) == -1)
		err(1, "sysctl hw.cpuspeed");
	
	fd = open_socket(sockname);
	if (fd == -1)
		errx(-1, "fatal socket error");
	
	bzero(&reply, sizeof reply);
	reply.batterystate.battery_state = APM_BATT_UNKNOWN;
	reply.batterystate.ac_state = APM_AC_UNKNOWN;
	reply.perfmode = PERF_MANUAL;
	reply.cpuspeed = cpuspeed;
	
	
	command.action = GETSTATUS;
	if (fd != -1 &&
		(rval = send_command(fd, &command, &reply)) != 0)
		errx(rval, "cannot get reply from APM daemon");
	
	close(fd);
	return reply;
}

XtAppContext app;
Widget text;
Widget minutes;
void textToggled(Widget widget, XtPointer client_data, XtPointer call_data) {
	XmToggleButtonCallbackStruct *state =
		(XmToggleButtonCallbackStruct *) call_data;
	state->set ? XmToggleButtonSetState(*((Widget *) client_data),
		False, False) :
		XmToggleButtonSetState(*((Widget *) client_data),
		True, False);
}
void sigalrm(XtPointer client_data, XtIntervalId id) {
	Widget *widget = (Widget *) client_data;
	struct apm_reply reply = getStatus();
	int result = reply.batterystate.ac_state;
	XmToggleButtonSetState(*widget, result == 1 ? True : False, False);
	XtAppAddTimeOut(app, 1000, (XtTimerCallbackProc) sigalrm,
	client_data);
}
void sigalrm2(XtPointer client_data, XtIntervalId id) {
	Widget *widget = (Widget *) client_data;
	struct apm_reply reply = getStatus();
	int result = reply.batterystate.minutes_left;
	static char snum[24];
	sprintf(snum, "%u (%u%%)", result,
		reply.batterystate.battery_life);
	if (reply.batterystate.ac_state == 1) {
		reply.batterystate.battery_state == APM_BATT_CHARGING ?
		sprintf(snum, "CHG (%u%%)",
			reply.batterystate.battery_life) :
		sprintf(snum, "AC (%u%%)",
			reply.batterystate.battery_life);
	} else {
		sprintf(snum, "%u (%u%%)", result,
			reply.batterystate.battery_life);
	}
	XmTextSetString(*widget, snum);
	XtAppAddTimeOut(app, 1000, (XtTimerCallbackProc) sigalrm2,
	client_data);
}

int
main(int argc, char *argv[])
{
	struct apm_reply init_reply = getStatus();
	
	Widget toplevel;
	XtSetLanguageProc(NULL, NULL, NULL);
	
	toplevel = XtVaAppInitialize(&app, "DtApm",
		NULL, 0, &argc, argv, NULL,
		XmNtitle, "Battery Status",
		XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
		XmNmwmDecorations, MWM_DECOR_RESIZEH,
		NULL);
	
	Widget frame = XtVaCreateManagedWidget("frame",
		xmRowColumnWidgetClass, toplevel,
		NULL);
	
	text = XtVaCreateManagedWidget("AC adapter connected",
		xmToggleButtonWidgetClass, frame,
		NULL);
	
	Widget minutesContainer = XtVaCreateManagedWidget
		("Text field and label",
		xmRowColumnWidgetClass, frame,
		XmNorientation, XmHORIZONTAL,
		NULL);
	
	Widget minutesLabel = XtVaCreateManagedWidget("Remaining:",
		xmLabelGadgetClass, minutesContainer,
		NULL);
	
	minutes = XtVaCreateManagedWidget("Text field",
		xmTextWidgetClass, minutesContainer,
		XmNeditable, False,
		XmNcursorPositionVisible, False,
		XmNcolumns, 10,
		NULL);
	
	XtAddCallback(text, XmNvalueChangedCallback, textToggled,
		(XtPointer) &text);
	XmToggleButtonSetState(text, False, False);
	
	XtAppAddTimeOut(app, 1000, (XtTimerCallbackProc) sigalrm,
		(XtPointer) &text);
	XtAppAddTimeOut(app, 000, (XtTimerCallbackProc) sigalrm2,
		(XtPointer) &minutes);
	XtRealizeWidget(toplevel);
	XtAppMainLoop(app);
	
	return (0);
}
