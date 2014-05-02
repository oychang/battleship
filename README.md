Battleship
==========

Joseph Choi & Oliver Chang, Project 4

CSC524: Computer Networks, University of Miami, Spring 2014

Issues
------

Does not support repeat moves on successful hits.

Protocol Design
---------------

TCP, with opcodes & data fields like TFTP.
Formally defined in `src/procotol.h`.

Usage
-----

    make
    ./server
    ./client <server address>
