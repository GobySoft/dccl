#!/bin/bash

here=`pwd`

header_strip()
{
    endtext='If not, see <http:\/\/www.gnu.org\/licenses\/>.'
    for i in `grep -lr "$endtext"`
    do 
        echo $i
        echo $i | grep -q "header_lib"  && continue;
        grep -n "$endtext" $i | tail -1 | cut -d ":" -f 1
        l=$(grep -n "$endtext" $i | tail -1 | cut -d ":" -f 1)
        l=$(($l+1))
        echo $l
        tail -n +$l $i | sed '/./,$!d' > $i.tmp;
        mv $i.tmp $i
    done
}

pushd ..
header_strip
for i in `find -regex ".*\.h$\|.*\.cpp$"`; 
do 
    if echo $i | grep -q pb_plugin; then
        cat $here/../share/header_lib_gs.txt $i > $i.tmp 
    elif echo $i | grep -q b64; then
        continue;
    else
        cat $here/../share/header_lib.txt $i > $i.tmp 
    fi
    mv $i.tmp $i;
done
popd

