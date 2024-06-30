
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10.0)

badgeros_executable(init sbin)
target_sources(init PRIVATE
    main.c
)
