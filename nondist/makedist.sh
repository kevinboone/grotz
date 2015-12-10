#!/bin/bash

(cd ..; tar cvfz grotz2/nondist/web/grotz2_winstuff.tar.gz grotz2/ --exclude "*nondist*" --exclude "*.o" --exclude "*.exe")

(cd ..; tar cvfz grotz2/nondist/web/grotz2.tar.gz grotz2/ --exclude "*nondist*" --exclude "*winstuff*" --exclude "*.o" --exclude "*.exe")

cp -pr grotz_setup.exe nondist/web

