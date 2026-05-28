cmake_minimum_required(VERSION 3.22)

project(r.plus.example)

include(build_addon)

build_addon(
  ${PROJECT_NAME}
  SOURCES
    r.plus.example.py
  DOCFILES
    r.plus.example.html
    r.plus.example.md)
