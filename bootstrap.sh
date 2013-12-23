#!/bin/bash

aclocal
automake --force --copy --install-missing
autoconf
