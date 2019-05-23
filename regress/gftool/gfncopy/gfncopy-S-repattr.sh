#!/bin/sh

. ./regress.conf

$regress/bin/am_I_gfarmadm
if test $? -ne 0; then
    exit $exit_unsupported
fi

dir=$gftmp
pat="g0:6, g1:1, g2:3"
g0s=g0:6
g1s=g1:1
g2s=g2:3

setup() {
    gfmkdir ${dir}
    gfncopy -r ${dir}

    for g in g0 g1 g2
    do
	gfhost -c -a dummy-arch -p 12345 dummy-$g
	gfhostgroup -s dummy-$g $g
    done
}

cleanup() {
    gfrmdir ${dir}

    for g in g0 g1 g2
    do
	gfhost -d dummy-$g
    done
}

trap 'cleanup; exit $exit_trap' $trap_sigs

setup

# specified fsngroup should exist
if gfncopy -S "XaXbXcShouldNotExist:1" ${dir} 2>&1 |
    grep 'no such group' >/dev/null
then
    :
else
    cleanup
    exit $exit_fail
fi

# each fsngroup is allowed to appear at most once
if gfncopy -S "g0:1,g0:1" ${dir} 2>&1 |
    grep 'invalid argument' >/dev/null
then
    :
else
    cleanup
    exit $exit_fail
fi

gfncopy -S "${pat}" ${dir}
if [ $? -ne 0 ]; then
    cleanup
    exit $exit_fail
fi

pat1=`gfncopy ${dir} | sed -e 's:,: :g'`
if [ $? -ne 0 -o "X${pat1}" = "X" ]; then
    cleanup
    exit $exit_fail
fi

gotcha=0
for i in ${pat1}; do
    if [ \
	"X${i}" = "X${g0s}" -o \
	"X${i}" = "X${g1s}" -o \
	"X${i}" = "X${g2s}" ]; then
	gotcha=`expr ${gotcha} + 1`
    fi
done

if [ ${gotcha} -ne 3 ]; then
    cleanup
    exit $exit_fail
fi

gfncopy -r ${dir}
st=$?

cleanup

if [ ${st} -eq 0 ]; then
    exit $exit_pass
else
    exit $exit_fail
fi

