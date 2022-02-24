#!/bin/bash

# Script to determine if source code in Pull Request has proper copyright headers.
# Exits with non-zero exit code if formatting is needed.
#
# This script assumes to be invoked at the project root directory.

FILES_TO_CHECK=$(git diff --name-only HEAD^ | grep -E ".*\.(cpp|cc|c\+\+|cxx|c|h|hpp)$")

if [ -z "$FILES_TO_CHECK" ]; then
  echo "No source code to check if the copyright headers are correct."
  exit 0
fi

FORMAT_DIFF=$(python3 - $FILES_TO_CHECK << EOF
import datetime
import re
import subprocess
import sys

current_year = datetime.datetime.now().date().year

COPYRIGHT_HDR_FULL = """
/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) {YR}                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
""".strip().replace("{YR}", f"{current_year}")

COPYRIGHT_HDR_TMPL = """
/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) {Y1} - {Y2}                                             *
 *                                                                         *
""".strip()

COPYRIGHT_HDR_RE_TMPL = COPYRIGHT_HDR_TMPL

for ch in "*()":
    COPYRIGHT_HDR_RE_TMPL = COPYRIGHT_HDR_RE_TMPL.replace(ch, "\\" + ch)

copyright_hdr_year_re = re.compile("^" + COPYRIGHT_HDR_RE_TMPL.replace("{Y1} - {Y2}",
                                                                       "([0-9]{4})       "))
copyright_hdr_rnge_re = re.compile("^" + COPYRIGHT_HDR_RE_TMPL.replace("{Y1}", "([0-9]{4})")
                                                              .replace("{Y2}", "([0-9]{4})"))

generic_hdr_re = re.compile("^/\\*.*?\\*/", re.DOTALL)

RETCODE = 0

for file_name in sys.argv[1:]:
    with open(file_name, "r", encoding="latin_1", errors="replace") as src_file:
        src = src_file.read()

        year_re_match = copyright_hdr_year_re.match(src)
        rnge_re_match = copyright_hdr_rnge_re.match(src)

        OK = False

        if year_re_match:
            if year_re_match.group(1) == f"{current_year}":
                OK = True
        elif rnge_re_match:
            if rnge_re_match.group(2) == f"{current_year}":
                OK = True

        if not OK:
            RETCODE = 1

            if year_re_match:
                HDR = COPYRIGHT_HDR_TMPL.replace("{Y1}", f"{year_re_match.group(1)}") \
                                        .replace("{Y2}", f"{current_year}")
                src = copyright_hdr_year_re.sub(HDR, src)
            elif rnge_re_match:
                HDR = COPYRIGHT_HDR_TMPL.replace("{Y1}", f"{rnge_re_match.group(1)}") \
                                        .replace("{Y2}", f"{current_year}")
                src = copyright_hdr_rnge_re.sub(HDR, src)
            elif generic_hdr_re.match(src):
                src = generic_hdr_re.sub(COPYRIGHT_HDR_FULL, src)
            else:
                src = COPYRIGHT_HDR_FULL + "\n\n" + src.lstrip()

            with open(file_name + ".tmp", "w", encoding="latin_1", errors="replace") as tmp_file:
                tmp_file.write(src)

                with subprocess.Popen(["diff", "-u", file_name, file_name + ".tmp"]) as diff_proc:
                    diff_proc.wait()

sys.exit(RETCODE)
EOF
)

if [ -z "$FORMAT_DIFF" ]; then
  echo "All source code in PR has proper copyright headers."
  exit 0
else
  echo "Found invalid copyright headers!"
  echo "$FORMAT_DIFF"
  exit 1
fi
