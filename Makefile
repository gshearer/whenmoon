# ================================================= 
#
#  ▄▄▌ ▐ ▄▌ ▄ .▄▄▄▄ . ▐ ▄ • ▌ ▄ ·.              ▐ ▄ 
#  ██· █▌▐███▪▐█▀▄.▀·•█▌▐█·██ ▐███▪▪     ▪     •█▌▐█
#  ██▪▐█▐▐▌██▀▐█▐▀▀▪▄▐█▐▐▌▐█ ▌▐▌▐█· ▄█▀▄  ▄█▀▄ ▐█▐▐▌
#  ▐█▌██▐█▌██▌▐▀▐█▄▄▌██▐█▌██ ██▌▐█▌▐█▌.▐▌▐█▌.▐▌██▐█▌
#   ▀▀▀▀ ▀▪▀▀▀ · ▀▀▀ ▀▀ █▪▀▀  █▪▀▀▀ ▀█▄▀▪ ▀█▄▀▪▀▀ █▪
#
#          Crytpocurrency Trading Bot
#  Written by George Shearer (george@shearer.tech)
#     Copyright (C) 2021 by George Shearer
#
# =================================================

CC = gcc
LD = $(CC)
LIBS = -lcurl -ljson-c -lpthread -lpq -lcrypto -lssl -lm tinyexpr/tinyexpr.o tulipindicators/libindicators.a
CFLAGS = -O3 -pipe
LDFLAGS = -s

CCVERSION = $(CC) --version | head -1
OSNAME = uname -sr
HOSTNAME = hostname
USERNAME = whoami
DATE = /bin/date

#############################################################################

OBJS =  \
	account.o     \
	bt.o          \
	candle.o      \
        config.o      \
	cm.o          \
	coinpp.o      \
	cpanic.o      \
        curl.o        \
        db.o          \
	exch.o        \
	exch_cbp.o    \
	init.o        \
        irc.o         \
        ircu.o        \
	main.o        \
	market.o      \
	mem.o         \
        misc.o        \
	order.o       \
        sock.o        \
	strat.o       \
	strat_dd.o    \
	strat_score.o \
	strat_pg.o    \
	strat_rand.o  \
	tulip.o       \
	wm.o

all:	whenmoon

whenmoon: $(OBJS)
	$(LD) $(LDFLAGS) -o whenmoon $(OBJS) $(LIBS)

#############################################################################

account.o: common.h exch.h market.h misc.h account.h account.c

bt.o: common.h exch.h init.h market.h misc.h strat.h bt.h bt.c

candle.o: common.h colors.h init.h misc.h candle.h candle.c

cm.o: common.h curl.h exch.h init.h main.h market.h misc.h cm.h cm.c

config.o: common.h init.h irc.h misc.h config.h config.c

cpanic.o: common.h curl.h init.h irc.h misc.h cpanic.h cpanic.c

coinpp.o: common.h curl.h init.h misc.h coinpp.h coinpp.c

curl.o: common.h init.h irc.h mem.h misc.h curl.h curl.c

db.o: common.h init.h misc.h db.h db.c

exch.o: common.h candle.h init.h irc.h market.h misc.h order.h exch.h exch.c

exch_cbp.o: common.h account.h curl.h exch.h init.h mem.h misc.h exch_cbp.h exch_cbp.c

init.o: common.h coinpp.h curl.h db.h exch.h irc.h market.h misc.h sock.h strat.h version.h init.h init.c

irc.o: common.h init.h ircu.h misc.h sock.h irc.h irc.c

ircu.o: common.h account.h cpanic.h curl.h coinpp.h exch.h init.h irc.h market.h misc.h sock.h ircu.h ircu.c

main.o: common.h bt.h cm.h init.h misc.h main.h main.c

market.o: common.h colors.h account.h candle.h exch.h init.h irc.h mem.h misc.h strat.h tulip.h market.h market.c

mem.o: common.h mem.h mem.c

misc.o: common.h colors.h misc.h misc.c

order.o: common.h irc.h misc.h market.h order.h order.c

sock.o: common.h init.h irc.h misc.h sock.h sock.c

strat.o: common.h misc.h strat.h strat.c

strat_dd.o: common.h init.h mem.h misc.h strat_flags.h strat_dd.h strat_dd.c

strat_pg.o: common.h init.h mem.h misc.h strat_flags.h strat_pg.h strat_pg.c

strat_rand.o: common.h misc.h strat_rand.h strat_rand.c

strat_score.o: common.h init.h market.h mem.h misc.h strat_flags.h strat_score.h strat_score.c

tulip.o: common.h init.h tulipindicators/indicators.h tulip.h tulip.c

wm.o: common.h wm.h wm.c

#############################################################################

version.h:	dummy
	@rm -f version.h
	@echo "/* this file is automatically generated */" >version.h
	@echo \#define BUILDDATE \"`${DATE}`\" >>version.h
	@echo \#define BUILDHOST \"`${HOSTNAME}`\" >>version.h
	@echo \#define BUILDUSER \"`${USERNAME}`\" >>version.h
	@echo \#define BUILDOS \"`${OSNAME}`\" >>version.h
	@echo \#define BUILDCC \"`${CCVERSION}`\" >>version.h
	@echo \#define VERSION \"`${DATE} '+%Y%m%d'`\" >>version.h
	@expr `cat BUILDNUM` + 1 >BUILDNUM
	@echo \#define BUILDNUM \"`cat BUILDNUM`\" >>version.h

dummy:

#############################################################################

clean:
	rm -f *.o a.out core whenmoon