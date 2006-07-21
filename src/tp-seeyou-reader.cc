/*
 * loggertools
 * Copyright (C) 2004 Max Kellermann (max@duempel.org)
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

class SeeYouTurnPointReader : public TurnPointReader {
private:
    FILE *file;
    int is_eof;
    unsigned num_columns;
    char **columns;
public:
    SeeYouTurnPointReader(FILE *_file);
    virtual ~SeeYouTurnPointReader();
public:
    virtual const TurnPoint *read();
};

static unsigned count_columns(const char *p) {
    unsigned count = 1;
    int in_string = 0;

    for (; *p; p++) {
        if (*p == '"')
            in_string = !in_string;
        else if (!in_string && *p == ',')
            count++;
    }

    return count;
}

static int read_column(const char **line, char *column, size_t column_max_len) {
    if (**line == 0)
        return 0;

    if (**line == '"') {
        (*line)++;

        for (; **line != 0 && **line != '"'; (*line)++) {
            if (column_max_len > 1) {
                *column++ = **line;
                column_max_len--;
            }
        }

        *column = 0;

        if (**line == '"') {
            (*line)++;

            while (**line > 0 && **line <= ' ')
                (*line)++;
        }
    } else {
        char *first_whitespace = NULL;

        for (; **line != 0 && **line != ','; (*line)++) {
            if (**line > 0 && **line <= ' ') {
                if (first_whitespace == NULL)
                    first_whitespace = column;
            } else {
                first_whitespace = NULL;
            }

            if (column_max_len > 1) {
                *column++ = **line;
                column_max_len--;
            }
        }

        if (first_whitespace == NULL)
            *column = 0;
        else
            *first_whitespace = 0;
    }

    if (**line == ',')
        (*line)++;

    return 1;
}

SeeYouTurnPointReader::SeeYouTurnPointReader(FILE *_file)
    :file(_file), is_eof(0),
     num_columns(0), columns(NULL) {
    char line[4096], column[1024];
    const char *p;
    unsigned z;
    int ret;

    p = fgets(line, sizeof(line), file);
    if (p == NULL)
        throw TurnPointReaderException("No header");

    num_columns = count_columns(line);
    if (num_columns == 0)
        throw TurnPointReaderException("No columns in header");

    columns = (char**)malloc(num_columns * sizeof(*columns));
    if (columns == NULL)
        throw TurnPointReaderException("out of memory");

    for (p = line, z = 0; z < num_columns; z++) {
        ret = read_column(&p, column, sizeof(column));
        if (!ret)
            column[0] = 0;

        if (column[0] == 0)
            columns[z] = NULL;
        else
            columns[z] = strdup(column);
    }
}

SeeYouTurnPointReader::~SeeYouTurnPointReader() {
    free(columns);
}

static Angle *parseAngle(const char *p, const char *letters) {
    unsigned long n1, n2;
    int sign, degrees;
    char *endptr;

    if (p == NULL || *p == 0)
        return NULL;

    n1 = strtoul(p, &endptr, 10);
    if (endptr == NULL || *endptr != '.')
        return NULL;

    n2 = strtoul(endptr + 1, &endptr, 10);
    if (n2 >= 1000 || endptr == NULL)
        return NULL;

    if (*endptr == letters[0])
        sign = -1;
    else if (*endptr == letters[1])
        sign = 1;
    else
        return NULL;

    degrees = (int)n1 / 100;
    n1 %= 100;

    if (degrees > 180 || n1 >= 60)
        return NULL;

    return new Angle(sign * (((degrees * 60) + n1) * 1000 + n2));
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

const TurnPoint *SeeYouTurnPointReader::read() {
    char line[4096], column[1024];
    const char *p;
    unsigned z;
    int ret;
    TurnPoint *tp = new TurnPoint();
    Angle *latitude = NULL, *longitude = NULL;
    Altitude *altitude = NULL;
    Runway::type_t rwy_type = Runway::TYPE_UNKNOWN;
    unsigned rwy_direction = UINT_MAX, rwy_length = 0;

    if (is_eof)
        return NULL;

    p = fgets(line, sizeof(line), file);
    if (p == NULL) {
        is_eof = 1;
        return NULL;
    }

    if (strncmp(p, "-----Related", 12) == 0) {
        is_eof = 1;
        return NULL;
    }

    for (z = 0; z < num_columns; z++) {
        ret = read_column(&p, column, sizeof(column));
        if (!ret)
            column[0] = 0;

        if (columns[z] == NULL)
            continue;

        if (strcmp(columns[z], "Title") == 0)
            tp->setTitle(column);
        else if (strcmp(columns[z], "Code") == 0)
            tp->setCode(column);
        else if (strcmp(columns[z], "Country") == 0)
            tp->setCountry(column);
        else if (strcmp(columns[z], "Latitude") == 0) {
            if (latitude != NULL)
                delete latitude;
            latitude = parseAngle(column, "SN");
        } else if (strcmp(columns[z], "Longitude") == 0) {
            if (longitude != NULL)
                delete longitude;
            longitude = parseAngle(column, "WE");
        } else if (strcmp(columns[z], "Elevation") == 0) {
            if (altitude != NULL)
                delete altitude;

            if (*column == 0)
                altitude = new Altitude();
            else
                altitude = new Altitude(strtol(column, NULL, 10),
                                        Altitude::UNIT_METERS,
                                        Altitude::REF_MSL);
        } else if (strcmp(columns[z], "Style") == 0) {
            TurnPoint::type_t type;

            switch (atoi(column)) {
            case 2:
                rwy_type = Runway::TYPE_GRASS;
                type = TurnPoint::TYPE_AIRFIELD;
                break;
            case 3:
                type = TurnPoint::TYPE_OUTLANDING;
                break;
            case 4:
                type = TurnPoint::TYPE_GLIDER_SITE;
                break;
            case 5:
                rwy_type = Runway::TYPE_ASPHALT;
                type = TurnPoint::TYPE_AIRFIELD;
                break;
            case 6:
                type = TurnPoint::TYPE_MOUNTAIN_PASS;
                break;
            case 7:
                type = TurnPoint::TYPE_MOUNTAIN_TOP;
                break;
            case 8:
                type = TurnPoint::TYPE_SENDER;
                break;
            case 9:
                type = TurnPoint::TYPE_VOR;
                break;
            case 10:
                type = TurnPoint::TYPE_NDB;
                break;
            case 11:
                type = TurnPoint::TYPE_COOL_TOWER;
                break;
            case 12:
                type = TurnPoint::TYPE_DAM;
                break;
            case 13:
                type = TurnPoint::TYPE_TUNNEL;
                break;
            case 14:
                type = TurnPoint::TYPE_BRIDGE;
                break;
            case 15:
                type = TurnPoint::TYPE_POWER_PLANT;
                break;
            case 16:
                type = TurnPoint::TYPE_CASTLE;
                break;
            case 17:
                type = TurnPoint::TYPE_INTERSECTION;
                break;
            default:
                type = TurnPoint::TYPE_UNKNOWN;
            }
            tp->setType(type);
        } else if (strcmp(columns[z], "Direction") == 0) {
            if (*column != 0)
                rwy_direction = (unsigned)atoi(column);
        } else if (strcmp(columns[z], "Length") == 0) {
            if (*column != 0)
                rwy_length = (unsigned)atoi(column);
        } else if (strcmp(columns[z], "Frequency") == 0)
            tp->setFrequency(parseFrequency(column));
        else if (strcmp(columns[z], "Description") == 0)
            tp->setDescription(column);
    }

    if (altitude == NULL)
        altitude = new Altitude();

    if (latitude != NULL && longitude != NULL)
        tp->setPosition(Position(*latitude,
                                 *longitude,
                                 *altitude));

    tp->setRunway(Runway(rwy_type, rwy_direction, rwy_length));

    if (latitude != NULL)
        delete latitude;
    if (longitude != NULL)
        delete longitude;
    delete altitude;

    return tp;
}

TurnPointReader *SeeYouTurnPointFormat::createReader(FILE *file) {
    return new SeeYouTurnPointReader(file);
}