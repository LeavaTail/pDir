# pDir
[![Build Status](https://travis-ci.org/LeavaTail/pDir.svg?branch=master)](https://travis-ci.org/LeavaTail/pHex)
[![codecov](https://codecov.io/gh/LeavaTail/pDir/branch/master/graph/badge.svg)](https://codecov.io/gh/LeavaTail/pDir)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/d2ed5007af074c9a815b468551e137fa)](https://app.codacy.com/app/LeavaTail/pDir?utm_source=github.com&utm_medium=referral&utm_content=LeavaTail/pDir&utm_campaign=Badge_Grade_Dashboard)

`pdir`(Print DIRectory) list directory contents that referenced `ls`(GNU Core Utilitied).

## Description
List information about the FILEs (the current directory by default).

We can use following option.
 * `-a`,`--all`: do not ignore entries starting with `.`
 * `-A`,`--almost-all`: do not list implied `.` and `..`
 * `-l`: use a long listing format

***DEMO:***
```
 $ pdir pDir/src
pDir/src:
error.c
error.h
gettext.h
list.c
list.h
main.c
pdir.h
```

***EXIT Status:***
 * 0: Success.
 * 1: allocation failed.
 * 2: invalid option.
 * 3: file cannot open.
 * 4: directory cannot open.


## Requirement

- Autoconf  <http://www.gnu.org/software/autoconf/>
- Automake  <http://www.gnu.org/software/automake/>
- Git       <http://git.or.cz/>

## Installation

1. Generate configure file. `./script/bootstrap.sh`
2. Configure the package for your system. `./configure`
3. Compile the package. `make`
4. Install the program. `make install`

## Authors

[LeavaTail](https://github.com/LeavaTail)

## License

[GNU](./COPYING)
