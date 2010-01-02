#!/bin/bash
make && ./vim --noplugin --cmd ':jsfile test.js' test.js
