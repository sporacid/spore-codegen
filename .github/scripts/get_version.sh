#!/bin/bash

GIT_COMMITS=$(git --no-pager log --format=%H --reverse)

MAJOR=1
MINOR=0
PATCH=0
FIRST=0

for GIT_COMMIT in $GIT_COMMITS; do
  if [[ $FIRST -eq 0 ]]; then
    FIRST=1
    continue
  fi

  GIT_MESSAGE=$(git --no-pager log -n1 --format=%B $GIT_COMMIT)
  if [[ "$GIT_MESSAGE" =~ "BREAKING CHANGES:" ]]; then
    # Major
    ((MAJOR++))
    MINOR=0
    PATCH=0
  elif [[ "$GIT_MESSAGE" =~ ^([a-zA-Z0-9_\-]+)(\([a-zA-Z0-9_\-]+\))?!: ]]; then
    # Major
    ((MAJOR++))
    MINOR=0
    PATCH=0
  elif [[ "$GIT_MESSAGE" =~ ^feat(\([a-zA-Z0-9_\-]+\))?: ]]; then
    # Minor
    ((MINOR++))
    PATCH=0
  elif [[ "$GIT_MESSAGE" =~ ^fix(\([a-zA-Z0-9_\-]+\))?: ]]; then
    # Patch
    ((PATCH++))
  fi
done

echo -n $MAJOR.$MINOR.$PATCH