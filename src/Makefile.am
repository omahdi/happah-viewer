# Copyright 2017
#   Pawel Herman - Karlsruhe Institute of Technology - pherman@ira.uka.de
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

bin_PROGRAMS = happah
happah_SOURCES = \
     main.cpp \
     Viewer.cpp \
     Window.cpp
AM_CFLAGS = -g -O2
happah_CPPFLAGS = $(eigen3_CFLAGS) $(BOOST_CPPFLAGS)
happah_CXXFLAGS = -std=c++1z
happah_LDFLAGS = $(eigen3_LDFLAGS) $(BOOST_LDFLAGS)
happah_LDADD = -lhappah -lhappah-graphics -lboost_iostreams -lglfw -lpthread -ldl -lstdc++fs
