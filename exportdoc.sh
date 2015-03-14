#!/bin/sh
if [ "$CC" = "gcc" ] && [ "$TRAVIS_REPO_SLUG" == "Lectem/libmpo" ] && [ "$TRAVIS_PULL_REQUEST" == "false" ] && [ "$TRAVIS_BRANCH" == "master" ]; then
doxygen Doxyfile
cd html
git add --all
git commit -m"Doc generated from Travis build #$TRAVIS_BUILD_NUMBER"
git push -f origin gh-pages

fi