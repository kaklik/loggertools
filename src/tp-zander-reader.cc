/*
 * loggertools
 * Copyright (C) 2004-2006 Max Kellermann <max@duempel.org>
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
 *
 * $Id$
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "tp.hh"

class ZanderTurnPointReader : public TurnPointReader {
private:
    FILE *file;
    int is_eof;
public:
    ZanderTurnPointReader(FILE *_file);
public:
    virtual const TurnPoint *read();
};

ZanderTurnPointReader::ZanderTurnPointReader(FILE *_file)
    :file(_file), is_eof(0) {}

static Angle *parseAngle(const char *p, const char *letters) {
    long n;
    int sign, degrees, minutes, seconds;
    char *endptr;

    if (p == NULL || *p == 0)
        return NULL;

    n = (long)strtoul(p, &endptr, 10);
    if (endptr == NULL)
        return NULL;

    if (*endptr == letters[0])
        sign = -1;
    else if (*endptr == letters[1])
        sign = 1;
    else
        return NULL;

    seconds = (int)(n % 100);
    n /= 100;
    minutes = (int)(n % 100);
    n /= 100;
    degrees = (int)n;

    if (degrees > 180 || minutes >= 60 || seconds >= 60)
        return NULL;

    return new Angle(sign * (((degrees * 60) + minutes) * 1000 +
                             (seconds * 1000 + 30) / 60));
}

static unsigned parseFrequency(const char *p) {
    char *endptr;
    unsigned long n1, n2;

    if (p == NULL || *p == 0)
        return 0;

    n1 = strtoul(p, &endptr, 10);
    if (endptr == NULL || *endptr != '.' )
        return 0;

    p = endptr + 1;

    if (*p)
        n2 = strtoul(endptr + 1, &endptr, 10);

    return (n1 * 1000 + n2) * 1000;
}

static bool is_whitespace(char ch) {
    return ch > 0 && ch <= 0x20;
}

static const char *get_next_column(char **pp, size_t width) {
    char *p = *pp, *end;

    while (is_whitespace(*p)) {
        ++p;
        --width;
    }

    end = (char*)memchr(p, 0, width);
    if (end == NULL)
        end = p + width;

    *pp = end;

    while (end > p && is_whitespace(end[-1]))
        --end;

    if (end == p)
        return NULL;

    *end = 0;

    return p;
}

const TurnPoint *ZanderTurnPointReader::read() {
    char line[256];
    char *p;
    const char *q;
    TurnPoint *tp = new TurnPoint();
    Angle *latitude = NULL, *longitude = NULL;
    Altitude *altitude = NULL;
    Runway::type_t rwy_type = Runway::TYPE_UNKNOWN;

    if (is_eof)
        return NULL;

    p = fgets(line, sizeof(line), file);
    if (p == NULL || *p == '\x1a') {
        is_eof = 1;
        return NULL;
    }

    tp->setTitle(get_next_column(&p, 13));

    latitude = parseAngle(get_next_column(&p, 8), "SN");
    longitude = parseAngle(get_next_column(&p, 9), "WE");
    q = get_next_column(&p, 5);
    if (q == NULL)
        altitude = new Altitude();
    else
        altitude = new Altitude(strtol(q, NULL, 10),
                                Altitude::UNIT_METERS,
                                Altitude::REF_MSL);
    tp->setPosition(Position(*latitude,
                             *longitude,
                             *altitude));

    tp->setFrequency(parseFrequency(get_next_column(&p, 8)));

    q = get_next_column(&p, 2);
    if (q != NULL) {
        if (*q == 'G') {
            rwy_type = Runway::TYPE_GRASS;
            tp->setType(TurnPoint::TYPE_AIRFIELD);
        } else if (*q == 'A' || *q == 'C') {
            rwy_type = Runway::TYPE_ASPHALT;
            tp->setType(TurnPoint::TYPE_AIRFIELD);
        } else if (*q == 'V') {
            tp->setType(TurnPoint::TYPE_AIRFIELD);
        } else if (*q == 'S') {
            tp->setType(TurnPoint::TYPE_OUTLANDING);
        }
    }
    tp->setRunway(Runway(rwy_type, UINT_MAX, 0));

    if (latitude != NULL)
        delete latitude;
    if (longitude != NULL)
        delete longitude;
    delete altitude;

    tp->setCountry(get_next_column(&p, 2));

    return tp;
}

TurnPointReader *ZanderTurnPointFormat::createReader(FILE *file) {
    return new ZanderTurnPointReader(file);
}