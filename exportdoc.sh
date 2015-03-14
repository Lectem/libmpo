#!/bin/sh
if [ "${CC}" == gcc ]; then

doxygen Doxyfile
cd html
git add --all
git commit -m"Doc generated from Travis build #$TRAVIS_BUILD_NUMBER"
git push -fq origin gh-pages > /dev/null

fi