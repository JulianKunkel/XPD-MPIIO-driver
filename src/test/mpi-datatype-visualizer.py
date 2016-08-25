#!/usr/bin/env python3

import sys
import re

typ = "STRUCT(count=4,blocklength=[1;3;4;1],displacement=[0;8;32;40000],typ=[NAMED(size=4,INT);NAMED(size=8,DOUBLE);RESIZED(lb=0,typ=HVECTOR(count=2,blocklength=3,stride=100,typ=NAMED(size=4,INT),size=24,extent=112),size=24,extent=1000);NAMED(size=0,UB)],size=400,extent=40000)"

out = "datatype.html"

if len(sys.argv) > 1:
  typ = sys.argv[1]

if len(sys.argv) > 2:
  out = sys.argv[2]

re_dtype = re.compile("([^(]+)\(((.(?!typ=))*),typ=(.*),size=([0-9]+),extent=([0-9]+)\)");
named_dtype = re.compile("NAMED\(size=([0-9]+),(.+)\)");

def repeat(typ, count, displ, o):
  for i in range(0, int(count)):
    #o.write('<div class="REPEAT"> %sx (displ: %s)' % (count, displ))
    processType(typ, o)
    #o.write('</div>')

def procStruct(attribs, childType, o):
  d = attribs["displacement"]
  b = attribs["blocklength"]
  # parse array of child types
  childType = childType[1:-1]
  childTypes = []
  brakets = 0
  start = 0
  for i in range(0, len(childType)):
    if childType[i] == "(":
      brakets = brakets + 1
    if childType[i] == ")":
      brakets = brakets - 1
      if brakets == 0:
        typ = childType[start:i+1]
        childTypes.append(typ)
        start = i + 2
  for i in range(0, int(attribs["count"])):
    repeat(childTypes[i], b[i], d[i], o)

def procHvector(attribs, childType, o):
  c = attribs["count"]
  s = attribs["stride"]
  b = attribs["blocklength"]
  offset = int(s)
  for i in range(0, int(c)):
    repeat(childType, b, i * offset, o)


def procResized(attribs, childType, o):
  lb = attribs["lb"]
  processType(childType, o)

def procContig(attribs, childType, o):
  for i in range(0, int(attribs["count"])):
    processType(childType, o)

def procNamed(typ, size, o):
  o.write('<div class="named">%s(%s)</div>' % (typ, size))

procMap = {
  "STRUCT" : procStruct,
  "NAMED": procNamed,
  "RESIZED" : procResized,
  "HVECTOR" : procHvector,
  "CONTIGUOUS" : procContig,
  }

def parseAttribs(attribs):
  d = attribs.split(",")
  mp = {}
  for a in d:
    s = a.split("=")
    if not s:
      mp["typ"] = s
    else:
      if s[1][0] == "[":
        mp[s[0]] = s[1][1:-1].split(";")
      else:
        mp[s[0]] = s[1]
  return mp

def processType(typ, o):
  m = re_dtype.match(typ)
  if m:
    name = m.group(1)
    attribs = m.group(2)
    childType = m.group(4)
    size = m.group(5)
    extent = m.group(6)
    o.write('<div class="%s">%s (size: %s, extent: %s, %s)' % (name, name, size, extent, attribs))
    if name in procMap:
      procMap[name]( parseAttribs(attribs), childType, o)
    o.write("</div>\n")
  else:
    m = named_dtype.match(typ)
    if m:
      size = m.group(1)
      typ = m.group(2)
      procMap["NAMED"](typ, size, o)
    else:
      print("Error cannot parse the type: " + typ)

o = open(out, "w")
o.write('<HTML><link rel="stylesheet" href="datatypes.css"><body><h1>MPI Datatype Visualizer</h1><h2>Input</h2><p>' + typ + "</p><h2>Graphical</h2>\n")

# start to parse the datatype
processType(typ, o)

o.write("\n</body></HTML>")
o.close()
