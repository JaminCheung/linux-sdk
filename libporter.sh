#!/bin/bash
#
#  Copyright (C) 2017, Wang Qiuwei <qiuwei.wang@ingenic.com / panddio@163.com>
#
#  Ingenic sdk Project
#
#  This program is free software; you can redistribute it and/or modify it
#  under  the terms of the GNU General  Public License as published by the
#  Free Software Foundation;  either version 2 of the License, or (at your
#  option) any later version.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  675 Mass Ave, Cambridge, MA 02139, USA.
#

argstr=$*
[[ $argstr =~ "-L" ]] || exit 0
str1=${argstr#*-Bdynamic }

mkdir -p out/lib

while(test -n "$str1")
do
    str1=${str1#*L*/lib/}
    str2=${str1%%-L*}
    libdir=${str1%% -l*}

 #   echo "str1=$str1"
 #   echo "str2=$str2"
 #   echo $str2 | awk -F " -l" '{for(i=1;i<=NF;i++)print $i}'

    nf=$(echo $str2 | awk -F " -l" '{print NF}')

    for((n=2;n<=$nf;n++));
    do
        libname=$(echo $str2 | awk -F " -l" '{for(i='$n';i=='$n';i++)print $i}')
        cp -af lib/$libdir/*$libname* out/lib
    done

    [[ $str1 =~ "-L" ]] || exit 0
done

exit 0
