#!/bin/bash

if [[ "$PLTFRM_32" ]]; then
    docker buildx build --platform linux/386 -f Dockerfile_x32 -t linux_venv_x86 . && 
    docker run  --platform linux/386 --privileged -it  -v /Users/prazd/Desktop/books/cpp/cpp/linux_venv/null/shared:/shared linux_venv_x86  /bin/bash;
elif [[ "$PLTFRM_ARM" ]]; then
    docker build -f Dockerfile_arm -t linux_venv_arm . &&
    docker run --privileged -it -v /Users/prazd/Desktop/books/cpp/cpp/linux_venv/null//shared:/shared linux_venv_arm  /bin/bash
elif [[ "$PLTFRM_64" ]]; then
    docker build -f Dockerfile_x64 -t linux_venv_x86_64 . &&
    docker run --privileged -it -v /Users/prazd/Desktop/books/cpp/cpp/linux_venv/null//shared:/shared linux_venv_x86_64  /bin/bash
fi
