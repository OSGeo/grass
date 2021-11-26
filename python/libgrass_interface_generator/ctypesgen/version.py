#!/usr/bin/env python3
# vim: ts=2:sw=2:tw=80:nowrap

from subprocess import Popen, PIPE
import os
from os import path

THIS_DIR = path.dirname(__file__)
VERSION_FILE = path.join(THIS_DIR, "VERSION")
DEFAULT_PREFIX = "ctypesgen"

__all__ = ["VERSION", "version_tuple", "version", "compatible"]


def version_tuple(v):
    try:
        vs = v.split("-")
        t = tuple(int(i) for i in vs[1].split("."))
        if len(vs) > 2:
            t += (int(vs[2]),)
        return t
    except:
        return (-1, -1, -1, v)


def read_file_version():
    f = open(VERSION_FILE)
    v = f.readline()
    f.close()
    return v.strip()


def version():
    try:
        args = {"cwd": THIS_DIR}
        devnull = open(os.devnull, "w")
        p = Popen(["git", "describe"], stdout=PIPE, stderr=devnull, **args)
        out, err = p.communicate()
        if p.returncode:
            raise RuntimeError("no version defined?")
        return out.strip().decode()
    except:
        # failover is to try VERSION_FILE instead
        try:
            return read_file_version()
        except:
            return DEFAULT_PREFIX + "-0.0.0"


def version_number():
    return version().partition("-")[-1]


def compatible(v0, v1):
    v0 = version_tuple(v0)
    v1 = version_tuple(v1)
    return v0[:2] == v1[:2]


def write_version_file(v=None):
    if v is None:
        v = version()
    f = open(VERSION_FILE, "w")
    f.write(v)
    f.close()


VERSION = version()
VERSION_NUMBER = version_number()


if __name__ == "__main__":
    import sys, argparse

    p = argparse.ArgumentParser()
    p.add_argument("--save", action="store_true", help="Store version to " + VERSION_FILE)
    p.add_argument(
        "--read-file-version",
        action="store_true",
        help="Read the version stored in " + VERSION_FILE,
    )
    args = p.parse_args()

    v = version()
    if args.save:
        write_version_file(v)
    if args.read_file_version:
        v = read_file_version()
    print(v)
