loggertools README
==================

This repository is a fork of Max Kellermann's original code http://max.kellermann.name/projects/loggertools/


What is loggertools?
--------------------

loggertools is a collection of free tools and libraries which talk to
GPS flight loggers.


Installation
------------

Install the neceseary tools.

    sudo apt-get install build-essential git

Then download and compile the source code.

    git clone https://github.com/kaklik/loggertools.git
    cd loggertools
    make

Now we have all binnaries prepared in the ./bin directory. There we should run it directly. For example: 

    ~/loggertools/bin/$ ./zander -h
    
    usage: zandertool [OPTIONS] COMMAND [ARGUMENTS]
    valid options:
     --help
     -h             help (this text)
     --tty DEVICE
     -t DEVICE      open this tty device (default /dev/ttyS0)
    valid commands:
      test
            test connection to the GP940
      list
            print a list of flights
      read_tp_tsk <out_filename.da4>
            read the TP and TSK database from the device
      write_tp_tsk <in_filename.da4>
            write the TP and TSK database to the device
      write_apt <data_dir>
            write the APT database to the device
      mem_section <start_adddress> <end_address>
            print memory section info
      raw_mem <start_adddress> <end_address>
            download raw memory

For installation to the system path run following command. 

    sudo makeinstall

Documentation
------------

All documentation has moved to the TeX documentation, and should be
available at ./doc/loggertools.pdf or ./doc/loggertools.tex.
