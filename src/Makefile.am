AM_YFLAGS = -d
AM_LFLAGS = --header-file="../$(<:.l=.h)"
AM_CPPFLAGS = -Wall -I../include `../svnver.sh`
LIBS=
BUILT_SOURCES = confpar.c confpar.h confsym.c confsym.h
CLEANFILES = confpar.c confpar.h confsym.c confsym.h
confsym.h: confsym.c

moddir = $(libdir)/@PACKAGE@

if ENABLE_IPV6
AM_CPPFLAGS += -DIPV6
endif

if ENABLE_TLS
AM_CPPFLAGS += -DTLS
endif

bin_PROGRAMS = boskop-ng 
mod_PROGRAMS = alignment.so ping.so join.so weil.so invite.so auth.so blinddate.so talk.so decide.so tourette.so quit.so nick.so hello.so version.so qauth.so horde.so jukebox.so love.so sercz.so scheme.so quote.so reminder.so eliza.so plugin.so nickserv.so achtball.so fortune.so help.so xyzzy.so yx.so fut.so
if ENABLE_XML
mod_PROGRAMS += rss.so search.so weather.so
endif
if ENABLE_LISP
mod_PROGRAMS += lisp.so
endif

boskop_ng_SOURCES = confpar.y confsym.l sha1.c hmac.c irc.c io.c buffer.c util.c plugins.c main.c dns.c http.c rrand.c sock.c
boskop_ng_CFLAGS=-DMODDIR="\"${moddir}\""
if WITH_RDYNAMIC
boskop_ng_LDFLAGS = -rdynamic -Wl,-E
endif
boskop_ng_LDADD = -lm

if ENABLE_XML
boskop_ng_SOURCES += xml.c
boskop_ng_LDADD += -lxml2
endif

if ENABLE_TLS
boskop_ng_LDADD += -lgnutls
endif

if WITH_LIBDL
boskop_ng_LDADD += -ldl
endif

if WITH_LIBSOCKET
boskop_ng_LDADD += -lsocket
endif

if WITH_LIBNSL
boskop_ng_LDADD += -lnsl
endif

nickserv_so_SOURCES = nickserv.c
nickserv_so_CFLAGS = -fPIC
nickserv_so_LDFLAGS = -shared

quote_so_SOURCES = quote.c
quote_so_CFLAGS = -fPIC
quote_so_LDFLAGS = -shared

scheme_so_SOURCES = scheme.c
scheme_so_CFLAGS = -fPIC
scheme_so_LDFLAGS = -shared

sercz_so_SOURCES = sercz.c
sercz_so_CFLAGS = -fPIC
sercz_so_LDFLAGS = -shared

alignment_so_SOURCES = alignment.c
alignment_so_CFLAGS = -fPIC
alignment_so_LDFLAGS = -shared

ping_so_SOURCES = ping.c
ping_so_CFLAGS = -fPIC
ping_so_LDFLAGS = -shared

join_so_SOURCES = join.c
join_so_CFLAGS = -fPIC
join_so_LDFLAGS = -shared

weil_so_SOURCES = weil.c
weil_so_CFLAGS = -fPIC
weil_so_LDFLAGS = -shared

invite_so_SOURCES = invite.c
invite_so_CFLAGS = -fPIC
invite_so_LDFLAGS = -shared

auth_so_SOURCES = auth.c md5.c mycrypt.c
auth_so_CFLAGS = -fPIC
auth_so_LDFLAGS = -shared

blinddate_so_SOURCES = blinddate.c
blinddate_so_CFLAGS = -fPIC
blinddate_so_LDFLAGS = -shared

talk_so_SOURCES = talk.c
talk_so_CFLAGS = -fPIC
talk_so_LDFLAGS = -shared

decide_so_SOURCES = decide.c
decide_so_CFLAGS = -fPIC
decide_so_LDFLAGS = -shared

tourette_so_SOURCES = tourette.c
tourette_so_CFLAGS = -fPIC
tourette_so_LDFLAGS = -shared

quit_so_SOURCES = quit.c
quit_so_CFLAGS = -fPIC
quit_so_LDFLAGS = -shared

nick_so_SOURCES = nick.c
nick_so_CFLAGS = -fPIC
nick_so_LDFLAGS = -shared

hello_so_SOURCES = hello.c
hello_so_CFLAGS = -fPIC
hello_so_LDFLAGS = -shared

version_so_SOURCES = version.c
version_so_CFLAGS = -fPIC
version_so_LDFLAGS = -shared

qauth_so_SOURCES = qauth.c md5.c
qauth_so_CFLAGS = -fPIC
qauth_so_LDFLAGS = -shared

horde_so_SOURCES = horde.c
horde_so_CFLAGS = -fPIC
horde_so_LDFLAGS = -shared

jukebox_so_SOURCES = jukebox.c
jukebox_so_CFLAGS = -fPIC
jukebox_so_LDFLAGS = -shared

lisp_so_SOURCE = lisp.c
lisp_so_CFLAGS = -fPIC -I../contrib/libisp/include
lisp_so_LDFLAGS = -shared
lisp_so_LDADD = ../contrib/libisp/bin/libisp.a -lpthread
../contrib/libisp/bin/libisp.a:
	$(MAKE) -C ../contrib/libisp/ bin/libisp.a

love_so_SOURCES = love.c
love_so_CFLAGS = -fPIC
love_so_LDFLAGS = -shared

rss_so_SOURCES = rss.c
rss_so_CFLAGS = -fPIC
rss_so_LDFLAGS = -shared

search_so_SOURCES = search.c
search_so_CFLAGS = -fPIC
search_so_LDFLAGS = -shared

reminder_so_SOURCES = reminder.c
reminder_so_CFLAGS = -fPIC
reminder_so_LDFLAGS = -shared

eliza_so_SOURCES = eliza.c
eliza_so_CFLAGS = -fPIC
eliza_so_LDFLAGS = -shared

weather_so_SOURCES = weather.c
weather_so_CFLAGS = -fPIC
weather_so_LDFLAGS = -shared

plugin_so_SOURCES = plugin.c
plugin_so_CFLAGS = -fPIC
plugin_so_LDFLAGS = -shared

achtball_so_SOURCES = achtball.c
achtball_so_CFLAGS = -fPIC
achtball_so_LDFLAGS = -shared

fortune_so_SOURCES = fortune.c
fortune_so_CFLAGS = -fPIC
fortune_so_LDFLAGS = -shared

help_so_SOURCES = help.c
help_so_CFLAGS = -fPIC
help_so_LDFLAGS = -shared

xyzzy_so_SOURCES = xyzzy.c
xyzzy_so_CFLAGS = -fPIC
xyzzy_so_LDFLAGS = -shared

yx_so_SOURCES = yx.c
yx_so_CFLAGS = -fPIC
yx_so_LDFLAGS = -shared

fut_so_SOURCES = fut.c
fut_so_CFLAGS = -fPIC
fut_so_LDFLAGS = -shared

