#!/usr/bin/env python
# Script to create a pair of case mapping tables for Radian string support.
# Input data comes from "UnicodeData.txt" and "SpecialCasing.txt"; this script
# expects to find those files in its directory.
# This script does not need to be a regular part of the Radian build process;
# those blobs can be committed to version control. You would only need to run
# this script after updating to a newer version of Unicode. 


# Where does this script live? We'll use this to find its input data files.
import sys, os
dirpath = os.path.abspath(os.path.dirname(sys.argv[0]))
unicodedata_path = os.path.join( dirpath, "UnicodeData.txt" )
specialcasing_path = os.path.join( dirpath, "SpecialCasing.txt" )
casefolding_path = os.path.join( dirpath, "CaseFolding.txt" )
# If the data files don't exist, we can't do this job.
if not os.path.exists(unicodedata_path):
	print "Can't find ", unicodedata_path
	print "Get it from <http://www.unicode.org/Public/UNIDATA/UnicodeData.txt>"
	sys.exit(1)
if not os.path.exists(specialcasing_path):
	print "Can't find ", specialcasing_path
	print "Get it from <http://www.unicode.org/Public/UNIDATA/SpecialCasing.txt>"
	sys.exit(1)
if not os.path.exists(casefolding_path):
	print "Cant find ", casefolding_path
	print "Get it from <http://www.unicode.org/Public/UNIDATA/CaseFolding.txt>"
	sys.exit(1)
unicodedata = open(unicodedata_path).readlines()
specialcasing = open(specialcasing_path).readlines()
casefolding = open(casefolding_path).readlines()

# We will build three separate tables from this information: one for normal-to-
# uppercase transformations, one for normal-to-lowercase transformations, and
# one for anything-to-normalized case-folding transformations. We'll start by
# scanning unicodedata, which contains all the single-char transformations,
# then add (and possibly replace) with the superior mappings in specialcasing.
# The casefolding data comes from its own file.
uppercasings = dict()
lowercasings = dict()
casefoldings = dict()

def unpick_unichars(chars_str):
	# We might have a single char expressed in hex notation.
	# We might have a series of such chars.
	splits = chars_str.strip().split(" ")
	chars = [int("0x0" + ch, 16) for ch in splits]
	return chars if len(chars) > 1 else chars[0]

for line in unicodedata:
	fields = line.split(";")
	basechar = unpick_unichars(fields[0])
	upchar = fields[12]
	downchar = fields[13]
	if len(upchar) > 0:
		uppercasings[basechar] = unpick_unichars(upchar)
	if len(downchar) > 0:
		lowercasings[basechar] = unpick_unichars(downchar)

specialcasing_version = ""
for line in specialcasing:
	line = line.strip()
	if len(specialcasing_version) == 0:
		# First line of the file is the version string
		specialcasing_version = line
	if line == "" or line[0] == "#":
		continue
	fields = line.split(";")
	if len(fields) > 5 and len(fields[4]) > 0:
		# we are not interested in locale-specific transformations
		continue
	basechar = unpick_unichars(fields[0])
	upchar_str = fields[3]
	upchar = unpick_unichars(upchar_str)
	downchar_str = fields[1]
	downchar = unpick_unichars(downchar_str)
	if len(upchar_str) > 0 and upchar != basechar:
		uppercasings[basechar] = upchar
	if len(downchar_str) > 0 and downchar != basechar:
		lowercasings[basechar] = downchar

casefolding_version = ""
for line in casefolding:
	line = line.strip()
	if len(casefolding_version) == 0:
		# First line of the file is the version string
		casefolding_version = line
	if line == "" or line[0] == "#":
		continue
	fields = line.split(";")
	# there are four types, distinguished by the letter in column 1:
	# C = common
	# F = full (multichar)
	# S = simple (one-char version of an F transform)
	# T = turkic
	# We will use C+F and ignore S+T. Sorry, Turks.
	foldtype = fields[1].strip()
	if foldtype[0] == "S" or foldtype[0] == "T":
		continue
	basechar = unpick_unichars(fields[0])
	folded = unpick_unichars(fields[2])
	casefoldings[basechar] = folded

def _Charmat(char):
	if char > 0x0FFFF:
		return "\\U" + ("00000" + (hex(char)[2:]))[-6:]
	elif char > 0x0FF:
		return "\\u" + ("000" + (hex(char)[2:]))[-4:]
	else:
		return "\\x" + ("0" + (hex(char)[2:]))[-2:]

def Outform(output):
    out = ""
    if isinstance(output, list):
        for char in output:
            out = out + _Charmat(char)
    else:
        out = _Charmat(output)
    return "\"" + out + "\""

def write_casemap(path, version, casemap):
	with open(os.path.join(dirpath, path), "wb") as dest:
		lines = [version]
		line = "def table = {0 => \"\\x00\""
		for key, value in casemap.items():
			line += ", "
			stub = hex(key) + " => " + Outform(value)
			attempt = (line + stub).replace("\t", "        ")
			if len(attempt) >= 80:
				lines.append(line)
				line = "\t"
			line += stub
		line += "}"
		lines.append(line)
		lines.append("\n")
		dest.write("\n".join(lines))
		dest.truncate


write_casemap("../library/_lowermap.radian",
		specialcasing_version, lowercasings)
write_casemap("../library/_uppermap.radian",
		specialcasing_version, uppercasings)
write_casemap("../library/_foldcasemap.radian",
		casefolding_version, casefoldings)
