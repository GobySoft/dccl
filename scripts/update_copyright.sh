#!/bin/bash

here=`pwd`

header_strip()
{
    endtext='If not, see <http:\/\/www.gnu.org\/licenses\/>.'
    for i in `grep -lr "$endtext" | egrep "\.cpp$|\.h$"`
    do 
        echo $i
        l=$(grep -n "$endtext" $i | tail -1 | cut -d ":" -f 1)
        l=$(($l+1))
        echo $l
        tail -n +$l $i | sed '/./,$!d' > $i.tmp;
        mv $i.tmp $i
    done
}

gen_authors()
{
    i=$1
    echo $i;
    mapfile -t authors < <(git blame --line-porcelain $i | grep "^author " | sort | uniq -c | sort -nr | sed 's/^ *//' | cut -d " " -f 3-)
    #    echo ${authors[@]}
    start_year=$(git log --follow --date=format:%Y --format=format:%ad $i | tail -n 1)
    end_year=$(git log --follow -n 1 --date=format:%Y --format=format:%ad $i)
    #    echo ${start_year}-${end_year}    

    if [[ "${start_year}" == "${end_year}" ]]; then
        years="${start_year}"
    else
        years="${start_year}-${end_year}"
    fi
    
    cat <<EOF > /tmp/dccl_authors.tmp
// Copyright ${years}:
EOF
    
    if (( $end_year >= 2013  )); then
        echo "//   GobySoft, LLC (2013-)" >>  /tmp/dccl_authors.tmp
    fi
    if (( $start_year <= 2014 )); then
       echo "//   Massachusetts Institute of Technology (2007-2014)" >>  /tmp/dccl_authors.tmp
    fi

    cat <<EOF >> /tmp/dccl_authors.tmp
//   Community contributors (see AUTHORS file)
// File authors:
EOF
    
    for author in "${authors[@]}"
    do
        # use latest email for author name here
        email=$(git log --use-mailmap --author "$author" -n 1 --format=format:%aE)
        if [ ! -z "$email" ]; then
            email=" <${email}>"
        fi
        echo "//   $author$email"  >> /tmp/dccl_authors.tmp
    done
}

prepend_header()
{
    i=$1
    cat /tmp/dccl_authors.tmp $here/../src/share/header_lib.txt $i > $i.tmp; mv $i.tmp $i;
}

set -x
pushd ../src
header_strip
export -f gen_authors prepend_header
export here
find -regex ".*\.h$\|.*\.cpp$" | parallel gen_authors
find -regex ".*\.h$\|.*\.cpp$" | parallel prepend_header

popd

rm /tmp/dccl_authors.tmp

