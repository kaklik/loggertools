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

#include "exception.hh"
#include "airspace.hh"
#include "airspace-io.hh"

#include <istream>

class CenfisTextAirspaceReader : public AirspaceReader {
public:
    std::istream *stream;
public:
    CenfisTextAirspaceReader(std::istream *stream);
public:
    virtual const Airspace *read();
};

CenfisTextAirspaceReader::CenfisTextAirspaceReader(std::istream *_stream)
    :stream(_stream) {}

static void chomp(char *p) {
    size_t length = strlen(p);

    while (length > 0 && p[length - 1] > 0 && p[length - 1] <= 0x20)
        --length;

    p[length] = 0;
}

static Airspace::type_t parse_type(const char *p) {
    if (strcmp(p, "A") == 0)
        return Airspace::TYPE_ALPHA;
    if (strcmp(p, "B") == 0)
        return Airspace::TYPE_BRAVO;
    if (strcmp(p, "C") == 0)
        return Airspace::TYPE_CHARLY;
    if (strcmp(p, "D") == 0)
        return Airspace::TYPE_DELTA;
    if (strcmp(p, "E") == 0)
        return Airspace::TYPE_ECHO_LOW;
    if (strcmp(p, "W") == 0)
        return Airspace::TYPE_ECHO_HIGH;
    if (strcmp(p, "F") == 0)
        return Airspace::TYPE_FOX;
    if (strcmp(p, "CTR") == 0)
        return Airspace::TYPE_CTR;
    if (strcmp(p, "TMZ") == 0)
        return Airspace::TYPE_TMZ;
    if (strcmp(p, "R") == 0 ||
        strcmp(p, "TRA") == 0 ||
        strcmp(p, "GP") == 0)
        return Airspace::TYPE_RESTRICTED;
    if (strcmp(p, "Q") == 0)
        return Airspace::TYPE_DANGER;
    if (strcmp(p, "GSEC") == 0)
        return Airspace::TYPE_GLIDER;
    return Airspace::TYPE_UNKNOWN;
}

static const Altitude
parse_altitude(const char *p)
{
    long value;
    Altitude::ref_t ref;
    char *endptr;

    if (p[0] == 'F' && p[1] == 'L') {
        ref = Altitude::REF_1013;
        value = strtol(p + 2, &endptr, 10) * 1000;
        if (value == 0 && *endptr != 0)
            ref = Altitude::REF_UNKNOWN;
    } else {
        value = strtol(p, &endptr, 10);
        while (*endptr == ' ')
            ++endptr;
        if (strcmp(endptr, "GND") == 0)
            ref = Altitude::REF_GND;
        else if (strcmp(endptr, "MSL") == 0)
            ref = Altitude::REF_MSL;
        else
            ref = Altitude::REF_UNKNOWN;
    }

    return Altitude(value, Altitude::UNIT_FEET, ref);
}

static int
char_is_delimiter(char ch)
{
    return ch == ' ' || ch == ',' || ch == ':';
}

static const char *
next_word(char *&p)
{
    char *ret = p;

    while (!char_is_delimiter(*p)) {
        if (*p == 0)
            return ret;
        ++p;
    }

    *p++ = 0;
    return ret;
}

template<class angle_class>
static const angle_class
parse_angle(char *&src)
{
    int degrees, minutes, seconds;
    int sign = 1;

    degrees = atoi(next_word(src));
    if (degrees < 0) {
        sign = -1;
        degrees = -degrees;
    }

    minutes = atoi(next_word(src));
    seconds = atoi(next_word(src));

    return angle_class(1, degrees, minutes, seconds);
}

static const SurfacePosition
parse_surface_position(char *&src)
{
    Latitude latitude = parse_angle<Latitude>(src);
    Longitude longitude = parse_angle<Longitude>(src);
    return SurfacePosition(latitude, longitude);
}

const Airspace *CenfisTextAirspaceReader::read() {
    char line[512], *p;
    Airspace::type_t type = Airspace::TYPE_UNKNOWN;
    std::string cmd, name, name2, name3, name4;
    Altitude bottom(0, Altitude::UNIT_METERS, Altitude::REF_GND), top;
    Airspace::EdgeList edges;

    while (!stream->eof()) {
        try {
            stream->getline(line, sizeof(line));
        } catch (const std::ios_base::failure &e) {
            if (stream->eof())
                break;
            else
                throw;
        }

        if (line[0] == '*') /* comment */
            continue;

        chomp(line);
        if (line[0] == 0) {
            if (edges.empty())
                continue;
            else
                break;
        }

        p = line;
        cmd = next_word(p);
        if (cmd.length() == 0)
            throw malformed_input("command expected");

        if (cmd == "AC") {
            type = parse_type(p);
        } else if (cmd == "AN") {
            name = p;
        } else if (cmd == "AN2") {
            name2 = p;
        } else if (cmd == "AN3") {
            name3 = p;
        } else if (cmd == "AN4") {
            name4 = p;
        } else if (cmd == "AL") {
            bottom = parse_altitude(p);
        } else if (cmd == "AH") {
            top = parse_altitude(p);
        } else if (cmd == "AH2") {
            // ???
        } else if (cmd == "S" || cmd == "L") {
            edges.push_back(parse_surface_position(p));
        } else if (cmd == "A") {
            // arc
        } else if (cmd == "C") {
            // circle
        } else if (cmd == "V") {
            // voice index
        } else if (cmd == "FIS") {
        } else if (cmd == "UPD") {
            // ignore UPD lines
        } else {
            throw malformed_input("invalid command: " + cmd);
        }
    }

    if (edges.size() == 0)
        return NULL;

    if (name2.length() > 0 || name3.length() > 0 || name4.length() > 0) {
        name += '|';
        name += name2;
    }

    if (name3.length() > 0 || name4.length() > 0) {
        name += '|';
        name += name3;
    }

    if (name4.length() > 0) {
        name += '|';
        name += name4;
    }

    return new Airspace(name, type,
                        bottom, top,
                        edges);
}

AirspaceReader *
CenfisTextAirspaceFormat::createReader(std::istream *stream) const {
    return new CenfisTextAirspaceReader(stream);
}

AirspaceWriter *
CenfisTextAirspaceFormat::createWriter(std::ostream *stream) const {
    (void)stream;
    return NULL;
}