#!/bin/bash

here=`pwd`
endtext='If not, see <http:\/\/www.gnu.org\/licenses\/>.'
set -x

header_strip()
{
    i=$1
    echo $i
    l=$(grep -n "$endtext" $i | tail -1 | cut -d ":" -f 1)
    l=$(($l+1))
    echo $l
    tail -n +$l $i | sed '/./,$!d' > $i.tmp;
    mv $i.tmp $i
}

gen_authors_prepend_header()
{
    i=$1
    echo $i;
    hash=$(echo "$i" | md5sum | cut -d " " -f1)
    temp="/tmp/dccl_authors.${hash}.tmp"
    echo $temp
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
    
    cat <<EOF > ${temp}
// Copyright ${years}:
EOF
    
    if (( $end_year >= 2013  )); then
        echo "//   GobySoft, LLC (2013-)" >> ${temp}
    fi
    if (( $start_year <= 2014 )); then
       echo "//   Massachusetts Institute of Technology (2007-2014)" >>  ${temp}
    fi

    cat <<EOF >> ${temp}
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
        echo "//   $author$email"  >> ${temp}
    done

    cat ${temp} $here/../src/share/header_lib.txt $i > $i.tmp; mv $i.tmp $i;
    rm ${temp}
}


pushd ../src
export -f gen_authors_prepend_header header_strip
export here endtext
grep -lr "$endtext" | egrep "\.cpp$|\.h$|\.proto$" | parallel header_strip
find -type f -regex ".*\.h$\|.*\.cpp$\|.*\.proto$" | parallel gen_authors_prepend_header

popd


