## Process this file with automake to produce Makefile.in

bin_PROGRAMS = sipxsaa

## disable tests, nothing appears to work in there anymore
SUBDIRS = .

EXTRA_DIST =

sipxsaa_LDADD = \
	@SIPXCOMMSERVER_LIBS@ \
        -lmongoclient \
        -lboost_system-mt	

sipxsaa_CXXFLAGS = \
        -I$(top_builddir)/config \
        -DSIPX_LOGDIR=\"@SIPX_LOGDIR@\" \
        -DSIPX_RUNDIR=\"@SIPX_RUNDIR@\" \
        -DSIPX_TMPDIR=\"@SIPX_TMPDIR@\" \
        -DSIPX_CONFDIR=\"@SIPX_CONFDIR@\"

sipxsaa_SOURCES =  \
	main.cpp \
	AppAgentSubscribePolicy.cpp \
	AppAgentTask.cpp \
	Appearance.cpp \
	AppearanceGroup.cpp \
	AppearanceGroupFileReader.cpp \
	AppearanceGroupSet.cpp \
	ResourceSubscriptionReceiver.cpp \
	ResourceNotifyReceiver.cpp \
	ResourceListMsg.cpp \
	AppearanceAgent.cpp

noinst_HEADERS = \
	AppAgentSubscribePolicy.h \
	AppAgentTask.h \
	Appearance.h \
	AppearanceGroup.h \
	AppearanceGroupFileReader.h \
	AppearanceGroupSet.h \
	AppearanceAgent.h \
	ResourceSubscriptionReceiver.h \
	ResourceNotifyReceiver.h \
	ResourceListMsg.h \
	main.h

install-exec-hook :
