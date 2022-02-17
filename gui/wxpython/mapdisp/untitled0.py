#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Feb  9 08:12:34 2022

@author: linwe
"""


class Spam(object):
    sausages = False

    @property
    def eggs(self):
        if self.sausages:
            return 42
        raise AttributeError("No eggs without sausages")

    @property
    def invalid(self):
        return self.foobar


spam = Spam()
print(hasattr(Spam, 'eggs'))

print(hasattr(spam, 'eggs'))

spam.sausages = True
print(hasattr(spam, 'eggs'))

print(hasattr(spam, 'invalid'))
print(getattr(spam, 'eggs'))