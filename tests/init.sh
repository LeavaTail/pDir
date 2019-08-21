#/bin/sh
# make sure pdir simply. FIXME

## Initialize
rm -rf dir
mkdir dir
touch dir/file1
echo "FILE" > dir/file2
echo "IGNORE" > dir/.file3

## Check routine
/.pdir
if [ $? -gt 0 ]; then
	return 1;
fi

./pdir -a
if [ $? -gt 0 ]; then
	return 2;
fi

./pdir -A
if [ $? -gt 0 ]; then
	return 3;
fi

./pdir -l
if [ $? -gt 0 ]; then
	return 4;
fi

## Clean up
rm -rf dir
