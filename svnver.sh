#!/bin/sh
LANG=C
SVNVER=$(which svnversion)
if [ "x$SVNVER" != "x" ]; then
   SVNRES=$($SVNVER -n)
   if [ "x$SVNRES" != "xexported" ]; then
      echo -n -DSVN_REV=\"$SVNRES\"
   fi
fi

