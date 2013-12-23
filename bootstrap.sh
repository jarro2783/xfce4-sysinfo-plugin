#!/bin/bash

aclocal
automake --force --copy --add-missing
autoconf
