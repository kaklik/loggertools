/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "airspace.hh"
#include "airspace-io.hh"
#include "exception.hh"

#include <fstream>
#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

using std::cout;
using std::cerr;
using std::endl;

static void usage(const char *argv0) {
    cout << "usage: " << argv0 << " [options] FILE1 ...\n"
        "options:\n"
        " -o outfile   write output to this file\n"
        " -f outformat write output to stdout with this format\n"
        " -h           help (this text)\n";
}

static void arg_error(const char *argv0, const char *msg)
    __attribute__((noreturn));
static void
arg_error(const char *argv0, const char *msg)
{
    if (msg != NULL)
        cerr << argv0 << ": " << msg << endl;
    cerr << "Try '" << argv0 << " --help' for more information." << endl;
    exit(1);
}

const AirspaceFormat *getFormatFromFilename(const char *filename) {
    const char *dot;
    const AirspaceFormat *format;

    dot = strrchr(filename, '.');
    if (dot == NULL || dot[1] == 0) {
        cerr << "No filename extension in " << filename << endl;
        exit(1);
    }

    format = getAirspaceFormat(dot + 1);
    if (format == NULL) {
        cerr << "Format '" << (dot + 1) << "' is not supported" << endl;
        exit(1);
    }

    return format;
}

int main(int argc, char **argv) {
    const char *out_filename = NULL, *stdout_format = NULL;
    const AirspaceFormat *out_format;
    std::ostream *out;
    AirspaceWriter *writer;

    /* parse command line arguments */
    while (1) {
        int c;

        c = getopt(argc, argv, "ho:f:");
        if (c == -1)
            break;

        switch (c) {
        case 'h':
            usage(argv[0]);
            return 0;

        case 'o':
            out_filename = optarg;
            stdout_format = NULL;
            break;

        case 'f':
            stdout_format = optarg;
            out_filename = NULL;
            break;

        case '?':
            arg_error(argv[0], NULL);

        default:
            exit(1);
        }
    }

    if (out_filename == NULL && stdout_format == NULL)
        arg_error(argv[0], "No output filename specified");

    if (optind >= argc)
        arg_error(argv[0], "No input filename specified");

    /* open output file */

    if (out_filename == NULL) {
        out_format = getAirspaceFormat(stdout_format);
        if (out_format == NULL) {
            cerr << "Format '" << stdout_format << "' is not supported"
                 << endl;
            exit(1);
        }

        out = &cout;
    } else {
        out_format = getFormatFromFilename(out_filename);

        out = new std::ofstream(out_filename);
        if (out->fail()) {
            cerr << "Failed to create " << out_filename
                 << ": " << strerror(errno) << endl;
            exit(2);
        }
    }

    out->exceptions(std::ios_base::badbit | std::ios_base::failbit);

    writer = out_format->createWriter(out);
    if (writer == NULL) {
        unlink(out_filename);
        cerr << "Writing this type is not supported" << endl;
        exit(1);
    }

    /* read all input files */

    while (optind < argc) {
        const char *in_filename = argv[optind++];

        const AirspaceFormat *in_format = getFormatFromFilename(in_filename);
        std::ifstream in(in_filename);
        if (in.fail()) {
            cerr << "Failed to open " << in_filename
                 << ": " << strerror(errno) << endl;
            exit(2);
        }

        in.exceptions(std::ios_base::badbit | std::ios_base::failbit);

        AirspaceReader *reader = in_format->createReader(&in);
        if (reader == NULL) {
            cerr << "Reading this type is not supported" << endl;
            exit(1);
        }

        /* transfer data */
        try {
            const Airspace *as;

            while ((as = reader->read()) != NULL) {
                writer->write(*as);
                delete as;
            }
        } catch (const malformed_input &e) {
            delete writer;
            delete reader;
            unlink(out_filename);
            if (e.get_location().defined())
                cerr << "line " << e.get_location().line << ": ";
            cerr << e.what() << endl;
            exit(2);
        } catch (const std::exception &e) {
            delete writer;
            delete reader;
            unlink(out_filename);
            cerr << e.what() << endl;
            exit(2);
        }

        delete reader;
    }

    writer->flush();
    delete writer;

    if (out == &cout)
        out->flush();
    else
        delete out;

    return 0;
}
